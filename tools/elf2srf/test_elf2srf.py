#!/usr/bin/env python3
"""Basic structural tests for elf2srf.py."""

import sys
import struct
import os
import subprocess
import tempfile
import hashlib

sys.path.insert(0, os.path.dirname(__file__))
from elf2srf import elf_to_srf, parse_elf, SCN_ID_CODE, SCN_ID_DATA, SCN_ID_BSS
from elf2srf import CTRL_HDR_SIZE, SCN_HDR_SIZE, SRF_RELOC_SIZE

SRF2ELF = os.path.join(os.path.dirname(__file__), '..', 'srf2elf', 'srf2elf.py')
MENU_ELF = os.path.join(os.path.dirname(__file__), '..', 'srf2elf', 'menu.o')

PASS = 0
FAIL = 0


def check(name, cond, msg=''):
    global PASS, FAIL
    if cond:
        print(f'  PASS  {name}')
        PASS += 1
    else:
        print(f'  FAIL  {name}' + (f': {msg}' if msg else ''))
        FAIL += 1


def parse_srf(data):
    """Minimal SRF parser for test validation."""
    c_fatt, c_pentry, c_ver, c_scncnt = struct.unpack_from('>HHHH', data, 0)
    c_scnptr, c_debptr = struct.unpack_from('>II', data, 8)
    sections = []
    ptr = c_scnptr
    while ptr:
        s_nxptr, = struct.unpack_from('>I', data, ptr)
        s_scntyp, s_lnktyp, s_scnatt = struct.unpack_from('>HHH', data, ptr + 4)
        s_off, = struct.unpack_from('>I', data, ptr + 10)
        s_rcptr, s_rcsiz, s_exptr, s_exsiz, s_excnt = \
            struct.unpack_from('>IIIII', data, ptr + 14)
        s_rdptr, s_dsiz = struct.unpack_from('>II', data, ptr + 34)
        s_scnndx, = struct.unpack_from('>H', data, ptr + 42)
        externs = []
        if s_exptr and s_excnt:
            off = s_exptr
            for _ in range(s_excnt):
                e_scnoff, = struct.unpack_from('>I', data, off)
                e_size,   = struct.unpack_from('>I', data, off + 4)
                e_scnndx, = struct.unpack_from('>H', data, off + 8)
                e_extyp,  = struct.unpack_from('>H', data, off + 10)
                e_namsiz, = struct.unpack_from('>B', data, off + 12)
                name = data[off + 13 : off + 13 + e_namsiz].decode('ascii', errors='?')
                externs.append((name, e_scnoff, e_size, e_scnndx, e_extyp))
                off += 13 + e_namsiz
        relocs = []
        if s_rcptr and s_rcsiz:
            n = s_rcsiz // 16
            for i in range(n):
                o = s_rcptr + i * 16
                r_type,   = struct.unpack_from('>H', data, o)
                r_scnoff, = struct.unpack_from('>I', data, o + 2)
                r_exndx,  = struct.unpack_from('>I', data, o + 6)
                r_scnndx, = struct.unpack_from('>H', data, o + 10)
                r_symoff, = struct.unpack_from('>I', data, o + 12)
                relocs.append((r_type, r_scnoff, r_exndx, r_scnndx, r_symoff))
        rawdata = data[s_rdptr:s_rdptr+s_dsiz] if s_rdptr and s_dsiz and s_scntyp != 3 else b''
        sections.append(dict(
            s_nxptr=s_nxptr, s_scntyp=s_scntyp, s_scnndx=s_scnndx,
            s_dsiz=s_dsiz, s_rdptr=s_rdptr,
            s_rcptr=s_rcptr, s_rcsiz=s_rcsiz,
            s_exptr=s_exptr, s_exsiz=s_exsiz, s_excnt=s_excnt,
            externs=externs, relocs=relocs, rawdata=rawdata,
        ))
        ptr = s_nxptr
    return dict(c_fatt=c_fatt, c_ver=c_ver, c_scncnt=c_scncnt,
                c_scnptr=c_scnptr, c_debptr=c_debptr, sections=sections)


