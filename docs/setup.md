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
| Python 3 | 3.6 以上 | `srf2elf.py` による SDK 変換 |
| zlib 開発ヘッダ | 任意 | ppack のリンク依存 |

Debian/Ubuntu 系での一括インストール例：

```sh
sudo apt install git cmake ninja-build g++ python3 zlib1g-dev
```

---

## 1. サブモジュールの初期化

LLVM 本体はサブモジュールとして管理されている。

```sh
git submodule update --init llvm
```

> **注意:** `llvm/` は llvm-project 全体（数 GB）をチェックアウトする。
> 通信帯域に制約がある場合は `--depth 1` を加えてシャロークローンにできる：
>
> ```sh
> git submodule update --init --depth 1 llvm
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
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="S1C33" \
  -DLLVM_DEFAULT_TARGET_TRIPLE="s1c33-none-elf" \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_INSTALL_UTILS=ON \
  -DLLVM_USE_LINKER=lld \
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

P/ECE 純正開発環境の `c:/usr/piece` 以下のうち、`include/` と `lib/` を `sdk/` ディレクトリ以下にコピーすること。今後カーネルのコンパイルもできるようになったら、同様に `sysdev/` 以下もここに置かれることになるはず。

### 4-1. SDK ヘッダのコピー

```sh
mkdir -p sysroot/s1c33-none-elf/include
cp sdk/include/*.h sysroot/s1c33-none-elf/include/
```

### 4-2. リンカースクリプトのコピー

```sh
mkdir -p sysroot/s1c33-none-elf/lib
cp tools/piece.ld sysroot/s1c33-none-elf/lib/piece.ld
```

### 4-3. sysroot の一括ビルド（CRT + SDK ライブラリ）

スタートアップオブジェクト・カーネル API スタブ・SDK ライブラリ変換をすべてまとめて実行する。
**手順 2 の LLVM ビルドが完了している必要がある。**

```sh
make -C tools/crt
```

以下が生成される：

| ファイル | 役割 |
|---|---|
| `sysroot/s1c33-none-elf/lib/crt0.o` | アプリヘッダ（`pceAppHead` @ 0x100000）、BSS ゼロクリア、コールバックラッパー |
| `sysroot/s1c33-none-elf/lib/crti.o` | `pceAppNotify` デフォルト実装（弱シンボル・上書き可能） |
| `sysroot/s1c33-none-elf/lib/libpceapi.a` | カーネル API スタブ + ユーティリティ |
| `sysroot/s1c33-none-elf/lib/lib{io,lib,math,string,ctype,fp,idiv}.a` | SDK ライブラリ（SRF33 → ELF 自動変換） |

> **注意:** `crt0.o` は `-O1` でコンパイルされる。BSS ゼロクリアループの
> カウンタ変数が `[SP+0]` に置かれると、カーネルが SP を bss_end に設定した場合に
> ループが自分のカウンタを上書きしてしまうため、`-O0` でのビルドは禁止。
> Makefile の `CFLAGS_CRT` は `-O1` が設定されており、変更しないこと。

音楽・スプライトライブラリは自動変換対象外。使用する場合は個別に変換する：

```sh
python3 tools/srf2elf/srf2elf.py sdk/lib/muslib.lib sysroot/s1c33-none-elf/lib/libmuslib.a
python3 tools/srf2elf/srf2elf.py sdk/lib/sprite.lib sysroot/s1c33-none-elf/lib/libsprite.a
```

### sysroot 完成後の確認

```sh
ls sysroot/s1c33-none-elf/lib/
```

以下がすべて揃っていれば準備完了：

```
crt0.o  crti.o  piece.ld
libctype.a  libfp.a  libidiv.a  libio.a  liblib.a
libmath.a   libpceapi.a  libstring.a
```

---

## 5. 動作確認

`hello/` にサンプルアプリが用意されている。

```sh
cd hello
make
```

`hello_l.pex` が生成されれば、ツールチェーンとして一通り動作している。

---

## 次のステップ

`docs/build-howto.md` を参照して、自分のアプリケーションをビルドする。
