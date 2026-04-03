#!/usr/bin/env python3
"""
objdump2asm.py — Convert S1C33 ELF objdump output into reassemblable .s files.

Usage:
    llvm-objdump -d -r --symbolize-operands libfp.a | ./objdump2asm.py [-o out/]
    ./objdump2asm.py dump.txt [-o out/]

Input:  stdout of llvm-objdump -d -r --symbolize-operands (multiple modules concatenated)
Output: one {obj_name}.s per module written to the output directory

Transformations applied:
  - Strip address and hex byte columns
  - REL8 relocation: replace operand with symbol name
  - REL_M + REL_L: delete ext instruction, emit basic instruction with symbol
  - REL_H + REL_M + REL_L: delete both ext instructions, emit basic instruction with symbol
  - Synthesized labels <LN> renamed to .L_{funcname}_{N} (globally unique)
  - Spurious synthesized labels (unreferenced by non-relocated instructions) deleted
  - ext instructions with no relocation preserved as-is
  - All real symbols declared .global
"""

import re
import sys
import argparse
from dataclasses import dataclass, field
from typing import Optional, List, Dict, Set, Tuple
from pathlib import Path


# ── Data classes ──────────────────────────────────────────────────────────────

@dataclass
class Reloc:
    rtype: str    # R_S1C33_REL8 / R_S1C33_REL_L / R_S1C33_REL_M / R_S1C33_REL_H
    symbol: str


@dataclass
class Item:
    pass


@dataclass
class ModuleHeader(Item):
    archive_path: str   # full path (e.g. "sysroot/.../libfp.a")
    obj_name: str       # without extension (e.g. "adddf3")


@dataclass
class SectionHeader(Item):
    name: str           # e.g. ".text"


@dataclass
class RealLabel(Item):
    addr: int
    name: str


@dataclass
class SynthLabel(Item):
    name: str           # e.g. "L0", "L1"


@dataclass
class Instruction(Item):
    addr: int
    mnemonic: str
    operands: str       # raw operand string (e.g. "4 <L2>", "%r3, 8")
    comment: str        # "; # ..." or ""
    reloc: Optional[Reloc] = None


# ── Regex patterns ────────────────────────────────────────────────────────────

# Module header: "libfp.a(adddf3.o):	file format elf32-s1c33"
_RE_MODULE = re.compile(r'^(\S+)\(([^)]+\.o)\):\s+file format')

# Section header: "Disassembly of section .text:"
_RE_SECTION = re.compile(r'^Disassembly of section (\S+?):')

# Real symbol label: "00000054 <ex1ltex2>:"  (has address prefix)
_RE_REAL_LABEL = re.compile(r'^([0-9a-f]+) <([^>]+)>:')

# Synthesized label: "<L0>:"  (no address prefix)
_RE_SYNTH_LABEL = re.compile(r'^<(L\d+)>:')

# Relocation line: "		0000003e:  R_S1C33_REL8	ex1ltex2"
_RE_RELOC = re.compile(r'^\s+([0-9a-f]+):\s+(R_S1C33_\w+)\s+(\S+)')

# Instruction line: "      a8: 04 0e        	jrle	4 <L2>"
# hex byte pairs ("XX ") followed by tab, mnemonic, optional operands
_RE_INSTRUCTION = re.compile(r'^\s+([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*\t(.+)')

# Symbol reference in operand: "4 <L2>", "-3 <L1>", "0 <L0>", "-3 <notunder>"
# --symbolize-operands emits all resolved branch targets as "offset <name>"
_RE_SYMREF = re.compile(r'-?\d+\s+<([^>]+)>')

# Alias for backward compatibility (tests)
_RE_SYNTH_REF = _RE_SYMREF

# Split operands from inline comment: "%r1, -1                 ; # 0x7ff"
_RE_COMMENT = re.compile(r'^(.*?)\s{2,}(;.*)$')


# ── Pass 1: parse ─────────────────────────────────────────────────────────────

