#!/usr/bin/env python3
"""asm33conv.py — pp33/ext33 拡張構文 → LLVM アセンブリ変換器

使い方:
    asm33conv.py input.s [-o output.s]

拡張命令を基本命令の悲観的展開に変換する。

変換対象:
  xld.w / xld.b / xld.uh — レジスタ間接（オフセット付き）ロード/ストア、即値ロード、絶対アドレス [label]
  xsrl / xsra / xsla / xsll — 拡張シフト (即値、レジスタ-レジスタ)
  xadd / xsub             — 3 オペランド加減算 (unsigned imm6 base)
  xand / xoor             — 3 オペランド論理演算 (signed sign6 base)
  xcmp                    — 拡張範囲比較
  xjp / xjr* / xcall      — 拡張範囲分岐 (x を外すだけ; AsmParser が自動 ext)
  .code / .half / .word   — ディレクティブを LLVM 標準 (.text / .short / .long) に翻訳

変換しないもの:
  ld.w, add, sub, cmp, ... などの基本命令はそのまま出力する。
  .global, ラベル定義, コメントもそのまま出力する。
  .endfile は削除する（LLVM アセンブラでは不要）。

アーキテクチャメモ:
  S1C33000 CPU マニュアル §2.5.2 (5): ext 命令を使用すると、通常のレジスタ
  間接アドレッシング [%rb] が ext で指定した即値をディスプレースメントとする
  アドレッシングモードに変わる。
    ext N
    ld.w %rd, [%rb]    ; [%rb + N] として機能する

  LLVM AsmParser は [%rb+N] 構文を受け付けないため、オフセットは ext 側にのみ
  記述し、ベース命令は常に [%rb]（オフセットなし）形式で出力する。

  reg-indirect のオフセット展開 (悲観的 2-ext):
    ext (N >> 13) & 0x1FFF   ; ビット 25:13
    ext  N        & 0x1FFF   ; ビット 12:0
    ld.w %rd, [%rb]          ; ディスプレースメントなし (ext で指定済み)
  MC リラクゼーションで不要な ext は除去される。

  シフト命令は ext を使えない（CPU マニュアル明記）。
  xsrl/xsra/xsla N は srl/sra/sll を N//8 回 + 1 回（余り）に分割する。

  3 オペランド ALU (xadd/xsub/xand/xoor %rd,%rs,imm) の展開:
    - rd == rs の場合は 2 オペランド形に縮退し、ext 即値展開を使う。
        unsigned 命令 (add/sub): combined = zero_ext_19/zero_ext_32
        signed   命令 (and/or):  combined = sign_ext_19/sign_ext_32
    - rd != rs の場合は Class 1 ALU 形式 (ext imm; op %rd, %rs) を用いる。
        この形式では imm は常に zero-extend (符号なし)。負の imm は使用不可。

  絶対アドレスメモリアクセス (xld.X %rd,[label] / xld.X [label],%rs) の展開:
    ext label@ah ; ext label@al ; ld.X ..., [%r8]
    @ah/@al は 26 ビット絶対アドレスを 13+13 に分割する relocation 修飾子。
    R8 は P/ECE カーネルにより 0 に初期化されているため [%r8] が絶対参照となる。

  拡張範囲分岐 (xjp/xjr*/xcall) は単純に x を外すだけ。LLVM の AsmParser は
  label が短距離で届けば ext を出さず、届かなければ自動で ext を挿入する。
"""

import re
import sys
import argparse


# ---------------------------------------------------------------------------
# ヘルパー
# ---------------------------------------------------------------------------

def parse_int(s):
    """0x... / 0b... / 十進数 / 負数を受け付ける。"""
    s = s.strip()
    try:
        return int(s, 0)
    except ValueError:
        raise ValueError(f"cannot parse integer: {s!r}")


def sign6(v):
    """値 v の下位 6 ビットを 6 ビット符号付き整数として返す (−32 〜 +31)。"""
    b = v & 0x3F
    return b if b < 32 else b - 64


def _to_signed_32(imm):
    """32 ビット符号なしリテラル (2^31..2^32-1) を符号付き 32 ビット相当に正規化する。

    as33 ソースは bitmask 用途の値を `0xfffffffc` のような正の hex で書くことが
    多いが、xand/xoor/xcmp など signed sign6 ベースの命令ではビットパターンと
    しての等価値 (-4) で扱う方が短い ext 列で表現できる。
    """
    if (1 << 31) <= imm < (1 << 32):
        return imm - (1 << 32)
    return imm


