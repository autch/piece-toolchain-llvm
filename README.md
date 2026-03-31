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
| トリプル | `s1c33-none-elf` |
| ステータス | **Phase 6 完了** — 実機動作確認済み（2026-03） |
| ベース LLVM | llvm-project (サブモジュール, `llvm/` 以下) |

P/ECE SDK の既製ライブラリ（`pceapi.lib`, `fp.lib`, `idiv.lib` 等）は EPSON 独自の
SRF33 形式で配布されています。それらはライセンス上このリポジトリ自体には含められないため、
本リポジトリに含まれる `tools/srf2elf/` と `tools/crt/` Makefile がそれらを ELF 形式へ
変換・再コンパイルし、`sysroot/` に配置します。

P/ECE SDK libraries (`pceapi.lib`, `fp.lib`, `idiv.lib`, etc.) are distributed in
EPSON's proprietary SRF33 format. They cannot be included in this repository due to licensing restrictions, so the `tools/srf2elf/` converter and `tools/crt/` Makefile translate them to ELF and install them into `sysroot/`.

---

## 動作確認済みアプリケーション / Verified Applications

以下のアプリケーションが実機 P/ECE で動作確認されています。

The following applications have been verified on a real P/ECE device.

| アプリ | ビルド方法 | 確認内容 |
|---|---|---|
| `mini_nocrt/` | crt0 手書き、手動リンク | 画面描画・ST+SL でメニュー復帰 |
| `minimal/` | sysroot の crt0 + pceapi | 同上 |
| `hello/` | EPSON SDK CRT 完全使用 | `printf` 表示・システムメニュー・メニュー復帰 |
| `print/` | EPSON SDK CRT 完全使用 | `pceFontPutStr` 複数呼び出し・`pcesprintf` フォーマット |
| `jien/` | EPSON SDK CRT + 描画ライブラリ | ビットマップ表示・構造体値渡し（`pceLCDDrawObject`） |
| `fpkplay/` | EPSON SDK CRT + muslib + LZSS | FPK 音楽再生（8kHz/16kHz波形合成） |
| `pmdplay/` | EPSON SDK CRT + muslib + PMD | PMD 音楽再生（複数楽曲切替・波形合成） |

---

## リポジトリ構成 / Repository Layout

```
llvm-c33/
├── llvm/                   LLVM サブモジュール (llvm-project)
│   └── llvm/lib/Target/S1C33/   バックエンド実装
├── build/                  CMake ビルドディレクトリ（初回 cmake 後に生成）
├── sdk/                    P/ECE SDK（別途入手・配置）
│   ├── include/
│   └── lib/
├── sysroot/s1c33-none-elf/ ビルド済み sysroot（make -C tools/crt で生成）
├── tools/
│   ├── crt/                crt0.c, defnotify.c, libpceapi.a 生成 Makefile
│   ├── srf2elf/            SRF33 → ELF 変換ツール（Python）
│   ├── elf2srf/            ELF → SRF33 変換ツール（Python, 実験的）
│   ├── ppack/              ELF → .pex パッケージャ（C++, cmake）
│   ├── asm33conv/          EPSON as33 アセンブリ → LLVM IR コンバータ
│   └── piece.ld            P/ECE アプリ用リンカースクリプト
├── hello/                  サンプルアプリ（EPSON SDK CRT 使用）
├── print/                  サンプルアプリ（SDK 文字列描画）
├── jien/                   サンプルアプリ（ビットマップ描画・構造体値渡し）
├── fpkplay/                サンプルアプリ（FPK 音楽再生・LZSS 展開・波形合成）
├── pmdplay/                サンプルアプリ（PMD 音楽再生・複数楽曲・波形合成）
├── minimal/                サンプルアプリ（sysroot crt0 使用）
├── mini_nocrt/             サンプルアプリ（crt0 手書き・最小構成）
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
# 1. サブモジュール取得 / Fetch LLVM submodule
git submodule update --init llvm

# 2. LLVM ビルド / Build LLVM
mkdir build && cd build
cmake -G Ninja ../llvm/llvm \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="S1C33" \
  -DLLVM_DEFAULT_TARGET_TRIPLE="s1c33-none-elf" \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_INSTALL_UTILS=ON \
  -DLLVM_USE_LINKER=lld \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
ninja
cd ..

# 3. ppack ビルド / Build ppack
cd tools/ppack && cmake -G Ninja -B _build -DCMAKE_BUILD_TYPE=Release . && ninja -C _build
cp _build/ppack ppack && cd ../..

# 4. SDK ヘッダコピー（sdk/ を別途配置済みの場合）
# Copy SDK headers (requires sdk/ placed separately)
mkdir -p sysroot/s1c33-none-elf/include sysroot/s1c33-none-elf/lib
cp sdk/include/*.h sysroot/s1c33-none-elf/include/
cp tools/piece.ld  sysroot/s1c33-none-elf/lib/piece.ld

# 5. sysroot 一括ビルド（CRT + SDK ライブラリ変換）
# Build sysroot: CRT objects + SDK library conversion
make -C tools/crt

# 6. サンプルアプリをビルド / Build sample app
cd hello && make
```

