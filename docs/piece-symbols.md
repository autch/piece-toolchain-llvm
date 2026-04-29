# P/ECE 固有のリンカシンボル / メモリマップ リファレンス

S1C33 LLVM ツールチェインが提供するリンカスクリプト (`tools/piece.ld` /
sysroot にコピーされる `piece.ld`) と CRT (`tools/crt/`) は、P/ECE 環境
固有のシンボルをいくつか定義しています。本書はそれらを一覧し、用途・
デフォルト値・上書き方法を示します。

アプリ作者が日常的に触るのは `_stacklen` と `_pceheapsize` 程度ですが、
特殊なメモリレイアウトが必要なアプリ (大型 RPG / DMA バッファ多用 /
等) はそれ以外も活用できます。

---

## 1. 全体メモリマップ

```
IRAM:
  0x001000 ┌──────────────────┐
           │ .fastdata        │  IRAM VMA、SRAM LMA で起動時にコピー
           │ .fastbss         │  IRAM、NOLOAD、crt0 がゼロクリア
           │ .fastrun         │  IRAM VMA、SRAM LMA で起動時にコピー
           │   __firam_end    │  ↑ 配置物の終端
           │   (空き)         │
           │   ↑ stack        │  _stacklen=0 のとき (SP 初期値 = 0x2000)
  0x002000 └──────────────────┘  (ここから上は kernel)

SRAM (アプリ領域):
  0x100000 ┌──────────────────┐
           │ pceAppHead       │  crt0.o の .rodata 先頭、固定アドレス
           │ .text / .rodata  │
           │ .init_array      │  C++ static ctor / __attribute__((constructor))
           │ .data            │  初期値あり globals
           │ .fastdata (LMA)  │  IRAM 配置物の image (起動時 IRAM へコピー)
           │ .fastrun  (LMA)  │  同上
           │ .bss             │  __START_DEFAULT_BSS .. __END_DEFAULT_BSS
           │   _pceheapstart  │  = pceAppHead.bss_end (kernel pceHeap 基点)
           │   kernel pceHeap │  default 8 KB (= _pceheapsize)
           │   newlib sbrk    │  低位 → 高位、上限チェックなし
           │   ↑ stack        │  _stacklen>0 のとき (SP = bss_end + stacklen)
           │   ...            │
           │ kernel work area │  zlib バッファ等、app から不可侵
  0x13c000 ├──────────────────┤  _def_vbuff = SYSERRVBUFF (4 KB)
           │ kernel SYSERRV   │  system menu / system error / version_check
  0x13d000 └──────────────────┘  物理 SRAM 末端 (LENGTH(SRAM))
  0x13e000 ┌──────────────────┐  INITAPPEXTBUFF (kernel 専用)
           │ kernel internal  │
           └──────────────────┘
```

---

## 2. シンボル一覧

凡例:
- **D**: piece.ld で `PROVIDE(...)` されており `-Wl,--defsym=NAME=VAL` で上書き可能
- **L**: piece.ld または crt0 が定義する読み出し専用ラベル (上書き禁止)
- **A**: 絶対アドレス (kernel 仕様で固定)

### 2-1. スタック / ヒープ制御 (アプリで一番触る)

| シンボル | 種別 | デフォルト | 用途 |
|---|---|---|---|
| `_stacklen` | D | 0 | `pceAppHead.stack_size` に格納される。0 = kernel 既定 (SP=0x2000、IRAM 内)。非零ならスタックは SRAM 内 [bss_end+_pceheapsize, ...] に確保され、SP = `_pceheapstart + _pceheapsize + _stacklen`。 |
| `_pceheapsize` | D | 0x2000 (8 KB) | kernel pceHeap が使う領域 (規約上のサイズ)。kernel best-fit はここに低位から allocate。超過すると newlib 領域を破壊。 |
| `_pceheapstart` | D | `__END_DEFAULT_BSS` | `pceAppHead.bss_end` に格納されるアドレス。kernel `ResetHeap()` の基点。通常は変更不要。 |

**典型的な上書き例:**