def xld_to_ld(xmnem):
    """"xld.w" → "ld.w" のように x を除去してベース命令名にする。"""
    return xmnem[1:]   # 先頭の 'x' を取り除く


# ---------------------------------------------------------------------------
# 展開関数
# ---------------------------------------------------------------------------

def _ext_lines(offset):
    """
    レジスタ間接 [%rb] 用のオフセットを ext 命令列に変換する（最小限の ext 数）。

    [%rb] 系命令に対する ext の変位計算:
      1 ext: displacement = sign_extend_13(imm13)  ← imm13 を直接変位として使う
      2 ext: displacement = (ext1_imm13 << 13) | ext2_imm13  (26 bit)

    ld.w/ld.b 等の即値命令とは式が異なる（即値命令は sign_extend_19((imm13<<6)|sign6)）。
    S1C33ExpandExtPseudos.cpp: ext_imm13 = Off & 0x1FFF で 1 ext を生成していることから確認済み。

    offset == 0     → [] (ext 不要)
    -4096..4095     → [ext offset]  (1 ext, sign13 直接)
    それ以外        → [ext hi, ext lo]  (2 ext, 26 bit 分割)

    注意: 「6 bit には収まるが sign6 解釈では負になる値」(32..4095 など) は
    そのまま imm13 として渡せばよく、sign_extend_13 が正しい変位を返す。
    """
    if offset == 0:
        return []
    # 1 ext: sign_extend_13(imm13) = offset  →  imm13 = offset, -4096 <= offset <= 4095
    if -(1 << 12) <= offset < (1 << 12):
        return [f"\text\t{offset & 0x1FFF}"]
    # 2 ext: (ext1 << 13) | ext2 = offset
    ext_hi = (offset >> 13) & 0x1FFF
    ext_lo = offset & 0x1FFF
    return [f"\text\t{ext_hi}", f"\text\t{ext_lo}"]


# SP-relative ld/st offsets are encoded in scaled units (word / halfword /
# byte) in the imm6 field. as33 source notation, however, uses the actual
# byte offset for all transfer sizes; we must divide it by the scale before
# emitting the LLVM `[%sp+N]` form. Register-indirect (non-%sp) operands keep
# the byte offset as-is because ext supplies an unscaled byte displacement.
_SP_SCALE_FOR_LD = {
    'ld.w':  4,
    'ld.h':  2, 'ld.uh': 2,
    'ld.b':  1, 'ld.ub': 1,
}


def _is_sp(reg):
    return reg.lower() == '%sp'


def _sp_offset(mnem, byte_offset):
    scale = _SP_SCALE_FOR_LD[mnem]
    if byte_offset % scale != 0:
        raise ValueError(
            f"{mnem} [%sp+{byte_offset}] is not aligned to {scale}-byte boundary"
        )
    return byte_offset // scale


def expand_xld_load(mnem, rd, rb, offset, comment):
    """
    xld.X %rd, [%rb+N]  → (最小限の ext) + ld.X %rd, [%rb]
    オフセットは ext に畳み込み、ベース命令は [%rb]（オフセットなし）にする。
    LLVM AsmParser は [%rb+N] 構文を受け付けないためこの形式で出力する。
    offset==0 の場合は ext を省略する（MCリラクゼーションは bare ext を除去しない）。

    %sp の場合は別形式 (SP-relative imm6) を使い、imm6 は要素サイズで割った
    値を直接エンコードする (LLVM は [%sp+N] でも N は imm6 ワード/ハーフ/バイト
    数として扱う)。LLVM AsmParser は [%sp] (オフセット無し) を受け付けないので
    オフセット 0 でも [%sp+0] を明示する。
    """
    if _is_sp(rb):
        scaled = _sp_offset(mnem, offset)
        return [f"\t{mnem}\t{rd}, [%sp+{scaled}]{comment}"]
    lines = _ext_lines(offset)
    lines.append(f"\t{mnem}\t{rd}, [{rb}]{comment}")
    return lines


