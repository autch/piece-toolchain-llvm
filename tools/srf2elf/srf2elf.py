#!/usr/bin/env python3
"""srf2elf.py — Convert SRF33 object/library files to ELF32.

Converts a standalone SRF33 relocatable object (.o/.srf) to an ELF32
relocatable object, or a lib33 archive (.lib) to a Unix ar archive (.a),
suitable for linking with ld.lld for the S1C33 target.

Format reference: S5U1C33000C Manual Appendix pp.475-480
Errata: e_scnndx is 2 bytes (not 4 as stated in the manual).
lib33 format: docs/lib33_format.md (empirical, manual is inaccurate)

ELF target: EM_SE_C33 (107), 32-bit, little-endian, ET_REL

Usage:
    python3 srf2elf.py <input.srf>             # writes <input>.o
    python3 srf2elf.py <input.srf> <output.o>
    python3 srf2elf.py <input.lib>             # writes <input>.a
    python3 srf2elf.py <input.lib> <output.a>
"""

import sys
import struct
import os

# ---------------------------------------------------------------------------
# SRF parsing (big-endian)
# ---------------------------------------------------------------------------

C_FATT_RELOC = 0x0001
C_FATT_ABS   = 0x0002
C_FATT_EXEC  = 0x0004
C_FATT_LIB   = 0x0010

SCNTYP_CODE  = 1
SCNTYP_DATA  = 2
SCNTYP_BSS   = 3

EXTYP_GLOBAL = 1
EXTYP_LOCAL  = 2
EXTYP_EXTERN = 3

# SRF reloc type → ELF R_S1C33_* type
SRF_RELOC_TO_ELF = {
    0x0001: 1,   # REL8    → R_S1C33_REL8
    0x0002: 7,   # REL_H   → R_S1C33_REL_H
    0x0003: 8,   # REL_M   → R_S1C33_REL_M
    0x0004: 9,   # REL_L   → R_S1C33_REL_L
    0x0005: 10,  # REL_AH  → R_S1C33_REL_AH (2×ext+branch, ext1 imm13 = word_offset[25:13])
    0x0006: 11,  # REL_AL  → R_S1C33_REL_AL (2×ext+branch, ext2 imm13 = word_offset[12:0])
    0x0007: 3,   # ABS_H   → R_S1C33_ABS_H
    0x0008: 4,   # ABS_M   → R_S1C33_ABS_M
    0x0009: 5,   # ABS_L   → R_S1C33_ABS_L
    0x000a: 2,   # ABS32   → R_S1C33_32
}


def srf_parse(data, base=0):
    """Parse SRF object starting at base. Returns dict with all parsed info."""
    # Control header
    c_fatt, c_pentry, c_ver, c_scncnt = struct.unpack_from('>HHHH', data, base)
    c_scnptr, c_debptr = struct.unpack_from('>II', data, base + 8)

    obj = {
        'c_fatt':   c_fatt,
        'c_pentry': c_pentry,
        'c_ver':    c_ver,
        'c_scncnt': c_scncnt,
        'c_scnptr': c_scnptr,
        'c_debptr': c_debptr,
        'sections': [],
    }

    # Sections
    ptr = c_scnptr
    while ptr:
        s = _parse_section(data, ptr)
        s['relocations'] = _parse_relocs(data, s['s_rcptr'], s['s_rcsiz'])
        s['externs']     = _parse_externs(data, s['s_exptr'], s['s_excnt'])
        s['rawdata']     = _read_rawdata(data, s)
        obj['sections'].append(s)
        ptr = s['s_nxptr']

    return obj


def _parse_section(data, off):
    (s_nxptr,) = struct.unpack_from('>I', data, off)
    s_scntyp, s_lnktyp, s_scnatt = struct.unpack_from('>HHH', data, off + 4)
    (s_off,) = struct.unpack_from('>I', data, off + 10)
    s_rcptr, s_rcsiz, s_exptr, s_exsiz, s_excnt = struct.unpack_from('>IIIII', data, off + 14)
    s_rdptr, s_dsiz = struct.unpack_from('>II', data, off + 34)
    (s_scnndx,) = struct.unpack_from('>H', data, off + 42)
    return dict(s_nxptr=s_nxptr, s_scntyp=s_scntyp, s_lnktyp=s_lnktyp,
                s_scnatt=s_scnatt, s_off=s_off, s_rcptr=s_rcptr, s_rcsiz=s_rcsiz,
                s_exptr=s_exptr, s_exsiz=s_exsiz, s_excnt=s_excnt,
                s_rdptr=s_rdptr, s_dsiz=s_dsiz, s_scnndx=s_scnndx)


