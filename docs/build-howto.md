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
│   ├── include/        piece.h など P/ECE 固有ヘッダ（SRF 変換元）
│   └── lib/            SDK ライブラリ（SRF 形式、変換元）
├── picolibc/           picolibc サブモジュール（標準 C ライブラリ、既定）
├── newlib/             newlib サブモジュール（フォールバック; make NEWLIB=1）
├── sysroot/
│   └── s1c33-none-elf/
│       ├── include/    picolibc ヘッダ + P/ECE 固有ヘッダ
│       └── lib/
│           ├── piece.ld                      P/ECE リンカースクリプト
│           ├── crt0.o                        アプリヘッダ + BSS 初期化（LLVM ビルド）
│           ├── crti.o                        pceAppNotify デフォルト実装（LLVM ビルド）
│           ├── libclang_rt.builtins-s1c33.a  compiler-rt（FP・整数除算ランタイム）
│           ├── libcxxrt.a                    C++ ランタイムスタブ（operator new/delete 等）
│           ├── libpceapi.a                   P/ECE カーネル API スタブ（LLVM ビルド）
│           ├── libpicortt.a                  picolibc リターゲット層（stdout 破棄 + sbrk）
│           ├── libpceshim.a                  __assert_func/rand シム
│           ├── libc.a, libm.a                picolibc 標準 C/数学
│           ├── libmuslib.a                   音楽ライブラリ（tools/muslib/ ソースからビルド）
│           ├── libsimple.a, libdefinst.a     シンプルライブラリ（tools/simple/ ソースからビルド）
│           └── libsprite.a                   スプライトライブラリ（tools/sprite/ ソースからビルド）
├── tools/
│   ├── piece.ld        リンカースクリプト（sysroot へコピー済み）
│   ├── ppack/          ppack ツール（ELF → .pex パッケージャ）
│   ├── muslib/         音楽ライブラリのソース
│   ├── simple/         シンプルライブラリのソース
│   ├── sprite/         スプライトライブラリのソース
│   ├── asm33conv/      as33 拡張ニーモニック → 標準 LLVM アセンブリ変換器
│   └── srf2elf/        SRF→ELF 変換ツール（regen-builtins-asm 専用、通常ビルドでは未使用）
└── app/                サンプルアプリ（hello, jien, pmdplay, BlackWings, ...）
```

---

## 1. sysroot の構築（初回のみ）

標準 C ヘッダ（picolibc）と P/ECE 固有ヘッダ、ランタイムライブラリをすべてまとめてビルドする。
ヘッダ・ライブラリの内容を変更しない限り再実行不要。

```sh
# CRT・カーネル API スタブ + picolibc + リターゲット層 + 各ライブラリ（一括）
make -C tools/crt
# newlib で組む場合（フォールバック）: make -C tools/crt NEWLIB=1
```

この 1 コマンドで以下がすべて自動的に実行される（既定 = picolibc）：

1. picolibc を meson + ninja でビルドし、標準 C ヘッダを sysroot にインストール
2. P/ECE 固有ヘッダ（`piece.h`、`draw.h` 等）を `tools/crt/include/` からコピー
3. Clang 組み込みと競合するヘッダ（`stddef.h`、`stdarg.h`、`float.h`）を除去
4. `crt0.o`・`crti.o`・`libpceapi.a` を LLVM でビルド
5. `libclang_rt.builtins-s1c33.a`（compiler-rt; fp.lib/idiv.lib の後継）を cmake でビルド
6. picolibc の `libc.a` / `libm.a` を sysroot へインストール
7. `libpicortt.a`（picolibc リターゲット層）、`libmuslib.a`（音楽）、`libpceshim.a`（シム）をソースからビルド

シンプルライブラリ `libsimple.a` とスプライトライブラリ `libsprite.a` は `tools/simple/` と `tools/sprite/` で個別に `make` する：

```sh
make -C tools/simple   # libsimple.a + libdefinst.a を sysroot へインストール
make -C tools/sprite   # libsprite.a を sysroot へインストール
```

両ライブラリは `tools/asm33conv/asm33conv.py` を経由して `.s` ファイル内の as33 拡張ニーモニックを LLVM 標準命令に展開してからアセンブルする。

### sysroot に含まれるファイルの内容

**スタートアップ（リンク先頭に自動追加）：**

| ファイル | ビルド元 | 役割 |
|---|---|---|
| `crt0.o` | `tools/crt/crt0.c`（LLVM コンパイル） | `pceAppHead` 構造体（@0x100000）、BSS ゼロクリア、コールバックラッパー |
| `crti.o` | `tools/crt/defnotify.c`（LLVM コンパイル） | `pceAppNotify` デフォルト実装 |

**デフォルトライブラリ（正式リンク順）：**

リンク順は `clang/lib/Driver/ToolChains/PIECE.cpp` の S1C33 分岐で固定:
```
-lclang_rt.builtins-s1c33
--start-group
  -lcxxrt -lpceapi -lpicortt -lpceshim
  -lc -lm                          ← picolibc
