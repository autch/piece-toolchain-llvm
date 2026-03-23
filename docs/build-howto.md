# P/ECE アプリケーション ビルド手順

S1C33 LLVM バックエンドを使って P/ECE 用アプリケーションをビルドし、
実機に転送できる `.pex` ファイルを生成するまでの手順を説明する。

## 前提

- このリポジトリの `build/` ディレクトリに Ninja ビルドが完了していること
- `build/bin/` に `clang`, `ld.lld`, `llvm-objcopy` があること
- `tools/ppack/ppack` がビルド済みであること
- sysroot が構築済みであること（→ §1）

---

## ディレクトリ構成（参考）

```
llvm-c33/
├── build/
│   └── bin/            clang, ld.lld, llvm-objcopy, llvm-objdump, ...
├── sdk/
│   ├── include/        piece.h など SDK ヘッダ（SRF 変換元）
│   └── lib/            SDK ライブラリ（SRF 形式、変換元）
├── sysroot/
│   └── s1c33-none-elf/
│       ├── include/    SDK ヘッダのコピー
│       └── lib/
│           ├── piece.ld    P/ECE リンカースクリプト
│           ├── crt0.o      アプリヘッダ + BSS 初期化（LLVM ビルド）
│           ├── crti.o      pceAppNotify デフォルト実装（LLVM ビルド）
│           ├── libfp.a     浮動小数点ランタイム
│           ├── libidiv.a   整数除算ランタイム
│           ├── libstring.a 文字列・メモリ操作
│           ├── liblib.a    P/ECE 標準ライブラリ（malloc/atoi 等）
│           ├── libpceapi.a P/ECE カーネル API スタブ（LLVM ビルド）
│           └── libctype.a  文字種判別
├── tools/
│   ├── piece.ld        リンカースクリプト（sysroot へコピー済み）
│   ├── ppack/          ppack ツール
│   └── srf2elf/        SRF→ELF 変換ツール
└── hello.c             サンプルアプリ
```

---

## 1. sysroot の構築（初回のみ）

SDK ライブラリは EPSON 独自の SRF33 形式。`srf2elf.py` で ELF に変換し、
sysroot に配置する。SDK を更新しない限り再実行不要。

```sh
mkdir -p sysroot/s1c33-none-elf/include
mkdir -p sysroot/s1c33-none-elf/lib

# SDK ヘッダをコピー
cp sdk/include/*.h sysroot/s1c33-none-elf/include/

# リンカースクリプトをコピー
cp tools/piece.ld sysroot/s1c33-none-elf/lib/piece.ld

# CRT・カーネル API スタブのビルド + SDK ライブラリの SRF→ELF 変換（一括）
make -C tools/crt

# オプションライブラリ（-lmuslib / -lsprite で明示指定して使用）
python3 tools/srf2elf/srf2elf.py sdk/lib/muslib.lib sysroot/s1c33-none-elf/lib/libmuslib.a
python3 tools/srf2elf/srf2elf.py sdk/lib/sprite.lib sysroot/s1c33-none-elf/lib/libsprite.a
```

### sysroot に含まれるファイルの内容

**スタートアップ（リンク先頭に自動追加）：**

| ファイル | ビルド元 | 役割 |
|---|---|---|
| `crt0.o` | `tools/crt/crt0.c`（LLVM コンパイル） | `pceAppHead` 構造体（@0x100000）、BSS ゼロクリア、コールバックラッパー |
| `crti.o` | `tools/crt/defnotify.c`（LLVM コンパイル） | `pceAppNotify` デフォルト実装 |

**デフォルトライブラリ（正式リンク順）：**