def _parse_relocs(data, rcptr, rcsiz):
    if not rcptr or not rcsiz:
        return []
    count = rcsiz // 16
    relocs = []
    for i in range(count):
        off = rcptr + i * 16
        (r_rctyp,) = struct.unpack_from('>H', data, off)
        (r_scnoff,) = struct.unpack_from('>I', data, off + 2)
        (r_exndx,)  = struct.unpack_from('>I', data, off + 6)
        (r_scnndx,) = struct.unpack_from('>H', data, off + 10)
        (r_symoff,) = struct.unpack_from('>I', data, off + 12)
        relocs.append(dict(r_rctyp=r_rctyp, r_scnoff=r_scnoff,
                           r_exndx=r_exndx, r_scnndx=r_scnndx, r_symoff=r_symoff))
    return relocs


def _parse_externs(data, exptr, excnt):
    if not exptr or not excnt:
        return []
    externs = []
    off = exptr
    for _ in range(excnt):
        (e_scnoff,) = struct.unpack_from('>I', data, off)
        (e_size,)   = struct.unpack_from('>I', data, off + 4)
        (e_scnndx,) = struct.unpack_from('>H', data, off + 8)   # errata: 2 bytes
        (e_extyp,)  = struct.unpack_from('>H', data, off + 10)
        (e_namsiz,) = struct.unpack_from('>B', data, off + 12)
        name = data[off + 13 : off + 13 + e_namsiz].decode('ascii', errors='replace')
        externs.append(dict(e_scnoff=e_scnoff, e_size=e_size, e_scnndx=e_scnndx,
                            e_extyp=e_extyp, e_namsiz=e_namsiz, e_exnam=name))
        off += 13 + e_namsiz
    return externs


def _read_rawdata(data, sec):
    if sec['s_rdptr'] and sec['s_dsiz'] and sec['s_scntyp'] != SCNTYP_BSS:
        return data[sec['s_rdptr'] : sec['s_rdptr'] + sec['s_dsiz']]
    return b''


# ---------------------------------------------------------------------------
# ELF32 builder (little-endian output, big-endian input from SRF)
# ---------------------------------------------------------------------------

# ELF constants
ET_REL       = 1
ET_EXEC      = 2
EM_SE_C33    = 107
EV_CURRENT   = 1
ELFCLASS32   = 1
ELFDATA2LSB  = 1   # little-endian
SHT_NULL     = 0
SHT_PROGBITS = 1
SHT_SYMTAB   = 2
SHT_STRTAB   = 3
SHT_RELA     = 4
SHT_NOBITS   = 8
SHT_REL      = 9
SHF_WRITE    = 0x1
SHF_ALLOC    = 0x2
SHF_EXECINSTR= 0x4
SHN_UNDEF    = 0
SHN_ABS      = 0xfff1
STB_LOCAL    = 0
STB_GLOBAL   = 1
STT_NOTYPE   = 0
STT_SECTION  = 3

EHDR_SIZE = 52
SHDR_SIZE = 40
SYM_SIZE  = 16
REL_SIZE  = 8
RELA_SIZE = 12


def pack_ehdr(e_type, e_machine, e_entry, e_phoff, e_shoff,
              e_flags, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx):
    ident = (b'\x7fELF' +
             bytes([ELFCLASS32, ELFDATA2LSB, EV_CURRENT, 0]) +
             b'\x00' * 8)
    return (ident +
            struct.pack('<HHIIIIIHHHHHH',
                        e_type, e_machine, EV_CURRENT,
                        e_entry, e_phoff, e_shoff,
                        e_flags, EHDR_SIZE,
                        e_phentsize, e_phnum,
                        e_shentsize, e_shnum, e_shstrndx))