def test_control_header(srf, label=''):
    print(f'\n[control header]{" "+label if label else ""}')
    check('c_fatt=0x0001',    srf['c_fatt']   == 0x0001)
    check('c_ver=0x3300',     srf['c_ver']    == 0x3300)
    check('c_scncnt=3',       srf['c_scncnt'] == 3)
    check('c_scnptr=16',      srf['c_scnptr'] == CTRL_HDR_SIZE)
    check('c_debptr=0',       srf['c_debptr'] == 0)


def test_section_structure(srf, label=''):
    print(f'\n[section structure]{" "+label if label else ""}')
    secs = srf['sections']
    check('3 sections',       len(secs) == 3)
    if len(secs) < 3:
        return
    check('CODE s_scntyp=1',  secs[0]['s_scntyp'] == 1)
    check('DATA s_scntyp=2',  secs[1]['s_scntyp'] == 2)
    check('BSS  s_scntyp=3',  secs[2]['s_scntyp'] == 3)
    check('CODE s_scnndx=1',  secs[0]['s_scnndx'] == SCN_ID_CODE)
    check('DATA s_scnndx=2',  secs[1]['s_scnndx'] == SCN_ID_DATA)
    check('BSS  s_scnndx=3',  secs[2]['s_scnndx'] == SCN_ID_BSS)
    # Linked list: CODE→DATA→BSS→0
    code_hdr = CTRL_HDR_SIZE
    data_hdr = code_hdr + SCN_HDR_SIZE
    bss_hdr  = data_hdr + SCN_HDR_SIZE
    check('CODE→DATA link',   secs[0]['s_nxptr'] == data_hdr)
    check('DATA→BSS  link',   secs[1]['s_nxptr'] == bss_hdr)
    check('BSS→0    link',    secs[2]['s_nxptr'] == 0)
    # BSS has no raw data pointer
    check('BSS rdptr=0',      secs[2]['s_rdptr'] == 0)


def test_text_bytes_preserved(elf_data, srf_data, label=''):
    print(f'\n[.text byte preservation]{" "+label if label else ""}')
    elf = parse_elf(elf_data)
    text_hdr = elf['text_hdr']
    orig_text = text_hdr['data'] if text_hdr else b''
    srf = parse_srf(srf_data)
    srf_code = srf['sections'][0]['rawdata']
    check('.text len matches',  len(srf_code) >= len(orig_text))
    if orig_text:
        match = srf_code[:len(orig_text)] == orig_text
        check('.text bytes identical', match,
              f'sha orig={hashlib.sha1(orig_text).hexdigest()[:8]} '
              f'srf={hashlib.sha1(srf_code[:len(orig_text)]).hexdigest()[:8]}')


def test_reloc_count(elf_data, srf_data, label=''):
    print(f'\n[relocation count]{" "+label if label else ""}')
    elf = parse_elf(elf_data)
    n_elf_text  = len(elf['text_rels']) + len(elf['rodata_rels'])
    n_elf_data  = len(elf['data_rels'])
    srf = parse_srf(srf_data)
    n_srf_code  = len(srf['sections'][0]['relocs'])
    n_srf_data  = len(srf['sections'][1]['relocs'])
    check(f'CODE relocs: ELF={n_elf_text} SRF={n_srf_code}', n_srf_code == n_elf_text)
    check(f'DATA relocs: ELF={n_elf_data} SRF={n_srf_data}', n_srf_data == n_elf_data)


def test_extern_names(elf_data, srf_data, global_syms, label=''):
    print(f'\n[extern names]{" "+label if label else ""}')
    srf = parse_srf(srf_data)
    all_names = set()
    for sec in srf['sections']:
        for (name, *_) in sec['externs']:
            all_names.add(name)
    for sym in global_syms:
        check(f"'{sym}' in extern table", sym in all_names)


