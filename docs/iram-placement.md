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
            │     (空き)       │    ← _stacklen=0 時、ここにスタックが降りてくる
            │                  │
            │       ↑          │
            │     stack        │  ← _stacklen=0 のとき。SP 初期値 = 0x2000
  0x002000  └──────────────────┘

  0x100000  ┌──────────────────┐
            │  pceAppHead      │
            │  .text / .rodata │
            │  .init_array     │
            │  .data           │
            │  .fastdata (LMA) │  ← 起動時に IRAM へコピーされる実体
            │  .fastrun  (LMA) │  ← 同上
            │  .bss            │  ← __END_DEFAULT_BSS = カーネルヒープ先頭
            │     ...          │
            │  heap / stack    │  ← _stacklen>0 のとき、ここ以降にスタック
            └──────────────────┘
```

- 内蔵 RAM ユーザ領域は **0x1000–0x2000 の 4 KiB**。`pceAppHead.stack_size` (`_stacklen`) が 0 のときはスタックが 0x2000 から低位方向に伸びて IRAM を共有するため、`__firam_end` とスタック先端 (SP) の衝突を避ける責任はアプリ側にあります。`_stacklen>0` を指定するとスタックは SRAM 側に移動し、IRAM 4 KiB を配置物だけに使えます。
- `.fastdata` / `.fastrun` は VMA (実行時アドレス) が IRAM、LMA (ロードアドレス) は SRAM。`.pex` イメージには SRAM 上の LMA 位置にデータが含まれ、起動時に `crt0` が IRAM へコピーします。
- **SRAM 側の配置順は `.data` → `.fastdata` LMA → `.fastrun` LMA → `.bss`**。`__END_DEFAULT_BSS`(カーネルが `pceAppHead.bss_end` 経由でヒープ先頭として使用)がすべての LMA より上に来るようにしてあります。そうしないと、カーネルの `ResetHeap(bss_end)` が `crt0` による IRAM コピー前に LMA バイトを書き換えて、配置コードが起動時点で壊れます。
- `.fastbss` は NOLOAD なので `.pex` イメージには含まれません。`crt0` がゼロクリアします。
- 内蔵 RAM を一切使わないアプリケーションでは、全セクションがサイズ 0 となりイメージ・ランタイムともゼロコストです。

### 配置順序 (data が先、code が後)

IRAM は **低位アドレスから data、末尾側に code** の順で配置します。こうしておくと、もし `.fastrun` のサイズが予想より大きくなった場合でも、変数領域 (低位側) を踏まず、高位側のスタック領域に衝突するかたちで顕在化するため、サイレントな変数破壊より検知しやすくなります。

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

これは `piece.ld` の `ASSERT(__fastrun_end <= 0x2000, ...)` によります。4 KiB は物理的な上限で、これ以上は IRAM に置けません。

ただし実際には 4 KiB をフルに使えるわけではなく、**スタックが IRAM を共有するかどうか** で使える量が変わります。

## スタックと IRAM 配置の関係

P/ECE カーネルは `pceAppHead.stack_size` (リンカシンボル `_stacklen`) の値で **スタックの置き場所を切り替えます** ([`sdk/sysdev/pcekn/runapp.c`](../sdk/sysdev/pcekn/runapp.c) の `InitHeapAndSP()`)。

| `_stacklen` | SP 初期値 | スタック領域 | ヒープ先頭 |
|---|---|---|---|
| 0 (既定) | `0x2000` | **IRAM `0x1000-0x2000`** | `bss_end` (SRAM) |
| `> 0` | `bss_end + stack_size` | SRAM (`bss_end` から上に `stack_size` バイト) | SRAM (スタックの直上) |

### `_stacklen == 0` の場合

スタックが IRAM 0x1000-0x2000 を共有します。配置物とスタックが向かい合う形で、スタックが `__firam_end` より下に降りて来ると **配置したコード・データが実行中に書き換わります**。典型的な症状:

- アドレス不整例外が関数先頭付近で発生する(書き換わった命令が想定外のレジスタを破壊)
- 特定の関数が壊れる(常に同じ場所、書き換わるバイト位置が固定されるため)
- 再現条件が呼び出し深さに依存し、軽い入力では通るが重い入力で壊れる

スタック余裕 = `0x2000 - __firam_end`。参考までに pmdplay(`__firam_end ≈ 0x1b94`、残り余裕 ≈ 1.1 KiB)は `_stacklen=0` でも MAXCH=26 で問題なく動作しています。これより余裕が狭くなるサイズで配置する、または再帰的な処理などで深いコールスタックが予想される場合は `_stacklen > 0` を検討してください。

### `_stacklen > 0` の場合

スタックは SRAM 側 `bss_end` 以降に確保され、IRAM は配置物専用になります。スタック余裕を気にしたくない・IRAM をほぼ 4 KiB 使い切る、といった場合はこちらが無難です。

設定方法は LDFLAGS に `--defsym` を渡します:

```make
LDFLAGS += -Wl,--defsym,_stacklen=0x1000
```

`0x1000` (4 KiB) がまず試す値。深いコールスタックが予想されるなら 0x2000 (8 KiB) 以上でも構いません。値は 4 バイト単位に切り上げられます。

### `_stacklen` を既定で強制しない理由

既存のアプリには `_stacklen=0` のまま IRAM 配置を控えめに使うものもあり、実際の余裕があれば十分動きます。リンカスクリプトではデフォルトを `0` のままにしてあり、必要に応じてプロジェクト個別に指定する方針です。

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
