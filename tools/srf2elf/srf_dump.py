#!/usr/bin/env python3
"""srf_dump.py — Dump contents of SRF33 object files.

SRF33 (Seiko Epson Relocatable File Format for S1C33) is the object/library
format used by the EPSON C compiler package tools (as33, lk33, lib33).

Format reference: S5U1C33000C Manual Appendix pp.475-480
Byte order: BIG-ENDIAN (all multi-byte fields)
Errata: e_scnndx is 2 bytes, not 4 as stated in the manual (piece-lab 2015).

Usage:
    python3 srf_dump.py <file.srf>
    python3 srf_dump.py <file.o>
"""

import sys
import struct

# ---------------------------------------------------------------------------
# SRF field sizes and struct formats (all big-endian)
# ---------------------------------------------------------------------------

# (1) Control header — 16 bytes
#   c_fatt(2), c_pentry(2), c_ver(2), c_scncnt(2), c_scnptr(4), c_debptr(4)
CTRL_HDR_SIZE = 16
CTRL_HDR_FMT  = '>HHHHI I'  # note: HHHH = 8 bytes, then two I = 8 bytes

# c_fatt flags
C_FATT_RELOC  = 0x0001
C_FATT_ABS    = 0x0002
C_FATT_EXEC   = 0x0004
C_FATT_DEBUG  = 0x0008
C_FATT_LIB    = 0x0010

# (2) Section info — 44 bytes
#   s_nxptr(4), s_scntyp(2), s_lnktyp(2), s_scnatt(2), _pad(2), s_off(4),
#   s_rcptr(4), s_rcsiz(4), s_exptr(4), s_exsiz(4), s_excnt(4),
#   s_rdptr(4), s_dsiz(4), s_scnndx(2)
# Layout: 4+2+2+2 = 10, then s_off at +10, then 8×4=32, then s_scnndx at +42
SECT_HDR_SIZE = 44

# Section types
SCNTYP = {1: 'CODE', 2: 'DATA', 3: 'BSS', 4: 'DUMMY'}
# Section attributes
SCNATT = {1: 'ABS', 2: 'RELOC'}

# (3) Relocation info — 16 bytes
#   r_rctyp(2), r_scnoff(4), r_exndx(4), r_scnndx(2), r_symoff(4)
RELOC_SIZE = 16

RCTYP = {
    0x0001: 'REL8    (8-bit PC-rel, SYMBOL<0x200)',
    0x0002: 'REL_H   (32-bit PC-rel [31:22], @rh)',
    0x0003: 'REL_M   (32-bit PC-rel [21:9],  @rm)',
    0x0004: 'REL_L   (32-bit PC-rel [8:1],   @rl)',
    0x0005: 'REL_AH  (26-bit PC-rel [25:13], +sign32@ah)',
    0x0006: 'REL_AL  (26-bit PC-rel [12:0],  +sign32@al)',
    0x0007: 'ABS_H   (32-bit ABS [31:19],    +imm32@h)',
    0x0008: 'ABS_M   (32-bit ABS [18:6],     +imm32@m)',
    0x0009: 'ABS_L   (32-bit ABS [5:0],      +imm32@l)',
    0x000a: 'ABS32   (32-bit ABS [31:0],     SYMBOL)',
}

# (4) Extern info — 13 + name_len bytes
#   e_scnoff(4), e_size(4), e_scnndx(2) [errata!], e_extyp(2), e_namsiz(1)
EXTERN_HDR_SIZE = 13  # before name
EXTYP = {1: 'GLOBAL', 2: 'LOCAL', 3: 'EXTERN'}

# (6) Debug control info — 36 bytes
DEBUG_CTRL_SIZE = 36


# ---------------------------------------------------------------------------
# Low-level helpers
# ---------------------------------------------------------------------------

def read_at(data, offset, fmt):
    """Unpack struct at given offset, return tuple."""
    size = struct.calcsize(fmt)
    return struct.unpack_from(fmt, data, offset)


