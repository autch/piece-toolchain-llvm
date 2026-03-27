#!/usr/bin/env python3
"""asm33conv.py — pp33/ext33 拡張構文 → LLVM アセンブリ変換器

使い方:
    asm33conv.py input.s [-o output.s]

拡張命令を基本命令の悲観的展開に変換する。

変換対象:
  xld.w / xld.b / xld.uh — レジスタ間接（オフセット付き）ロード/ストア、即値ロード
  xsrl / xsra / xsla      — 拡張シフト

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


def xld_to_ld(xmnem):
    """"xld.w" → "ld.w" のように x を除去してベース命令名にする。"""
    return xmnem[1:]   # 先頭の 'x' を取り除く


# ---------------------------------------------------------------------------
# 展開関数
# ---------------------------------------------------------------------------

def _ext_lines(offset):
    """
    オフセット値を ext 命令列に変換する（最小限の ext 数）:
      offset == 0         → [] (ext 不要)
      -8192 <= offset <= 8191  → [ext offset & 0x1FFF]  (1 ext)
      それ以外             → [ext hi, ext lo]           (2 ext)

    1 ext の場合: CPU は (sign_extend_19(imm13 << 6) | sign6 of base insn) として
    実効値を計算する。base insn の sign6 は 0 なので実効値 = sign_extend_19(imm13 << 6)。
    これが offset に等しくなるには offset が 64 の倍数（bits[5:0]==0）である必要がある。
    一般オフセットは 2 ext が必要なため、sign13 (-4096..4095) かつ 64 の倍数の場合のみ
    1 ext を使う。それ以外は 2 ext（悲観的展開、MCリラクゼーションは ext を縮小しない）。
    """
    if offset == 0:
        return []
    # 1 ext が正確に表現できる条件: offset は符号付き 19 ビット範囲かつ bits[5:0]==0
    # sign_extend_19(imm13 << 6) == offset  →  imm13 == offset >> 6, bits[5:0] == 0
    if (offset & 0x3F) == 0 and -(1 << 18) <= offset < (1 << 18):
        imm13 = (offset >> 6) & 0x1FFF
        return [f"\text\t{imm13}"]
    ext_hi = (offset >> 13) & 0x1FFF
    ext_lo = offset & 0x1FFF
    return [f"\text\t{ext_hi}", f"\text\t{ext_lo}"]


def expand_xld_load(mnem, rd, rb, offset, comment):
    """
    xld.X %rd, [%rb+N]  → (最小限の ext) + ld.X %rd, [%rb]
    オフセットは ext に畳み込み、ベース命令は [%rb]（オフセットなし）にする。
    LLVM AsmParser は [%rb+N] 構文を受け付けないためこの形式で出力する。
    offset==0 の場合は ext を省略する（MCリラクゼーションは bare ext を除去しない）。
    """
    lines = _ext_lines(offset)
    lines.append(f"\t{mnem}\t{rd}, [{rb}]{comment}")
    return lines


def expand_xld_store(mnem, rb, offset, rs, comment):
    """
    xld.X [%rb+N], %rs  → (最小限の ext) + ld.X [%rb], %rs
    """
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
    """
    base = sign6(value)
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

# xsrl / xsra / xsla %rd, N
#   group1=mnem, group2=rd, group3=amount文字列
_PAT_XSHIFT = re.compile(
    r'^(xsrl|xsra|xsla)\s+(' + _REG + r'),\s*' + _IMM + r'\s*$',
    re.IGNORECASE,
)

_XSHIFT_TO_LLVM = {
    'xsrl': 'srl',
    'xsra': 'sra',
    'xsla': 'sll',  # shift-left-arithmetic = shift-left-logical for integers
}


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

    # xsrl / xsra / xsla  (シフト)
    m = _PAT_XSHIFT.match(stripped)
    if m:
        llvm_mnem = _XSHIFT_TO_LLVM[m.group(1).lower()]
        rd        = m.group(2)
        amount    = parse_int(m.group(3))
        return expand_xshift(llvm_mnem, rd, amount, comment)

    # その他（基本命令・ディレクティブ・ラベル）はそのまま通す
    return [raw]


# ---------------------------------------------------------------------------
# メイン
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(
        description='pp33/ext33 拡張構文 → LLVM アセンブリ変換器',
        epilog='変換後のファイルは clang --target=s1c33-none-elf -c でアセンブル可能。')
    ap.add_argument('input', help='入力アセンブリファイル (.s)')
    ap.add_argument('-o', '--output', default=None,
                    help='出力ファイル (省略時は標準出力)')
    args = ap.parse_args()

    with open(args.input, encoding='cp932', errors='replace') as f:
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