--end-group
```

`libpicortt.a` は `-lc` の前に置かれ、その強シンボル `stdin`/`stdout`/`stderr`（stdout 破棄コンソール）と `sbrk` が picolibc の弱い既定を上書きします（picolibc 内蔵の `picosbrk.o` が `__heap_start`/`__heap_end` を要求するのを抑止）。`libpceshim.a` も `-lc` の前に置かれ、picolibc の `__assert_func`（放置すると stdio を引き込む）を小型版で上書きします。`-lc -lm` (picolibc) が標準 C/数学関数の全シンボル (`printf`, `malloc`, `sin`, ...) を供給します（数学は `libc.a` に統合され `libm.a` は空スタブ）。

| ライブラリ | 提供元 | 主な提供シンボル |
|---|---|---|
| `libclang_rt.builtins-s1c33.a` | compiler-rt（LLVM ビルド） | `__addsf3`、`__divsi3`、`__fixsfdi`、`__floatdisf` 等 FP・整数除算・i64 変換ランタイム |
| `libcxxrt.a` | `tools/crt/cxxrt.c` 等（LLVM ビルド） | `operator new/delete`（→ `pceHeapAlloc`/`pceHeapFree`）、`__cxa_pure_virtual` 等 C++ ランタイム |
| `libpceapi.a` | `tools/crt/gen_pceapi.py`（LLVM ビルド） | `pceLCDTrans`、`pcePadGet`、`pcesprintf` 等 カーネル API スタブ |
| `libpicortt.a` | `tools/picortt/`（LLVM ビルド） | `stdin`/`stdout`/`stderr`（出力は破棄、入力は EOF）、`sbrk`（カーネル pceHeap zone の上に伸びる libc ヒープ） |
| `libpceshim.a` | `tools/pceshim/`（LLVM ビルド） | picolibc の `__assert_func`（+ `rand`/`srand`）を小型の単一スレッド版で上書き |
| `libc.a` | picolibc (`picolibc/` サブモジュール、tools/crt/Makefile が meson でビルド) | `printf`、`malloc`、`strtod`、`strtok`、`setjmp`、`atoi`、`rand`、`memset`、`memcpy`、`strlen`、`isalpha`、`isdigit`、`tolower` 等 ANSI C 標準（数学関数も統合） |
| `libm.a` | picolibc（同上） | 空スタブ（数学は `libc.a` 側）。`-lm` 互換のために存在 |

> **picolibc 移行完了 (2026-06)**: 既知の EPSON `lib.lib` バグ (sin / pow / strtod / strtok / ispunct) は、まず newlib (Phase 2 Stage A/B、2026-05) で解消され、2026-06 に picolibc へ移行しました。picolibc の tinystdio は `printf`/`sprintf` が malloc 非依存で、`malloc`/`strdup` 等を呼ばないアプリには `sbrk` もリンクされません（`--gc-sections` で除去）。newlib は `make -C tools/crt NEWLIB=1` のフォールバックとして保持。printf/scanf のバリアントは `-mprintf=`/`-mscanf=` でリンク時選択できます（後述）。

**オプションライブラリ（明示指定のみ）：**

| ライブラリ | ビルド元 | 使い方 |
|---|---|---|
| `libmuslib.a` | `tools/muslib/`（ソースから LLVM ビルド、`make -C tools/crt` で自動） | 音楽再生ライブラリ（`-lmuslib` で指定） |
| `libsimple.a` / `libdefinst.a` | `tools/simple/`（ソースから LLVM ビルド、`make -C tools/simple` で生成） | シンプルライブラリ（`-lsimple` で指定） |
| `libsprite.a` | `tools/sprite/`（ソースから LLVM ビルド、`make -C tools/sprite` で生成。`tools/asm33conv/` で as33 拡張ニーモニックを展開） | スプライト描画（`-lsprite` で指定） |

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
    -c hello.c -o hello.o
```