`hello/hello_l.pex` が生成されれば成功です。
If `hello/hello_l.pex` is produced, the toolchain is working.

---

## アプリケーションのビルド / Building Your Application

詳細は [`docs/build-howto.md`](docs/build-howto.md) を参照してください。
See [`docs/build-howto.md`](docs/build-howto.md) for full instructions.

### コンパイル＋リンク＋パッケージ（最小例）
### Compile + Link + Package (minimal example)

```sh
# コンパイル＋リンク / Compile + link
build/bin/clang \
    --target=s1c33-none-elf \
    --sysroot=sysroot/s1c33-none-elf \
    -O2 -Wno-incompatible-library-redeclaration \
    myapp.c -o myapp.elf

# .pex パッケージ生成 / Generate .pex package
tools/ppack/ppack -e myapp.elf -omyapp.pex -n"My App"
```

`clang` は自動的に crt0.o / piece.ld / `-lpceapi -lio -llib -lmath -lstring -lctype -lfp -lidiv` を追加します。

`clang` automatically adds crt0.o, piece.ld, and
`-lpceapi -lio -llib -lmath -lstring -lctype -lfp -lidiv`.

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
- **SRF33 変換** — EPSON 独自形式 SDK ライブラリを ELF/GNU ar 形式へ変換
- **構造体値渡し (byval)** — §6.5.4 準拠、全メンバをスタック経由で渡す（レジスタ不使用）

---

## 既知の制限 / Known Limitations

- **P/ECE 専用** — 汎用 S1C33 ターゲット向けクロスコンパイルには未対応箇所あり
- **GP 最適化未実装** — R8 はカーネル ABI 規約（R8=0x0）を尊重して Reserved のみ
- **64-bit 整数演算** — `__fixsfdi` 等は compiler-rt が未提供（実用上は問題なし）
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
別途入手してリポジトリルートに配置してください。

The P/ECE SDK (`sdk/`) is **not included** in this repository.
Obtain it separately and place it at the repository root.

---

## ライセンス / License

LLVM バックエンドのソースコード（`llvm/llvm/lib/Target/S1C33/` およびその他本リポジトリ独自ファイル）は
[Apache License 2.0 with LLVM Exception](https://llvm.org/LICENSE.txt) でライセンスされます。

The backend source code under `llvm/llvm/lib/Target/S1C33/` and other original files
in this repository are licensed under the
[Apache License 2.0 with LLVM Exception](https://llvm.org/LICENSE.txt).

LLVM サブモジュール自体のライセンスは `llvm/llvm/LICENSE.TXT` を参照してください。
For the LLVM submodule itself, see `llvm/llvm/LICENSE.TXT`.