| ライブラリ | 変換元 | 主な提供シンボル |
|---|---|---|
| `libpceapi.a` | `tools/crt/gen_pceapi.py`（LLVM ビルド） | `pceLCDTrans`, `pcePadGet` 等 カーネル API スタブ |
| `libio.a` | `io.lib` | `printf`, `scanf`, `fopen` 等 I/O |
| `liblib.a` | `lib.lib` | `malloc`, `atoi`, `rand` 等 標準 C ライブラリ |
| `libmath.a` | `math.lib` | `sin`, `cos`, `sqrt` 等 数学関数 |
| `libstring.a` | `string.lib` | `memset`, `memcpy`, `strlen`, `strcmp` 等 |
| `libctype.a` | `ctype.lib` | `isalpha`, `isdigit`, `tolower` 等 |
| `libfp.a` | `fp.lib` | `__addsf3`, `__mulsf3` 等 float/double 演算ランタイム |
| `libidiv.a` | `idiv.lib` | `__divsi3`, `__modsi3` 整数除算ランタイム |

**オプションライブラリ（明示指定のみ）：**

| ライブラリ | 変換元 | 使い方 |
|---|---|---|
| `libmuslib.a` | `muslib.lib` | 音楽再生ライブラリ（`-lmuslib` で指定） |
| `libsprite.a` | `sprite.lib` | スプライト描画（`-lsprite` で指定） |

---

## 2. アプリケーションのコンパイル

### ソースの構成

P/ECE アプリは以下の 3 つのコールバックを実装する：

```c
#include <piece.h>

void pceAppInit(void)    { /* 起動時に1回呼ばれる */ }
void pceAppProc(int cnt) { /* フレームごとに呼ばれる */ }
void pceAppExit(void)    { /* 終了時に1回呼ばれる */ }
```

`pceAppNotify` コールバックは `crti.o`（`tools/crt/defnotify.c` 由来）に
デフォルト実装が含まれるため、独自定義が不要な場合は省略できる。

### コンパイルコマンド

```sh
build/bin/clang \
    --sysroot=sysroot/s1c33-none-elf \
    -O2 \
    -Wno-incompatible-library-redeclaration \
    -c hello.c -o hello.o
```

| オプション | 説明 |
|---|---|
| `--sysroot=sysroot/s1c33-none-elf` | ヘッダとライブラリのルートを指定 |
| `-O2` | 最適化レベル（`-O0` でデバッグ用） |
| `-Wno-incompatible-library-redeclaration` | SDK の stdlib.h 再宣言警告を抑制 |
| `-c` | オブジェクトファイル生成（リンクしない） |

`--sysroot` を指定すると `sysroot/s1c33-none-elf/include` が自動的に
インクルードパスに加わるため、`-I sdk/include` は不要。

#### HW 乗算器オプション

P/ECE の SoC（S1C33209）には**ハードウェア乗算器**が搭載されている。
デフォルト CPU は `s1c33209` に設定済みのため、追加指定なしで
`mlt.w`/`mlt.h`/`mltu.w` 命令が生成される。

汎用 S1C33 コア（乗算器なし）向けにビルドする場合のみ明示指定が必要：

```sh
# 乗算器なしコア向け（通常は不要）
build/bin/clang --mcpu=s1c33 ...

# デフォルトと同じ（明示する場合）
build/bin/clang --mcpu=s1c33209 ...
```

> **注意:** `mlt.w` 等の乗算命令は S1C33209 専用。乗算器なしのコアで
> 実行するとトラップする。P/ECE 以外の S1C33 ファミリ向けには必ず
> `--mcpu=s1c33` を指定すること。

#### アセンブリ出力を確認したい場合

```sh
build/bin/clang \
    --sysroot=sysroot/s1c33-none-elf \
    -O2 -S hello.c -o hello.s
```

#### LLVM IR を確認したい場合

```sh
build/bin/clang \
    --sysroot=sysroot/s1c33-none-elf \
    -O2 -emit-llvm -S hello.c -o hello.ll
```

---

## 3. リンク

### 推奨：clang に一括させる（sysroot 使用）

コンパイルとリンクを同時に行う場合、または `-c` で生成した `.o` を
まとめてリンクする場合：

```sh
# コンパイル＋リンクを一括
build/bin/clang \
    --sysroot=sysroot/s1c33-none-elf \
    -O2 \
    -Wno-incompatible-library-redeclaration \
    hello.c -o hello.elf

# .o をまとめてリンク
build/bin/clang \
    --sysroot=sysroot/s1c33-none-elf \
    hello.o sub.o -o hello.elf
```