def expand_xld_store(mnem, rb, offset, rs, comment):
    """
    xld.X [%rb+N], %rs  → (最小限の ext) + ld.X [%rb], %rs
    %sp の場合は expand_xld_load と同じく SP-relative 形式を出力する。
    """
    if _is_sp(rb):
        scaled = _sp_offset(mnem, offset)
        return [f"\t{mnem}\t[%sp+{scaled}], {rs}{comment}"]
    lines = _ext_lines(offset)
    lines.append(f"\t{mnem}\t[{rb}], {rs}{comment}")
    return lines


def expand_xld_imm(rd, value, comment):
    """
    xld.w %rd, imm32 の最適展開:
      sign6 範囲 (-32..31):  ld.w %rd, value         (0 ext)
      sign19 範囲:            ext imm13; ld.w %rd, base  (1 ext)
      それ以外:               ext hi; ext mid; ld.w %rd, base  (2 ext)

    1 ext の実効値: sign_extend_19((imm13 << 6) | (base & 0x3F)) = value
      imm13 = (value >> 6) & 0x1FFF, base = sign6(value)
    2 ext の実効値: (ext_hi << 19) | (ext_mid << 6) | (base & 0x3F) = value

    0x80000000..0xFFFFFFFF を 32-bit unsigned hex で書いた場合は -2^31..-1 の
    符号付き値として正規化する。エンコーディングは同じだが範囲判定が短くなる。
    """
    value = _to_signed_32(value)
    base  = sign6(value)
    # sign6 範囲内は ext 不要
    if -32 <= value <= 31:
        return [f"\tld.w\t{rd}, {value}{comment}"]
    # 1 ext で表現できるか: -(2^18) <= value < 2^18
    if -(1 << 18) <= value < (1 << 18):
        imm13 = (value >> 6) & 0x1FFF
        return [
            f"\text\t{imm13}",
            f"\tld.w\t{rd}, {base}{comment}",
        ]
    # 2 ext（悲観的展開）
    ext_hi  = (value >> 19) & 0x1FFF
    ext_mid = (value >> 6)  & 0x1FFF
    return [
        f"\text\t{ext_hi}",
        f"\text\t{ext_mid}",
        f"\tld.w\t{rd}, {base}{comment}",
    ]


def expand_xshift(llvm_mnem, rd, amount, comment):
    """
    xsrl %rd, N → 複数の srl %rd, step  (step ≤ 8 ずつ)
    xsra %rd, N → 複数の sra %rd, step
    xsla %rd, N → 複数の sll %rd, step  (sla = sll for integers)
    xsll %rd, N → 複数の sll %rd, step

    S1C33 シフト命令は ext を使えない（CPU マニュアル明記）。
    1命令あたりの最大シフト量は 8。
    N = 0 の場合は命令を出力しない（シフト不要）。
    """
    if amount == 0:
        return []
    lines = []
    remaining = amount
    while remaining > 0:
        step = min(remaining, 8)
        lines.append(f"\t{llvm_mnem}\t{rd}, {step}")
        remaining -= step
    # 末尾行にのみコメントを付ける
    lines[-1] += comment
    return lines


def expand_xld_load_label(mnem, rd, label, comment):
    """
    xld.X %rd, [label] (絶対アドレス参照)
      ext label@ah ; ext label@al ; ld.X %rd, [%r8]
    R8 = 0 が前提 (P/ECE カーネルにより初期化される)。relocation 修飾子
    @ah/@al は 26-bit 絶対アドレスを 13+13 に分割する。
    """
    return [
        f"\text\t{label}@ah",
        f"\text\t{label}@al",
        f"\t{mnem}\t{rd}, [%r8]{comment}",
    ]


def expand_xld_store_label(mnem, label, rs, comment):
    """xld.X [label], %rs → ext label@ah ; ext label@al ; ld.X [%r8], %rs"""
    return [
        f"\text\t{label}@ah",
        f"\text\t{label}@al",
        f"\t{mnem}\t[%r8], {rs}{comment}",
    ]