def parse(lines: List[str]) -> List[Item]:
    """Parse input lines into a flat list of Items."""
    items: List[Item] = []
    last_instr: Optional[Instruction] = None

    for raw_line in lines:
        line = raw_line.rstrip('\n')

        # Module header
        m = _RE_MODULE.match(line)
        if m:
            obj_full = m.group(2)                       # "adddf3.o"
            obj_name = obj_full.rsplit('.', 1)[0]       # "adddf3"
            items.append(ModuleHeader(archive_path=m.group(1), obj_name=obj_name))
            last_instr = None
            continue

        # Section header
        m = _RE_SECTION.match(line)
        if m:
            items.append(SectionHeader(name=m.group(1)))
            continue

        # Real symbol label (has address prefix)
        m = _RE_REAL_LABEL.match(line)
        if m:
            items.append(RealLabel(addr=int(m.group(1), 16), name=m.group(2)))
            last_instr = None
            continue

        # Synthesized label (no address prefix)
        m = _RE_SYNTH_LABEL.match(line)
        if m:
            items.append(SynthLabel(name=m.group(1)))
            continue

        # Relocation line — checked before instruction because formats are similar
        m = _RE_RELOC.match(line)
        if m:
            reloc_addr = int(m.group(1), 16)
            reloc = Reloc(rtype=m.group(2), symbol=m.group(3))
            # Attach to the immediately preceding instruction, verified by address
            if last_instr is not None and last_instr.addr == reloc_addr:
                last_instr.reloc = reloc
            continue

        # Instruction line
        m = _RE_INSTRUCTION.match(line)
        if m:
            addr = int(m.group(1), 16)
            rest = m.group(2)   # "mnemonic\toperands_comment" or just "mnemonic"
            parts = rest.split('\t', 1)
            mnemonic = parts[0]
            operands_comment = parts[1] if len(parts) > 1 else ''

            # Separate inline comment "; # ..."
            cm = _RE_COMMENT.match(operands_comment)
            if cm:
                operands = cm.group(1)
                comment  = cm.group(2)
            else:
                operands = operands_comment
                comment  = ''

            instr = Instruction(addr=addr, mnemonic=mnemonic,
                                operands=operands, comment=comment)
            items.append(instr)
            last_instr = instr
            continue

        # Blank lines and other noise — ignored; last_instr is NOT reset here
        # because relocation lines always immediately follow their instruction.

    return items


# ── Pass 2 helpers ────────────────────────────────────────────────────────────

def split_modules(items: List[Item]) -> List[List[Item]]:
    """Split the flat item list into per-module sublists."""
    modules: List[List[Item]] = []
    current: List[Item] = []
    for item in items:
        if isinstance(item, ModuleHeader):
            if current:
                modules.append(current)
            current = [item]
        else:
            current.append(item)
    if current:
        modules.append(current)
    return modules


def _build_label_info(
    func_name: str,
    func_items: List[Item],
) -> Tuple[Set[str], Dict[str, str]]:
    """Analyse synthesized labels within a function context.

    Returns:
        spurious_labels: set of label names not referenced by any non-relocated instruction
        rename_map: {original_name → .L_{funcname}_{N}} for non-spurious labels only
    """
    all_synths: Set[str] = set()
    non_reloc_refs: Set[str] = set()

    for item in func_items:
        if isinstance(item, SynthLabel):
            all_synths.add(item.name)
        elif isinstance(item, Instruction) and item.reloc is None:
            for m in _RE_SYNTH_REF.finditer(item.operands):
                non_reloc_refs.add(m.group(1))

    spurious = all_synths - non_reloc_refs

    rename_map: Dict[str, str] = {}
    for name in all_synths - spurious:
        n = name[1:]   # "L0" → "0", "L1" → "1"
        rename_map[name] = f'.L_{func_name}_{n}'

    return spurious, rename_map


def _rename_synth_refs(operands: str, rename_map: Dict[str, str]) -> str:
    """Replace every "N <name>" pattern in an operand string with the symbol name.

    - Synthesized label: "4 <L2>" → ".L_shftm1_2" if in rename_map
    - Real symbol: "-3 <notunder>" → "notunder"  (strip angle brackets only)

    --symbolize-operands emits all resolved branch targets as "offset <name>".
    The assembler cannot parse the <> syntax, so every pattern must be handled.
    """
    def replace(m: re.Match) -> str:
        name = m.group(1)
        if name in rename_map:
            return rename_map[name]
        # Real symbol name (notunder, L1, L2, etc.) — use as-is
        return name

    return _RE_SYMREF.sub(replace, operands)


def _write_instr(f, mnemonic: str, operands: str, comment: str) -> None:
    """Write a single instruction line."""
    line = f'\t{mnemonic}'
    if operands:
        line += f'\t{operands}'
    if comment:
        line += f'\t{comment}'
    f.write(line + '\n')


# ── Pass 2: emit module ────────────────────────────────────────────────────────

