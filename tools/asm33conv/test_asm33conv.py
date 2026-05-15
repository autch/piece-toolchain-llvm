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


# All xld expansions emit the minimum number of ext instructions: zero for the
# tightest forms (offset 0, sign6 immediate), one for the next tier, two only
# when truly necessary. The MC layer cannot grow an instruction during
# relaxation, only shrink, so this is the appropriate strategy.

class TestXldLoad(unittest.TestCase):
    def test_no_offset(self):
        # Offset 0 → no ext needed.
        result = translate_line('\txld.w\t%r7,[%r12]\n')
        self.assertEqual(result, ['\tld.w\t%r7, [%r12]'])

    def test_with_offset(self):
        result = translate_line('\txld.w\t%r5,[%r12+4]\n')
        self.assertEqual(result, ['\text\t4', '\tld.w\t%r5, [%r12]'])

    def test_byte_load(self):
        result = translate_line('\txld.b\t%r4,[%r10]\n')
        self.assertEqual(result, ['\tld.b\t%r4, [%r10]'])

    def test_byte_load_offset(self):
        result = translate_line('\txld.b\t%r11,[%r10+1]\n')
        self.assertEqual(result, ['\text\t1', '\tld.b\t%r11, [%r10]'])

    def test_uh_load(self):
        result = translate_line('\txld.uh\t%r10,[%r13]\n')
        self.assertEqual(result, ['\tld.uh\t%r10, [%r13]'])

    def test_uh_load_offset(self):
        result = translate_line('\txld.uh\t%r6,[%r12+8]\n')
        self.assertEqual(result, ['\text\t8', '\tld.uh\t%r6, [%r12]'])

    def test_with_comment(self):
        result = translate_line('\txld.w\t%r7,[%r12]\t; tbl\n')
        self.assertEqual(result, ['\tld.w\t%r7, [%r12]\t; tbl'])

    def test_large_offset(self):
        result = translate_line('\txld.w\t%r0,[%r1+64]\n')
        self.assertEqual(result, ['\text\t64', '\tld.w\t%r0, [%r1]'])

    def test_hex_offset(self):
        result = translate_line('\txld.w\t%r0,[%r1+0x10]\n')
        self.assertEqual(result, ['\text\t16', '\tld.w\t%r0, [%r1]'])


class TestXldStore(unittest.TestCase):
    def test_no_offset(self):
        result = translate_line('\txld.w\t[%r12],%r5\n')
        self.assertEqual(result, ['\tld.w\t[%r12], %r5'])

    def test_with_offset(self):
        result = translate_line('\txld.w\t[%r12+4],%r5\n')
        self.assertEqual(result, ['\text\t4', '\tld.w\t[%r12], %r5'])

    def test_with_comment(self):
        result = translate_line('\txld.w\t[%r12+4],%r5\t; freqwk\n')
        self.assertEqual(result, ['\text\t4', '\tld.w\t[%r12], %r5\t; freqwk'])


class TestXldImm(unittest.TestCase):
    def test_zero(self):
        # 0 fits in sign6 → no ext.
        result = translate_line('\txld.w\t%r0, 0\n')
        self.assertEqual(result, ['\tld.w\t%r0, 0'])

    def test_0x3fff(self):
        # 0x3fff = 16383, fits sign19 → 1 ext.
        # imm13 = (0x3fff >> 6) & 0x1FFF = 0xff = 255
        # base  = sign6(0x3fff) = sign6(63) = -1
        result = translate_line('\txld.w\t%r9, 0x3fff\n')
        self.assertEqual(result, ['\text\t255', '\tld.w\t%r9, -1'])

    def test_reconstruct_0x3fff(self):
        # Verify value reconstruction: sign_ext_19((255 << 6) | 63) = 0x3fff
        ext_mid = 255
        base_bits = 63           # sign6(-1) → bit pattern 0x3F
        value = (ext_mid << 6) | base_bits
        self.assertEqual(value, 0x3fff)

    def test_small_positive(self):
        # 5 fits sign6 → no ext.
        result = translate_line('\txld.w\t%r0, 5\n')
        self.assertEqual(result, ['\tld.w\t%r0, 5'])

    def test_large_value(self):
        # 0x100000 (1MB) needs 2 ext.
        # ext_hi  = (0x100000 >> 19) & 0x1FFF = 2
        # ext_mid = (0x100000 >> 6)  & 0x1FFF = 0
        # base    = sign6(0)        = 0
        result = translate_line('\txld.w\t%r0, 0x100000\n')
        self.assertEqual(result, ['\text\t2', '\text\t0', '\tld.w\t%r0, 0'])

    def test_negative_imm(self):
        # -1 fits sign6 → no ext.
        result = translate_line('\txld.w\t%r0, -1\n')
        self.assertEqual(result, ['\tld.w\t%r0, -1'])


