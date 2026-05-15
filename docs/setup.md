# セットアップ手順

リポジトリを clone してから、アプリケーションのビルド手順書（`docs/build-howto.md`）
に着手できるようになるまでの準備手順。

---

## 必要なソフトウェア

| ソフトウェア | 最低バージョン | 用途 |
|---|---|---|
| Git | 2.13 以上 | サブモジュール取得 |
| CMake | 3.13 以上 | LLVM・ppack のビルド設定 |
| Ninja | 任意 | LLVM のビルド実行 |
| C/C++ コンパイラ | GCC 7 / Clang 6 以上 | LLVM・ppack のホストコンパイル |
| Python 3 | 3.6 以上 | `gen_pceapi.py` (libpceapi スタブ生成)・`asm33conv.py` (シンプル/スプライト ライブラリの asm 変換) ほか |
| zlib 開発ヘッダ | 任意 | ppack のリンク依存 |
| iconv | GNU libc 標準 | tools/crt/ 内 UTF-8 ソースの SJIS 変換 (使う場合のみ) |
| autoconf 2.69 / automake 1.15.1 | 厳密一致 | newlib の `configure` / `Makefile.in` 再生成 (滅多に使わない) |

Debian/Ubuntu 系での一括インストール例：

```sh
sudo apt install git cmake ninja-build g++ python3 zlib1g-dev
```

> **autoconf/automake について**: newlib のサブモジュールには既に再生成済みの `configure` と `Makefile.in` が同梱されており、通常のビルドで autotools は呼ばれません。`newlib/newlib/configure.host` や `acinclude.m4` を編集して再生成が必要になった場合のみ、autoconf 2.69 / automake 1.15.1 を **厳密に一致するバージョン** で用意する必要があります (newer/older は互換性なし)。Debian 13 など 2.72 / 1.17 が標準の環境では `~/local/autotools/` 等にソースから入れることになります。詳細は `docs/build-howto.md` の「newlib ポートのメンテナンス」節を参照。

---

## 1. サブモジュールの初期化

LLVM 本体と newlib はサブモジュールとして管理されている。

```sh
git submodule update --init llvm newlib
```

> **注意:** `llvm/` は llvm-project 全体（数 GB）をチェックアウトする。
> 通信帯域に制約がある場合は `--depth 1` を加えてシャロークローンにできる：
>
> ```sh
> git submodule update --init --depth 1 llvm newlib
> ```
>
> ただしシャロークローンでは `git log` の履歴が欠落するため、
> 後から `git fetch --unshallow` で完全取得できる。

---

## 2. LLVM のビルド

`build/` ディレクトリを作成して cmake を実行する。
**build ディレクトリの場所は `build/`（リポジトリ直下）固定。**
他の場所で cmake を実行すると `tools/crt/Makefile` が壊れる。

```sh
mkdir build
cd build

cmake -G Ninja ../llvm/llvm \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_TARGETS_TO_BUILD="" \
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="S1C33" \
  -DLLVM_DEFAULT_TARGET_TRIPLE="s1c33-none-elf" \
  -DLLVM_ENABLE_PROJECTS="clang;lld;lldb" \
  -DLLVM_INSTALL_UTILS=ON \
  -DLLVM_USE_LINKER=mold \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

ninja -j4
```

ビルド完了後、`build/bin/` に以下が揃っていることを確認する：

```
build/bin/clang
build/bin/ld.lld
build/bin/llvm-ar
build/bin/llvm-objcopy
build/bin/llvm-objdump
build/bin/llvm-readelf
```

> `-j4` のジョブ数はメモリと CPU コア数に応じて調整してよい。
> ただし LLVM のリンクは RAM を大量に消費するため、
> 8GB 未満の環境では `-j2` 程度に抑えることを推奨する。
>
> **`-DCMAKE_BUILD_TYPE=Debug`** はデバッグビルドのため最終バイナリが大きく
> なるが、バックエンド開発中は `-gline-tables-only` や `assert()` が有効になる
> ため推奨する。リリースビルドが必要な場合は `RelWithDebInfo` に変更する。

---

## 3. ppack のビルド

P/ECE アプリのパッケージファイル（`.pex`）を生成するツール。
LLVM の cmake キャッシュとは独立して cmake を実行する。

```sh
cd tools/ppack
cmake -G Ninja -B _build -DCMAKE_BUILD_TYPE=Release .
ninja -C _build
cp _build/ppack ppack
cd ../..
```

完成物: `tools/ppack/ppack`

---

## 4. sysroot の構築

コンパイラが参照するヘッダとライブラリを `sysroot/s1c33-none-elf/` に配置する。

P/ECE 純正開発環境の `c:/usr/piece` 以下のうち、`include/` と `lib/` を `sdk/` ディレクトリ以下にコピーすること。標準 C ヘッダは newlib サブモジュールから自動的にインストールされる。

### 4-1. sysroot の一括ビルド（CRT + newlib + SDK ライブラリ）

スタートアップオブジェクト・newlib (libc / libm)・カーネル API スタブ・SDK ライブラリ変換・compiler-rt をすべてまとめて実行する。
**手順 2 の LLVM ビルドが完了している必要がある。**

```sh
make -C tools/crt
```

以下が自動的に実行される：