def expand_xalu_3op(xmnem, rd, rs, imm, comment):
    """
    3 オペランド ALU (xadd/xsub/xand/xoor %rd, %rs, imm) を展開する。

    xadd/xsub は unsigned imm6 ベース (combined = zero_ext)、
    xand/xoor は signed sign6 ベース (combined = sign_ext)。

    rd == rs の場合は 2 オペランド形 + ext 即値展開:
      signed   imm:  ext (imm>>6) ; op %rd, sign6(imm)             (1 ext, sign_ext_19)
                     ext hi ; ext lo ; op %rd, sign6(imm)          (2 ext, sign_ext_32)
      unsigned imm:  ext (imm>>6) ; op %rd, imm6(imm)              (1 ext, zero_ext_19)
                     ext hi ; ext lo ; op %rd, imm6(imm)           (2 ext, zero_ext_32)

    rd != rs の場合は Class 1 3 オペランド形 (常に zero_ext):
      ext imm ; op %rd, %rs                                        (1 ext, zero_ext_13)
      ext hi ; ext lo ; op %rd, %rs                                (2 ext, zero_ext_26)
    この形式は負の imm を表現できない。signed 命令 (xand/xoor) で負の imm を
    使うコードはほぼ rd == rs であり、本ルートには到達しないはず。安全のため
    rd != rs かつ imm < 0 は ValueError を投げる。
    """
    llvm_mnem, is_signed = _XALU_TO_LLVM[xmnem.lower()]
    if is_signed:
        imm = _to_signed_32(imm)

    # add/sub %sp, imm10 encodes imm10 as a word count (imm12 = {imm10,00}).
    # The as33 source supplies the byte count, so divide by 4 before emission.
    if _is_sp(rd) and _is_sp(rs):
        if llvm_mnem not in ('add', 'sub'):
            raise ValueError(f"{xmnem} %sp form is only valid for add/sub")
        if imm % 4 != 0:
            raise ValueError(
                f"{xmnem} %sp,%sp,{imm}: stack adjustment must be a multiple of 4"
            )
        return [f"\t{llvm_mnem}\t%sp, {imm // 4}{comment}"]

    if rd == rs:
        # 2-operand form with optional ext expansion.
        if is_signed:
            if -32 <= imm <= 31:
                return [f"\t{llvm_mnem}\t{rd}, {imm}{comment}"]
            if -(1 << 18) <= imm < (1 << 18):
                ext_lo = (imm >> 6) & 0x1FFF
                return [
                    f"\text\t{ext_lo}",
                    f"\t{llvm_mnem}\t{rd}, {sign6(imm)}{comment}",
                ]
            ext_hi = (imm >> 19) & 0x1FFF
            ext_lo = (imm >> 6)  & 0x1FFF
            return [
                f"\text\t{ext_hi}",
                f"\text\t{ext_lo}",
                f"\t{llvm_mnem}\t{rd}, {sign6(imm)}{comment}",
            ]
        # Unsigned imm6 base (add / sub).
        if imm < 0:
            raise ValueError(
                f"{xmnem} immediate must be non-negative (got {imm}); "
                f"use the matching signed mnemonic if a negative immediate is intended."
            )
        if 0 <= imm <= 63:
            return [f"\t{llvm_mnem}\t{rd}, {imm}{comment}"]
        if imm < (1 << 19):
            ext_lo = (imm >> 6) & 0x1FFF
            imm6   = imm & 0x3F
            return [
                f"\text\t{ext_lo}",
                f"\t{llvm_mnem}\t{rd}, {imm6}{comment}",
            ]
        ext_hi = (imm >> 19) & 0x1FFF
        ext_lo = (imm >> 6)  & 0x1FFF
        imm6   = imm & 0x3F
        return [
            f"\text\t{ext_hi}",
            f"\text\t{ext_lo}",
            f"\t{llvm_mnem}\t{rd}, {imm6}{comment}",
        ]

    # rd != rs: Class 1 register-register form; the immediate from ext
    # replaces the %rs operand of the encoded instruction. The combined
    # value is always zero-extended.
    if imm < 0:
        raise ValueError(
            f"{xmnem} %rd != %rs requires a non-negative immediate (got {imm}); "
            f"rewrite the source so %rd == %rs."
        )
    if imm < (1 << 13):
        return [
            f"\text\t{imm}",
            f"\t{llvm_mnem}\t{rd}, {rs}{comment}",
        ]
    if imm >= (1 << 26):
        raise ValueError(
            f"{xmnem} 3-operand immediate too large for 2-ext encoding: {imm}"
        )
    ext_hi = (imm >> 13) & 0x1FFF
    ext_lo = imm & 0x1FFF
    return [
        f"\text\t{ext_hi}",
        f"\text\t{ext_lo}",
        f"\t{llvm_mnem}\t{rd}, {rs}{comment}",
    ]