```sh
# スタックを 8 KB に拡張 (SRAM 側に切る)
clang ... -Wl,--defsym=_stacklen=0x2000 ...

# 大きな malloc を多用するアプリで pceHeap zone を 16 KB に拡大
clang ... -Wl,--defsym=_pceheapsize=0x4000 ...
```

### 2-2. アプリ BSS 境界

| シンボル | 種別 | 用途 |
|---|---|---|
| `__START_DEFAULT_BSS` | L | `.bss` セクション開始アドレス。crt0 がゼロクリア開始位置。 |
| `__END_DEFAULT_BSS` | L | `.bss` セクション終了アドレス。crt0 ゼロクリア終端。`_pceheapstart` のデフォルト値。 |
| `__SIZEOF_DEFAULT_BSS` | L | `__END - __START`。crt0 のループサイズ。 |
| `pce_app_bss_end` | L | `_pceheapstart` のレガシー alias。SDK 由来コード互換のため残置。 |

これらはアプリから読み出すことはできるが、`--defsym` で上書きすると
kernel に嘘の bss_end を渡すことになるので通常変更しない。

### 2-3. 特殊アドレス (kernel との接点)

| シンボル | 種別 | 値 | 用途 |
|---|---|---|---|
| `_def_vbuff` | D / A | `0x13c000` | `pceAppNotify(APPNF_SMREQVBUF)` で kernel に渡す vbuff のデフォルト。SYSERRVBUFF と同じ kernel reserved 領域を alias し、BSS 実体を持たない (= 11 KB BSS の節約)。 |
| `_start` | L | `0x100000` | リンカへの "entry point" (実際は使われない。`pceAppHead` が固定アドレスにあるため kernel は entry を見ない)。lld の警告抑制用。 |
| `pceAppHead` | C 変数 | `0x100000` | `crt0.c` の `static const pceAPPHEAD`。kernel が直接このアドレスを読む。リンカスクリプトで `KEEP(*crt0.o(.rodata.*))` で先頭固定。 |

`_def_vbuff` を別の場所に置きたい場合 (例: app が独自 11 KB バッファを
用意する):

```sh
clang ... -Wl,--defsym=_def_vbuff=0x10A000 ...
```

ただし `pceLCDSetBuffer(_def_vbuff)` を経由して kernel が書く場合、
書込み量によっては隣接 BSS を壊すので注意。デフォルト (= SYSERRVBUFF
alias) のままが最も安全。

### 2-4. IRAM 配置 (`.fastdata` / `.fastrun` / `.fastbss`)

| シンボル | 種別 | 用途 |
|---|---|---|
| `__fastdata_start` / `_end` | L | IRAM 上の VMA 範囲 (= `0x1000..` 内) |
| `__fastdata_load` | L | SRAM 側の LMA 開始 (= `LOADADDR(.fastdata)`)。crt0 がここから VMA へコピー。 |
| `__fastbss_start` / `_end` | L | IRAM 上の NOLOAD ゼロクリア範囲 |
| `__fastrun_start` / `_end` | L | IRAM 上のホットコード VMA |
| `__fastrun_load` | L | SRAM 側の LMA 開始 (`LOADADDR(.fastrun)`) |
| `__firam_end` | L (PROVIDE) | IRAM 配置物の最後尾 (= `__fastrun_end`)。`_stacklen=0` のときスタック衝突検出に使える。 |

詳細は `docs/iram-placement.md` 参照。アプリ側はソースで
`__attribute__((section(".fastrun")))` 等を使うのが通常で、これらの
シンボルを直接 `--defsym` で操作する用途はほぼない。

### 2-5. C++ コンストラクタ

| シンボル | 種別 | 用途 |
|---|---|---|
| `__init_array_start` | L (PROVIDE_HIDDEN) | C++ static ctor / `__attribute__((constructor))` のテーブル開始 |
| `__init_array_end` | L (PROVIDE_HIDDEN) | 同テーブル終了 |

`crt0.c` の `pceAppInit00` がループで `(*fn)()` 呼び出し。アプリは触らない。

### 2-6. kernel 側の絶対アドレス (pcekn.h より)

参考情報。`piece.ld` には登場しないが、kernel API の挙動を理解する上で
重要。詳細は `sdk/sysdev/pcekn/pcekn.h` 参照。

