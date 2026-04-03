#!/usr/bin/env python3
"""test_objdump2asm.py — unit tests for objdump2asm.py"""

import io
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(__file__))
from objdump2asm import (
    parse, split_modules, emit_module,
    _build_label_info, _rename_synth_refs,
    ModuleHeader, SectionHeader, RealLabel, SynthLabel, Instruction, Reloc,
)


# ── Helpers ────────────────────────────────────────────────────────────────────

def make_module_header(obj_name='adddf3'):
    return [
        f'libfp.a({obj_name}.o):\tfile format elf32-s1c33\n',
        '\n',
        'Disassembly of section .text:\n',
        '\n',
    ]


def parse_text(text: str):
    return parse(text.splitlines(keepends=True))


def emit_to_str(module_items) -> str:
    """Return the output of emit_module as a string."""
    with tempfile.TemporaryDirectory() as tmp:
        out_dir = Path(tmp)
        emit_module(module_items, out_dir)
        # Locate the written file
        files = list(out_dir.glob('*.s'))
        if not files:
            return ''
        return files[0].read_text()


# ── Parse tests ───────────────────────────────────────────────────────────────

class TestParse(unittest.TestCase):

    def test_module_header(self):
        items = parse_text('libfp.a(adddf3.o):\tfile format elf32-s1c33\n')
        self.assertEqual(len(items), 1)
        h = items[0]
        self.assertIsInstance(h, ModuleHeader)
        self.assertEqual(h.obj_name, 'adddf3')
        self.assertIn('libfp.a', h.archive_path)

    def test_section_header(self):
        items = parse_text('Disassembly of section .text:\n')
        self.assertEqual(len(items), 1)
        self.assertIsInstance(items[0], SectionHeader)
        self.assertEqual(items[0].name, '.text')

    def test_real_label(self):
        items = parse_text('00000054 <ex1ltex2>:\n')
        self.assertEqual(len(items), 1)
        lbl = items[0]
        self.assertIsInstance(lbl, RealLabel)
        self.assertEqual(lbl.addr, 0x54)
        self.assertEqual(lbl.name, 'ex1ltex2')

    def test_real_label_named_l1(self):
        # muldf3.o: L1 has an address → RealLabel, not SynthLabel
        items = parse_text('00000148 <L1>:\n')
        self.assertIsInstance(items[0], RealLabel)
        self.assertEqual(items[0].name, 'L1')

    def test_synth_label(self):
        items = parse_text('<L0>:\n')
        self.assertEqual(len(items), 1)
        lbl = items[0]
        self.assertIsInstance(lbl, SynthLabel)
        self.assertEqual(lbl.name, 'L0')

    def test_instruction_no_operands(self):
        items = parse_text('      22: 40 07        \tret.d\n')
        self.assertEqual(len(items), 1)
        instr = items[0]
        self.assertIsInstance(instr, Instruction)
        self.assertEqual(instr.mnemonic, 'ret.d')
        self.assertEqual(instr.operands, '')
        self.assertEqual(instr.comment, '')
        self.assertIsNone(instr.reloc)

    def test_instruction_with_register_operand(self):
        items = parse_text('       6: 03 02        \tpushn\t%r3\n')
        instr = items[0]
        self.assertIsInstance(instr, Instruction)
        self.assertEqual(instr.mnemonic, 'pushn')
        self.assertEqual(instr.operands, '%r3')

    def test_instruction_with_comment(self):
        items = parse_text('      20: f1 6b        \tcmp\t%r1, -1                 ; # 0x7ff\n')
        instr = items[0]
        self.assertIsInstance(instr, Instruction)
        self.assertEqual(instr.mnemonic, 'cmp')
        self.assertEqual(instr.operands, '%r1, -1')
        self.assertEqual(instr.comment, '; # 0x7ff')

    def test_instruction_synth_ref(self):
        items = parse_text('      a8: 04 0e        \tjrle\t4 <L2>\n')
        instr = items[0]
        self.assertEqual(instr.mnemonic, 'jrle')
        self.assertEqual(instr.operands, '4 <L2>')

    def test_instruction_negative_ref(self):
        items = parse_text('      ac: fd 1f        \tjp.d\t-3 <L1>\n')
        instr = items[0]
        self.assertEqual(instr.operands, '-3 <L1>')

    def test_reloc_attached_to_instruction(self):
        text = (
            '      3e: 00 0d        \tjrlt.d\t0 <L2>\n'
            '\t\t\t\t0000003e:  R_S1C33_REL8\tex1ltex2\n'
        )
        items = parse_text(text)
        instrs = [i for i in items if isinstance(i, Instruction)]
        self.assertEqual(len(instrs), 1)
        instr = instrs[0]
        self.assertIsNotNone(instr.reloc)
        self.assertEqual(instr.reloc.rtype, 'R_S1C33_REL8')
        self.assertEqual(instr.reloc.symbol, 'ex1ltex2')

    def test_reloc_rel_m(self):
        text = (
            '      22: 00 c0        \text\t0\n'
            '\t\t\t\t00000022:  R_S1C33_REL_M\toverflow\n'
        )
        items = parse_text(text)
        instrs = [i for i in items if isinstance(i, Instruction)]
        self.assertEqual(instrs[0].reloc.rtype, 'R_S1C33_REL_M')
        self.assertEqual(instrs[0].reloc.symbol, 'overflow')

    def test_reloc_rel_h(self):
        text = (
            '     16a: 00 c0        \text\t0\n'
            '\t\t\t\t0000016a:  R_S1C33_REL_H\t__scan64\n'
        )
        items = parse_text(text)
        instrs = [i for i in items if isinstance(i, Instruction)]
        self.assertEqual(instrs[0].reloc.rtype, 'R_S1C33_REL_H')
        self.assertEqual(instrs[0].reloc.symbol, '__scan64')

    def test_blank_lines_ignored(self):
        items = parse_text('\n\n\n')
        self.assertEqual(items, [])

    def test_ext_no_reloc_preserved(self):
        # ext with no reloc (for data immediate) → just an Instruction
        items = parse_text('      1e: 1f c0        \text\t31\n')
        instr = items[0]
        self.assertIsInstance(instr, Instruction)
        self.assertEqual(instr.mnemonic, 'ext')
        self.assertEqual(instr.operands, '31')
        self.assertIsNone(instr.reloc)

    def test_ext_with_comment(self):
        items = parse_text('       4: 0f 78        \txor\t%r15, 0                 ; # -0x80000000\n')
        instr = items[0]
        self.assertEqual(instr.comment, '; # -0x80000000')
        self.assertEqual(instr.operands, '%r15, 0')


