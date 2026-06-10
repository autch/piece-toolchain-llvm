# llvm-c33 — LLVM backend for EPSON S1C33000

EPSON S1C33000 32ビット RISC CPU 向けの LLVM/Clang バックエンドです。
ターゲットは [アクアプラス P/ECE](https://aquaplus.jp/piece/) (S1C33209 CPU) で、
既存の P/ECE SDK ライブラリと ABI 互換でリンクできるバイナリを生成します。

An LLVM/Clang backend for the EPSON S1C33000 32-bit RISC CPU,
targeting the [Aquaplus P/ECE handheld](https://aquaplus.jp/piece/) (S1C33209 CPU).
Generates binaries ABI-compatible with the existing P/ECE SDK libraries.

---

## 概要 / Overview

| 項目 | 内容 |
|---|---|
| ターゲット CPU | EPSON S1C33000 (S1C33209) — 32-bit RISC, 16-bit fixed-width instructions |
| ターゲットデバイス | Aquaplus P/ECE |
| トリプル | `s1c33-none-piece`（P/ECE 向け、ビルド既定）/ `s1c33-none-elf`（ベアメタル） |
| ステータス | **Phase 6 完了 + compiler-rt Phase 1 / newlib Phase 2 Stage B 完了 + simple/sprite ライブラリのソースビルド化完了** — 実機動作確認済み（2026-03〜05） |
| ベース LLVM | llvm-project (サブモジュール, `llvm/` 以下) |

P/ECE SDK のバイナリ配布ライブラリ群はすべて LLVM 上で再実装するかソースから再ビルドするよう移行済みです:
- カーネル API スタブ (`pceapi.lib`) → **`tools/crt/gen_pceapi.py` がカーネル ROM のシンボルテーブル (`vector.h`) から関数スタブを自動生成**
- 浮動小数点・整数除算 (`fp.lib`, `idiv.lib`) → **LLVM compiler-rt** (`libclang_rt.builtins-s1c33.a`)
- 標準 C / 数学 (`lib.lib`, `math.lib`, `io.lib`, `string.lib`, `ctype.lib`) → **newlib** `libc.a` / `libm.a` (Phase 2 Stage B、2026-05)
- シンプル / スプライト / 音楽 (`simple.lib`, `sprite.lib`, `muslib.lib`) → **`tools/{simple,sprite,muslib}/` の C/asm ソースからビルド**

これにより通常ビルドフロー (`make`、`make -C tools/crt`、`make -C tools/{simple,sprite,muslib}`) は EPSON SDK のバイナリ `.lib` を一切読みません。`tools/srf2elf/` は `make regen-builtins-asm` 開発時専用ターゲットでのみ使われ、compiler-rt が必要とする関数を `fp.lib` / `idiv.lib` から回収するためのアセンブリ抽出に用います。

Every binary distribution library from the P/ECE SDK has been replaced by
either an LLVM-side re-implementation or a from-source rebuild:
- Kernel API stubs (`pceapi.lib`) → generated from the kernel ROM symbol
  table (`vector.h`) by **`tools/crt/gen_pceapi.py`**.
- Floating-point / integer division (`fp.lib`, `idiv.lib`) → **LLVM
  compiler-rt** (`libclang_rt.builtins-s1c33.a`).
- Standard C / math (`lib.lib`, `math.lib`, `io.lib`, `string.lib`,
  `ctype.lib`) → **newlib** `libc.a` / `libm.a` (Phase 2 Stage B, 2026-05).
- Simple / sprite / music (`simple.lib`, `sprite.lib`, `muslib.lib`) →
  built from the C / asm sources under **`tools/{simple,sprite,muslib}/`**.

As a result the regular build flow no longer reads any EPSON SDK `.lib`
binary; `tools/srf2elf/` is exercised only by the manual
`make regen-builtins-asm` developer target, which re-extracts assembly
from `fp.lib` / `idiv.lib` to maintain compiler-rt's recovered builtins.

---

## 動作確認済みアプリケーション / Verified Applications

以下のアプリケーションが実機 P/ECE で動作確認されています。

The following applications have been verified on a real P/ECE device.

| アプリ (`app/` 配下) | ビルド方法 | 確認内容 |
|---|---|---|
| `mini_nocrt/` | crt0 手書き、手動リンク | 画面描画・ST+SL でメニュー復帰 |
| `minimal/` | sysroot の crt0 + pceapi | 同上 |
| `hello/` | sysroot CRT + newlib (`printf` 等) | `printf` 表示・システムメニュー・メニュー復帰 |
| `jien/` | sysroot CRT + newlib + 描画ライブラリ | ビットマップ表示・構造体値渡し（`pceLCDDrawObject`） |
| `fpkplay/` | sysroot CRT + libmuslib (ソースビルド) + LZSS | FPK 音楽再生（8kHz/16kHz波形合成） |
| `cpptest/` | sysroot CRT + libcxxrt | C++ クラス・仮想関数・例外なし RTTI なし運用 |
| `menu2/` | sysroot CRT + 旧 SDK 比較ベース | gcc33 とのバイナリ差分検証 |
| `pcecircle/` | sysroot CRT + 描画 | 円描画 (gcc33 比較用) |

加えて、本リポジトリには収録されていない外部アプリも実機動作確認済みです:

- **BlackWings**（アクアプラスのアクション RPG 商用タイトル、配布物の著作権上リポジトリ非収録）— ゲーム全体の完走動作（ゲームループ・音楽・ステージ進行・セーブデータ）
- **odemaru**（同上、リポジトリ非収録）— `libsimple` / `libsprite` のソースビルド版を使った完走動作（スプライトレンダリング・パッド入力・タイトル → ゲームプレイ → エンド）

---

## リポジトリ構成 / Repository Layout

```
llvm-c33/
├── llvm/                   LLVM サブモジュール (llvm-project)
│   ├── llvm/lib/Target/S1C33/   バックエンド実装
│   └── compiler-rt/lib/builtins/s1c33/  compiler-rt S1C33 builtins
├── newlib/                 newlib サブモジュール（標準 C ヘッダ提供）
├── build/                  CMake ビルドディレクトリ（初回 cmake 後に生成）
├── sdk/                    P/ECE SDK（別途入手・配置）
│   ├── include/
│   └── lib/
├── sysroot/s1c33-none-piece/ ビルド済み sysroot（make -C tools/crt で生成）
├── tools/
│   ├── crt/                crt0.c, libpceapi.a, libcxxrt.a 生成 Makefile
│   │   └── include/        SDK 由来ヘッダのローカル正本（s1c33cpu.h は LLVM 対応に書き換え）
│   ├── simple/             シンプルライブラリのソースビルド（libsimple.a）
│   ├── sprite/             スプライトライブラリのソースビルド（libsprite.a）
│   ├── muslib/             音楽ライブラリのソースビルド（libmuslib.a）
│   ├── pceshim/            newlib の rand/srand/__assert_func を上書きする軽量シム
│   ├── srf2elf/            SRF33 → ELF 変換ツール（Python; 通常ビルドでは未使用、`make regen-builtins-asm` 開発時専用）
│   ├── elf2srf/            ELF → SRF33 変換ツール（Python, 実験的）
│   ├── ppack/              ELF → .pex パッケージャ（C++, cmake）
│   ├── asm33conv/          EPSON as33 拡張ニーモニック → LLVM 標準アセンブリ変換器
│   └── piece.ld            P/ECE アプリ用リンカースクリプト
├── app/                    サンプルアプリ群（実機検証対象）
│   ├── hello/              `printf` + システムメニュー復帰
│   ├── jien/               ビットマップ描画・構造体値渡し
│   ├── fpkplay/            FPK 音楽再生・LZSS 展開・波形合成
│   ├── minimal/            sysroot crt0 使用の最小サンプル
│   ├── mini_nocrt/         crt0 手書き・最小構成
│   ├── cpptest/            C++ 動作確認（例外なし RTTI なし）
│   ├── menu2/              メニュー (gcc33 比較用)
│   └── pcecircle/          円描画（gcc33 比較用）
├── docs/
│   ├── setup.md            セットアップ手順（← まずここを読む）
│   ├── build-howto.md      アプリビルド手順
│   ├── errata.md           CPU・コンパイラ・ライブラリのエラッタ
│   └── DESIGN_SPEC.md      → DESIGN_SPEC.md（リポジトリルート）
├── DESIGN_SPEC.md          アーキテクチャ仕様・設計判断・フェーズ管理
└── CLAUDE.md               AI アシスタント向けクイックリファレンス
```

---

## セットアップ / Setup

詳細は [`docs/setup.md`](docs/setup.md) を参照してください。
See [`docs/setup.md`](docs/setup.md) for full instructions.

### 必要なツール / Prerequisites

```sh
# Debian/Ubuntu
sudo apt install git cmake ninja-build g++ python3 zlib1g-dev ccache
```

| ツール | 最低バージョン |
|---|---|
| Git | 2.13 |
| CMake | 3.13 |
| Ninja | any |
| GCC / Clang (ホスト用) | GCC 7 / Clang 6 |
| Python 3 | 3.6 |
| ccache | any（推奨） |

### クイックスタート / Quick Start

```sh
# 1. サブモジュール取得 / Fetch submodules (LLVM + newlib)
git submodule update --init llvm newlib

# 2. LLVM ビルド / Build LLVM
mkdir build && cd build
cmake -G Ninja ../llvm/llvm \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_TARGETS_TO_BUILD="" \
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="S1C33" \
  -DLLVM_DEFAULT_TARGET_TRIPLE="s1c33-none-piece" \
  -DLLVM_ENABLE_PROJECTS="clang;lld;lldb" \
  -DLLVM_INSTALL_UTILS=ON \
  -DLLVM_USE_LINKER=mold \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
ninja
cd ..

# 3. ppack ビルド / Build ppack
cd tools/ppack && cmake -G Ninja -B _build -DCMAKE_BUILD_TYPE=Release . && ninja -C _build
cp _build/ppack ppack && cd ../..

# 4. sysroot 一括ビルド（CRT + newlib + compiler-rt + pceapi + muslib + pceshim）
# Build sysroot: CRT + newlib + compiler-rt + pceapi + muslib + pceshim
make -C tools/crt

# 5. シンプル / スプライトライブラリ (使うアプリ向け) / Optional libraries
make -C tools/simple
make -C tools/sprite

# 6. サンプルアプリをビルド / Build sample app
cd app/hello && make
```

`app/hello/hello_l.pex` が生成されれば成功です。
If `app/hello/hello_l.pex` is produced, the toolchain is working.

---

## アプリケーションのビルド / Building Your Application

詳細は [`docs/build-howto.md`](docs/build-howto.md) を参照してください。
See [`docs/build-howto.md`](docs/build-howto.md) for full instructions.

### コンパイル＋リンク＋パッケージ（最小例）
### Compile + Link + Package (minimal example)

```sh
# コンパイル＋リンク / Compile + link
build/bin/clang \
    --sysroot=sysroot/s1c33-none-piece \
    -O2 \
    myapp.c -o myapp.elf

# .pex パッケージ生成 / Generate .pex package
tools/ppack/ppack -e myapp.elf -omyapp.pex -n"My App"
```

`clang` は自動的に crt0.o / piece.ld / `-lclang_rt.builtins-s1c33 --start-group -lcxxrt -lpceapi -lpceshim -lc -lm --end-group` を追加します。

`clang` automatically adds crt0.o, piece.ld, and
`-lclang_rt.builtins-s1c33 --start-group -lcxxrt -lpceapi -lpceshim -lc -lm --end-group`.

### アプリケーションが実装するコールバック / Application Callbacks

```c
#include <piece.h>

void pceAppInit(void)    { /* called once at startup */ }
void pceAppProc(int cnt) { /* called every frame     */ }
void pceAppExit(void)    { /* called at termination  */ }
```

---

## 主な実装済み機能 / Implemented Features

- **命令セット全般** — 16-bit 固定長命令、`ext` 即値拡張（最大2段）、遅延分岐スロット
- **ABI (S5U1C33000C)** — R12–R15 引数、R10 返り値、R0–R3 callee-saved、可変引数は全スタック渡し、構造体は全スタック渡し
- **MC レイヤー** — ELF オブジェクト出力、ext+call/jp の3命令→1命令リラクゼーション
- **逆アセンブラ** — `ext` 拡張後の実効値をコメント表示（例: `; # 0x2c00`）
- **遅延スロットフィラー** — 安全な命令でスロットを充填、不可の場合は `nop`
- **HW 乗算器** — S1C33209 の `mlt.w`/`mlt.h`/`mltu.w` 命令を生成
- **ext+ALU 3-operand** — `ext imm / op %rd, %rs` → `rd = rs <op> imm`（レジスタコピー削減）
- **crt0** — `pceAPPHEAD` 構造体配置・BSS ゼロクリア・バージョンチェック・コールバックラッパー
- **libpceapi** — カーネル API スタブ自動生成（`gen_pceapi.py` + `vector.h`）
- **compiler-rt** — 浮動小数点・整数除算・64bit 整数演算ランタイム (`libclang_rt.builtins-s1c33.a`)。fp.lib/idiv.lib を完全置き換え
- **newlib** — 標準 C ヘッダ + `libc.a` / `libm.a` 本体（`printf` / `malloc` / `sin` / `strtod` / `setjmp` 等）。`lib.lib` / `math.lib` / `io.lib` / `string.lib` / `ctype.lib` を完全置き換え（Phase 2 Stage B、2026-05）
- **simple / sprite / muslib ソースビルド** — シンプル・スプライト・音楽ライブラリは `tools/{simple,sprite,muslib}/` 配下の C/asm ソースから LLVM でビルド。asm ソースは `tools/asm33conv/` で as33 拡張ニーモニックを LLVM 標準命令に展開してからアセンブル
- **C++ サポート** — `libcxxrt.a` による `__cxa_*` スタブ・`operator new/delete`（`-fno-exceptions -fno-rtti` 前提）
- **`libpceapi.a` のソース生成** — カーネル ROM シンボルテーブル `vector.h` から `tools/crt/gen_pceapi.py` がスタブを自動生成（`pceapi.lib` の SRF33 変換は不要）
- **構造体値渡し (byval)** — §6.5.4 準拠、全メンバをスタック経由で渡す（レジスタ不使用）

---

## 既知の制限 / Known Limitations

- **P/ECE 専用** — 汎用 S1C33 ターゲット向けクロスコンパイルには未対応箇所あり
- **MIPS スタイルの可変 GP 最適化は未実装** — R8 はカーネル ABI 規約（R8=0x0）により Reserved 扱い (アロケータ非対象)。一方で `R8 == 0` という事実は最適化材料として活用しており、グローバルへの load/store は `ext sym@ah / ext sym@al / ld.* [%r8]` の 6 バイト形式に畳み込まれる (R8 を可変な `.sdata` 起点として扱うわけではない)
- **`jp.d %rb` 禁止** — ハードウェアバグのため使用しない（詳細: `docs/errata.md`）

---

## ドキュメント / Documentation

| ファイル | 内容 |
|---|---|
| [`docs/setup.md`](docs/setup.md) | セットアップ手順（初回ビルド・sysroot 構築） |
| [`docs/build-howto.md`](docs/build-howto.md) | アプリケーションのビルド手順 |
| [`DESIGN_SPEC.md`](DESIGN_SPEC.md) | アーキテクチャ仕様・ABI・設計判断 |
| [`docs/errata.md`](docs/errata.md) | CPU・コンパイラ・ライブラリのエラッタ |
| [`CLAUDE.md`](CLAUDE.md) | 実装クイックリファレンス（AI 向け） |

参考資料（`docs/*.pdf`、日本語）:
Reference materials in `docs/` (Japanese PDFs):

- `S1C33000_コアCPUマニュアル_2001-03.pdf` — 命令セット・エンコーディング・パイプライン
- `S1C33_Family_Cコンパイラパッケージ.pdf` — ABI (§6.5)・SRF 形式仕様
- `S1C33209_201_222テクニカルマニュアル_PRODUCT_FUNCTION.pdf` — メモリマップ・周辺機器
- `S1C33_family_スタンダードコア用アプリケーションノート.pdf` — 割り込み・ブート手順

---

## P/ECE SDK について / About the P/ECE SDK

P/ECE SDK（`sdk/`）は **このリポジトリには含まれていません**。

**ビルドフローは P/ECE SDK を必要としません** — `make -C tools/crt` で sysroot を構築し、`make -C tools/{simple,sprite}` でライブラリをビルドし、`app/<sample>` で `.pex` を生成するところまで、ホスト上で完結します（カーネル API スタブは `gen_pceapi.py`、標準 C/数学は newlib、シンプル/スプライト/音楽は `tools/{simple,sprite,muslib}/` のソース、浮動小数点・整数除算は compiler-rt が供給）。

SDK が必要になるのは以下の用途のみ:

- 生成した `.pex` を **実機 P/ECE に転送して動作させる**（純正の P/ECE SDK 付属の転送ツールおよびホスト USB ドライバが必要）
- 開発時専用の `make regen-builtins-asm` ターゲットで `fp.lib` / `idiv.lib` のアセンブリを回収する（compiler-rt の S1C33 builtins を再生成するためのメンテナンス作業。常用しない）

これらを必要としない場合、リポジトリ直下に `sdk/` を配置する必要はありません。

The P/ECE SDK (`sdk/`) is **not included** in this repository.

**The build flow does not require the P/ECE SDK** — `make -C tools/crt` builds the sysroot, `make -C tools/{simple,sprite}` builds the libraries, and `app/<sample>` builds a `.pex` entirely on the host (kernel API stubs come from `gen_pceapi.py`, standard C / math from newlib, simple / sprite / music from sources under `tools/{simple,sprite,muslib}/`, and FP / integer division from compiler-rt).

The SDK is only needed for:

- **Transferring a `.pex` to a real P/ECE unit** (which requires the EPSON SDK's transfer tool and host USB driver).
- The developer-only `make regen-builtins-asm` target, which extracts assembly from `fp.lib` / `idiv.lib` to refresh compiler-rt's S1C33 builtins. This is a one-shot maintenance task, not part of regular builds.

If neither of these applies, you don't need to drop a `sdk/` tree at the repository root.

---

## ライセンス / License

LLVM バックエンドのソースコード（`llvm/llvm/lib/Target/S1C33/` およびその他本リポジトリ独自ファイル）は
[Apache License 2.0 with LLVM Exception](https://llvm.org/LICENSE.txt) でライセンスされます。

The backend source code under `llvm/llvm/lib/Target/S1C33/` and other original files
in this repository are licensed under the
[Apache License 2.0 with LLVM Exception](https://llvm.org/LICENSE.txt).

LLVM サブモジュール自体のライセンスは `llvm/llvm/LICENSE.TXT` を参照してください。
For the LLVM submodule itself, see `llvm/llvm/LICENSE.TXT`.