def pack_shdr(sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size,
              sh_link, sh_info, sh_addralign, sh_entsize):
    return struct.pack('<IIIIIIIIII',
                       sh_name, sh_type, sh_flags, sh_addr,
                       sh_offset, sh_size, sh_link, sh_info,
                       sh_addralign, sh_entsize)


def pack_sym(st_name, st_value, st_size, st_info, st_other, st_shndx):
    return struct.pack('<IIIBBH', st_name, st_value, st_size, st_info, st_other, st_shndx)


def pack_rel(r_offset, r_info):
    return struct.pack('<II', r_offset, r_info)


def pack_rela(r_offset, r_info, r_addend):
    # r_addend is a signed 32-bit value (ELF Elf32_Sword).
    # Python's struct 'i' format handles the sign correctly.
    return struct.pack('<IIi', r_offset, r_info, r_addend)


class StringTable:
    def __init__(self):
        self._data = b'\x00'   # index 0 = empty string
        self._index = {}

    def add(self, s):
        if s not in self._index:
            self._index[s] = len(self._data)
            self._data += s.encode('ascii') + b'\x00'
        return self._index[s]

    def get(self, s):
        return self._index.get(s, 0)

    def data(self):
        return self._data


# ---------------------------------------------------------------------------
# lib33 archive parser (big-endian, format per docs/lib33_format.md)
# ---------------------------------------------------------------------------

LIB33_MAGIC = b'!<lib33>\x00'   # 9 bytes: "!<lib33>" + NUL terminator


def lib33_parse(data):
    """Parse a lib33 archive. Returns list of (filename, srf_bytes) in file order.

    Format (docs/lib33_format.md, confirmed by binary analysis):
      0x00-08: "!<lib33>\\0" magic (9 bytes, includes NUL terminator)
      0x09-0C: symtab_size (4 bytes BE) = 4 + syminfo_size + 4 + symname_size
      0x0D:    name_len (1 byte)
      0x0E...: name (name_len bytes, "SYMDEF")
      symtab at symtab_start = 0x0E + name_len (= 0x14 for "SYMDEF"):
        +0x00: syminfo_size (4 bytes BE) = N × 8
        +0x04: N entries × {module_pos (4 BE), name_off (4 BE)}
        +0x04+N×8: symname_size (4 bytes BE)
        +0x08+N×8: symnames (NUL-terminated strings, total = symname_size bytes)
      module entries immediately follow symnames
    Module entry format:
      srf_size  (4 bytes BE): size of SRF data
      name_len  (1 byte):     filename length (NOT including NUL; no NUL terminator)
      filename  (name_len bytes): filename (not NUL-terminated)
      srf_data  (srf_size bytes)
    """
    if len(data) < 9 or data[:9] != LIB33_MAGIC:
        raise ValueError("not a lib33 archive")

    name_len     = data[0x0D]
    symtab_start = 0x0E + name_len           # = 0x14 when name = "SYMDEF"

    syminfo_size, = struct.unpack_from('>I', data, symtab_start)
    syminfo_off   = symtab_start + 4         # = 0x18 when name = "SYMDEF"

    symname_size_off = syminfo_off + syminfo_size
    symname_size, = struct.unpack_from('>I', data, symname_size_off)

    first_mod_off = symname_size_off + 4 + symname_size

    # Walk module entries sequentially from first_mod_off to EOF
    modules = []
    off = first_mod_off
    while off < len(data):
        if off + 5 > len(data):
            break
        srf_size, = struct.unpack_from('>I', data, off)
        if srf_size == 0:
            break
        fname_len = data[off + 4]
        end = off + 5 + fname_len + srf_size
        if end > len(data):
            break
        filename = data[off + 5 : off + 5 + fname_len].decode('ascii', errors='replace')
        srf_bytes = data[off + 5 + fname_len : end]
        modules.append((filename, srf_bytes))
        off = end

    return modules


# ---------------------------------------------------------------------------
# ar archive builder (GNU format with symbol table)
# ---------------------------------------------------------------------------