class TestXldLabel(unittest.TestCase):
    """xld.X with [label] absolute-address operand → ext sym@ah / @al / ld.X [%r8]."""

    def test_load_word(self):
        result = translate_line('\txld.w\t%r10,[bg0a]\n')
        self.assertEqual(result, [
            '\text\tbg0a@ah', '\text\tbg0a@al', '\tld.w\t%r10, [%r8]'
        ])

    def test_load_halfword(self):
        result = translate_line('\txld.h\t%r10,[ready]\n')
        self.assertEqual(result, [
            '\text\tready@ah', '\text\tready@al', '\tld.h\t%r10, [%r8]'
        ])

    def test_load_unsigned_byte(self):
        result = translate_line('\txld.ub\t%r0,[cur_uy]\n')
        self.assertEqual(result, [
            '\text\tcur_uy@ah', '\text\tcur_uy@al', '\tld.ub\t%r0, [%r8]'
        ])

    def test_store_byte(self):
        result = translate_line('\txld.b\t[cursor_x],%r10\n')
        self.assertEqual(result, [
            '\text\tcursor_x@ah', '\text\tcursor_x@al', '\tld.b\t[%r8], %r10'
        ])

    def test_store_word(self):
        result = translate_line('\txld.w\t[bg0a],%r13\n')
        self.assertEqual(result, [
            '\text\tbg0a@ah', '\text\tbg0a@al', '\tld.w\t[%r8], %r13'
        ])


class TestXshift(unittest.TestCase):
    def test_xsrl(self):
        result = translate_line('\txsrl\t%r10, 14\n')
        self.assertEqual(result, ['\tsrl\t%r10, 8', '\tsrl\t%r10, 6'])

    def test_xsra(self):
        result = translate_line('\txsra\t%r11, 14\n')
        self.assertEqual(result, ['\tsra\t%r11, 8', '\tsra\t%r11, 6'])

    def test_xsla_becomes_sll(self):
        result = translate_line('\txsla\t%r5, 14\n')
        self.assertEqual(result, ['\tsll\t%r5, 8', '\tsll\t%r5, 6'])

    def test_xsll_becomes_sll(self):
        result = translate_line('\txsll\t%r10, 16\n')
        self.assertEqual(result, ['\tsll\t%r10, 8', '\tsll\t%r10, 8'])

    def test_with_comment(self):
        result = translate_line('\txsra\t%r4, 8\t; d1>>=8\n')
        self.assertEqual(result, ['\tsra\t%r4, 8\t; d1>>=8'])

    def test_xsra_16(self):
        result = translate_line('\txsra\t%r11, 16\n')
        self.assertEqual(result, ['\tsra\t%r11, 8', '\tsra\t%r11, 8'])

    def test_xsrl_31(self):
        result = translate_line('\txsrl\t%r10, 31\n')
        self.assertEqual(result, ['\tsrl\t%r10, 8', '\tsrl\t%r10, 8',
                                  '\tsrl\t%r10, 8', '\tsrl\t%r10, 7'])