clang は自動的に以下を行う：
- リンカーとして `ld.lld` を使用（`--fuse-ld=lld` 不要）
- `-m elf32ls1c33` エミュレーション指定
- `sysroot/s1c33-none-elf/lib/crt0.o` をスタートアップとして追加
- `sysroot/s1c33-none-elf/lib/piece.ld` をデフォルトリンカースクリプトとして使用
- `--start-group -lpceapi -lio -llib -lmath -lstring -lctype -lfp -lidiv --end-group` を自動リンク

`-nostdlib` を指定するとスタートアップもライブラリも追加されない（手動リンク用）。

### 参考：ld.lld を直接呼ぶ場合

```sh
build/bin/ld.lld \
    -m elf32ls1c33 \
    -T sysroot/s1c33-none-elf/lib/piece.ld \
    sysroot/s1c33-none-elf/lib/crt0.o \
    sysroot/s1c33-none-elf/lib/crti.o \
    hello.o \
    -Lsysroot/s1c33-none-elf/lib \
    --start-group \
    -lpceapi -lio -llib -lmath -lstring -lctype -lfp -lidiv \
    --end-group \
    -o hello.elf
```

### リンカースクリプト `piece.ld` の概要

```
MEMORY {
    SRAM (rwx) : ORIGIN = 0x100000, LENGTH = 0x3D000
}
SECTIONS {
    .text  → crt0.o が先頭（pceAppHead @ 0x100000）、続いてアプリとライブラリ
    .data  → 初期値あり変数
    .bss   → ゼロ初期化変数（crt0.o が起動時にゼロクリア）
}
```

`crt0.o` が提供する `pceAppHead` 構造体が `0x100000` に配置され、
P/ECE カーネルがここから実行を開始する。

### リンク結果の確認

```sh
# セクション情報
build/bin/llvm-readelf --sections hello.elf

# 主要シンボルのアドレス確認
build/bin/llvm-readelf -s hello.elf | \
    grep -E "pceApp|pceLCD|pcePad|vbuff|__START|__END"

# 逆アセンブル（--mcpu=s1c33209 で HW 乗算器命令も正しく表示）
build/bin/llvm-objdump \
    --triple=s1c33-none-elf --mcpu=s1c33209 \
    -d hello.elf | head -80
```

---

## 4. バイナリ生成

ELF から生のバイナリ（.bin）を生成する（デバッグ・検証用）：

```sh
build/bin/llvm-objcopy -O binary hello.elf hello.bin
```

先頭 4 バイトが `pCeA` であれば正常：

```sh
hd hello.bin | head -2
# 00000000  70 43 65 41 ...   |pCeA...|
```

---

## 5. P/ECE パッケージ（.pex）の生成

`.pex` は P/ECE カーネルがロード・実行できる形式。
ELF の PROGBITS セクションを zlib 圧縮してヘッダを付加したもの。

```sh
tools/ppack/ppack -e hello.elf -ohello.pex -n"Hello World"
```

アイコン（256 バイトの .pid ファイル）を付ける場合：

```sh
tools/ppack/ppack -e hello.elf -ohello.pex -n"Hello World" -iicon.pid
```

| オプション | 説明 |
|---|---|
| `-e` | エンコード（ELF → .pex）モード |
| `-o<file>` | 出力ファイル名（`-o` と直結、スペース不可） |
| `-n<name>` | アプリ名（最大 24 文字、P/ECE メニューに表示） |
| `-i<file>` | アイコン画像（256 バイト .pid 形式） |

### 出力確認

```sh
hd hello.pex | head -2
# 00000000  58 02 ...  00 00 10 00  ...
#           ^  ^        ^^^^^^^^^^^
#           X  EXE2     top_adrs = 0x100000
```

---

## 6. まとめ：ワンライナー