def expand_xcmp(rd, imm, comment):
    """xcmp %rd, imm (signed sign6 base, like xand/xoor 2-operand)."""
    imm = _to_signed_32(imm)
    if -32 <= imm <= 31:
        return [f"\tcmp\t{rd}, {imm}{comment}"]
    if -(1 << 18) <= imm < (1 << 18):
        ext_lo = (imm >> 6) & 0x1FFF
        return [
            f"\text\t{ext_lo}",
            f"\tcmp\t{rd}, {sign6(imm)}{comment}",
        ]
    ext_hi = (imm >> 19) & 0x1FFF
    ext_lo = (imm >> 6)  & 0x1FFF
    return [
        f"\text\t{ext_hi}",
        f"\text\t{ext_lo}",
        f"\tcmp\t{rd}, {sign6(imm)}{comment}",
    ]


# ---------------------------------------------------------------------------
# 正規表現パターン
# ---------------------------------------------------------------------------

# 汎用レジスタ・特殊レジスタ (%r0〜%r15, %sp, %alr, %ahr, %psr)
_REG = r'%(?:r(?:1[0-5]|[0-9])|sp|alr|ahr|psr)'

# メモリオペランド: [%rb] または [%rb+N]
# group 1 = ベースレジスタ,  group 2 = オフセット文字列（None なら 0）
_MEM = r'\[(' + _REG + r')(?:\+([^\]]+))?\]'

# 符号付き即値（0x.../0b.../十進数/負数）
_IMM = r'(-?(?:0x[0-9A-Fa-f]+|0b[01]+|[0-9]+))'

# xld.X %rd, [%rb+N]  (ロード)
#   group1=mnem, group2=rd, group3=rb, group4=offset|None
_PAT_XLD_LOAD = re.compile(
    r'^(xld\.[wbuh]+)\s+(' + _REG + r'),\s*' + _MEM + r'\s*$',
    re.IGNORECASE,
)

# xld.X [%rb+N], %rs  (ストア)
#   group1=mnem, group2=rb, group3=offset|None, group4=rs
_PAT_XLD_STORE = re.compile(
    r'^(xld\.[wbuh]+)\s+' + _MEM + r',\s*(' + _REG + r')\s*$',
    re.IGNORECASE,
)

# xld.w %rd, imm32  (即値ロード — ブラケットなし)
#   group1=rd, group2=imm文字列
_PAT_XLD_IMM = re.compile(
    r'^xld\.w\s+(' + _REG + r'),\s*' + _IMM + r'\s*$',
    re.IGNORECASE,
)

# xsrl / xsra / xsla / xsll %rd, N
#   group1=mnem, group2=rd, group3=amount文字列
_PAT_XSHIFT = re.compile(
    r'^(xsrl|xsra|xsla|xsll)\s+(' + _REG + r'),\s*' + _IMM + r'\s*$',
    re.IGNORECASE,
)

_XSHIFT_TO_LLVM = {
    'xsrl': 'srl',
    'xsra': 'sra',
    'xsla': 'sll',  # shift-left-arithmetic = shift-left-logical for integers
    'xsll': 'sll',
}

# xsll/xsra/xsrl %rd, %rs  (register-register shift; x prefix is redundant)
_PAT_XSHIFT_RR = re.compile(
    r'^(xsrl|xsra|xsla|xsll)\s+(' + _REG + r'),\s*(' + _REG + r')\s*$',
    re.IGNORECASE,
)

# xld.X %rd, [label]  (絶対アドレスロード — label は %register ではない識別子)
_SYM = r'[A-Za-z_.][A-Za-z0-9_.$]*'
_MEM_LABEL = r'\[(' + _SYM + r')\]'

_PAT_XLD_LOAD_LABEL = re.compile(
    r'^(xld\.[wbuh]+)\s+(' + _REG + r'),\s*' + _MEM_LABEL + r'\s*$',
    re.IGNORECASE,
)

_PAT_XLD_STORE_LABEL = re.compile(
    r'^(xld\.[wbuh]+)\s+' + _MEM_LABEL + r',\s*(' + _REG + r')\s*$',
    re.IGNORECASE,
)