def c_fatt_str(fatt):
    parts = []
    if fatt & C_FATT_RELOC: parts.append('RELOC')
    if fatt & C_FATT_ABS:   parts.append('ABS')
    if fatt & C_FATT_EXEC:  parts.append('EXEC')
    if fatt & C_FATT_DEBUG: parts.append('DEBUG')
    if fatt & C_FATT_LIB:   parts.append('LIB')
    return '|'.join(parts) if parts else '0'


# ---------------------------------------------------------------------------
# SRF object file parser
# ---------------------------------------------------------------------------

def parse_ctrl_header(data, off=0):
    c_fatt, c_pentry, c_ver, c_scncnt = struct.unpack_from('>HHHH', data, off)
    c_scnptr, c_debptr = struct.unpack_from('>II', data, off + 8)
    return {
        'c_fatt':   c_fatt,
        'c_pentry': c_pentry,
        'c_ver':    c_ver,
        'c_scncnt': c_scncnt,
        'c_scnptr': c_scnptr,
        'c_debptr': c_debptr,
    }


def parse_section(data, off):
    (s_nxptr,) = struct.unpack_from('>I', data, off)
    s_scntyp, s_lnktyp, s_scnatt = struct.unpack_from('>HHH', data, off + 4)
    (s_off,) = struct.unpack_from('>I', data, off + 10)
    s_rcptr, s_rcsiz, s_exptr, s_exsiz, s_excnt = struct.unpack_from('>IIIII', data, off + 14)
    s_rdptr, s_dsiz = struct.unpack_from('>II', data, off + 34)
    (s_scnndx,) = struct.unpack_from('>H', data, off + 42)
    return {
        's_nxptr':  s_nxptr,
        's_scntyp': s_scntyp,
        's_lnktyp': s_lnktyp,
        's_scnatt': s_scnatt,
        's_off':    s_off,
        's_rcptr':  s_rcptr,
        's_rcsiz':  s_rcsiz,
        's_exptr':  s_exptr,
        's_exsiz':  s_exsiz,
        's_excnt':  s_excnt,
        's_rdptr':  s_rdptr,
        's_dsiz':   s_dsiz,
        's_scnndx': s_scnndx,
    }


def parse_all_sections(data, first_ptr):
    sections = []
    ptr = first_ptr
    while ptr:
        sec = parse_section(data, ptr)
        sec['_offset'] = ptr
        sections.append(sec)
        ptr = sec['s_nxptr']
    return sections


def parse_relocations(data, rcptr, rcsiz):
    relocs = []
    if not rcptr or not rcsiz:
        return relocs
    count = rcsiz // RELOC_SIZE
    for i in range(count):
        off = rcptr + i * RELOC_SIZE
        (r_rctyp,) = struct.unpack_from('>H', data, off)
        (r_scnoff,) = struct.unpack_from('>I', data, off + 2)
        (r_exndx,)  = struct.unpack_from('>I', data, off + 6)
        (r_scnndx,) = struct.unpack_from('>H', data, off + 10)
        (r_symoff,) = struct.unpack_from('>I', data, off + 12)
        relocs.append({
            'r_rctyp':  r_rctyp,
            'r_scnoff': r_scnoff,
            'r_exndx':  r_exndx,
            'r_scnndx': r_scnndx,
            'r_symoff': r_symoff,
        })
    return relocs


def parse_externs(data, exptr, excnt):
    externs = []
    if not exptr or not excnt:
        return externs
    off = exptr
    for _ in range(excnt):
        (e_scnoff,) = struct.unpack_from('>I', data, off)
        (e_size,)   = struct.unpack_from('>I', data, off + 4)
        (e_scnndx,) = struct.unpack_from('>H', data, off + 8)  # 2 bytes (errata!)
        (e_extyp,)  = struct.unpack_from('>H', data, off + 10)
        (e_namsiz,) = struct.unpack_from('>B', data, off + 12)
        name = data[off + 13 : off + 13 + e_namsiz].decode('ascii', errors='replace')
        externs.append({
            'e_scnoff': e_scnoff,
            'e_size':   e_size,
            'e_scnndx': e_scnndx,
            'e_extyp':  e_extyp,
            'e_namsiz': e_namsiz,
            'e_exnam':  name,
            '_offset':  off,
        })
        off += EXTERN_HDR_SIZE + e_namsiz
    return externs


