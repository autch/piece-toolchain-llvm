#!/usr/bin/env python3
"""test_asm33conv.py — asm33conv.py のユニットテスト"""

import sys
import os
import unittest

sys.path.insert(0, os.path.dirname(__file__))
from asm33conv import translate_line, parse_int, sign6, expand_xld_imm


class TestParseInt(unittest.TestCase):
    def test_decimal(self):
        self.assertEqual(parse_int('42'), 42)

    def test_hex(self):
        self.assertEqual(parse_int('0x1F'), 31)

    def test_negative(self):
        self.assertEqual(parse_int('-1'), -1)

    def test_binary(self):
        self.assertEqual(parse_int('0b1010'), 10)


class TestSign6(unittest.TestCase):
    def test_zero(self):
        self.assertEqual(sign6(0), 0)

    def test_positive(self):
        self.assertEqual(sign6(31), 31)   # max positive in 6-bit signed

    def test_negative_wrap(self):
        self.assertEqual(sign6(32), -32)  # 0x20 → -32

    def test_minus_one(self):
        self.assertEqual(sign6(63), -1)   # 0x3F → -1

    def test_large_value_ignored(self):
        # Only low 6 bits matter
        self.assertEqual(sign6(0x3fff), sign6(63))  # 0x3fff & 0x3f == 63 → -1


class TestXldLoad(unittest.TestCase):
    # Pessimistic 2-ext expansion; MC relaxation removes unnecessary ext later.
    def test_no_offset(self):
        result = translate_line('\txld.w\t%r7,[%r12]\n')
        self.assertEqual(result, ['\text\t0', '\text\t0', '\tld.w\t%r7, [%r12]'])

    def test_with_offset(self):
        result = translate_line('\txld.w\t%r5,[%r12+4]\n')
        self.assertEqual(result, ['\text\t0', '\text\t4', '\tld.w\t%r5, [%r12]'])

    def test_byte_load(self):
        result = translate_line('\txld.b\t%r4,[%r10]\n')
        self.assertEqual(result, ['\text\t0', '\text\t0', '\tld.b\t%r4, [%r10]'])

    def test_byte_load_offset(self):
        result = translate_line('\txld.b\t%r11,[%r10+1]\n')
        self.assertEqual(result, ['\text\t0', '\text\t1', '\tld.b\t%r11, [%r10]'])

    def test_uh_load(self):
        result = translate_line('\txld.uh\t%r10,[%r13]\n')
        self.assertEqual(result, ['\text\t0', '\text\t0', '\tld.uh\t%r10, [%r13]'])

    def test_uh_load_offset(self):
        result = translate_line('\txld.uh\t%r6,[%r12+8]\n')
        self.assertEqual(result, ['\text\t0', '\text\t8', '\tld.uh\t%r6, [%r12]'])

    def test_with_comment(self):
        result = translate_line('\txld.w\t%r7,[%r12]\t; tbl\n')
        self.assertEqual(result, ['\text\t0', '\text\t0', '\tld.w\t%r7, [%r12]\t; tbl'])

    def test_large_offset(self):
        result = translate_line('\txld.w\t%r0,[%r1+64]\n')
        self.assertEqual(result, ['\text\t0', '\text\t64', '\tld.w\t%r0, [%r1]'])

    def test_hex_offset(self):
        result = translate_line('\txld.w\t%r0,[%r1+0x10]\n')
        self.assertEqual(result, ['\text\t0', '\text\t16', '\tld.w\t%r0, [%r1]'])


class TestXldStore(unittest.TestCase):
    # Pessimistic 2-ext expansion; MC relaxation removes unnecessary ext later.
    def test_no_offset(self):
        result = translate_line('\txld.w\t[%r12],%r5\n')
        self.assertEqual(result, ['\text\t0', '\text\t0', '\tld.w\t[%r12], %r5'])

    def test_with_offset(self):
        result = translate_line('\txld.w\t[%r12+4],%r5\n')
        self.assertEqual(result, ['\text\t0', '\text\t4', '\tld.w\t[%r12], %r5'])

    def test_with_comment(self):
        result = translate_line('\txld.w\t[%r12+4],%r5\t; freqwk\n')
        self.assertEqual(result, ['\text\t0', '\text\t4', '\tld.w\t[%r12], %r5\t; freqwk'])


