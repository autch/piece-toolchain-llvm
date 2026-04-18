#!/usr/bin/env python3
"""elf2srf.py — Convert ELF32 (S1C33) object file to SRF33 object file.

Inverse of srf2elf.py. Converts ELF32 relocatable objects produced by
    clang --target=s1c33-none-elf -c
to SRF33 objects compatible with the EPSON lk33 linker.

SRF format reference: S1C33 C Compiler Package Manual Appendix A-1.
Errata: e_scnndx is 2 bytes (not 4 as stated in the manual).

Section mapping:
    .text             → CODE (s_scntyp=0x01)
    .rodata           → CODE (appended after .text)
    .data             → DATA (s_scntyp=0x02)
    .bss              → BSS  (s_scntyp=0x03)

Usage:
    python3 elf2srf.py input.o [-o output.srf.o]
"""

import sys
import struct
import os
import argparse

# ---------------------------------------------------------------------------
# ELF32 constants (little-endian input)
# ---------------------------------------------------------------------------

ET_REL        = 1
ET_EXEC       = 2
ET_DYN        = 3
EM_SE_C33     = 107
SHT_NULL      = 0
SHT_PROGBITS  = 1
SHT_SYMTAB    = 2
SHT_STRTAB    = 3
SHT_RELA      = 4
SHT_REL       = 9
SHT_NOBITS    = 8

# R_S1C33 type number for ABS32 (the only type with an implicit addend in data)
R_S1C33_32 = 2
SHF_EXECINSTR = 0x4
STT_SECTION   = 3
STT_FILE      = 4
STB_LOCAL     = 0
STB_GLOBAL    = 1
STB_WEAK      = 2
SHN_UNDEF     = 0
SHN_ABS       = 0xfff1
SHN_COMMON    = 0xfff2

SHF_ALLOC     = 0x2

# ---------------------------------------------------------------------------
# SRF33 constants (big-endian output)
# ---------------------------------------------------------------------------

C_FATT_RELOC = 0x0001
C_FATT_ABS   = 0x0002
C_FATT_EXEC  = 0x0004

SCNTYP_CODE  = 0x01
SCNTYP_DATA  = 0x02
SCNTYP_BSS   = 0x03

# e_extyp values in extern table entries
EXTYP_GLOBAL = 0x01   # defined, globally exported
EXTYP_LOCAL  = 0x02   # defined in this module (local or global — "defined here")
EXTYP_EXTERN = 0x03   # undefined external reference

# SRF section IDs used in s_scnndx / r_scnndx
SCN_ID_CODE  = 1
SCN_ID_DATA  = 2
SCN_ID_BSS   = 3

CTRL_HDR_SIZE = 16   # SRF control header
SCN_HDR_SIZE  = 44   # SRF section header
SRF_RELOC_SIZE = 16  # SRF relocation entry
SRF_EXTERN_FIXED = 13  # fixed bytes per extern entry (before name)

# ELF R_S1C33_* type number → SRF reloc type
# (Inverse of SRF_RELOC_TO_ELF in srf2elf.py)
ELF_TO_SRF_RELOC = {
    1: 0x0001,   # R_S1C33_REL8  → REL8
    2: 0x000a,   # R_S1C33_32    → ABS32
    3: 0x0007,   # R_S1C33_ABS_H → ABS_H
    4: 0x0008,   # R_S1C33_ABS_M → ABS_M
    5: 0x0009,   # R_S1C33_ABS_L → ABS_L
    6: 0x0005,   # R_S1C33_REL21 → REL_AH
    7: 0x0002,   # R_S1C33_REL_H → REL_H
    8: 0x0003,   # R_S1C33_REL_M → REL_M
    9: 0x0004,   # R_S1C33_REL_L → REL_L
}

# ---------------------------------------------------------------------------
# ELF parsing
# ---------------------------------------------------------------------------