```sh
# コンパイル＋リンク＋.pex 生成（最小コマンド数）
build/bin/clang \
    --sysroot=sysroot/s1c33-none-elf \
    -O2 -Wno-incompatible-library-redeclaration \
    hello.c -o hello.elf

tools/ppack/ppack -e hello.elf -ohello.pex -n"Hello World"
```

複数ソースの場合（並列コンパイル後にリンク）：

```sh
build/bin/clang --sysroot=sysroot/s1c33-none-elf \
    -O2 -Wno-incompatible-library-redeclaration -c main.c -o main.o
build/bin/clang --sysroot=sysroot/s1c33-none-elf \
    -O2 -Wno-incompatible-library-redeclaration -c sub.c  -o sub.o

build/bin/clang --sysroot=sysroot/s1c33-none-elf \
    main.o sub.o -o hello.elf

tools/ppack/ppack -e hello.elf -ohello.pex -n"Hello World"
```

---

## トラブルシューティング

### `undefined symbol: memset` 等

sysroot が構築されていないか、`--sysroot` が指定されていない。
§1 の手順で sysroot を構築してから `--sysroot=sysroot/s1c33-none-elf` を指定する。

ライブラリとシンボルの対応：

| シンボル | ライブラリ |
|---|---|
| `pceLCDTrans`, `pcePadGet` 等 | `libpceapi.a`（`pceapi.lib` から変換） |
| `printf`, `fopen` 等 I/O | `libio.a`（`io.lib` から変換） |
| `malloc`, `atoi`, `rand` 等 | `liblib.a`（`lib.lib` から変換） |
| `sin`, `cos`, `sqrt` 等 | `libmath.a`（`math.lib` から変換） |
| `memset`, `memcpy`, `strlen` 等 | `libstring.a`（`string.lib` から変換） |
| `isalpha`, `tolower` 等 | `libctype.a`（`ctype.lib` から変換） |
| `__addsf3`, `__mulsf3` 等（float） | `libfp.a`（`fp.lib` から変換） |
| `__divsi3`, `__modsi3` 等（int 除算） | `libidiv.a`（`idiv.lib` から変換） |

### `mlt.w: instruction requires a CPU feature not currently enabled`

アセンブル時に `--mcpu` が `s1c33`（基本コア）になっている。
P/ECE 向けには `--mcpu=s1c33209`（デフォルト）のままにすること。
`clang --target=s1c33-none-elf` は自動的に `s1c33209` を使うため、
通常はこのエラーは発生しない。`llvm-mc` や `clang -mcpu=s1c33` を
明示した場合にのみ起こる。

### `pceAppNotify` が未定義

`crti.o`（`tools/crt/defnotify.c` 由来）にデフォルト実装が含まれる。
`--sysroot` を使ったビルドでは自動的にリンクされる。
独自実装が必要な場合は `tools/crt/defnotify.c` を参考に作成し、
アプリの `.o` として追加すれば上書きできる（リンカーは先に見つけた
シンボルを優先するため、ユーザ `.o` は `crti.o` より後に並ぶ）。

### `lld: error: truncated or malformed archive`

`srf2elf.py` で変換したアーカイブが古い形式の場合。最新の `srf2elf.py` で
再変換する（GNU ar のロングネーム形式 `/<offset>` が必要）。

### `llvm-objdump` が `elf32-unknown` と表示する

ELF ファイル形式の表示上の問題で、逆アセンブル自体は正常に動作する。
`--triple=s1c33-none-elf --mcpu=s1c33209` を明示することで
HW 乗算器命令（`mlt.w` 等）も正しく表示される：

```sh
build/bin/llvm-objdump \
    --triple=s1c33-none-elf --mcpu=s1c33209 \
    -d hello.elf
```

### 関数ポインタ経由の呼び出しでクラッシュする

`mus.c` 等で構造体メンバの関数ポインタ経由の間接呼び出し
（`mp->genwave(mp, ...)` 等）が正しく動作しない場合、
`S1C33ISelLowering.cpp` の `LowerCall` に問題がある可能性がある。
現在の実装では間接呼び出しに `call %rb` 命令が正しく選択される。