| オプション | 説明 |
|---|---|
| `--sysroot=sysroot/s1c33-none-elf` | ヘッダとライブラリのルートを指定 |
| `-O2` | 最適化レベル（`-O0` でデバッグ用） |
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

#### LTO（リンク時最適化）を使う場合

アプリケーションコードに `-flto=full` を使用できる。LTO により関数のインライン展開や
デッドコード削除がリンク時に行われるためコードサイズが縮小しやすい：

```sh
# コンパイル時に -flto=full
build/bin/clang \
    --sysroot=sysroot/s1c33-none-elf \
    -O2 -flto=full \
    -c hello.c -o hello.o

# リンク時にも同じオプションが必要
build/bin/clang \
    --sysroot=sysroot/s1c33-none-elf \
    -O2 -flto=full \
    hello.o -o hello.elf
```

> **注意:** `crt0.o`・`libcxxrt.a`・`libclang_rt.builtins-s1c33.a` は
> ネイティブ ELF でビルドされており LTO ビットコードを含まないが、
> アプリ側が `-flto` を使っていても正常にリンクされる。
> LLVM LTO は `abort()`・`memcpy()` など一部の libc 関数を
> 「既知外部関数」として扱うため、ランタイムスタブは
> ネイティブ ELF でなければならない（LTO ビットコード版では解決されない）。

#### printf / scanf バリアントの選択（`-mprintf=` / `-mscanf=`）

picolibc は printf/scanf を機能別の複数バリアント（`double` / `float` /
`long-long` / `integer` / `minimal`、機能が小さいほどコードも小さい）で
提供する。既定は `double`（float も long long も扱える）。アプリ側で
`-mprintf=`/`-mscanf=` を指定すると、リンク時に該当バリアントへ切り替わる
（内部的には `--defsym=vfprintf=__<v>_vfprintf` を発行）。

```sh
# 整数のみで十分なアプリ（大幅に小さくなる）
build/bin/clang --sysroot=sysroot/s1c33-none-elf -O2 \
    -mprintf=integer -mscanf=integer app.c -o app.elf
```

| 値 | 機能 | 目安（snprintf 1 個のアプリの .text） |
|---|---|---|
| `double`（既定） | float/double + C99 + 位置指定 | ~7.2 KB |
| `float` | float（double は float に縮約） | （double より小） |
| `long-long` | 整数 + long long | |
| `integer` | 整数のみ | ~3.0 KB |
| `minimal` | 最小（幅指定等も削減） | ~2.1 KB |

補足:
- **`sprintf`/`vsprintf` は piece.h が既定でカーネルの `pcesprintf`/`pcevsprintf`
  （整数のみ）に `#define` する**ため、`-mprintf=` の対象は主に `printf` /
  `snprintf` / `fprintf`。float が必要で libc 側 `sprintf` を使いたい場合は
  `NOPCESPRINTF` を定義してから `<stdio.h>` を include する。
- **LTO 併用時は `--defsym` 別名が効かない**ため、ライブラリ既定（double）が
  使われる。サイズを詰めたい整数アプリは LTO と `-mprintf=integer` のどちらを
  取るか選ぶことになる。

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
- `-lclang_rt.builtins-s1c33 --start-group -lcxxrt -lpceapi -lpicortt -lpceshim -lc -lm --end-group` を自動リンク

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
    -L build/lib/clang/22/lib/s1c33-unknown-none-elf \
    -lclang_rt.builtins-s1c33 \
    --start-group \
    -lcxxrt -lpceapi -lpicortt -lpceshim -lc -lm \
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
| `-n<name>` | アプリ名（最大 24 バイト、P/ECE メニューに表示）。**raw バイトをそのまま埋め込む** — 日本語名は SJIS で渡す必要がある |
| `-N<name>` | アプリ名。**UTF-8 入力を CP932 に変換**して埋め込む（最大 24 バイト）。UTF-8 ソースツリーから日本語キャプションを付ける場合はこちら |
| `-i<file>` | アイコン画像（256 バイト .pid 形式） |