| アドレス | 名前 | 用途 |
|---|---|---|
| `0x100000` | SRAMTOP | アプリ SRAM 先頭 |
| `(runtime)` | SRAMEND = `system_info.sram_end` | kernel が `ResetHeap` で heap 末端として使う実際のアドレス。app からはリンク時に分からない |
| `0x13c000` | SYSERRVBUFF | kernel が system error / system menu 時に使用する 4 KB vbuff (= `_def_vbuff` alias) |
| `0x13e000` | INITAPPEXTBUFF | kernel boot 時の拡張バッファ (アプリ SRAM 範囲外) |
| `0x100000` | APPSTARTPOS1 | kernel が `pCeA` シグネチャを探す先頭位置 |
| `0x138000` | APPSTARTPOS2 | 同上、第二候補 (内蔵起動アプリ用) |

---

## 3. 上書き戦略の早見表

| やりたいこと | 推奨手段 |
|---|---|
| スタック容量を増やしたい | `-Wl,--defsym=_stacklen=0xN000` (SRAM 側へ移動) |
| `malloc` を多用するので newlib heap を増やしたい | `-Wl,--defsym=_pceheapsize=0xN000` を増やす — newlib 側は kernel pceHeap zone を超えた所から始まるので、`_pceheapsize` が小さいほうが newlib の使える容量は大きい (kernel に渡す予約量を絞る) |
| 逆に kernel pceHeap が足りない | `_pceheapsize` を大きく |
| 自前のヒープ管理を入れたい | newlib `_sbrk` を override (アプリの `.o` に `_sbrk` を定義してリンク順で `-lc` より前に置く) |
| IRAM に関数を置きたい | ソースに `__attribute__((section(".fastrun")))`、変数なら `.fastdata` / `.fastbss` |
| IRAM に置きすぎてないか確認 | リンク時 `ASSERT(__fastrun_end <= 0x2000, ...)` が piece.ld にあり、エラーで弾かれる |
| version_check のエラー画面 vbuff を別に置きたい | アプリ側で 11 KB の `unsigned char my_vbuff[128*88]` を用意して `-Wl,--defsym=_def_vbuff=&my_vbuff` (実際にはシンボル指定構文の制約があるので、リンカスクリプト経由のほうが楽) |

---

## 4. 上書きが効く / 効かないシンボルの判別

ld.lld は **セクションサイズ式に登場するシンボル** に対して `--defsym`
を反映しない (script-internal 評価が `--defsym` 適用より早い段階で
固まる)。本書のシンボルのうち、`PROVIDE` で定義されていて、かつ C
コードや crt0 から **値として参照される** ものは `--defsym` で
上書きできます。

セクションサイズ式に登場する仮想シンボル (例えば過去に検討された
`_heaplen` を `. = . + _heaplen` で使うパターン) は `--defsym` で上書き
できません。本書の現行設計はこの問題を回避するように作られています。
詳細は `docs/build-howto.md` の「newlib ポートのメンテナンス」節を参照。

---

## 5. 関連文書

| 文書 | 内容 |
|---|---|
| `docs/setup.md` | 環境セットアップ手順 |
| `docs/build-howto.md` | アプリのビルドフロー、リンク順序、newlib メンテナンス |
| `docs/iram-placement.md` | IRAM 配置の詳細と制約 |
| `docs/errata.md` | CPU / kernel / SDK 由来の既知バグ |
| `docs/s1c33000_quick_reference.md` | CPU 命令セット・レジスタ・トラップ |
| `tools/piece.ld` | リンカスクリプト本体 (本書の根拠) |
| `tools/crt/crt0.c` | スタートアップ。`pceAppHead` 構造体定義 |
| `sdk/sysdev/pcekn/pcekn.h` | kernel が使う絶対アドレスの定義 (SRAMTOP / SYSERRVBUFF 等) |
| `sdk/sysdev/pcekn/runapp.c` | `InitHeapAndSP` / `ResetHeap` を呼ぶ位置 |
| `sdk/sysdev/pcekn/heapman.c` | kernel pceHeap の実装 |
