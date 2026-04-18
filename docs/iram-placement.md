# 内蔵RAMへのコード・データの置き方

P/ECE (S1C33209) は CPU コア内蔵の高速 RAM を持っており、ここに配置したコード・データへのアクセスは外部 SRAM より高速です。本 LLVM ツールチェインはスタートアップコード (`crt0.o`) とリンカスクリプト (`piece.ld`) 側で、任意の関数・変数を内蔵 RAM に配置する機能を標準サポートします。

本文書はアプリケーション開発者向けに、その使い方・制約・注意事項を説明します。

## メモリ配置の全体像

```
  0x001000  ┌──────────────────┐
            │  .fastdata       │  ← 初期化あり変数 (IRAM VMA, SRAM LMA)
            ├──────────────────┤
            │  .fastbss        │  ← 初期化なし変数 (IRAM, NOLOAD)
            ├──────────────────┤
            │  .fastrun        │  ← コード (IRAM VMA, SRAM LMA)
            ├──────────────────┤  __firam_end
            │                  │
            │     (空き)       │    ← 上からは配置物、下からはスタックが侵入
            │                  │
            │       ↑          │
            │     stack        │
  0x002000  └──────────────────┘  ← スタック底 (カーネル既定、SP 初期値)

  0x100000  ┌──────────────────┐
            │  pceAppHead      │
            │  .text / .rodata │
            │  .init_array     │
            │  .data           │
            │  .fastdata (LMA) │  ← 起動時に IRAM へコピーされる実体
            │  .fastrun  (LMA) │  ← 同上
            │  .bss            │
            │     ...          │
            └──────────────────┘
```

- 内蔵 RAM ユーザ領域は **0x1000–0x2000 の 4 KiB**。スタックは 0x2000 を底として **低位方向** に伸びるため、IRAM 配置物とスタックは同じ 4 KiB 領域を向かい合う形で共有します。配置物の末尾 `__firam_end` とスタック先端 (SP) の間に十分な余裕を確保する責任はアプリ側にあります。
- `.fastdata` / `.fastrun` は VMA (実行時アドレス) が IRAM、LMA (ロードアドレス) は SRAM。`.pex` イメージには SRAM 上の LMA 位置にデータが含まれ、起動時に `crt0` が IRAM へコピーします。
- `.fastbss` は NOLOAD なので `.pex` イメージには含まれません。`crt0` がゼロクリアします。
- 内蔵 RAM を一切使わないアプリケーションでは、全セクションがサイズ 0 となりイメージ・ランタイムともゼロコストです。

### 配置順序 (data が先、code が後)

IRAM は **低位アドレスから data、末尾側に code** の順で配置します。これは一部のアプリケーションが `.fastrun` 領域を実行時に書き換えて関数をオーバーレイする場合の安全側の配慮です。オーバーレイが想定サイズを超過しても変数領域 (低位側) を踏まず、高位側のスタック領域と衝突するかたちで顕在化するため、サイレントな変数破壊より検知しやすくなります。オーバーレイは本プロジェクトの公式サポート対象ではありませんが、レイアウトはできる限り事故が起きないようにしてあります。

## ソースコードでの指定方法

### 範囲で指定する: `#pragma clang section`

gcc33 の「ここからここまで」に近い書き方です。pragma で宣言したセクションは以降の **宣言** に対して適用され、空文字列を渡すと解除されます。

```c
#pragma clang section text=".fastrun" data=".fastdata" bss=".fastbss"

void hot_inner_loop(int *p, int n) {
    for (int i = 0; i < n; i++) p[i] = p[i] * 3 + 1;
}

int  hot_counter;                  /* 初期化なし → .fastbss */
int  hot_table[4] = {1,2,3,4};     /* 初期化あり → .fastdata */

#pragma clang section text="" data="" bss=""
```

注意:
- pragma は **宣言** に作用します。関数本体の途中に挟んでもその関数には効きません。
- `text=` / `data=` / `bss=` は独立に指定できます。初期化なし変数だけ IRAM に置くといった使い方も可能です。

### 個別に指定する: `__attribute__((section(...)))`

1 要素だけ IRAM に置きたい場合はこちらが簡潔です。

```c
__attribute__((section(".fastrun")))
void vbl_irq(void) { ... }

__attribute__((section(".fastdata")))
static int lookup[256] = { ... };
```

両方の方法は共存できます。

## リンカが提供するシンボル

以下のシンボルがリンカスクリプトから提供されます。通常アプリから直接触る必要はありませんが、スタック再配置などの判断に使えます。