def parse_debug_ctrl(data, ptr):
    if not ptr:
        return []
    dbgs = []
    while ptr:
        (d_nxptr, d_fiptr, d_flsiz, d_flcnt) = struct.unpack_from('>IIII', data, ptr)
        (d_stptr, d_stsiz) = struct.unpack_from('>II', data, ptr + 16)
        (d_syptr, d_sysiz, d_sycnt) = struct.unpack_from('>III', data, ptr + 24)
        dbgs.append({
            'd_nxptr': d_nxptr,
            'd_fiptr': d_fiptr,
            'd_flsiz': d_flsiz,
            'd_flcnt': d_flcnt,
            'd_stptr': d_stptr,
            'd_stsiz': d_stsiz,
            'd_syptr': d_syptr,
            'd_sysiz': d_sysiz,
            'd_sycnt': d_sycnt,
            '_offset': ptr,
        })
        ptr = d_nxptr
    return dbgs


# ---------------------------------------------------------------------------
# Dump functions
# ---------------------------------------------------------------------------

def dump_srf(data, base=0, label=None):
    """Dump one SRF object. base = byte offset of SRF data within file."""
    if label:
        print(f'\n{"="*60}')
        print(f'  {label}')
        print(f'{"="*60}')

    hdr = parse_ctrl_header(data, base)
    # c_ver = 0x3300: high byte = SRF version (0x33=51? actually version "33"),
    # low byte = revision. Display as original "33.00" not decimal.
    ver_str = f"srf{(hdr['c_ver'] >> 8):02x} rev{hdr['c_ver'] & 0xff:02x}"

    print(f"\n[Control Header]  @ 0x{base:08x}")
    print(f"  c_fatt   = 0x{hdr['c_fatt']:04x}  ({c_fatt_str(hdr['c_fatt'])})")
    print(f"  c_pentry = 0x{hdr['c_pentry']:04x}")
    print(f"  c_ver    = 0x{hdr['c_ver']:04x}  ({ver_str})")
    print(f"  c_scncnt = {hdr['c_scncnt']}")
    print(f"  c_scnptr = 0x{hdr['c_scnptr']:08x}")
    print(f"  c_debptr = 0x{hdr['c_debptr']:08x}")

    # Sections
    sections = parse_all_sections(data, base + hdr['c_scnptr'] - (0 if base == 0 else base))
    # Note: chain values are absolute file offsets from the start of data
    # Re-parse with correct base handling
    sections = _parse_sections_abs(data, hdr['c_scnptr'])

    print(f"\n[Sections]  ({hdr['c_scncnt']} total)")
    for i, sec in enumerate(sections):
        typ  = SCNTYP.get(sec['s_scntyp'], f"0x{sec['s_scntyp']:04x}")
        att  = SCNATT.get(sec['s_scnatt'], f"0x{sec['s_scnatt']:04x}")
        print(f"  [{i}] @ 0x{sec['_offset']:08x}  id={sec['s_scnndx']}  "
              f"{typ:<5}  {att:<6}  "
              f"addr=0x{sec['s_off']:08x}  "
              f"data_size=0x{sec['s_dsiz']:x}  "
              f"rdptr=0x{sec['s_rdptr']:08x}")
        if sec['s_rcptr']:
            print(f"       relocs: ptr=0x{sec['s_rcptr']:08x}  size=0x{sec['s_rcsiz']:x}")
        if sec['s_exptr']:
            print(f"       externs: ptr=0x{sec['s_exptr']:08x}  size=0x{sec['s_exsiz']:x}  "
                  f"count={sec['s_excnt']}")

    # Relocations per section
    has_relocs = any(s['s_rcptr'] for s in sections)
    if has_relocs:
        print(f"\n[Relocations]")
        for i, sec in enumerate(sections):
            if not sec['s_rcptr']:
                continue
            typ = SCNTYP.get(sec['s_scntyp'], '?')
            relocs = parse_relocations(data, sec['s_rcptr'], sec['s_rcsiz'])
            print(f"  Section [{i}] {typ} id={sec['s_scnndx']}  ({len(relocs)} entries)")
            for r in relocs:
                rname = RCTYP.get(r['r_rctyp'], f"0x{r['r_rctyp']:04x}")
                print(f"    scnoff=0x{r['r_scnoff']:08x}  exndx={r['r_exndx']}  "
                      f"scnndx={r['r_scnndx']}  symoff=0x{r['r_symoff']:08x}  "
                      f"type={rname}")

    # Externs per section
    has_externs = any(s['s_exptr'] for s in sections)
    if has_externs:
        print(f"\n[Externs]")
        for i, sec in enumerate(sections):
            if not sec['s_exptr']:
                continue
            typ = SCNTYP.get(sec['s_scntyp'], '?')
            externs = parse_externs(data, sec['s_exptr'], sec['s_excnt'])
            print(f"  Section [{i}] {typ} id={sec['s_scnndx']}  ({len(externs)} symbols)")
            for j, e in enumerate(externs):
                etype = EXTYP.get(e['e_extyp'], f"0x{e['e_extyp']:04x}")
                print(f"    [{j}] @ 0x{e['_offset']:08x}  scnoff=0x{e['e_scnoff']:08x}  "
                      f"size={e['e_size']}  scnndx={e['e_scnndx']}  "
                      f"{etype:<8}  '{e['e_exnam']}'")

    # Debug info (brief)
    if hdr['c_debptr']:
        dbgs = parse_debug_ctrl(data, hdr['c_debptr'])
        print(f"\n[Debug Control Info]  ({len(dbgs)} module(s))")
        for i, d in enumerate(dbgs):
            print(f"  [{i}] @ 0x{d['_offset']:08x}  "
                  f"files={d['d_flcnt']}  syms={d['d_sycnt']}")

    print()