# ── _build_label_info tests ───────────────────────────────────────────────────

class TestBuildLabelInfo(unittest.TestCase):

    def _instr(self, operands, has_reloc=False, rtype='R_S1C33_REL8', sym='foo'):
        reloc = Reloc(rtype=rtype, symbol=sym) if has_reloc else None
        return Instruction(addr=0, mnemonic='jrle', operands=operands,
                           comment='', reloc=reloc)

    def test_spurious_label_only_referenced_by_reloc(self):
        items = [
            SynthLabel('L0'),
            self._instr('0 <L0>', has_reloc=True, sym='target'),
        ]
        spurious, rename = _build_label_info('myfunc', items)
        self.assertIn('L0', spurious)
        self.assertEqual(rename, {})

    def test_valid_label_referenced_by_non_reloc(self):
        items = [
            SynthLabel('L1'),
            self._instr('%r3, 8', has_reloc=False),   # non-branch, no ref
            self._instr('4 <L1>', has_reloc=False),    # references L1
        ]
        spurious, rename = _build_label_info('myfunc', items)
        self.assertNotIn('L1', spurious)
        self.assertIn('L1', rename)
        self.assertEqual(rename['L1'], '.L_myfunc_1')

    def test_rename_format(self):
        items = [
            SynthLabel('L0'),
            self._instr('-3 <L0>', has_reloc=False),
        ]
        _, rename = _build_label_info('__adddf3', items)
        self.assertEqual(rename['L0'], '.L___adddf3_0')

    def test_two_labels_one_spurious(self):
        items = [
            SynthLabel('L0'),
            self._instr('0 <L0>', has_reloc=True, sym='ext_target'),
            SynthLabel('L1'),
            self._instr('4 <L1>', has_reloc=False),
        ]
        spurious, rename = _build_label_info('func', items)
        self.assertIn('L0', spurious)
        self.assertNotIn('L1', spurious)
        self.assertIn('L1', rename)

    def test_unreferenced_label_is_spurious(self):
        items = [SynthLabel('L2')]
        spurious, _ = _build_label_info('func', items)
        self.assertIn('L2', spurious)