| シンボル | 意味 |
|---|---|
| `__fastdata_start` / `__fastdata_end` | IRAM 上の `.fastdata` 範囲 (VMA) |
| `__fastdata_load` | SRAM 上の `.fastdata` ロード元 (LMA) |
| `__fastbss_start` / `__fastbss_end` | IRAM 上の `.fastbss` 範囲 |
| `__fastrun_start` / `__fastrun_end` | IRAM 上の `.fastrun` 範囲 (VMA) |
| `__fastrun_load` | SRAM 上の `.fastrun` ロード元 (LMA) |
| `__firam_end` | IRAM 配置物の末尾 (= `__fastrun_end`)。スタック底までの残り領域を判断する際の基準 |

C コードから参照する場合の典型:

```c
extern unsigned char __firam_end[];
extern unsigned char __fastrun_start[];
extern unsigned char __fastrun_end[];
```

## サイズ制約とオーバーフロー検出

`.fastdata` + `.fastbss` + `.fastrun` の合計が **0x1000 バイト (4 KiB)** を超えるとリンク時に以下のエラーで停止します:

```
ld.lld: error: ... Internal RAM overflow: .fastdata/.fastbss/.fastrun exceed user area (0x1000-0x2000)
```

これは `piece.ld` の `ASSERT(__fastrun_end <= 0x2000, ...)` によります。サイズが 4 KiB に収まらない場合の選択肢は以下のとおりです。

1. **IRAM に置く対象を減らす**: 最もホットな部分だけに絞る。全部 IRAM に詰め込んでも性能は上がりません。
2. **スタックを SRAM に再配置する**: `pceAppHead.stack_size` を非 0 にすると、カーネルはスタックを SRAM 側に確保します。これにより IRAM 4 KiB をまるごと配置物に使えるようになります (物理的な上限はあくまで 4 KiB)。スタック再配置は別途アプリヘッダ設定の話題なので本文書では扱いません。

## スタートアップでの動作

`crt0.o` (`tools/crt/crt0.c`) の `pceAppInit00` が以下を順に行います。

1. `.bss` ゼロクリア (従来どおり)
2. `.fastrun` を SRAM LMA → IRAM VMA にワード単位コピー
3. `.fastdata` を同様にコピー
4. `.fastbss` ゼロクリア
5. `__version_check` / `__init_array` / `pceAppInit()` の順に実行

すべて word (4 バイト) 単位のループで、アラインメントはリンカの `ALIGN(4)` で保証されています。対象セクションが空 (size 0) の場合はループが 0 回で抜けるので、IRAM 非使用アプリでもオーバヘッドはありません。

## よくある落とし穴

### 割り込みハンドラを IRAM に置く

`__attribute__((interrupt_handler))` と `__attribute__((section(".fastrun")))` は併用できます。ただし、割り込みが **発生する前に** `crt0` のコピーが完了している必要があります。P/ECE では `pceAppInit00` 内でコピーが完了してから `pceAppInit()` が呼ばれ、そこからユーザコードが走るため通常は問題になりません。自作 `_start` を書くなど特殊な場合のみ注意してください。

### `.fastrun` 内関数からの外部シンボル参照

IRAM (0x1000 付近) と SRAM (0x100000 付近) はアドレスが離れています。S1C33 は 28 bit アドレス空間なので通常の `ext+ext+call` / `ext+ext+ld.w` で問題なく届きます。`call.d` 系の PC 相対呼び出しは符号付き 21 bit 範囲を超えれば自動的に長い形式にリラックスされます。ユーザが意識する必要はありません。

### ポインタ初期値

`.fastdata` に `int *p = &global_var;` のようにグローバル変数へのポインタを置いても問題ありません。LMA 上の初期値は SRAM 上の正しい絶対アドレスで、コピー後も同じ値のまま IRAM に載ります。

### `const` データ

`const` を付けた初期化済みデータは通常 `.rodata` (SRAM 側) に置かれます。これを IRAM に移したい場合は明示的にセクション指定が必要です:

```c
__attribute__((section(".fastdata")))
static const unsigned short sine_table[256] = { ... };
```

ただし `.fastdata` に置くと起動時にコピーされるため、`.pex` のイメージサイズ + IRAM 使用量の両方を食います。SRAM 側でも十分な速度が出る `const` テーブルは `.rodata` のままにしておくのが通例です。

### オーバーレイは非サポート

複数の関数群を同じ IRAM 領域に切り替えながら走らせるオーバーレイ手法は、本ツールチェインでは直接はサポートしません。必要な場合はアプリケーション側で `__fastrun_load` / `__fastrun_start` のコピーロジックを自前で書くことになります。配置順序 (data が先、code が後) はオーバーレイ時に変数が破壊されないよう安全側に倒してありますが、それ以上の保証はありません。

## 参考: 実装箇所

- リンカスクリプト: `tools/piece.ld`
- スタートアップ: `tools/crt/crt0.c` の `pceAppInit00`
- インストール先: `sysroot/s1c33-none-elf/lib/{piece.ld,crt0.o}` (`make` で自動更新)