> P/ECE メニューのキャプションは SJIS (CP932)。`-n` はバイト列を無変換で書き込むため、ターミナル／Makefile が UTF-8 の現環境では日本語キャプションは `-N` を使うのが安全。ASCII 名なら `-n` / `-N` どちらでもよい。

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
    -O2 \
    hello.c -o hello.elf

tools/ppack/ppack -e hello.elf -ohello.pex -n"Hello World"
```

複数ソースの場合（並列コンパイル後にリンク）：

```sh
build/bin/clang --sysroot=sysroot/s1c33-none-elf \
    -O2 -c main.c -o main.o
build/bin/clang --sysroot=sysroot/s1c33-none-elf \
    -O2 -c sub.c  -o sub.o

build/bin/clang --sysroot=sysroot/s1c33-none-elf \
    main.o sub.o -o hello.elf

tools/ppack/ppack -e hello.elf -ohello.pex -n"Hello World"
```

---

## トラブルシューティング

### `undefined symbol: memset` 等

sysroot が構築されていないか、`--sysroot` が指定されていない。
§1 の手順で sysroot を構築してから `--sysroot=sysroot/s1c33-none-elf` を指定する。

ライブラリとシンボルの対応 (picolibc 移行後):

| シンボル | 提供元 |
|---|---|
| `pceLCDTrans`, `pcePadGet`, `pcesprintf` 等 | `libpceapi.a`（`gen_pceapi.py` + `vector.h` から自動生成） |
| `printf`, `snprintf`, `vfprintf` 等 stdio | `libc.a` (picolibc) |
| `malloc`, `free`, `realloc`, `calloc` | `libc.a` (picolibc) |
| `atoi`, `strtol`, `strtod`, `rand` 等 | `libc.a` (picolibc) |
| `sin`, `cos`, `pow`, `sqrt`, `atan2` 等 | `libc.a` (picolibc; 数学も統合済み) |
| `memset`, `memcpy`, `strlen`, `strcpy` 等 | `libc.a` (picolibc) |
| `isalpha`, `tolower`, `isdigit` 等 | `libc.a` (picolibc; `_ctype_b` 等の表も内蔵) |
| `setjmp`, `longjmp` | `libc.a` (picolibc `libc/machine/s1c33/`) |
| `stdin`, `stdout`, `stderr`, `sbrk` | `libpicortt.a`（`tools/picortt/`） |
| `_exit`, `abort` | `libcxxrt.a`（`tools/crt/halt.c`） |
| `__addsf3`, `__mulsf3`, `__fixsfdi` 等（float） | `libclang_rt.builtins-s1c33.a`（compiler-rt） |
| `__divsi3`, `__modsi3`, `__divdi3` 等（除算） | `libclang_rt.builtins-s1c33.a`（compiler-rt） |
| `__assert_func`, `rand`, `srand`（shim） | `libpceshim.a`（picolibc 版をシャドウ） |

### `mlt.w: instruction requires a CPU feature not currently enabled`

アセンブル時に `--mcpu` が `s1c33`（基本コア）になっている。
P/ECE 向けには `--mcpu=s1c33209`（デフォルト）のままにすること。
`clang`（既定で `s1c33-none-elf` ターゲット）は自動的に `s1c33209` を使うため、
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

### `llvm-objdump` が正しく逆アセンブルできない

S1C33 ELF を `--triple` なしで逆アセンブルする場合、EM_SE_C33 ELF マシン番号から
自動的に s1c33 トリプルが選択される（`ELFObjectFile.h` に登録済み）。
ただし HW 乗算器命令（`mlt.w` 等）は `--mcpu=s1c33209` を明示しないと
「unknown instruction」になる場合がある：

```sh
# --triple は省略可能。--mcpu は P/ECE 向けには明示推奨
build/bin/llvm-objdump --mcpu=s1c33209 -d hello.elf
```

### 関数ポインタ経由の呼び出しでクラッシュする

`mus.c` 等で構造体メンバの関数ポインタ経由の間接呼び出し
（`mp->genwave(mp, ...)` 等）が正しく動作しない場合、
`S1C33ISelLowering.cpp` の `LowerCall` に問題がある可能性がある。
現在の実装では間接呼び出しに `call %rb` 命令が正しく選択される。

### `malloc` でヒープが壊れる / alignment exception が出る

libc のヒープ領域 (`sbrk` が割り当てる範囲) を超えて kernel pceHeap が
書き込んでいる可能性がある。アプリ作者ができる調整:

- `-Wl,--defsym=_pceheapsize=N` で kernel pceHeap zone のサイズを増やす
  (デフォルト 0x2000 = 8 KB)
- 自前の `sbrk` を実装してリンクし、別領域でヒープを管理する（`libpicortt.a`
  より前に置けば上書きできる）
- 詳細は `docs/piece-symbols.md` 参照

### システムメニューが開けない / `pceLCDSetBuffer(_def_vbuff)` で挙動がおかしい

`_def_vbuff` は piece.ld で **絶対アドレス 0x13c000 = SYSERRVBUFF** に
alias されており、BSS 実体は持ちません。kernel の system menu / system
error / version_check の 3 経路は同じ 4 KB 領域を時間多重で使うことを
前提にした設計です。`_def_vbuff` を超えて書き込むコード (例: 古い SDK
コードを移植したもので 11 KB の memset を行う) は kernel-private 領域に
食い込みますが、その経路は app が終了する直前にしか走らないため実害は
ありません。

### P/ECE 固有のリンカシンボルを把握したい

`docs/piece-symbols.md` に `_stacklen` / `_pceheapstart` / `_pceheapsize` /
`_def_vbuff` 等、`-Wl,--defsym` で上書き可能なシンボルと役割の一覧があります。

---

## picolibc ポートのメンテナンス

通常のアプリ開発者には不要な情報。既定 libc である picolibc は
meson でビルドされる（autotools は不要）。S1C33 固有部分は picolibc
サブモジュール内の以下に閉じている:

| ファイル | 役割 |
|---|---|
| `picolibc/libc/machine/s1c33/{setjmp,longjmp}.S` | setjmp / longjmp（S5U1C33000C ABI、`_JBLEN=6`） |
| `picolibc/libc/machine/s1c33/meson.build` | machine ソース登録 |
| `picolibc/libc/include/machine/setjmp.h` | `__s1c33__`: `_JBLEN=6` / `_JBTYPE=int` |
| `picolibc/libc/include/machine/ieeefp.h` | `__s1c33__`: little-endian 宣言 |
| `tools/crt/cross-s1c33-none-piece.txt` | meson クロスファイル（Makefile が生成） |
| `tools/crt/Makefile`（`PICOLIBC_MESON_OPTS`） | tinystdio / single-thread / global errno / no-TLS / `io-float-exact=false` / `format-default=double` 等のビルドオプション |
| `tools/picortt/{console,picesbrk}.c` | stdout 破棄コンソール + sbrk リターゲット |

`PICOLIBC_MESON_OPTS` を変更すると、`build.ninja` が Makefile を
prerequisite に持つため自動で再構成される。これらを編集して
`make -C tools/crt` すれば反映される。

> **printf/scanf バリアントの仕組み**: picolibc は variant ごとの
> `__d_/__f_/__l_/__i_/__m_vfprintf` を全て持ち、リンク時に
> `--defsym=vfprintf=__<v>_vfprintf` で選ぶ。本ツールチェインでは
> `-mprintf=`/`-mscanf=`（`PIECE.cpp`）がこの `--defsym` を発行する。
> `format-default=double` が、`-mprintf=` 未指定時の既定を決める。

---

## newlib ポートのメンテナンス（フォールバック）

picolibc 移行 (2026-06) 以降、newlib は `make -C tools/crt NEWLIB=1` で
ビルドするフォールバック扱い。通常は不要。`newlib/newlib/libc/sys/s1c33/`
や `newlib/newlib/libc/machine/s1c33/` を編集するときのみ参照する。

### 既存ソースの編集 (configure.host / *.c / *.S)

`newlib/newlib/libc/{machine,sys}/s1c33/` 内の C / アセンブリを編集した場合、
通常の `make -C tools/crt` で newlib が再ビルドされる。再生成は不要。

### 構造変更 (新ファイル追加 / configure.host への新エントリ等)

`newlib/newlib/configure` および `newlib/newlib/Makefile.in` の再生成が必要。

```sh
# autoconf 2.69 / automake 1.15.1 が PATH にあること
cd newlib/newlib
autoreconf -i
```

`autoreconf` を実行するには **autoconf 2.69 と automake 1.15.1 が厳密に必要** (newlib の `../config/override.m4` がバージョン一致を強制)。Debian 13 等は標準で 2.72 / 1.17 なので、ローカルにビルドして PATH を通す:

```sh
mkdir -p ~/local/src && cd ~/local/src
curl -sSLO https://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.xz
curl -sSLO https://ftp.gnu.org/gnu/automake/automake-1.15.1.tar.xz
tar xf autoconf-2.69.tar.xz && tar xf automake-1.15.1.tar.xz