# xadd / xsub / xand / xoor %rd, %rs, imm  (3 オペランド)
_PAT_XALU_3OP = re.compile(
    r'^(xadd|xsub|xand|xoor)\s+(' + _REG + r'),\s*(' + _REG + r'),\s*' + _IMM + r'\s*$',
    re.IGNORECASE,
)

_XALU_TO_LLVM = {
    'xadd': ('add', False),  # unsigned imm (zero_ext with ext)
    'xsub': ('sub', False),
    'xand': ('and', True),   # signed   imm (sign_ext with ext)
    'xoor': ('or',  True),
}

# xcmp %rd, imm
_PAT_XCMP = re.compile(
    r'^xcmp\s+(' + _REG + r'),\s*' + _IMM + r'\s*$',
    re.IGNORECASE,
)

# 拡張範囲分岐: xjp / xjr{eq,ne,ge,gt,le,lt,ugt,ule,ult,uge}[.d] / xcall
# x を取るだけ。AsmParser が必要に応じて自動で ext を挿入する。
_PAT_XBRANCH = re.compile(
    r'^(xjp|xjr(?:eq|ne|ge|gt|le|lt|ugt|ule|ult|uge)|xcall)(\.d)?\s+(.+?)\s*$',
    re.IGNORECASE,
)

# ディレクティブ翻訳: .code → .text, .half → .short, .word → .long
_DIRECTIVE_MAP = {
    '.code': '.text',
    '.half': '.short',
    '.word': '.long',
}
_PAT_DIRECTIVE = re.compile(
    r'^(\.code|\.half|\.word)\b(.*)$',
    re.IGNORECASE,
)

# as33 .comm / .lcomm は空白区切りで引数を取るが、LLVM はコンマ区切りを要求する:
#   as33  : .lcomm  sym 4
#   llvm  : .lcomm  sym, 4
_PAT_COMM = re.compile(
    r'^(\.l?comm)\s+(' + _SYM + r')\s+(.+?)\s*$',
    re.IGNORECASE,
)


# ---------------------------------------------------------------------------
# 行変換
# ---------------------------------------------------------------------------

def split_comment(raw):
    """
    行末のコメント（'; ...'）を切り出す。
    戻り値: (body, comment) — comment は '\t; ...' 形式または ''。
    """
    idx = raw.find(';')
    if idx == -1:
        return raw.rstrip(), ''
    body    = raw[:idx].rstrip()
    comment = '\t' + raw[idx:].rstrip()
    return body, comment