def _parse_sections_abs(data, first_ptr):
    """Parse sections using absolute file offsets."""
    sections = []
    ptr = first_ptr
    while ptr:
        sec = parse_section(data, ptr)
        sec['_offset'] = ptr
        sections.append(sec)
        ptr = sec['s_nxptr']
    return sections


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <file.srf>", file=sys.stderr)
        sys.exit(1)

    path = sys.argv[1]
    with open(path, 'rb') as f:
        data = f.read()

    print(f"File: {path}  ({len(data)} bytes)")

    # Reject ELF files immediately.
    if data[:4] == b'\x7fELF':
        print(f"ERROR: {path}: ELF file detected — expected SRF33 input", file=sys.stderr)
        sys.exit(1)

    # Detect file type by first 2 bytes (c_fatt, big-endian)
    if len(data) < 16:
        print("ERROR: file too short to be a valid SRF file", file=sys.stderr)
        sys.exit(1)

    c_fatt = struct.unpack_from('>H', data, 0)[0]

    if c_fatt & C_FATT_LIB:
        print("NOTE: This appears to be a library file (c_fatt & 0x0010).")
        print("      Library file support not yet implemented.")
        sys.exit(0)

    c_ver = struct.unpack_from('>H', data, 4)[0]
    if c_ver != 0x3300:
        print(f"WARNING: unexpected c_ver=0x{c_ver:04x} (expected 0x3300)", file=sys.stderr)

    dump_srf(data, base=0)


if __name__ == '__main__':
    main()