class TestXshiftRegReg(unittest.TestCase):
    """xsll/xsra/xsrl %rd, %rs — register-register shifts have no extended
    semantics; the x prefix is just cosmetic and is stripped."""

    def test_xsll_reg(self):
        self.assertEqual(translate_line('\txsll\t%r5, %r4\n'),
                         ['\tsll\t%r5, %r4'])

    def test_xsra_reg(self):
        self.assertEqual(translate_line('\txsra\t%r10, %r11\n'),
                         ['\tsra\t%r10, %r11'])


class TestXaluRdEqRs(unittest.TestCase):
    """xadd/xsub/xand/xoor with rd == rs — collapses to 2-operand form."""

    def test_xadd_fits_imm6(self):
        # 32 fits imm6 (0..63) → no ext.
        self.assertEqual(translate_line('\txadd\t%r1,%r1,32\n'),
                         ['\tadd\t%r1, 32'])

    def test_xadd_needs_1ext(self):
        # 832 > 63 but < 2^19 → 1 ext, zero-extended.
        # imm13 = (832 >> 6) & 0x1FFF = 13
        # imm6  =  832 & 0x3F = 0
        self.assertEqual(translate_line('\txadd\t%r12,%r12,832\n'),
                         ['\text\t13', '\tadd\t%r12, 0'])

    def test_xsub_fits_imm6(self):
        self.assertEqual(translate_line('\txsub\t%r10,%r10,1\n'),
                         ['\tsub\t%r10, 1'])

    def test_xsub_needs_1ext(self):
        # 24576 = 0x6000 < 2^19; imm13 = 384, imm6 = 0
        self.assertEqual(translate_line('\txsub\t%r0,%r0,24576\n'),
                         ['\text\t384', '\tsub\t%r0, 0'])

    def test_xand_fits_sign6(self):
        # 1 fits sign6 → no ext.
        self.assertEqual(translate_line('\txand\t%r10,%r10,1\n'),
                         ['\tand\t%r10, 1'])

    def test_xand_needs_1ext(self):
        # 0x1FFF = 8191, fits sign19. imm13 = 127, sign6 = -1.
        self.assertEqual(translate_line('\txand\t%r11,%r11,0x00001fff\n'),
                         ['\text\t127', '\tand\t%r11, -1'])

    def test_xand_needs_2ext_negative(self):
        # 0xfffffffc = -4. Doesn't fit sign6 (-4 fits actually -32..31 yes).
        # -4 IS in sign6, so this should be 0 ext.
        self.assertEqual(translate_line('\txand\t%r0,%r0,0xfffffffc\n'),
                         ['\tand\t%r0, -4'])

    def test_xand_needs_2ext_large_negative(self):
        # 0xffffffc0 = -64. sign6 range is -32..31, so -64 doesn't fit.
        # Fits sign19 (-2^18 ≤ -64 < 2^18) → 1 ext.
        # imm13 = (-64 >> 6) & 0x1FFF = (-1) & 0x1FFF = 0x1FFF = 8191
        # sign6 = sign6(-64) = sign6(0) = 0
        self.assertEqual(translate_line('\txand\t%r11,%r11,0xffffffc0\n'),
                         ['\text\t8191', '\tand\t%r11, 0'])

    def test_xoor_fits_sign6(self):
        self.assertEqual(translate_line('\txoor\t%r9,%r9,0x10\n'),
                         ['\tor\t%r9, 16'])

    def test_xoor_needs_1ext(self):
        # 0x100 = 256, fits sign19. imm13 = 4, sign6 = 0
        self.assertEqual(translate_line('\txoor\t%r9,%r9,0x100\n'),
                         ['\text\t4', '\tor\t%r9, 0'])

    def test_xoor_needs_1ext_700(self):
        # 0x700 = 1792, imm13 = 0x1c = 28, sign6 = 0
        self.assertEqual(translate_line('\txoor\t%r9,%r9,0x700\n'),
                         ['\text\t28', '\tor\t%r9, 0'])