# ── _rename_synth_refs tests ──────────────────────────────────────────────────

class TestRenameSynthRefs(unittest.TestCase):

    def test_basic_rename(self):
        rename = {'L2': '.L_shftm1_2'}
        result = _rename_synth_refs('4 <L2>', rename)
        self.assertEqual(result, '.L_shftm1_2')

    def test_negative_offset(self):
        rename = {'L1': '.L_shftm1_1'}
        result = _rename_synth_refs('-3 <L1>', rename)
        self.assertEqual(result, '.L_shftm1_1')

    def test_zero_offset(self):
        rename = {'L0': '.L_func_0'}
        result = _rename_synth_refs('0 <L0>', rename)
        self.assertEqual(result, '.L_func_0')

    def test_no_synth_ref(self):
        rename = {'L0': '.L_func_0'}
        result = _rename_synth_refs('%r3, 8', rename)
        self.assertEqual(result, '%r3, 8')

    def test_real_symbol_ref_stripped(self):
        # Names not in rename_map → strip angle brackets, return bare name
        # (e.g. L1 is a real label, notunder is a real symbol)
        self.assertEqual(_rename_synth_refs('4 <L1>', {}), 'L1')
        self.assertEqual(_rename_synth_refs('-3 <notunder>', {}), 'notunder')


# ── emit_module integration tests ─────────────────────────────────────────────