def test_roundtrip_bytes(elf_data, srf_data, label=''):
    """Convert SRF→ELF and check .text bytes match original."""
    print(f'\n[round-trip .text]{" "+label if label else ""}')
    # Write SRF to temp file, run srf2elf.py
    with tempfile.NamedTemporaryFile(suffix='.srf.o', delete=False) as f:
        f.write(srf_data)
        srf_path = f.name
    rt_path = srf_path + '.elf.o'
    try:
        r = subprocess.run(
            [sys.executable, SRF2ELF, srf_path, rt_path],
            capture_output=True, timeout=10)
        check('srf2elf succeeds', r.returncode == 0,
              r.stderr.decode(errors='?'))
        if r.returncode == 0:
            with open(rt_path, 'rb') as f:
                rt_data = f.read()
            rt_elf = parse_elf(rt_data)
            orig_elf = parse_elf(elf_data)
            orig_text = orig_elf['text_hdr']['data'] if orig_elf['text_hdr'] else b''
            rt_text   = rt_elf['text_hdr']['data']   if rt_elf['text_hdr']   else b''
            check('.text bytes preserved through SRF round-trip',
                  orig_text == rt_text,
                  f'orig={hashlib.sha1(orig_text).hexdigest()[:8]} '
                  f'rt={hashlib.sha1(rt_text).hexdigest()[:8]}')
    except subprocess.TimeoutExpired:
        check('srf2elf timeout', False)
    finally:
        for p in (srf_path, rt_path):
            try: os.unlink(p)
            except: pass


# ---------------------------------------------------------------------------

def main():
    print('=== elf2srf structural tests ===')

    # ---- Test 1: menu.o (complex real-world object) ----
    if not os.path.exists(MENU_ELF):
        print(f'SKIP: {MENU_ELF} not found')
    else:
        with open(MENU_ELF, 'rb') as f:
            elf_data = f.read()
        srf_data = elf_to_srf(elf_data)

        test_control_header(parse_srf(srf_data), 'menu.o')
        test_section_structure(parse_srf(srf_data), 'menu.o')
        test_text_bytes_preserved(elf_data, srf_data, 'menu.o')
        test_reloc_count(elf_data, srf_data, 'menu.o')
        test_extern_names(elf_data, srf_data,
            ['getfh', 'nameck', 'getdir', 'geticondata', 'pceAppInit',
             'pceAppProc', 'pceAppExit'], 'menu.o')
        test_roundtrip_bytes(elf_data, srf_data, 'menu.o')

    # ---- Test 2: minimal synthetic ELF (no relocs, no BSS) ----
    print('\n[minimal ELF: empty sections]')
    # Build a tiny ELF with only a .text section containing 4 NOP bytes
    # This tests that empty sections don't crash the converter.
    # (We use the actual hello.c output if available, else skip.)
    hello_elf = '/tmp/hello.o'
    if os.path.exists(hello_elf):
        with open(hello_elf, 'rb') as f:
            elf_data2 = f.read()
        srf_data2 = elf_to_srf(elf_data2)
        test_control_header(parse_srf(srf_data2), 'hello.o')
        test_section_structure(parse_srf(srf_data2), 'hello.o')
        test_text_bytes_preserved(elf_data2, srf_data2, 'hello.o')
        test_reloc_count(elf_data2, srf_data2, 'hello.o')
        test_extern_names(elf_data2, srf_data2,
            ['pceAppInit', 'pceAppProc', 'pceAppExit', 'name_buf'], 'hello.o')
        test_roundtrip_bytes(elf_data2, srf_data2, 'hello.o')
    else:
        print('  SKIP: /tmp/hello.o not found (run clang to generate)')

    # ---- Summary ----
    print(f'\n=== Results: {PASS} passed, {FAIL} failed ===')
    sys.exit(0 if FAIL == 0 else 1)


if __name__ == '__main__':
    main()