def translate_line(line):
    """
    1 行を変換して展開後の行リストを返す。
    変換不要な行は [line.rstrip()] を返す。
    .endfile は [] (削除) を返す。
    """
    raw = line.rstrip('\n\r')
    body, comment = split_comment(raw)
    stripped = body.strip()

    # 空行・コメント行
    if not stripped or stripped.startswith(';'):
        return [raw]

    # .endfile → 削除
    if re.match(r'^\.endfile\s*$', stripped, re.IGNORECASE):
        return []

    # ディレクティブ翻訳: .code / .half / .word
    m = _PAT_DIRECTIVE.match(stripped)
    if m:
        new_name = _DIRECTIVE_MAP[m.group(1).lower()]
        rest     = m.group(2)
        return [f"\t{new_name}{rest}{comment}"]

    # as33 .comm / .lcomm: 空白区切り → コンマ区切り
    m = _PAT_COMM.match(stripped)
    if m:
        directive = m.group(1)
        sym       = m.group(2)
        rest      = m.group(3)
        return [f"\t{directive}\t{sym}, {rest}{comment}"]

    # --- 拡張命令パターンを順にチェック ---

    # xld.w %rd, imm32  (即値ロード — ブラケットなし; LOAD より先にチェック)
    m = _PAT_XLD_IMM.match(stripped)
    if m:
        rd, imm_str = m.group(1), m.group(2)
        return expand_xld_imm(rd, parse_int(imm_str), comment)

    # xld.X %rd, [%rb+N]  (ロード)
    m = _PAT_XLD_LOAD.match(stripped)
    if m:
        mnem    = xld_to_ld(m.group(1))
        rd      = m.group(2)
        rb      = m.group(3)
        off_str = m.group(4)
        offset  = parse_int(off_str) if off_str else 0
        return expand_xld_load(mnem, rd, rb, offset, comment)

    # xld.X [%rb+N], %rs  (ストア)
    m = _PAT_XLD_STORE.match(stripped)
    if m:
        mnem    = xld_to_ld(m.group(1))
        rb      = m.group(2)
        off_str = m.group(3)
        rs      = m.group(4)
        offset  = parse_int(off_str) if off_str else 0
        return expand_xld_store(mnem, rb, offset, rs, comment)

    # xld.X %rd, [label]  (絶対アドレスロード)
    m = _PAT_XLD_LOAD_LABEL.match(stripped)
    if m:
        mnem  = xld_to_ld(m.group(1))
        rd    = m.group(2)
        label = m.group(3)
        return expand_xld_load_label(mnem, rd, label, comment)

    # xld.X [label], %rs  (絶対アドレスストア)
    m = _PAT_XLD_STORE_LABEL.match(stripped)
    if m:
        mnem  = xld_to_ld(m.group(1))
        label = m.group(2)
        rs    = m.group(3)
        return expand_xld_store_label(mnem, label, rs, comment)

    # xsrl / xsra / xsla / xsll  %rd, imm  (即値シフト)
    m = _PAT_XSHIFT.match(stripped)
    if m:
        llvm_mnem = _XSHIFT_TO_LLVM[m.group(1).lower()]
        rd        = m.group(2)
        amount    = parse_int(m.group(3))
        return expand_xshift(llvm_mnem, rd, amount, comment)

    # xsrl / xsra / xsla / xsll  %rd, %rs  (レジスタ-レジスタシフト; x プレフィクスは冗長)
    m = _PAT_XSHIFT_RR.match(stripped)
    if m:
        llvm_mnem = _XSHIFT_TO_LLVM[m.group(1).lower()]
        rd        = m.group(2)
        rs        = m.group(3)
        return [f"\t{llvm_mnem}\t{rd}, {rs}{comment}"]

    # xadd / xsub / xand / xoor %rd, %rs, imm
    m = _PAT_XALU_3OP.match(stripped)
    if m:
        xmnem = m.group(1).lower()
        rd    = m.group(2)
        rs    = m.group(3)
        imm   = parse_int(m.group(4))
        return expand_xalu_3op(xmnem, rd, rs, imm, comment)

    # xcmp %rd, imm
    m = _PAT_XCMP.match(stripped)
    if m:
        rd  = m.group(1)
        imm = parse_int(m.group(2))
        return expand_xcmp(rd, imm, comment)

    # xjp / xjr* / xcall — x を外すだけで AsmParser が ext を補う
    m = _PAT_XBRANCH.match(stripped)
    if m:
        base   = m.group(1)[1:]          # drop leading 'x'
        dotd   = m.group(2) or ''
        target = m.group(3)
        return [f"\t{base}{dotd}\t{target}{comment}"]

    # その他（基本命令・ディレクティブ・ラベル）はそのまま通す
    return [raw]


# ---------------------------------------------------------------------------
# メイン
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(
        description='pp33/ext33 拡張構文 → LLVM アセンブリ変換器',
        epilog='変換後のファイルは clang --target=s1c33-none-piece -c でアセンブル可能。')
    ap.add_argument('input', help='入力アセンブリファイル (.s)')
    ap.add_argument('-o', '--output', default=None,
                    help='出力ファイル (省略時は標準出力)')
    args = ap.parse_args()

    with open(args.input, encoding='utf-8', errors='replace') as f:
        lines = f.readlines()

    import datetime
    header = (
        f"// This file was automatically converted by asm33conv.py\n"
        f"// from: {args.input}\n"
        f"// on:   {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n"
        f"// Do not edit — re-run asm33conv to regenerate.\n"
    )

    out_lines = []
    for lineno, line in enumerate(lines, 1):
        try:
            expanded = translate_line(line)
        except Exception as e:
            print(f"警告: 行 {lineno}: {e}", file=sys.stderr)
            expanded = [line.rstrip()]
        out_lines.extend(expanded)

    text = header + '\n'.join(out_lines)
    if out_lines:
        text += '\n'

    if args.output:
        with open(args.output, 'w', encoding='utf-8') as f:
            f.write(text)
    else:
        sys.stdout.write(text)


if __name__ == '__main__':
    main()