class TestXaluRdNeRs(unittest.TestCase):
    """xadd/xsub/xand/xoor with rd != rs — Class 1 3-operand form.

    The encoded instruction is rd = rs <op> ext_immediate (zero-extended).
    Negative immediates cannot be represented in this form and are rejected.
    """

    def test_xadd_small(self):
        # rd != rs, imm = 3, fits 13-bit unsigned → 1 ext.
        self.assertEqual(translate_line('\txadd\t%r0,%r3,3\n'),
                         ['\text\t3', '\tadd\t%r0, %r3'])

    def test_xadd_24576(self):
        # 24576 > 8191 → 2 ext.
        # ext_hi = (24576 >> 13) & 0x1FFF = 3, ext_lo = 24576 & 0x1FFF = 0
        self.assertEqual(translate_line('\txadd\t%r11,%r0,24576\n'),
                         ['\text\t3', '\text\t0', '\tadd\t%r11, %r0'])

    def test_xsub_64(self):
        self.assertEqual(translate_line('\txsub\t%r13,%r12,64\n'),
                         ['\text\t64', '\tsub\t%r13, %r12'])

    def test_xsub_60160(self):
        # 60160 = 0xEB00; > 8191 → 2 ext.
        # ext_hi = (60160 >> 13) & 0x1FFF = 7, ext_lo = 60160 & 0x1FFF = 0xB00 = 2816
        self.assertEqual(translate_line('\txsub\t%r11,%r2,60160\n'),
                         ['\text\t7', '\text\t2816', '\tsub\t%r11, %r2'])


class TestXcmp(unittest.TestCase):
    def test_fits_sign6(self):
        # 10 fits sign6 → no ext.
        self.assertEqual(translate_line('\txcmp\t%r0,10\n'),
                         ['\tcmp\t%r0, 10'])

    def test_needs_1ext(self):
        # 100 doesn't fit sign6 (31). imm13 = 1, sign6 = 36 & 0x3F = sign6(36) =
        # 36 - 64 = -28. Verify: (1<<6) | (-28 & 0x3F) = 64 | 36 = 100 ✓
        self.assertEqual(translate_line('\txcmp\t%r11,100\n'),
                         ['\text\t1', '\tcmp\t%r11, -28'])


class TestXbranch(unittest.TestCase):
    """xjp/xjr*/xcall — just drop the x prefix; AsmParser auto-emits ext."""

    def test_xjp(self):
        self.assertEqual(translate_line('\txjp\t__L10\n'),
                         ['\tjp\t__L10'])

    def test_xjreq(self):
        self.assertEqual(translate_line('\txjreq\tfoo\n'),
                         ['\tjreq\tfoo'])

    def test_xjrne_d(self):
        self.assertEqual(translate_line('\txjrne.d\tbar\n'),
                         ['\tjrne.d\tbar'])

    def test_xcall(self):
        self.assertEqual(translate_line('\txcall\twrite_bg0\n'),
                         ['\tcall\twrite_bg0'])

    def test_xjrugt(self):
        self.assertEqual(translate_line('\txjrugt\t__L4\n'),
                         ['\tjrugt\t__L4'])


class TestDirectives(unittest.TestCase):
    def test_code_to_text(self):
        self.assertEqual(translate_line('\t.code\n'), ['\t.text'])

    def test_half_to_short(self):
        self.assertEqual(translate_line('\t.half\t0\n'), ['\t.short\t0'])

    def test_word_to_long(self):
        self.assertEqual(translate_line('\t.word\t-1\n'), ['\t.long\t-1'])

    def test_lcomm_space_to_comma(self):
        self.assertEqual(translate_line('\t.lcomm\tbg0a 4\n'),
                         ['\t.lcomm\tbg0a, 4'])

    def test_comm_space_to_comma(self):
        self.assertEqual(translate_line('\t.comm\tfoo 16\n'),
                         ['\t.comm\tfoo, 16'])