def parse_elf(data):
    """Parse ELF32 LE relocatable object. Returns dict with parsed contents."""
    if len(data) < 52:
        raise ValueError("file too short for ELF header")
    if data[:4] != b'\x7fELF':
        raise ValueError("not an ELF file")
    if data[4] != 1:
        raise ValueError("not ELF32 (ELFCLASS32)")
    if data[5] != 1:
        raise ValueError("not little-endian ELF (ELFDATA2LSB)")

    e_type,   = struct.unpack_from('<H', data, 16)
    e_machine, = struct.unpack_from('<H', data, 18)

    if e_type != ET_REL:
        type_names = {ET_EXEC: 'ET_EXEC (linked executable)',
                      ET_DYN:  'ET_DYN (shared library)'}
        desc = type_names.get(e_type, f'e_type=0x{e_type:x}')
        raise ValueError(
            f"input is {desc}; elf2srf only supports ET_REL relocatable objects (.o)")

    if e_machine != EM_SE_C33:
        raise ValueError(f"unexpected e_machine=0x{e_machine:x}, expected EM_SE_C33 (107)")

    e_shoff,    = struct.unpack_from('<I', data, 32)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHH', data, 46)

    if e_shoff == 0 or e_shnum == 0:
        raise ValueError("no section headers")

    def read_shdr(i):
        o = e_shoff + i * e_shentsize
        flds = struct.unpack_from('<IIIIIIIIII', data, o)
        keys = ('sh_name', 'sh_type', 'sh_flags', 'sh_addr', 'sh_offset',
                'sh_size', 'sh_link', 'sh_info', 'sh_addralign', 'sh_entsize')
        return dict(zip(keys, flds))

    # Section name string table
    shstrtab_hdr = read_shdr(e_shstrndx)
    shstrtab = data[shstrtab_hdr['sh_offset']:
                    shstrtab_hdr['sh_offset'] + shstrtab_hdr['sh_size']]

    def shdr_name(hdr):
        off = hdr['sh_name']
        end = shstrtab.index(b'\x00', off)
        return shstrtab[off:end].decode('ascii', errors='replace')

    shdrs = []
    for i in range(e_shnum):
        h = read_shdr(i)
        h['index'] = i
        h['name']  = shdr_name(h)
        if h['sh_type'] != SHT_NOBITS:
            h['data'] = data[h['sh_offset'] : h['sh_offset'] + h['sh_size']]
        else:
            h['data'] = b''
        shdrs.append(h)

    # Symbol table
    symtab_hdr = next((h for h in shdrs if h['sh_type'] == SHT_SYMTAB), None)
    if symtab_hdr is None:
        raise ValueError("no .symtab section")

    strtab_hdr = shdrs[symtab_hdr['sh_link']]
    strtab = strtab_hdr['data']

    sym_entsize = symtab_hdr['sh_entsize'] or 16
    sym_count   = symtab_hdr['sh_size'] // sym_entsize
    symbols = []
    for i in range(sym_count):
        o = symtab_hdr['sh_offset'] + i * sym_entsize
        st_name, st_value, st_size, st_info, st_other, st_shndx = \
            struct.unpack_from('<IIIBBH', data, o)
        name_end = strtab.index(b'\x00', st_name)
        sym_name = strtab[st_name:name_end].decode('ascii', errors='replace')
        symbols.append({
            'name':  sym_name,
            'value': st_value,
            'size':  st_size,
            'bind':  st_info >> 4,
            'type':  st_info & 0xf,
            'shndx': st_shndx,
        })

    # Find key sections by name (take first match for each)
    def find_shdr(name):
        for h in shdrs:
            if h['name'] == name:
                return h
        return None

    text_hdr   = find_shdr('.text')
    rodata_hdr = find_shdr('.rodata')
    data_hdr   = find_shdr('.data')
    bss_hdr    = find_shdr('.bss')

    text_idx   = text_hdr['index']   if text_hdr   else -1
    rodata_idx = rodata_hdr['index'] if rodata_hdr else -1
    data_idx   = data_hdr['index']   if data_hdr   else -1
    bss_idx    = bss_hdr['index']    if bss_hdr    else -1

    # Collect relocations for each target section
    def find_rels_for(target_idx, section_data=None):
        """Find all SHT_REL/RELA sections whose sh_info == target_idx.

        For SHT_REL, the addend is implicit:
          - R_S1C33_32 (ABS32): the 4-byte LE value stored in section_data at r_offset
          - all other types: 0 (S1C33 instruction fields are zeroed before relocation)
        For SHT_RELA, the addend is taken directly from the r_addend field.
        """
        rels = []
        for h in shdrs:
            if h['sh_info'] != target_idx:
                continue
            if h['sh_type'] == SHT_REL:
                entsize = h['sh_entsize'] or 8
                for j in range(h['sh_size'] // entsize):
                    o = h['sh_offset'] + j * entsize
                    r_offset, r_info = struct.unpack_from('<II', data, o)
                    r_type = r_info & 0xff
                    addend = 0
                    if r_type == R_S1C33_32 and section_data is not None:
                        if r_offset + 4 <= len(section_data):
                            addend, = struct.unpack_from('<i', section_data, r_offset)
                    rels.append({
                        'offset': r_offset,
                        'sym':    r_info >> 8,
                        'type':   r_type,
                        'addend': addend,
                    })
            elif h['sh_type'] == SHT_RELA:
                entsize = h['sh_entsize'] or 12
                for j in range(h['sh_size'] // entsize):
                    o = h['sh_offset'] + j * entsize
                    r_offset, r_info, r_addend = struct.unpack_from('<IIi', data, o)
                    rels.append({
                        'offset': r_offset,
                        'sym':    r_info >> 8,
                        'type':   r_info & 0xff,
                        'addend': r_addend,
                    })
        return rels

    text_data_bytes   = text_hdr['data']   if text_hdr   else b''
    rodata_data_bytes = rodata_hdr['data'] if rodata_hdr else b''
    data_data_bytes   = data_hdr['data']   if data_hdr   else b''

    text_rels   = find_rels_for(text_idx,   text_data_bytes)   if text_idx   >= 0 else []
    rodata_rels = find_rels_for(rodata_idx, rodata_data_bytes) if rodata_idx >= 0 else []
    data_rels   = find_rels_for(data_idx,   data_data_bytes)   if data_idx   >= 0 else []

    return {
        'symbols':     symbols,
        'shdrs':       shdrs,
        'text_hdr':    text_hdr,
        'rodata_hdr':  rodata_hdr,
        'data_hdr':    data_hdr,
        'bss_hdr':     bss_hdr,
        'text_idx':    text_idx,
        'rodata_idx':  rodata_idx,
        'data_idx':    data_idx,
        'bss_idx':     bss_idx,
        'text_rels':   text_rels,
        'rodata_rels': rodata_rels,
        'data_rels':   data_rels,
    }


# ---------------------------------------------------------------------------
# SRF packing helpers (big-endian output)
# ---------------------------------------------------------------------------

def pack_ctrl_hdr(c_fatt, c_pentry, c_ver, c_scncnt, c_scnptr, c_debptr):
    """16-byte SRF control header."""
    return (struct.pack('>HHHH', c_fatt, c_pentry, c_ver, c_scncnt) +
            struct.pack('>II',   c_scnptr, c_debptr))


def pack_scn_hdr(s_nxptr, s_scntyp, s_lnktyp, s_scnatt, s_off,
                 s_rcptr, s_rcsiz, s_exptr, s_exsiz, s_excnt,
                 s_rdptr, s_dsiz, s_scnndx):
    """44-byte SRF section header."""
    return (struct.pack('>I',     s_nxptr) +
            struct.pack('>HHH',   s_scntyp, s_lnktyp, s_scnatt) +
            struct.pack('>I',     s_off) +
            struct.pack('>IIIII', s_rcptr, s_rcsiz, s_exptr, s_exsiz, s_excnt) +
            struct.pack('>II',    s_rdptr, s_dsiz) +
            struct.pack('>H',     s_scnndx))


def pack_reloc(r_type, r_scnoff, r_exndx, r_scnndx, r_symoff):
    """16-byte SRF relocation entry."""
    return (struct.pack('>H', r_type) +
            struct.pack('>I', r_scnoff) +
            struct.pack('>I', r_exndx) +
            struct.pack('>H', r_scnndx) +
            struct.pack('>I', r_symoff))


def pack_extern(e_scnoff, e_size, e_scnndx, e_extyp, name):
    """Variable-length SRF extern entry (13 + len(name) bytes)."""
    name_bytes = name.encode('ascii', errors='replace')
    return (struct.pack('>I', e_scnoff) +
            struct.pack('>I', e_size) +
            struct.pack('>H', e_scnndx) +    # errata: 2 bytes, not 4
            struct.pack('>H', e_extyp) +
            struct.pack('>B', len(name_bytes)) +
            name_bytes)


# ---------------------------------------------------------------------------
# Conversion logic
# ---------------------------------------------------------------------------

def elf_to_srf(elf_data):
    """Convert ELF32 LE relocatable object to SRF33. Returns bytes."""

    elf = parse_elf(elf_data)
    symbols    = elf['symbols']
    text_idx   = elf['text_idx']
    rodata_idx = elf['rodata_idx']
    data_idx   = elf['data_idx']
    bss_idx    = elf['bss_idx']

    # Raw section data
    text_data   = elf['text_hdr']['data']   if elf['text_hdr']   else b''
    rodata_data = elf['rodata_hdr']['data'] if elf['rodata_hdr'] else b''
    data_data   = elf['data_hdr']['data']   if elf['data_hdr']   else b''
    bss_size    = elf['bss_hdr']['sh_size'] if elf['bss_hdr']    else 0

    # .rodata is merged into CODE section, appended after .text
    rodata_base = len(text_data)
    code_data   = text_data + rodata_data

    # ------------------------------------------------------------------
    # Extern table construction
    #
    # Each SRF section has its own extern list.  A relocation's r_exndx
    # indexes into the containing section's extern list; r_scnndx gives
    # the target section's s_scnndx (for defined symbols) or 0.
    #
    # Strategy:
    #   1. Add all globally-visible defined symbols to the extern list of
    #      their defining section (so the linker can see them).
    #   2. When processing relocations, add any not-yet-present symbol to
    #      the relocation's section extern list on demand.
    # ------------------------------------------------------------------

    def shndx_to_scn_id(shndx):
        """Map ELF section index → SRF section ID (0 = undefined/abs)."""
        if shndx in (text_idx, rodata_idx):
            return SCN_ID_CODE
        elif shndx == data_idx:
            return SCN_ID_DATA
        elif shndx == bss_idx:
            return SCN_ID_BSS
        else:
            return 0

    def sym_offset_in_section(sym, scn_id):
        """Symbol's byte offset within the merged SRF section."""
        shndx = sym['shndx']
        val   = sym['value']
        if scn_id == SCN_ID_CODE:
            if shndx == rodata_idx:
                return val + rodata_base
        return val

    # Per-section extern lists: list of entry dicts.
    # sym_to_ext[scn_id][sym_idx] → index within that section's list.
    ext_lists   = {SCN_ID_CODE: [], SCN_ID_DATA: [], SCN_ID_BSS: []}
    sym_to_ext  = {SCN_ID_CODE: {}, SCN_ID_DATA: {}, SCN_ID_BSS: {}}

    def ensure_extern(scn_id, sym_idx, first_ref_offset=None):
        """Add symbol to section's extern list if not already present.
        Returns the index within that section's extern list."""
        sym_map  = sym_to_ext[scn_id]
        ext_list = ext_lists[scn_id]

        if sym_idx in sym_map:
            # Update e_scnoff for EXTERN if we have a reference offset and
            # the entry was inserted without one.
            if first_ref_offset is not None:
                entry = ext_list[sym_map[sym_idx]]
                if entry['e_extyp'] == EXTYP_EXTERN and entry['e_scnoff'] == 0:
                    entry['e_scnoff'] = first_ref_offset
            return sym_map[sym_idx]

        sym = symbols[sym_idx]

        # Skip unnamed symbols (null, section symbols without a label)
        if not sym['name']:
            return None

        shndx = sym['shndx']

        if shndx in (SHN_UNDEF, SHN_COMMON):
            # Undefined external reference
            e_extyp  = EXTYP_EXTERN
            e_scnoff = first_ref_offset if first_ref_offset is not None else 0
            e_size   = 0
            e_scnndx = 0
        else:
            # Defined symbol
            e_extyp  = EXTYP_LOCAL      # "defined here"; per user spec §3
            sym_scn  = shndx_to_scn_id(shndx)
            e_scnoff = sym_offset_in_section(sym, sym_scn)
            e_size   = sym['size']
            e_scnndx = sym_scn

        idx = len(ext_list)
        ext_list.append({
            'name':     sym['name'],
            'e_scnoff': e_scnoff,
            'e_size':   e_size,
            'e_scnndx': e_scnndx,
            'e_extyp':  e_extyp,
        })
        sym_map[sym_idx] = idx
        return idx

    # Pass 1: add all globally visible defined symbols
    for i, sym in enumerate(symbols):
        if not sym['name']:
            continue
        if sym['type'] in (STT_FILE,):
            continue
        bind  = sym['bind']
        shndx = sym['shndx']
        if shndx in (SHN_UNDEF, SHN_COMMON, SHN_ABS):
            continue    # not defined here
        if bind in (STB_GLOBAL, STB_WEAK):
            scn_id = shndx_to_scn_id(shndx)
            if scn_id:
                ensure_extern(scn_id, i)

    # Pass 2: process relocations, collecting SRF reloc entries
    def process_relocs(elf_rels, scn_offset_adj, scn_id):
        """Convert ELF relocations to SRF relocation entries for one section.
        scn_offset_adj: byte offset of the ELF section within the SRF section
                        (non-zero for .rodata which follows .text in CODE).
        Returns list of packed 16-byte SRF reloc entries.
        """
        srf_rels = []
        for rel in elf_rels:
            sym_idx  = rel['sym']
            elf_type = rel['type']

            srf_type = ELF_TO_SRF_RELOC.get(elf_type)
            if srf_type is None:
                print(f"WARNING: unknown ELF reloc type {elf_type}", file=sys.stderr)
                continue

            sym = symbols[sym_idx]

            # Unnamed symbol (section symbol) — try to synthesize a name
            if not sym['name']:
                sym_scn_id = shndx_to_scn_id(sym['shndx'])
                if sym_scn_id == SCN_ID_CODE:
                    sym['name'] = '.text'
                elif sym_scn_id == SCN_ID_DATA:
                    sym['name'] = '.data'
                elif sym_scn_id == SCN_ID_BSS:
                    sym['name'] = '.bss'
                else:
                    print(f"WARNING: reloc references unnamed/unknown symbol {sym_idx}",
                          file=sys.stderr)
                    continue

            r_scnoff = rel['offset'] + scn_offset_adj

            # Add symbol to this section's extern list
            ext_idx = ensure_extern(scn_id, sym_idx, first_ref_offset=r_scnoff)
            if ext_idx is None:
                print(f"WARNING: could not add extern for sym {sym_idx}", file=sys.stderr)
                continue

            # r_scnndx: the SRF section ID where this symbol is defined (0=undef)
            sym_scn_id = shndx_to_scn_id(sym['shndx'])

            # r_symoff: the relocation addend (NOT the symbol's section offset,
            # which is already captured in the extern entry's e_scnoff).
            # lk33 computes: target = (section_base + e_scnoff) + r_symoff
            r_symoff = rel.get('addend', 0) & 0xFFFFFFFF

            srf_rels.append(pack_reloc(srf_type, r_scnoff, ext_idx,
                                        sym_scn_id, r_symoff))
        return srf_rels

    code_rels = (process_relocs(elf['text_rels'],   0,           SCN_ID_CODE) +
                 process_relocs(elf['rodata_rels'], rodata_base, SCN_ID_CODE))
    data_rels = process_relocs(elf['data_rels'], 0, SCN_ID_DATA)

    # Serialize extern lists
    def build_externs(scn_id):
        out = b''
        for entry in ext_lists[scn_id]:
            out += pack_extern(entry['e_scnoff'], entry['e_size'],
                               entry['e_scnndx'], entry['e_extyp'],
                               entry['name'])
        return out

    code_rels_bin = b''.join(code_rels)
    code_ext_bin  = build_externs(SCN_ID_CODE)
    data_rels_bin = b''.join(data_rels)
    data_ext_bin  = build_externs(SCN_ID_DATA)
    bss_ext_bin   = build_externs(SCN_ID_BSS)

    # ------------------------------------------------------------------
    # Layout:
    #   [0]              Control header          (16 bytes)
    #   [16]             CODE section header     (44 bytes)
    #   [60]             DATA section header     (44 bytes)
    #   [104]            BSS  section header     (44 bytes)
    #   [148]            CODE relocs             (N × 16 bytes)
    #   [148+Rcode]      CODE externs            (variable)
    #   [...]            CODE data               (len(code_data) bytes)
    #   [...]            DATA relocs             (M × 16 bytes)
    #   [...]            DATA externs            (variable)
    #   [...]            DATA data               (len(data_data) bytes)
    #   [...]            BSS  externs            (variable)
    # ------------------------------------------------------------------

    out = bytearray(CTRL_HDR_SIZE + 3 * SCN_HDR_SIZE)  # reserve header region

    # CODE section payload
    code_rc_ptr = len(out) if code_rels_bin else 0
    out.extend(code_rels_bin)

    code_ex_ptr = len(out) if code_ext_bin else 0
    out.extend(code_ext_bin)

    code_rd_ptr = len(out) if code_data else 0
    out.extend(code_data)

    # DATA section payload
    data_rc_ptr = len(out) if data_rels_bin else 0
    out.extend(data_rels_bin)

    data_ex_ptr = len(out) if data_ext_bin else 0
    out.extend(data_ext_bin)

    data_rd_ptr = len(out) if data_data else 0
    out.extend(data_data)

    # BSS section payload (externs only; no raw data, no relocs)
    bss_ex_ptr = len(out) if bss_ext_bin else 0
    out.extend(bss_ext_bin)

    # Section header offsets
    CODE_HDR_OFF = CTRL_HDR_SIZE
    DATA_HDR_OFF = CTRL_HDR_SIZE +     SCN_HDR_SIZE
    BSS_HDR_OFF  = CTRL_HDR_SIZE + 2 * SCN_HDR_SIZE

    # Fill in control header
    out[0:CTRL_HDR_SIZE] = pack_ctrl_hdr(
        c_fatt   = C_FATT_RELOC,
        c_pentry = 0x0000,
        c_ver    = 0x3300,
        c_scncnt = 3,
        c_scnptr = CODE_HDR_OFF,   # first section header immediately follows
        c_debptr = 0x00000000,
    )

    # CODE section header
    out[CODE_HDR_OFF : CODE_HDR_OFF + SCN_HDR_SIZE] = pack_scn_hdr(
        s_nxptr  = DATA_HDR_OFF,
        s_scntyp = SCNTYP_CODE,
        s_lnktyp = 0,
        s_scnatt = 0,
        s_off    = 0,
        s_rcptr  = code_rc_ptr,
        s_rcsiz  = len(code_rels_bin),
        s_exptr  = code_ex_ptr,
        s_exsiz  = len(code_ext_bin),
        s_excnt  = len(ext_lists[SCN_ID_CODE]),
        s_rdptr  = code_rd_ptr,
        s_dsiz   = len(code_data),
        s_scnndx = SCN_ID_CODE,
    )

    # DATA section header
    out[DATA_HDR_OFF : DATA_HDR_OFF + SCN_HDR_SIZE] = pack_scn_hdr(
        s_nxptr  = BSS_HDR_OFF,
        s_scntyp = SCNTYP_DATA,
        s_lnktyp = 0,
        s_scnatt = 0,
        s_off    = 0,
        s_rcptr  = data_rc_ptr,
        s_rcsiz  = len(data_rels_bin),
        s_exptr  = data_ex_ptr,
        s_exsiz  = len(data_ext_bin),
        s_excnt  = len(ext_lists[SCN_ID_DATA]),
        s_rdptr  = data_rd_ptr,
        s_dsiz   = len(data_data),
        s_scnndx = SCN_ID_DATA,
    )

    # BSS section header (last: s_nxptr = 0)
    out[BSS_HDR_OFF : BSS_HDR_OFF + SCN_HDR_SIZE] = pack_scn_hdr(
        s_nxptr  = 0,
        s_scntyp = SCNTYP_BSS,
        s_lnktyp = 0,
        s_scnatt = 0,
        s_off    = 0,
        s_rcptr  = 0,
        s_rcsiz  = 0,
        s_exptr  = bss_ex_ptr,
        s_exsiz  = len(bss_ext_bin),
        s_excnt  = len(ext_lists[SCN_ID_BSS]),
        s_rdptr  = 0,
        s_dsiz   = bss_size,
        s_scnndx = SCN_ID_BSS,
    )

    return bytes(out)


# ---------------------------------------------------------------------------
# ET_EXEC → C_FATT_EXEC SRF conversion
# ---------------------------------------------------------------------------

def exec_elf_to_srf(elf_data):
    """Convert ELF32 LE linked executable (ET_EXEC) to SRF33 C_FATT_EXEC. Returns bytes.

    Section grouping:
      SHF_EXECINSTR + SHF_ALLOC              → SRF CODE section
      SHT_PROGBITS  + SHF_ALLOC (no EXECINSTR) → SRF DATA section (.rodata, .data, …)
      SHT_NOBITS    + SHF_ALLOC              → SRF BSS section

    Multiple sections within each group are merged in address order; gaps
    between them are zero-filled.  s_off is set to the actual load VMA.

    Symbol table entries are preserved as LOCAL extern entries with
    e_scnoff = absolute address (st_value).  No relocation entries are
    emitted because all references are already resolved in the linked ELF.
    """
    e_entry, = struct.unpack_from('<I', elf_data, 24)
    e_phoff,  = struct.unpack_from('<I', elf_data, 28)
    e_shoff,  = struct.unpack_from('<I', elf_data, 32)
    e_phentsize, e_phnum = struct.unpack_from('<HH', elf_data, 42)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHH', elf_data, 46)

    # Build a VMA→LMA mapping from PT_LOAD program headers so that sections
    # placed with `AT > ...` in the linker script (e.g. .fastrun / .fastdata
    # whose VMA is in internal RAM but whose bytes live in SRAM) are merged
    # by their storage address, not their runtime address.  Using sh_addr
    # alone would collapse sections from disjoint memory regions into one
    # huge zero-filled range and emit the wrong s_off.
    PT_LOAD_CONST = 1
    load_segments = []  # list of (p_vaddr, p_paddr, p_memsz)
    for i in range(e_phnum):
        o = e_phoff + i * e_phentsize
        p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = \
            struct.unpack_from('<IIIIIIII', elf_data, o)
        if p_type == PT_LOAD_CONST and p_memsz > 0:
            load_segments.append((p_vaddr, p_paddr, p_memsz))

    def vma_to_lma(vma):
        """Map a VMA to its load (storage) address via PT_LOAD segments.
        Falls back to VMA when no covering segment is found."""
        for vaddr, paddr, memsz in load_segments:
            if vaddr <= vma < vaddr + memsz:
                return paddr + (vma - vaddr)
        return vma

    def read_shdr(i):
        o = e_shoff + i * e_shentsize
        flds = struct.unpack_from('<IIIIIIIIII', elf_data, o)
        keys = ('sh_name', 'sh_type', 'sh_flags', 'sh_addr', 'sh_offset',
                'sh_size', 'sh_link', 'sh_info', 'sh_addralign', 'sh_entsize')
        return dict(zip(keys, flds))

    shstrtab_hdr = read_shdr(e_shstrndx)
    shstrtab = elf_data[shstrtab_hdr['sh_offset']:
                        shstrtab_hdr['sh_offset'] + shstrtab_hdr['sh_size']]

    def shdr_name(hdr):
        n = hdr['sh_name']
        end = shstrtab.index(b'\x00', n)
        return shstrtab[n:end].decode('ascii', errors='replace')

    code_shdrs = []   # SHF_EXECINSTR + SHF_ALLOC
    data_shdrs = []   # SHT_PROGBITS  + SHF_ALLOC  (no EXECINSTR)
    bss_shdrs  = []   # SHT_NOBITS    + SHF_ALLOC
    symtab_hdr = None

    for i in range(e_shnum):
        h = read_shdr(i)
        h['index'] = i
        h['name']  = shdr_name(h)

        if h['sh_type'] == SHT_SYMTAB:
            symtab_hdr = h
            continue

        if not (h['sh_flags'] & SHF_ALLOC) or h['sh_size'] == 0:
            continue

        # Record both the runtime VMA (sh_addr) and the storage LMA.
        # For normally-linked sections the two coincide; for sections placed
        # with `AT > ...` in the linker script they differ.
        h['load_addr'] = vma_to_lma(h['sh_addr'])

        if h['sh_flags'] & SHF_EXECINSTR:
            h['data'] = elf_data[h['sh_offset'] : h['sh_offset'] + h['sh_size']]
            code_shdrs.append(h)
        elif h['sh_type'] == SHT_PROGBITS:
            h['data'] = elf_data[h['sh_offset'] : h['sh_offset'] + h['sh_size']]
            data_shdrs.append(h)
        elif h['sh_type'] == SHT_NOBITS:
            h['data'] = b''
            bss_shdrs.append(h)

    def merge_sections(hdrs):
        """Merge CODE/DATA sections into (base_lma, bytes) using LMA.
        Gaps are zero-filled.  SRF's s_off is the load (storage) address."""
        if not hdrs:
            return 0, b''
        hdrs = sorted(hdrs, key=lambda h: h['load_addr'])
        base = hdrs[0]['load_addr']
        end  = max(h['load_addr'] + h['sh_size'] for h in hdrs)
        buf  = bytearray(end - base)
        for h in hdrs:
            off = h['load_addr'] - base
            buf[off : off + len(h['data'])] = h['data']
        return base, bytes(buf)

    def bss_extent(hdrs):
        """Return (base_vma, total_size) for BSS sections.
        BSS has no storage (NOBITS); the runtime VMA is what the loader must
        zero, so we use sh_addr here, not LMA."""
        if not hdrs:
            return 0, 0
        hdrs = sorted(hdrs, key=lambda h: h['sh_addr'])
        base = hdrs[0]['sh_addr']
        end  = max(h['sh_addr'] + h['sh_size'] for h in hdrs)
        return base, end - base

    code_addr, code_data = merge_sections(code_shdrs)
    data_addr, data_data = merge_sections(data_shdrs)
    bss_addr,  bss_size  = bss_extent(bss_shdrs)

    # Map ELF section index → SRF section ID
    shndx_to_scn = {}
    for h in code_shdrs:
        shndx_to_scn[h['index']] = SCN_ID_CODE
    for h in data_shdrs:
        shndx_to_scn[h['index']] = SCN_ID_DATA
    for h in bss_shdrs:
        shndx_to_scn[h['index']] = SCN_ID_BSS

    # Build extern lists from ELF symbol table.
    # For a C_FATT_EXEC SRF, e_scnoff = absolute address (st_value).
    ext_lists = {SCN_ID_CODE: [], SCN_ID_DATA: [], SCN_ID_BSS: []}

    if symtab_hdr:
        strtab_hdr  = read_shdr(symtab_hdr['sh_link'])
        strtab_data = elf_data[strtab_hdr['sh_offset'] :
                               strtab_hdr['sh_offset'] + strtab_hdr['sh_size']]
        sym_entsize = symtab_hdr['sh_entsize'] or 16
        sym_data    = elf_data[symtab_hdr['sh_offset'] :
                               symtab_hdr['sh_offset'] + symtab_hdr['sh_size']]

        for i in range(len(sym_data) // sym_entsize):
            o = i * sym_entsize
            st_name, st_value, st_size, st_info, _, st_shndx = \
                struct.unpack_from('<IIIBBH', sym_data, o)
            st_type = st_info & 0xf

            if st_type in (STT_FILE, STT_SECTION):
                continue
            if st_shndx in (SHN_UNDEF, SHN_ABS, SHN_COMMON, 0):
                continue

            nend = strtab_data.index(b'\x00', st_name)
            name = strtab_data[st_name:nend].decode('ascii', errors='replace')
            if not name:
                continue

            scn_id = shndx_to_scn.get(st_shndx)
            if scn_id is None:
                continue

            ext_lists[scn_id].append({
                'name':     name,
                'e_scnoff': st_value,   # absolute address
                'e_size':   st_size,
                'e_scnndx': scn_id,
                'e_extyp':  EXTYP_LOCAL,
            })

    def build_externs(scn_id):
        out = b''
        for e in ext_lists[scn_id]:
            out += pack_extern(e['e_scnoff'], e['e_size'],
                               e['e_scnndx'], e['e_extyp'], e['name'])
        return out

    code_ext_bin = build_externs(SCN_ID_CODE)
    data_ext_bin = build_externs(SCN_ID_DATA)
    bss_ext_bin  = build_externs(SCN_ID_BSS)

    # Layout (same structure as relocatable, no relocation entries)
    out = bytearray(CTRL_HDR_SIZE + 3 * SCN_HDR_SIZE)

    code_ex_ptr = len(out) if code_ext_bin else 0
    out.extend(code_ext_bin)
    code_rd_ptr = len(out) if code_data else 0
    out.extend(code_data)

    data_ex_ptr = len(out) if data_ext_bin else 0
    out.extend(data_ext_bin)
    data_rd_ptr = len(out) if data_data else 0
    out.extend(data_data)

    bss_ex_ptr = len(out) if bss_ext_bin else 0
    out.extend(bss_ext_bin)

    CODE_HDR_OFF = CTRL_HDR_SIZE
    DATA_HDR_OFF = CTRL_HDR_SIZE +     SCN_HDR_SIZE
    BSS_HDR_OFF  = CTRL_HDR_SIZE + 2 * SCN_HDR_SIZE

    # c_pentry is a 16-bit field in the SRF control header.
    # For P/ECE, the application entry is through the pceAPPHEAD function
    # table, so c_pentry is informational; store the low 16 bits of e_entry.
    c_pentry = e_entry & 0xFFFF

    out[0:CTRL_HDR_SIZE] = pack_ctrl_hdr(
        c_fatt   = C_FATT_EXEC | C_FATT_ABS,
        c_pentry = c_pentry,
        c_ver    = 0x3300,
        c_scncnt = 3,
        c_scnptr = CODE_HDR_OFF,
        c_debptr = 0,
    )

    out[CODE_HDR_OFF : CODE_HDR_OFF + SCN_HDR_SIZE] = pack_scn_hdr(
        s_nxptr  = DATA_HDR_OFF,
        s_scntyp = SCNTYP_CODE,
        s_lnktyp = 0,
        s_scnatt = 0,
        s_off    = code_addr,
        s_rcptr  = 0,
        s_rcsiz  = 0,
        s_exptr  = code_ex_ptr,
        s_exsiz  = len(code_ext_bin),
        s_excnt  = len(ext_lists[SCN_ID_CODE]),
        s_rdptr  = code_rd_ptr,
        s_dsiz   = len(code_data),
        s_scnndx = SCN_ID_CODE,
    )

    out[DATA_HDR_OFF : DATA_HDR_OFF + SCN_HDR_SIZE] = pack_scn_hdr(
        s_nxptr  = BSS_HDR_OFF,
        s_scntyp = SCNTYP_DATA,
        s_lnktyp = 0,
        s_scnatt = 0,
        s_off    = data_addr,
        s_rcptr  = 0,
        s_rcsiz  = 0,
        s_exptr  = data_ex_ptr,
        s_exsiz  = len(data_ext_bin),
        s_excnt  = len(ext_lists[SCN_ID_DATA]),
        s_rdptr  = data_rd_ptr,
        s_dsiz   = len(data_data),
        s_scnndx = SCN_ID_DATA,
    )

    out[BSS_HDR_OFF : BSS_HDR_OFF + SCN_HDR_SIZE] = pack_scn_hdr(
        s_nxptr  = 0,
        s_scntyp = SCNTYP_BSS,
        s_lnktyp = 0,
        s_scnatt = 0,
        s_off    = bss_addr,
        s_rcptr  = 0,
        s_rcsiz  = 0,
        s_exptr  = bss_ex_ptr,
        s_exsiz  = len(bss_ext_bin),
        s_excnt  = len(ext_lists[SCN_ID_BSS]),
        s_rdptr  = 0,
        s_dsiz   = bss_size,
        s_scnndx = SCN_ID_BSS,
    )

    return bytes(out)


# ---------------------------------------------------------------------------
# Diagnostic dump (--dump flag)
# ---------------------------------------------------------------------------

def dump_srf(data):
    """Print a human-readable summary of an SRF33 object to stdout."""
    c_fatt, c_pentry, c_ver, c_scncnt = struct.unpack_from('>HHHH', data, 0)
    c_scnptr, c_debptr = struct.unpack_from('>II', data, 8)
    print(f"Control header:")
    print(f"  c_fatt=0x{c_fatt:04x}  c_pentry=0x{c_pentry:04x}  "
          f"c_ver=0x{c_ver:04x}  c_scncnt={c_scncnt}")
    print(f"  c_scnptr=0x{c_scnptr:08x}  c_debptr=0x{c_debptr:08x}")

    SCNTYP_NAMES = {1: 'CODE', 2: 'DATA', 3: 'BSS'}
    EXTYP_NAMES  = {1: 'GLOBAL', 2: 'LOCAL', 3: 'EXTERN'}
    RELTYP_NAMES = {
        0x01: 'REL8',   0x02: 'REL_H',  0x03: 'REL_M',  0x04: 'REL_L',
        0x05: 'REL_AH', 0x06: 'REL_AL', 0x07: 'ABS_H',  0x08: 'ABS_M',
        0x09: 'ABS_L',  0x0a: 'ABS32',
    }

    ptr = c_scnptr
    sec_num = 0
    while ptr:
        s_nxptr, = struct.unpack_from('>I', data, ptr)
        s_scntyp, s_lnktyp, s_scnatt = struct.unpack_from('>HHH', data, ptr + 4)
        s_off, = struct.unpack_from('>I', data, ptr + 10)
        s_rcptr, s_rcsiz, s_exptr, s_exsiz, s_excnt = \
            struct.unpack_from('>IIIII', data, ptr + 14)
        s_rdptr, s_dsiz = struct.unpack_from('>II', data, ptr + 34)
        s_scnndx, = struct.unpack_from('>H', data, ptr + 42)

        tname = SCNTYP_NAMES.get(s_scntyp, f'0x{s_scntyp:02x}')
        print(f"\nSection[{sec_num}] {tname} (s_scnndx={s_scnndx}) @ 0x{ptr:08x}")
        print(f"  s_off=0x{s_off:08x}  s_dsiz={s_dsiz}  s_rdptr=0x{s_rdptr:08x}")
        print(f"  s_rcptr=0x{s_rcptr:08x}  s_rcsiz={s_rcsiz}  "
              f"s_exptr=0x{s_exptr:08x}  s_exsiz={s_exsiz}  s_excnt={s_excnt}")

        if s_excnt and s_exptr:
            print(f"  Externs ({s_excnt}):")
            off = s_exptr
            for i in range(s_excnt):
                e_scnoff, = struct.unpack_from('>I', data, off)
                e_size,   = struct.unpack_from('>I', data, off + 4)
                e_scnndx, = struct.unpack_from('>H', data, off + 8)
                e_extyp,  = struct.unpack_from('>H', data, off + 10)
                e_namsiz, = struct.unpack_from('>B', data, off + 12)
                name = data[off + 13 : off + 13 + e_namsiz].decode('ascii', errors='?')
                ename = EXTYP_NAMES.get(e_extyp, f'0x{e_extyp:02x}')
                print(f"    [{i:3d}] {ename:7s}  off=0x{e_scnoff:08x}  "
                      f"size={e_size:4d}  scnndx={e_scnndx}  '{name}'")
                off += 13 + e_namsiz

        if s_rcsiz and s_rcptr:
            n_rels = s_rcsiz // 16
            print(f"  Relocations ({n_rels}):")
            for i in range(n_rels):
                o = s_rcptr + i * 16
                r_type,   = struct.unpack_from('>H', data, o)
                r_scnoff, = struct.unpack_from('>I', data, o + 2)
                r_exndx,  = struct.unpack_from('>I', data, o + 6)
                r_scnndx, = struct.unpack_from('>H', data, o + 10)
                r_symoff, = struct.unpack_from('>I', data, o + 12)
                rname = RELTYP_NAMES.get(r_type, f'0x{r_type:04x}')
                print(f"    [{i:3d}] {rname:7s}  scnoff=0x{r_scnoff:08x}  "
                      f"exndx={r_exndx:3d}  scnndx={r_scnndx}  symoff=0x{r_symoff:08x}")

        ptr = s_nxptr
        sec_num += 1


# ---------------------------------------------------------------------------
# Round-trip check: convert output back with srf2elf and compare symbols
# ---------------------------------------------------------------------------

def _roundtrip_check(srf_data, orig_elf_data):
    """Best-effort: warn if any global symbol from ELF is absent in the SRF."""
    # Collect global defined ELF symbols
    elf_globals = set()
    e_shoff, = struct.unpack_from('<I', orig_elf_data, 32)
    e_shentsize, e_shnum, _ = struct.unpack_from('<HHH', orig_elf_data, 46)
    # Find symtab
    for i in range(e_shnum):
        o = e_shoff + i * e_shentsize
        sh_type, = struct.unpack_from('<I', orig_elf_data, o + 4)
        if sh_type == SHT_SYMTAB:
            sh_offset, sh_size, sh_link, sh_info = \
                struct.unpack_from('<IIII', orig_elf_data, o + 16)
            sh_entsize, = struct.unpack_from('<I', orig_elf_data, o + 36)
            sh_entsize = sh_entsize or 16
            # strtab
            so = e_shoff + sh_link * e_shentsize
            str_off, str_sz = struct.unpack_from('<II', orig_elf_data, so + 16)
            strtab = orig_elf_data[str_off : str_off + str_sz]
            count = sh_size // sh_entsize
            for j in range(sh_info, count):   # sh_info = first global
                eo = sh_offset + j * sh_entsize
                st_name, st_value, _, st_info, _, st_shndx = \
                    struct.unpack_from('<IIIBBH', orig_elf_data, eo)
                if (st_info >> 4) == STB_GLOBAL and st_shndx != SHN_UNDEF:
                    ne = strtab.index(b'\x00', st_name)
                    elf_globals.add(strtab[st_name:ne].decode('ascii', errors='?'))
            break

    # Collect all names from SRF extern tables
    srf_names = set()
    ptr = struct.unpack_from('>I', srf_data, 8)[0]
    while ptr:
        s_nxptr, = struct.unpack_from('>I', srf_data, ptr)
        s_exptr, _, s_excnt = struct.unpack_from('>III', srf_data, ptr + 22)
        s_excnt_actual, = struct.unpack_from('>I', srf_data, ptr + 30)
        # re-parse properly
        s_exptr, s_exsiz, s_excnt2 = struct.unpack_from('>III', srf_data, ptr + 22)
        off = s_exptr
        if off:
            for _ in range(s_excnt2):
                e_namsiz, = struct.unpack_from('>B', srf_data, off + 12)
                name = srf_data[off + 13 : off + 13 + e_namsiz].decode('ascii', errors='?')
                srf_names.add(name)
                off += 13 + e_namsiz
        ptr = s_nxptr

    missing = elf_globals - srf_names
    if missing:
        print(f"WARNING: round-trip check: {len(missing)} global symbol(s) "
              f"missing from SRF extern tables:", file=sys.stderr)
        for name in sorted(missing):
            print(f"  {name}", file=sys.stderr)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(
        description='Convert ELF32 S1C33 object or executable to SRF33 (inverse of srf2elf.py)')
    ap.add_argument('input',  help='Input ELF32 object (.o) or linked executable (.elf)')
    ap.add_argument('-o', '--output',
                    help='Output SRF33 file (default: input.srf.o for ET_REL, input.srf for ET_EXEC)')
    ap.add_argument('--dump', action='store_true',
                    help='Dump SRF structure of the output to stdout')
    ap.add_argument('--check', action='store_true',
                    help='Run round-trip symbol check after conversion (ET_REL only)')
    args = ap.parse_args()

    in_path  = args.input

    with open(in_path, 'rb') as f:
        elf_data = f.read()

    if elf_data[:4] != b'\x7fELF':
        # Give a clear error for archive types that srf2elf handles but elf2srf does not
        if elf_data[:9] == b'!<lib33>\x00':
            print(f"ERROR: {in_path}: lib33 archive (.lib) is not supported by elf2srf; "
                  "use srf2elf to convert .lib to .a", file=sys.stderr)
        elif elf_data[:8] == b'!<arch>\n':
            print(f"ERROR: {in_path}: Unix ar archive (.a) is not supported by elf2srf",
                  file=sys.stderr)
        else:
            print(f"ERROR: {in_path}: not an ELF file", file=sys.stderr)
        sys.exit(1)

    e_type = ET_REL
    if len(elf_data) >= 18:
        e_type, = struct.unpack_from('<H', elf_data, 16)

    if e_type not in (ET_REL, ET_EXEC):
        type_names = {ET_DYN: 'ET_DYN (shared library)'}
        desc = type_names.get(e_type, f'e_type=0x{e_type:x}')
        print(f"ERROR: {in_path}: input is {desc}; "
              "elf2srf supports ET_REL (.o) and ET_EXEC (.elf) only", file=sys.stderr)
        sys.exit(1)

    if e_type == ET_EXEC:
        out_path = args.output or (os.path.splitext(in_path)[0] + '.srf')
        try:
            srf_data = exec_elf_to_srf(elf_data)
        except Exception as e:
            print(f"ERROR: {in_path}: {e}", file=sys.stderr)
            import traceback; traceback.print_exc()
            sys.exit(1)
    else:
        out_path = args.output or (os.path.splitext(in_path)[0] + '.srf.o')
        try:
            srf_data = elf_to_srf(elf_data)
        except Exception as e:
            print(f"ERROR: {in_path}: {e}", file=sys.stderr)
            import traceback; traceback.print_exc()
            sys.exit(1)

    with open(out_path, 'wb') as f:
        f.write(srf_data)

    print(f"{in_path} → {out_path}  ({len(elf_data)} bytes ELF → {len(srf_data)} bytes SRF)")

    if args.dump:
        print()
        dump_srf(srf_data)

    if args.check and e_type == ET_REL:
        _roundtrip_check(srf_data, elf_data)


if __name__ == '__main__':
    main()