class TestEmitModule(unittest.TestCase):

    def _parse_module(self, text: str):
        items = parse_text(text)
        return split_modules(items)[0]

    def test_header_comment(self):
        text = (
            'libfp.a(adddf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__adddf3>:\n'
            '       0: 03 02        \tpushn\t%r3\n'
        )
        out = emit_to_str(self._parse_module(text))
        self.assertIn('; Recovered from libfp.a', out)
        self.assertIn('adddf3.o', out)
        self.assertIn('Auto-generated by objdump2asm.py', out)

    def test_section_directive(self):
        text = (
            'libfp.a(negsf2.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__negsf2>:\n'
            '       0: 03 02        \tpushn\t%r3\n'
        )
        out = emit_to_str(self._parse_module(text))
        self.assertIn('\t.text', out)

    def test_global_declarations(self):
        text = (
            'libfp.a(adddf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__subdf3>:\n'
            '       0: 00 d0        \text\t4096\n'
            '00000006 <__adddf3>:\n'
            '       6: 03 02        \tpushn\t%r3\n'
        )
        out = emit_to_str(self._parse_module(text))
        self.assertIn('\t.global\t__subdf3', out)
        self.assertIn('\t.global\t__adddf3', out)

    def test_rel8_replacement(self):
        """REL8 relocation: replace operand with symbol name."""
        text = (
            'libfp.a(adddf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000006 <__adddf3>:\n'
            '<L2>:\n'
            '      3e: 00 0d        \tjrlt.d\t0 <L2>\n'
            '\t\t\t\t0000003e:  R_S1C33_REL8\tex1ltex2\n'
        )
        out = emit_to_str(self._parse_module(text))
        self.assertIn('\tjrlt.d\tex1ltex2', out)
        # spurious label must not appear in output
        self.assertNotIn('<L2>', out)
        self.assertNotIn('.L_', out)

    def test_rel_m_l_ext_deleted(self):
        """REL_M + REL_L: delete ext, emit base instruction with symbol."""
        text = (
            'libfp.a(adddf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000006 <__adddf3>:\n'
            '      22: 00 c0        \text\t0\n'
            '\t\t\t\t00000022:  R_S1C33_REL_M\toverflow\n'
            '<L0>:\n'
            '      24: 00 0a        \tjrge\t0 <L0>\n'
            '\t\t\t\t00000024:  R_S1C33_REL_L\toverflow\n'
        )
        out = emit_to_str(self._parse_module(text))
        self.assertIn('\tjrge\toverflow', out)
        # ext instruction must be deleted
        self.assertNotIn('\text\t', out)
        # spurious label must not appear
        self.assertNotIn('<L0>', out)
        self.assertNotIn('.L_', out)

    def test_rel_h_m_l_both_exts_deleted(self):
        """REL_H + REL_M + REL_L: delete both ext instructions, emit base + symbol."""
        text = (
            'libfp.a(adddf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000166 <count>:\n'
            '     16a: 00 c0        \text\t0\n'
            '\t\t\t\t0000016a:  R_S1C33_REL_H\t__scan64\n'
            '     16c: 00 c0        \text\t0\n'
            '\t\t\t\t0000016c:  R_S1C33_REL_M\t__scan64\n'
            '<L0>:\n'
            '     16e: 00 1c        \tcall\t0 <L0>\n'
            '\t\t\t\t0000016e:  R_S1C33_REL_L\t__scan64\n'
        )
        out = emit_to_str(self._parse_module(text))
        self.assertIn('\tcall\t__scan64', out)
        self.assertNotIn('\text\t', out)
        self.assertNotIn('.L_', out)

    def test_non_reloc_ext_preserved(self):
        """ext instruction without relocation (data immediate) must be kept."""
        text = (
            'libfp.a(adddf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__subdf3>:\n'
            '       0: 00 d0        \text\t4096\n'
            '       2: 00 c0        \text\t0\n'
            '       4: 0f 78        \txor\t%r15, 0                 ; # -0x80000000\n'
        )
        out = emit_to_str(self._parse_module(text))
        self.assertIn('\text\t4096', out)
        self.assertIn('\text\t0', out)
        self.assertIn('\txor\t%r15, 0', out)
        self.assertIn('; # -0x80000000', out)

    def test_synth_label_rename(self):
        """Non-spurious synthesized labels are renamed to .L_{func}_{N}."""
        text = (
            'libfp.a(adddf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '0000009a <shftm1>:\n'
            '<L1>:\n'
            '      a6: 83 68        \tcmp\t%r3, 8\n'
            '      a8: 04 0e        \tjrle\t4 <L2>\n'
            '      aa: 8c 88        \tsrl\t%r12, 8\n'
            '      ac: fd 1f        \tjp.d\t-3 <L1>\n'
            '      ae: 03 64        \tsub\t%r3, 8\n'
            '<L2>:\n'
            '      b0: 2c 8e        \tsrl\t%r12, %r3\n'
        )
        out = emit_to_str(self._parse_module(text))
        self.assertIn('.L_shftm1_1:', out)
        self.assertIn('.L_shftm1_2:', out)
        self.assertIn('\tjrle\t.L_shftm1_2', out)
        self.assertIn('\tjp.d\t.L_shftm1_1', out)
        # No raw <LN> references should remain
        self.assertNotIn('<L1>', out)
        self.assertNotIn('<L2>', out)

    def test_real_label_l1_not_renamed(self):
        """Real label L1 (has an address) is treated as RealLabel and not renamed."""
        text = (
            'libfp.a(muldf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__muldf3>:\n'
            '       0: 03 02        \tpushn\t%r3\n'
            '00000148 <L1>:\n'
            '     148: 03 03        \tpopn\t%r3\n'
        )
        out = emit_to_str(self._parse_module(text))
        self.assertIn('\t.global\tL1', out)
        self.assertIn('\nL1:\n', out)

    def test_fallthrough_between_symbols(self):
        """__subdf3 → __adddf3 fallthrough: no separator between them."""
        text = (
            'libfp.a(adddf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__subdf3>:\n'
            '       0: 00 d0        \text\t4096\n'
            '00000006 <__adddf3>:\n'
            '       6: 03 02        \tpushn\t%r3\n'
        )
        out = emit_to_str(self._parse_module(text))
        # Both symbols present and consecutive
        pos_sub = out.find('__subdf3:')
        pos_add = out.find('__adddf3:')
        self.assertGreater(pos_sub, 0)
        self.assertGreater(pos_add, pos_sub)

    def test_no_raw_addresses_in_output(self):
        """No raw hex addresses (XX: XX XX) should remain in output."""
        text = (
            'libfp.a(adddf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__adddf3>:\n'
            '       0: 03 02        \tpushn\t%r3\n'
            '       2: d0 2e        \tld.w\t%r0, %r13\n'
        )
        out = emit_to_str(self._parse_module(text))
        import re
        # No raw byte sequences like "XX: XX XX" should remain
        self.assertIsNone(re.search(r'[0-9a-f]+: [0-9a-f]{2} [0-9a-f]{2}', out))

    def test_no_x_prefixed_mnemonics(self):
        """No x-prefixed mnemonics (xjrge, xcall, etc.) should appear in output."""
        # These shouldn't appear in real data, but guard against regressions
        text = (
            'libfp.a(test.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__test>:\n'
            '      22: 00 c0        \text\t0\n'
            '\t\t\t\t00000022:  R_S1C33_REL_M\ttarget\n'
            '      24: 00 0a        \tjrge\t0\n'
            '\t\t\t\t00000024:  R_S1C33_REL_L\ttarget\n'
        )
        out = emit_to_str(self._parse_module(text))
        import re
        self.assertIsNone(re.search(r'\bx[a-z]', out))