AR_MAGIC    = b'!<arch>\n'
AR_HDR_SIZE = 60


def _ar_header(ar_name, data_size):
    """Build a 60-byte ar member header. ar_name is bytes, ≤16 bytes."""
    # Layout: name(16) mtime(12) uid(6) gid(6) mode(8) size(10) fmag(2) = 60
    name_field = (ar_name + b' ' * 16)[:16]
    size_field = (str(data_size).encode('ascii') + b' ' * 10)[:10]
    return (name_field +
            b'0           ' +   # mtime (12)
            b'0     ' +         # uid (6)
            b'0     ' +         # gid (6)
            b'644     ' +       # mode (8)
            size_field +        # size (10)
            b'`\n')             # fmag (2)


def _elf_global_symbols(elf_bytes):
    """Extract names of global defined symbols from ELF32 LE bytes."""
    if len(elf_bytes) < 52 or elf_bytes[:4] != b'\x7fELF':
        return []
    e_shoff, = struct.unpack_from('<I', elf_bytes, 32)
    e_shentsize, e_shnum, _ = struct.unpack_from('<HHH', elf_bytes, 46)
    if e_shoff == 0 or e_shnum == 0 or e_shentsize < 40:
        return []

    def shdr(i):
        o = e_shoff + i * e_shentsize
        flds = struct.unpack_from('<IIIIIIIIII', elf_bytes, o)
        keys = ('sh_name','sh_type','sh_flags','sh_addr','sh_offset',
                'sh_size','sh_link','sh_info','sh_addralign','sh_entsize')
        return dict(zip(keys, flds))

    sym_s = str_s = None
    for i in range(e_shnum):
        s = shdr(i)
        if s['sh_type'] == SHT_SYMTAB:
            sym_s = s
            str_s = shdr(s['sh_link'])
            break
    if sym_s is None or sym_s['sh_entsize'] == 0:
        return []

    strtab = elf_bytes[str_s['sh_offset'] : str_s['sh_offset'] + str_s['sh_size']]
    sym_count    = sym_s['sh_size'] // sym_s['sh_entsize']
    first_global = sym_s['sh_info']
    names = []
    for i in range(first_global, sym_count):
        o = sym_s['sh_offset'] + i * sym_s['sh_entsize']
        st_name, _val, _sz, st_info, _other, st_shndx = struct.unpack_from('<IIIBBH', elf_bytes, o)
        if (st_info >> 4) == STB_GLOBAL and st_shndx not in (SHN_UNDEF, SHN_ABS):
            end = strtab.index(b'\x00', st_name)
            names.append(strtab[st_name:end].decode('ascii', errors='replace'))
    return names