def emit_module(module_items: List[Item], out_dir: Path) -> str:
    """Write a .s file for one module.

    Returns:
        Path of the written file, or '' if no ModuleHeader was found.
    """
    header = next((i for i in module_items if isinstance(i, ModuleHeader)), None)
    if header is None:
        return ''

    archive_basename = Path(header.archive_path).name  # "libfp.a"
    obj_name = header.obj_name                          # "adddf3"

    # Collect all RealLabels for .global declarations
    real_labels = [i for i in module_items if isinstance(i, RealLabel)]

    # Use the first SectionHeader encountered
    section = next((i for i in module_items if isinstance(i, SectionHeader)), None)

    # Pre-compute per-function-context label info.
    # A context spans from one RealLabel to the next (or end of module).
    ctx_map: Dict[str, Tuple[Set[str], Dict[str, str]]] = {}
    cur_func: Optional[str] = None
    cur_func_items: List[Item] = []

    for item in module_items:
        if isinstance(item, RealLabel):
            if cur_func is not None:
                ctx_map[cur_func] = _build_label_info(cur_func, cur_func_items)
            cur_func = item.name
            cur_func_items = []
        elif cur_func is not None:
            cur_func_items.append(item)

    if cur_func is not None:
        ctx_map[cur_func] = _build_label_info(cur_func, cur_func_items)

    # ── Write output file ──────────────────────────────────────────────────
    out_path = out_dir / f'{obj_name}.s'
    out_path.parent.mkdir(parents=True, exist_ok=True)

    with out_path.open('w') as f:
        f.write(f'; Recovered from {archive_basename} — {obj_name}.o\n')
        f.write('; Auto-generated by objdump2asm.py, do not edit\n\n')

        if section:
            f.write(f'\t{section.name}\n\n')

        for lbl in real_labels:
            f.write(f'\t.global\t{lbl.name}\n')
        f.write('\n')

        # ── Emit code sequentially ────────────────────────────────────────
        cur_func = None
        spurious: Set[str] = set()
        rename_map: Dict[str, str] = {}
        # Symbol accumulated from REL_H / REL_M; consumed by REL_L
        pending_symbol: Optional[str] = None

        for item in module_items:
            if isinstance(item, (ModuleHeader, SectionHeader)):
                continue

            elif isinstance(item, RealLabel):
                cur_func = item.name
                spurious, rename_map = ctx_map.get(cur_func, (set(), {}))
                pending_symbol = None
                f.write(f'\n{item.name}:\n')

            elif isinstance(item, SynthLabel):
                if item.name not in spurious:
                    new_name = rename_map.get(item.name, item.name)
                    f.write(f'{new_name}:\n')
                # else: spurious — skip

            elif isinstance(item, Instruction):
                reloc = item.reloc

                if reloc is None:
                    # No relocation: emit as-is, renaming any synth refs
                    ops = _rename_synth_refs(item.operands, rename_map)
                    _write_instr(f, item.mnemonic, ops, item.comment)

                elif reloc.rtype in ('R_S1C33_REL_H', 'R_S1C33_REL_M'):
                    # ext instruction — delete it, accumulate the symbol
                    pending_symbol = reloc.symbol

                elif reloc.rtype == 'R_S1C33_REL_L':
                    # Last instruction of ext+…+instr sequence; emit with symbol
                    _write_instr(f, item.mnemonic, reloc.symbol, '')
                    pending_symbol = None

                elif reloc.rtype == 'R_S1C33_REL8':
                    # Simple substitution: replace operand with symbol name
                    _write_instr(f, item.mnemonic, reloc.symbol, '')
                    pending_symbol = None

                else:
                    # Unknown relocation type: warn and emit as-is
                    print(f'warning: unknown reloc {reloc.rtype} in {obj_name}',
                          file=sys.stderr)
                    ops = _rename_synth_refs(item.operands, rename_map)
                    _write_instr(f, item.mnemonic, ops, item.comment)

    return str(out_path)


# ── Main ───────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description='Convert llvm-objdump -d -r --symbolize-operands output to .s files'
    )
    parser.add_argument('input', nargs='?',
                        help='input file (default: stdin)')
    parser.add_argument('-o', '--output', default='out',
                        help='output directory (default: out/)')
    args = parser.parse_args()

    if args.input:
        with open(args.input, encoding='utf-8') as f:
            lines = f.readlines()
    else:
        lines = sys.stdin.readlines()

    out_dir = Path(args.output)
    out_dir.mkdir(parents=True, exist_ok=True)

    items = parse(lines)
    modules = split_modules(items)

    count = 0
    for module_items in modules:
        out = emit_module(module_items, out_dir)
        if out:
            count += 1

    print(f'Generated {count} .s file(s) in {out_dir}/', file=sys.stderr)


if __name__ == '__main__':
    main()