class TestSpRelative(unittest.TestCase):
    """as33 source notation for %sp uses byte units, but LLVM's [%sp+imm6]
    encodes the offset in word/halfword/byte units depending on the transfer
    size. asm33conv divides by the scale and always emits the [%sp+N] form
    (the bare [%sp] form is rejected by the LLVM AsmParser)."""

    def test_load_word_zero(self):
        # xld.w [%sp], %r12 means store r12 to sp+0; sp+0 is also word 0.
        self.assertEqual(translate_line('\txld.w\t%r12,[%sp]\n'),
                         ['\tld.w\t%r12, [%sp+0]'])

    def test_load_word_4byte(self):
        # byte offset 4 → word offset 1.
        self.assertEqual(translate_line('\txld.w\t%r11,[%sp+4]\n'),
                         ['\tld.w\t%r11, [%sp+1]'])

    def test_load_word_8byte(self):
        self.assertEqual(translate_line('\txld.w\t%r11,[%sp+8]\n'),
                         ['\tld.w\t%r11, [%sp+2]'])

    def test_store_word_zero(self):
        self.assertEqual(translate_line('\txld.w\t[%sp],%r12\n'),
                         ['\tld.w\t[%sp+0], %r12'])

    def test_load_halfword_2byte(self):
        # byte offset 2 → halfword offset 1.
        self.assertEqual(translate_line('\txld.h\t%r10,[%sp+2]\n'),
                         ['\tld.h\t%r10, [%sp+1]'])

    def test_load_byte_3(self):
        # byte offset 3 → byte offset 3 (scale 1).
        self.assertEqual(translate_line('\txld.b\t%r10,[%sp+3]\n'),
                         ['\tld.b\t%r10, [%sp+3]'])

    def test_misaligned_word_raises(self):
        # byte offset 2 with ld.w → not 4-aligned, must reject.
        from asm33conv import translate_line as tl
        with self.assertRaises(Exception):
            tl('\txld.w\t%r10,[%sp+2]\n')


class TestSpAlu(unittest.TestCase):
    """add/sub %sp uses imm10 in word units (scaled ×4 by the hardware).
    as33 supplies the byte count; asm33conv divides by 4."""

    def test_xsub_4bytes(self):
        # xsub %sp,%sp,4 = 4 bytes = 1 imm10.
        self.assertEqual(translate_line('\txsub\t%sp,%sp,4\n'),
                         ['\tsub\t%sp, 1'])

    def test_xadd_4bytes(self):
        self.assertEqual(translate_line('\txadd\t%sp,%sp,4\n'),
                         ['\tadd\t%sp, 1'])

    def test_xsub_8bytes(self):
        self.assertEqual(translate_line('\txsub\t%sp,%sp,8\n'),
                         ['\tsub\t%sp, 2'])

    def test_xsub_12bytes(self):
        self.assertEqual(translate_line('\txsub\t%sp,%sp,12\n'),
                         ['\tsub\t%sp, 3'])

    def test_misaligned_sp_raises(self):
        from asm33conv import translate_line as tl
        with self.assertRaises(Exception):
            tl('\txsub\t%sp,%sp,3\n')


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
        self.assertEqual(translate_line('\n'), [''])

    def test_comment_line(self):
        line = '; this is a comment\n'
        self.assertEqual(translate_line(line), ['; this is a comment'])

    def test_endfile_removed(self):
        self.assertEqual(translate_line('\t.endfile\n'), [])
        self.assertEqual(translate_line('.endfile\n'), [])

    def test_plain_ld_uh_passthrough(self):
        line = '\tld.uh\t%r10,[%r13]\n'
        self.assertEqual(translate_line(line), ['\tld.uh\t%r10,[%r13]'])

    def test_ext_passthrough(self):
        line = '\text\t0\n'
        self.assertEqual(translate_line(line), ['\text\t0'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