1. `newlib/newlib/libc/include/` から標準 C ヘッダを `sysroot/s1c33-none-elf/include/` にインストール
2. `tools/crt/include/` から P/ECE 固有ヘッダ（`piece.h`、`draw.h`、`s1c33cpu.h` 等）をコピー (オリジナルは `sdk/include/` だが、ビルドからは参照しない reference material 化済み)
3. `tools/sprite/pclsprite.h` と `tools/simple/{simple,thread}.h` をシンボリックな canonical 元としてコピー
4. Clang 組み込みと競合するヘッダ（`stddef.h`、`stdarg.h`、`float.h`）を除去
5. `crt0.o`・`crti.o`・`libpceapi.a` を LLVM でビルド
6. `libclang_rt.builtins-s1c33.a`（compiler-rt; fp.lib/idiv.lib の後継）を cmake でビルド
7. **newlib の `libc.a` / `libm.a` を S1C33 向けにビルド** (`build/crt/newlib/` 内で `configure` + `make`)
8. `libmuslib.a`（音楽ライブラリ、`tools/muslib/` のソースからビルド）と `libpceshim.a`（newlib の `rand` / `__assert_func` を小型版で上書きするシム）をビルド・インストール

初回ビルドは newlib のフルビルドに数分かかる。以降は差分ビルドで `tools/crt/` 内の変更のみ再ビルドされる。

以下が生成される：

| ファイル | 役割 |
|---|---|
| `sysroot/s1c33-none-elf/lib/crt0.o` | アプリヘッダ（`pceAppHead` @ 0x100000）、BSS ゼロクリア、コールバックラッパー |
| `sysroot/s1c33-none-elf/lib/crti.o` | `pceAppNotify` デフォルト実装（弱シンボル・上書き可能） |
| `sysroot/s1c33-none-elf/lib/libpceapi.a` | カーネル API スタブ + ユーティリティ |
| `sysroot/s1c33-none-elf/lib/libclang_rt.builtins-s1c33.a` | compiler-rt（FP 演算・整数除算・i64 算術ランタイム） |
| `sysroot/s1c33-none-elf/lib/libcxxrt.a` | C++ ランタイムスタブ（operator new/delete 等） |
| `sysroot/s1c33-none-elf/lib/libc.a` | newlib libc (printf / malloc / strtod / setjmp / 等) |
| `sysroot/s1c33-none-elf/lib/libm.a` | newlib libm (sin / cos / pow / sqrt / 等) |
| `sysroot/s1c33-none-elf/lib/piece.ld` | リンカスクリプト (P/ECE メモリマップ + ヒープ配置) |

> **newlib Phase 2 完了 (2026-05)**: 既知の EPSON SDK バグ (`sin`, `strtok`, `pow`, `strtod`, `ispunct`) を持つ `lib.lib` / `math.lib` 等は newlib で完全に置き換え済み。Stage A (newlib と SDK 並列リンク) と Stage B (SDK ライブラリ完全削除) を経て、現在のリンク行は `-lclang_rt.builtins-s1c33 --start-group -lcxxrt -lpceapi -lpceshim -lc -lm --end-group` のみ。詳細は `docs/build-howto.md` の「リンク順序」節を参照。

> **注意:** `crt0.o` は `-O1` でコンパイルされる。BSS ゼロクリアループの
> カウンタ変数が `[SP+0]` に置かれると、カーネルが SP を bss_end に設定した場合に
> ループが自分のカウンタを上書きしてしまうため、`-O0` でのビルドは禁止。
> Makefile の `CFLAGS_CRT` は `-O1` が設定されており、変更しないこと。

音楽ライブラリ `libmuslib.a` は `tools/muslib/` のソースから LLVM でビルドされ、上記 `make -C tools/crt` で sysroot に自動インストールされる。

シンプル / スプライトライブラリ (`libsimple.a` / `libsprite.a`) は各々のソースディレクトリで個別に make する：

```sh
make -C tools/simple
make -C tools/sprite
```

どちらも `tools/asm33conv/asm33conv.py` を経由して `.s` ファイル中の as33 拡張ニーモニックを LLVM 標準命令に展開する。アプリ側で `-lmuslib` / `-lsimple` / `-lsprite` を明示指定してリンクする。

### sysroot 完成後の確認

```sh
ls sysroot/s1c33-none-elf/lib/
```

以下がすべて揃っていれば準備完了：

```
crt0.o  crti.o  piece.ld
libclang_rt.builtins-s1c33.a
libc.a      libm.a                              ← newlib
libcxxrt.a  libpceapi.a  libpceshim.a           ← C++ ランタイム + カーネル API + newlib シム
libmuslib.a                                     ← 音楽ライブラリ (-lmuslib で明示指定)
libsimple.a libdefinst.a                        ← シンプルライブラリ (-lsimple で明示指定; make -C tools/simple で生成)
libsprite.a                                     ← スプライトライブラリ (-lsprite で明示指定; make -C tools/sprite で生成)
```

---

## 5. 動作確認

サンプルアプリは `app/` 以下に集約されている（`app/hello/`、`app/jien/`、`app/fpkplay/` など）。

```sh
cd app/hello
make
```

`hello_l.pex` が生成されれば、ツールチェーンとして一通り動作している。

---

## 次のステップ

`docs/build-howto.md` を参照して、自分のアプリケーションをビルドする。