def build_ar(members):
    """Build GNU ar archive from list of (filename, elf_bytes).
    Includes a GNU-format '/' symbol table and '//' long-name table if needed.
    Returns bytes.
    """
    # Pad each member to even size (ar requirement)
    padded = []
    for fname, elf_bytes in members:
        pad = b'\n' if len(elf_bytes) % 2 else b''
        padded.append((fname, elf_bytes, pad))

    # Build GNU '//' long-name table for member names > 15 chars (after adding '/')
    ar_strtab = b''
    long_name_off = {}
    for fname, _, _ in padded:
        ar_name = fname + '/'
        if len(ar_name) > 15:
            long_name_off[fname] = len(ar_strtab)
            ar_strtab += ar_name.encode('ascii') + b'/\n'
    if ar_strtab and len(ar_strtab) % 2:
        ar_strtab += b'\n'

    # Collect global symbols per member
    sym_entries = []   # list of (symbol_name, member_index)
    for i, (fname, elf_bytes, _) in enumerate(padded):
        for sym in _elf_global_symbols(elf_bytes):
            sym_entries.append((sym, i))

    # Compute symbol table payload size (needed before we know member offsets)
    sym_names_bytes = b''.join(s.encode('ascii') + b'\x00' for s, _ in sym_entries)
    sym_payload_size = 4 + 4 * len(sym_entries) + len(sym_names_bytes)
    if sym_payload_size % 2:
        sym_payload_size += 1

    # Compute archive member offsets
    cur = len(AR_MAGIC)
    cur += AR_HDR_SIZE + sym_payload_size          # '/' symbol table
    if ar_strtab:
        cur += AR_HDR_SIZE + len(ar_strtab)        # '//' long-name table
    member_offsets = []
    for fname, elf_bytes, pad in padded:
        member_offsets.append(cur)
        cur += AR_HDR_SIZE + len(elf_bytes) + len(pad)

    # Build symbol table payload with real member offsets
    sym_offsets = [member_offsets[i] for _, i in sym_entries]
    sym_payload  = struct.pack('>I', len(sym_entries))
    sym_payload += b''.join(struct.pack('>I', o) for o in sym_offsets)
    sym_payload += sym_names_bytes
    if len(sym_payload) % 2:
        sym_payload += b'\n'
    assert len(sym_payload) == sym_payload_size

    # Assemble archive
    out = bytearray()
    out += AR_MAGIC
    out += _ar_header(b'/ ', len(sym_payload))
    out += sym_payload
    if ar_strtab:
        out += _ar_header(b'// ', len(ar_strtab))
        out += ar_strtab
    for fname, elf_bytes, pad in padded:
        if fname in long_name_off:
            ar_name = f'/{long_name_off[fname]}'.encode('ascii')
        else:
            ar_name = (fname + '/').encode('ascii')
        out += _ar_header(ar_name, len(elf_bytes))
        out += elf_bytes
        out += pad

    return bytes(out)


# ---------------------------------------------------------------------------
# Conversion logic
# ---------------------------------------------------------------------------