# ── Multi-module tests ─────────────────────────────────────────────────────────

class TestMultiModule(unittest.TestCase):

    def test_split_modules(self):
        text = (
            'libfp.a(adddf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__adddf3>:\n'
            '       0: 03 02        \tpushn\t%r3\n'
            'libfp.a(muldf3.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__muldf3>:\n'
            '       0: 03 02        \tpushn\t%r3\n'
        )
        items = parse_text(text)
        modules = split_modules(items)
        self.assertEqual(len(modules), 2)
        h0 = next(i for i in modules[0] if isinstance(i, ModuleHeader))
        h1 = next(i for i in modules[1] if isinstance(i, ModuleHeader))
        self.assertEqual(h0.obj_name, 'adddf3')
        self.assertEqual(h1.obj_name, 'muldf3')

    def test_emit_two_modules(self):
        text = (
            'libfp.a(negsf2.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__negsf2>:\n'
            '       0: 03 02        \tpushn\t%r3\n'
            'libfp.a(negdf2.o):\tfile format elf32-s1c33\n'
            'Disassembly of section .text:\n'
            '00000000 <__negdf2>:\n'
            '       0: 03 02        \tpushn\t%r3\n'
        )
        items = parse_text(text)
        modules = split_modules(items)
        with tempfile.TemporaryDirectory() as tmp:
            out_dir = Path(tmp)
            for m in modules:
                emit_module(m, out_dir)
            files = sorted(p.name for p in out_dir.glob('*.s'))
        self.assertEqual(files, ['negdf2.s', 'negsf2.s'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