class TestXldImm(unittest.TestCase):
    def test_zero(self):
        # xld.w %r0, 0
        result = translate_line('\txld.w\t%r0, 0\n')
        self.assertEqual(result, ['\text\t0', '\text\t0', '\tld.w\t%r0, 0'])

    def test_0x3fff(self):
        # xld.w %r9, 0x3fff
        # ext_hi  = (0x3fff >> 19) & 0x1FFF = 0
        # ext_mid = (0x3fff >> 6)  & 0x1FFF = 0xff = 255
        # base    = sign6(0x3fff) = sign6(0x3f) = sign6(63) = -1
        result = translate_line('\txld.w\t%r9, 0x3fff\n')
        self.assertEqual(result, ['\text\t0', '\text\t255', '\tld.w\t%r9, -1'])

    def test_reconstruct_0x3fff(self):
        # Verify value reconstruction: (0 << 19) | (255 << 6) | 63 = 0x3fff
        ext_hi, ext_mid = 0, 255
        base = -1  # sign6(63) = -1, but bit pattern is 63
        base_bits = base & 0x3F  # = 63
        value = (ext_hi << 19) | (ext_mid << 6) | base_bits
        self.assertEqual(value, 0x3fff)

    def test_small_positive(self):
        # xld.w %r0, 5 → ext 0, ext 0, ld.w %r0, 5
        result = translate_line('\txld.w\t%r0, 5\n')
        self.assertEqual(result, ['\text\t0', '\text\t0', '\tld.w\t%r0, 5'])

    def test_large_value(self):
        # xld.w %r0, 0x100000 (1MB)
        # ext_hi  = (0x100000 >> 19) & 0x1FFF = 0x800 >> 19... wait
        # 0x100000 = 1048576
        # ext_hi  = (1048576 >> 19) & 0x1FFF = 2 & 0x1FFF = 2
        # ext_mid = (1048576 >> 6)  & 0x1FFF = 16384 & 0x1FFF = 0
        # base    = sign6(1048576) = sign6(0) = 0
        result = translate_line('\txld.w\t%r0, 0x100000\n')
        self.assertEqual(result, ['\text\t2', '\text\t0', '\tld.w\t%r0, 0'])

    def test_negative_imm(self):
        # xld.w %r0, -1 (0xFFFFFFFF in 32-bit)
        # Python int: -1
        # ext_hi  = ((-1) >> 19) & 0x1FFF = 0x1FFF = 8191
        # ext_mid = ((-1) >> 6)  & 0x1FFF = 0x1FFF = 8191
        # base    = sign6(-1) = sign6(0x3f) = -1
        result = translate_line('\txld.w\t%r0, -1\n')
        self.assertEqual(result, ['\text\t8191', '\text\t8191', '\tld.w\t%r0, -1'])


class TestXshift(unittest.TestCase):
    def test_xsrl(self):
        # 14 = 8 + 6 — split into two shifts (ext doesn't work with shifts)
        result = translate_line('\txsrl\t%r10, 14\n')
        self.assertEqual(result, ['\tsrl\t%r10, 8', '\tsrl\t%r10, 6'])

    def test_xsra(self):
        result = translate_line('\txsra\t%r11, 14\n')
        self.assertEqual(result, ['\tsra\t%r11, 8', '\tsra\t%r11, 6'])

    def test_xsla_becomes_sll(self):
        # xsla → sll (sla = sll for left shift)
        result = translate_line('\txsla\t%r5, 14\n')
        self.assertEqual(result, ['\tsll\t%r5, 8', '\tsll\t%r5, 6'])

    def test_with_comment(self):
        # 8 fits in one instruction; comment on the (only) line
        result = translate_line('\txsra\t%r4, 8\t; d1>>=8\n')
        self.assertEqual(result, ['\tsra\t%r4, 8\t; d1>>=8'])

    def test_xsra_16(self):
        # 16 = 8 + 8
        result = translate_line('\txsra\t%r11, 16\n')
        self.assertEqual(result, ['\tsra\t%r11, 8', '\tsra\t%r11, 8'])

    def test_xsrl_31(self):
        # 31 = 8 + 8 + 8 + 7
        result = translate_line('\txsrl\t%r10, 31\n')
        self.assertEqual(result, ['\tsrl\t%r10, 8', '\tsrl\t%r10, 8',
                                  '\tsrl\t%r10, 8', '\tsrl\t%r10, 7'])


class TestPassthrough(unittest.TestCase):
    def test_plain_ld(self):
        line = '\tld.w\t%r10,%r5\n'
        self.assertEqual(translate_line(line), ['\tld.w\t%r10,%r5'])

    def test_label(self):
        line = '__LX16:\n'
        self.assertEqual(translate_line(line), ['__LX16:'])

    def test_global(self):
        line = '\t.global\tMakeWaveLP_fast\n'
        self.assertEqual(translate_line(line), ['\t.global\tMakeWaveLP_fast'])

    def test_blank_line(self):
        # translate_line strips trailing newline; blank line → ['']
        self.assertEqual(translate_line('\n'), [''])

    def test_comment_line(self):
        line = '; this is a comment\n'
        self.assertEqual(translate_line(line), ['; this is a comment'])

    def test_endfile_removed(self):
        self.assertEqual(translate_line('\t.endfile\n'), [])
        self.assertEqual(translate_line('.endfile\n'), [])

    def test_plain_ld_uh_passthrough(self):
        # ld.uh (no x prefix) should pass through unchanged
        line = '\tld.uh\t%r10,[%r13]\n'
        self.assertEqual(translate_line(line), ['\tld.uh\t%r10,[%r13]'])

    def test_ext_passthrough(self):
        # Already-existing ext instructions pass through
        line = '\text\t0\n'
        self.assertEqual(translate_line(line), ['\text\t0'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