def srf_to_elf(srf_data, base=0):
    """Convert one SRF object to ELF32 relocatable. Returns bytes."""

    obj = srf_parse(srf_data, base)
    sections = obj['sections']

    # Determine output type
    is_reloc = bool(obj['c_fatt'] & C_FATT_RELOC)
    is_exec  = bool(obj['c_fatt'] & C_FATT_EXEC)

    # --- Merge SRF sections by type ---
    # Multiple CODE/DATA/BSS sections get concatenated in order.
    code_parts = []  # (srf_sec_idx, rawdata, section)
    data_parts = []
    bss_total  = 0
    bss_addr   = 0   # start address of first BSS

    for i, sec in enumerate(sections):
        if sec['s_scntyp'] == SCNTYP_CODE:
            code_parts.append((i, sec))
        elif sec['s_scntyp'] == SCNTYP_DATA:
            data_parts.append((i, sec))
        elif sec['s_scntyp'] == SCNTYP_BSS:
            if bss_total == 0:
                bss_addr = sec['s_off']
            bss_total += sec['s_dsiz']

    code_data = b''.join(sec['rawdata'] for _, sec in code_parts)
    data_data = b''.join(sec['rawdata'] for _, sec in data_parts)

    # --- Build symbol table ---
    # srf_sym_index[(srf_sec_idx, extern_idx)] → elf_sym_idx
    strtab = StringTable()
    symtab = []   # list of (st_name_str, st_value, st_size, st_info, st_shndx)

    # ELF section indices we'll assign:
    # 0=NULL, 1=.text, 2=.data, 3=.bss, [4=.rel.text], [5=.rel.data], 6=.symtab, 7=.strtab, 8=.shstrtab
    TEXT_IDX = 1
    DATA_IDX = 2
    BSS_IDX  = 3

    # Map SRF section type → ELF section index
    scntyp_to_elf = {SCNTYP_CODE: TEXT_IDX, SCNTYP_DATA: DATA_IDX, SCNTYP_BSS: BSS_IDX}

    # Symbol index 0: undefined (required)
    symtab.append(('', 0, 0, 0, SHN_UNDEF))
    srf_sym_index = {}

    # Collect all extern symbols (locals first, then globals)
    # Two passes: locals, then globals (ELF .symtab requires locals before globals)
    local_syms = []
    global_syms = []

    for srf_sec_idx, sec in enumerate(sections):
        elf_shndx = scntyp_to_elf.get(sec['s_scntyp'], SHN_ABS)
        # Accumulate code offset for multi-section CODE
        sec_base_in_elf = _elf_section_base(srf_sec_idx, sections, code_parts, data_parts)

        for ext_idx, ext in enumerate(sec['externs']):
            sym_value = ext['e_scnoff'] + sec_base_in_elf
            sym_size  = ext['e_size']
            name      = ext['e_exnam']

            if ext['e_extyp'] == EXTYP_LOCAL:
                sym_info = (STB_LOCAL << 4) | STT_NOTYPE
                local_syms.append((srf_sec_idx, ext_idx, name, sym_value, sym_size,
                                   sym_info, elf_shndx))
            elif ext['e_extyp'] == EXTYP_GLOBAL:
                sym_info = (STB_GLOBAL << 4) | STT_NOTYPE
                global_syms.append((srf_sec_idx, ext_idx, name, sym_value, sym_size,
                                    sym_info, elf_shndx))
            else:  # EXTERN: undefined reference
                sym_info = (STB_GLOBAL << 4) | STT_NOTYPE
                global_syms.append((srf_sec_idx, ext_idx, name, 0, 0,
                                    sym_info, SHN_UNDEF))

    # Add locals to symtab (index 1..)
    for (srf_sec_idx, ext_idx, name, val, sz, info, shndx) in local_syms:
        idx = len(symtab)
        symtab.append((name, val, sz, info, shndx))
        srf_sym_index[(srf_sec_idx, ext_idx)] = idx

    first_global = len(symtab)

    # Add globals
    for (srf_sec_idx, ext_idx, name, val, sz, info, shndx) in global_syms:
        idx = len(symtab)
        symtab.append((name, val, sz, info, shndx))
        srf_sym_index[(srf_sec_idx, ext_idx)] = idx

    # --- Build relocation entries ---
    text_rels = []   # (r_offset, r_info, r_addend)
    data_rels = []

    for srf_sec_idx, sec in enumerate(sections):
        is_text = sec['s_scntyp'] == SCNTYP_CODE
        sec_base = _elf_section_base(srf_sec_idx, sections, code_parts, data_parts)

        for rel in sec['relocations']:
            elf_reltype = SRF_RELOC_TO_ELF.get(rel['r_rctyp'], 0)
            if elf_reltype == 0:
                print(f"WARNING: unknown SRF reloc type 0x{rel['r_rctyp']:04x}", file=sys.stderr)
                continue

            # Look up symbol index
            # r_exndx is index into THIS section's extern list
            sym_key = (srf_sec_idx, rel['r_exndx'])
            if sym_key not in srf_sym_index:
                # Might be in a different section (r_scnndx tells which)
                # Find the actual section by r_scnndx
                target_sec_idx = _find_section_by_id(sections, rel['r_scnndx'])
                sym_key = (target_sec_idx, rel['r_exndx'])

            elf_sym_idx = srf_sym_index.get(sym_key, 0)
            r_offset = rel['r_scnoff'] + sec_base
            r_info   = (elf_sym_idx << 8) | elf_reltype
            # r_symoff is the SRF addend (symbol-relative offset).  Interpret as
            # signed 32-bit so that negative offsets (e.g. 0xfffffc80 = -896)
            # round-trip correctly through the ELF RELA r_addend field.
            r_symoff = rel['r_symoff']
            r_addend = r_symoff if r_symoff < 0x80000000 else r_symoff - 0x100000000

            if is_text:
                text_rels.append(pack_rela(r_offset, r_info, r_addend))
            else:
                data_rels.append(pack_rela(r_offset, r_info, r_addend))

    # --- Assign ELF section indices ---
    # Layout: NULL(0), .text(1), .data(2), .bss(3),
    #         [.rela.text(4)], [.rela.data(5)], .symtab(N), .strtab(N+1), .shstrtab(N+2)

    shstrtab = StringTable()
    shstrtab.add('')         # index 0
    n_text_name  = shstrtab.add('.text')
    n_data_name  = shstrtab.add('.data')
    n_bss_name   = shstrtab.add('.bss')

    elf_sections = []  # (name_idx, type, flags, addr, data, link, info, align, entsize)

    elf_sections.append((0, SHT_NULL, 0, 0, b'', 0, 0, 0, 0))  # NULL

    code_addr = sections[code_parts[0][0]]['s_off'] if code_parts else 0
    data_addr = sections[data_parts[0][0]]['s_off'] if data_parts else 0

    elf_sections.append((n_text_name, SHT_PROGBITS,
                         SHF_ALLOC | SHF_EXECINSTR,
                         code_addr if is_exec else 0,
                         code_data, 0, 0, 4, 0))

    elf_sections.append((n_data_name, SHT_PROGBITS,
                         SHF_ALLOC | SHF_WRITE,
                         data_addr if is_exec else 0,
                         data_data, 0, 0, 1, 0))

    elf_sections.append((n_bss_name,  SHT_NOBITS,
                         SHF_ALLOC | SHF_WRITE,
                         bss_addr if is_exec else 0,
                         b'', 0, 0, 1, 0))

    # .rela.text (if any)
    rel_text_idx = None
    if text_rels:
        n_rel_text = shstrtab.add('.rela.text')
        rel_text_idx = len(elf_sections)
        # link and info filled in below once we know .symtab index
        elf_sections.append((n_rel_text, SHT_RELA, 0, 0,
                              b''.join(text_rels), 0, TEXT_IDX, 4, RELA_SIZE))

    # .rela.data (if any)
    rel_data_idx = None
    if data_rels:
        n_rel_data = shstrtab.add('.rela.data')
        rel_data_idx = len(elf_sections)
        elf_sections.append((n_rel_data, SHT_RELA, 0, 0,
                             b''.join(data_rels), 0, DATA_IDX, 4, RELA_SIZE))

    # .symtab
    n_symtab = shstrtab.add('.symtab')
    symtab_idx = len(elf_sections)

    # Build symtab binary
    sym_strtab = StringTable()
    symtab_data = b''
    for (name, val, sz, info, shndx) in symtab:
        name_off = sym_strtab.add(name) if name else 0
        symtab_data += pack_sym(name_off, val, sz, info, 0, shndx)

    elf_sections.append((n_symtab, SHT_SYMTAB, 0, 0, symtab_data,
                         symtab_idx + 1,   # sh_link → .strtab (next section)
                         first_global, 4, SYM_SIZE))

    # .strtab
    n_strtab = shstrtab.add('.strtab')
    strtab_idx = len(elf_sections)
    elf_sections.append((n_strtab, SHT_STRTAB, 0, 0, sym_strtab.data(), 0, 0, 1, 0))

    # Fix up .rel sections' sh_link now that we know symtab_idx
    def fixup_rel(idx):
        if idx is not None:
            nm, typ, flg, adr, dat, lnk, inf, aln, esz = elf_sections[idx]
            elf_sections[idx] = (nm, typ, flg, adr, dat, symtab_idx, inf, aln, esz)
    fixup_rel(rel_text_idx)
    fixup_rel(rel_data_idx)

    # .shstrtab
    n_shstrtab = shstrtab.add('.shstrtab')
    shstrtab_idx = len(elf_sections)
    elf_sections.append((n_shstrtab, SHT_STRTAB, 0, 0, shstrtab.data(), 0, 0, 1, 0))

    # --- Lay out file ---
    # ELF header (52), then section data, then section header table
    e_type = ET_EXEC if is_exec else ET_REL
    n_shdrs = len(elf_sections)

    # Calculate offsets for each section's data
    offsets = []
    cur = EHDR_SIZE
    for (nm, typ, flg, adr, dat, lnk, inf, aln, esz) in elf_sections:
        if aln > 1 and cur % aln:
            cur += aln - (cur % aln)
        offsets.append(cur)
        if typ != SHT_NOBITS:
            cur += len(dat)

    # Section header table (align to 4)
    if cur % 4:
        cur += 4 - (cur % 4)
    shoff = cur

    # Build ELF
    out = bytearray()
    out += pack_ehdr(e_type, EM_SE_C33, obj['c_pentry'], 0, shoff,
                     0, 0, 0, SHDR_SIZE, n_shdrs, shstrtab_idx)

    # Pad and write section data
    for i, (nm, typ, flg, adr, dat, lnk, inf, aln, esz) in enumerate(elf_sections):
        while len(out) < offsets[i]:
            out += b'\x00'
        if typ != SHT_NOBITS:
            out += dat

    # Pad to shoff
    while len(out) < shoff:
        out += b'\x00'

    # Section header table
    for i, (nm, typ, flg, adr, dat, lnk, inf, aln, esz) in enumerate(elf_sections):
        size = len(dat) if typ != SHT_NOBITS else (bss_total if nm == n_bss_name else 0)
        out += pack_shdr(nm, typ, flg, adr, offsets[i], size,
                         lnk, inf, aln, esz)

    return bytes(out)