# autoconf 2.69
cd autoconf-2.69 && ./configure --prefix=$HOME/local/autotools && make && make install

# automake 1.15.1 (autoconf 2.69 が PATH にあること)
cd ../automake-1.15.1
PATH=$HOME/local/autotools/bin:$PATH ./configure --prefix=$HOME/local/autotools
PATH=$HOME/local/autotools/bin:$PATH make && PATH=$HOME/local/autotools/bin:$PATH make install

# 以降は autoreconf 実行時に PATH を通す
PATH=$HOME/local/autotools/bin:$PATH autoreconf -i
```

再生成後、`configure` と `Makefile.in` の差分を `git add` してコミットする (これらは newlib submodule にチェックインされる)。

### S1C33 向けポート構成ファイル一覧

| ファイル | 内容 |
|---|---|
| `newlib/newlib/configure.host` | s1c33 CPU / host エントリ (3 箇所の case 文) |
| `newlib/newlib/libc/acinclude.m4` | `m4_foreach_w` の MACHINE / SYS_DIR リストに `s1c33` を追加 |
| `newlib/newlib/libc/machine/Makefile.inc` | `HAVE_LIBC_MACHINE_S1C33` 条件 include |
| `newlib/newlib/libc/sys/Makefile.inc` | `HAVE_LIBC_SYS_S1C33_DIR` 条件 include |
| `newlib/newlib/libc/include/machine/{ieeefp,setjmp}.h` | S1C33 little-endian / `_JBLEN=6` |
| `newlib/newlib/libc/machine/s1c33/{setjmp,longjmp}.S` | setjmp / longjmp 実装 (S5U1C33000C ABI) |
| `newlib/newlib/libc/machine/s1c33/Makefile.inc` | machine ソース登録 |
| `newlib/newlib/libc/sys/s1c33/{syscalls,sbrk,_exit,write}.c` | POSIX syscall スタブ |
| `newlib/newlib/libc/sys/s1c33/Makefile.inc` | sys ソース登録 |
| `newlib/config.sub` | `s1c33` を CPU として認識させる行 |

### tools/crt/ で日本語文字列を扱うソースを足したい場合

clang は `-fexec-charset=` を実装上無視する (`InitPreprocessor.cpp` の FIXME 参照) ため、UTF-8 ソース → SJIS 実行バイト列の自動変換はできない。代わりに **iconv パイプを Makefile に挟む** パターンを使う:

```make
$(BUILDDIR)/foo.sjis.c: foo.c | $(BUILDDIR)
	iconv -f UTF-8 -t CP932 $< -o $@

$(BUILDDIR)/foo.o: $(BUILDDIR)/foo.sjis.c | $(BUILDDIR)
	$(CLANG) $(CFLAGS_CRT) -Wno-invalid-source-encoding $< -o $@
```

CP932 を指定するのは、SHIFT_JIS 厳密版だと em dash (`—`) や `〜` 等の MS 拡張文字を弾くため。CP932 はそれらを SJIS 拡張領域に変換する。

または既存の慣行どおり **ソースを Shift_JIS で直接保存** することもできる (例: `tools/crt/version_check.c`)。clang は SJIS を invalid UTF-8 とみなして警告を出すが、string literal の raw バイトとして通すため runtime 動作は正しい。`-Wno-invalid-source-encoding` で警告抑制。`CLAUDE.md` の "Source File Encoding" 節も参照。