def _elf_section_base(srf_sec_idx, sections, code_parts, data_parts):
    """Byte offset of this SRF section's data within the merged ELF section."""
    sec = sections[srf_sec_idx]
    base = 0
    if sec['s_scntyp'] == SCNTYP_CODE:
        for idx, s in code_parts:
            if idx == srf_sec_idx:
                break
            base += s['s_dsiz']
    elif sec['s_scntyp'] == SCNTYP_DATA:
        for idx, s in data_parts:
            if idx == srf_sec_idx:
                break
            base += s['s_dsiz']
    return base


def _find_section_by_id(sections, scnndx):
    for i, sec in enumerate(sections):
        if sec['s_scnndx'] == scnndx:
            return i
    return 0


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <input.srf|input.lib> [output.o|output.a]",
              file=sys.stderr)
        sys.exit(1)

    in_path = sys.argv[1]
    with open(in_path, 'rb') as f:
        in_data = f.read()

    if in_data[:4] == b'\x7fELF':
        print(f"ERROR: {in_path}: ELF file detected — expected SRF33 input", file=sys.stderr)
        sys.exit(1)

    # --- lib33 archive ---
    if in_data[:9] == LIB33_MAGIC:
        out_path = sys.argv[2] if len(sys.argv) > 2 else os.path.splitext(in_path)[0] + '.a'
        try:
            modules = lib33_parse(in_data)
        except Exception as e:
            print(f"ERROR: {in_path}: {e}", file=sys.stderr)
            sys.exit(1)

        if not modules:
            print(f"WARNING: {in_path}: no modules found in library", file=sys.stderr)

        members = []
        errors  = 0
        for fname, srf_bytes in modules:
            c_ver = struct.unpack_from('>H', srf_bytes, 4)[0] if len(srf_bytes) >= 6 else 0
            if c_ver != 0x3300:
                print(f"  WARNING: {fname}: unexpected c_ver=0x{c_ver:04x}", file=sys.stderr)
            try:
                elf_bytes = srf_to_elf(srf_bytes)
                members.append((fname, elf_bytes))
                print(f"  {fname}: {len(srf_bytes)} bytes SRF → {len(elf_bytes)} bytes ELF")
            except Exception as e:
                print(f"  ERROR: {fname}: {e}", file=sys.stderr)
                errors += 1

        if errors:
            print(f"ERROR: {errors} module(s) failed to convert", file=sys.stderr)
            sys.exit(1)

        ar_data = build_ar(members)
        with open(out_path, 'wb') as f:
            f.write(ar_data)
        print(f"{in_path} → {out_path}  ({len(modules)} modules, {len(ar_data)} bytes)")
        return

    # --- standalone SRF object ---
    out_path = sys.argv[2] if len(sys.argv) > 2 else os.path.splitext(in_path)[0] + '.o'

    if len(in_data) < 16:
        print(f"ERROR: {in_path}: file too short", file=sys.stderr)
        sys.exit(1)

    c_ver = struct.unpack_from('>H', in_data, 4)[0]
    if c_ver != 0x3300:
        print(f"WARNING: {in_path}: unexpected c_ver=0x{c_ver:04x}", file=sys.stderr)

    elf_data = srf_to_elf(in_data)

    with open(out_path, 'wb') as f:
        f.write(elf_data)

    print(f"{in_path} → {out_path}  ({len(elf_data)} bytes)")


if __name__ == '__main__':
    main()
