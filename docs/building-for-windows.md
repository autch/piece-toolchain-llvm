# Windows (MSVC) でビルドするための setup.md への差分

`docs/setup.md` は Linux を前提とした手順書。ここでは **Windows + MSVC
(Visual Studio 2026)** で git clone からサンプルアプリのビルドまでを通すために
必要な差分だけをまとめる。節番号・見出しは `docs/setup.md` に対応させてある
（差分が無い節は省略）。

以下のコマンドは基本的に **cmd.exe** から実行してよい。自分で bash / MSYS2 shell
に入る必要はない（`make` がレシピ実行時に内部でシェルを使うだけ）。

---

## 必要なソフトウェア（setup.md「必要なソフトウェア」への差分）

| ソフトウェア | Windows での要点 |
|---|---|
| **GNU make** | **十分に新しいものを使うこと。** sysroot ビルド（`tools/crt/Makefile` ほか）は、レシピ内で POSIX シェルと coreutils（`cp` / `rm` / `install` / `sed` など）を呼ぶため、それらを spawn できる make が必要。MSYS2 の `make` + coreutils を入れるのが手軽（`c:\msys64\usr\bin\make`）。 |
| C/C++ コンパイラ | LLVM / ppack のホストコンパイルは **MSVC (Visual Studio 2026)**。 |
| CMake | LLVM は Ninja ではなく **Visual Studio ジェネレータ**（マルチ構成）でビルドする。成果物が `build/Release/bin/` に入る点が Linux（`build/bin/`）と異なる（後述の `BIN=` で吸収）。 |
| Meson / Ninja / Python 3 | 提供元は問わない（MSYS2 でも python.org でも可）。**Python 3 のコマンド名が `python3` でない場合**は、後述のとおり make に `PYTHON=<コマンド名>` を渡す。 |
| vcpkg | ppack の zlib 依存を入れるのに使う。提供元・配置場所は問わない。 |

---

## 1. サブモジュールの初期化

clone と同時に済ませる場合の例：

```
git clone --depth=1 --recurse-submodules --shallow-submodules https://github.com/autch/piece-toolchain-llvm.git
```

---

## 2. LLVM のビルド

Visual Studio 2026 ジェネレータでの CMake コマンドラインの例：

```
cmake -G "Visual Studio 18 2026" ../llvm/llvm ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DLLVM_TARGETS_TO_BUILD="" ^
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="S1C33" ^
  -DLLVM_DEFAULT_TARGET_TRIPLE="s1c33-none-piece" ^
  -DLLVM_ENABLE_PROJECTS="clang;lld;lldb" ^
  -DLLVM_INSTALL_UTILS=ON ^
  -DLLVM_OPTIMIZED_TABLEGEN=ON ^
  -DLLVM_PARALLEL_LINK_JOBS=1 ^
  -DLLVM_BUILD_TESTS=OFF ^
  -DLLVM_INCLUDE_TESTS=OFF

cmake --build . --config Release
```

- setup.md の Ninja / `-DCMAKE_BUILD_TYPE=Debug` / `mold` / `ccache` は Linux 向け。
  Windows では上記のとおり Visual Studio ジェネレータ + `Release` 構成に置き換える。
- マルチ構成ジェネレータのため、成果物は **`build/Release/bin/`** に入る
  （Linux の `build/bin/` に相当）。以降の `make` にはこのパスを `BIN=` で渡す。
- 成果物は 2GB ほどある。

---

## 3. ppack のビルド

先に `vcpkg install zlib` を済ませておく。vcpkg のツールチェインファイルを指定して
ビルドする（パスは各自の vcpkg 配置に合わせる）：

```
cmake -B _build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake .

cmake --build _build --config Release
copy _build\Release\ppack.exe .
copy _build\Release\z.dll .
```

`z.dll` も `ppack.exe` と同じ場所に置いておかないと起動に失敗する。

---

## 4. sysroot の構築

### 4-1. sysroot の一括ビルド（CRT + picolibc + ライブラリ）

`tools/crt/Makefile` を、手順 2 でビルドした LLVM ツール群を使って実行する。
Linux との差分は make に渡す 2 つの変数だけ：

- `BIN=` … LLVM の成果物ディレクトリ。MSVC マルチ構成なので `build/Release/bin`
  を指す（setup.md の既定 `build/bin` に対応）。
- `PYTHON=` … Python 3 のインタプリタのコマンド名（既定は `python3`）。

```
c:\msys64\usr\bin\make -C tools/crt BIN=f:/src/piece-toolchain-llvm/build/Release/bin PYTHON=python
```

- `BIN=` のパスは各自の clone 先に合わせて読み替えること。
- **`BIN=` のパスはフォワードスラッシュ（`/`）で書くこと。** バックスラッシュ
  （`f:\src\...`）にすると失敗する。MSYS の make なら POSIX 形式の
  `/f/src/piece-toolchain-llvm/build/Release/bin` への読み替えもうまく処理してくれる。

### sysroot 完成後の確認

setup.md と同じく、`sysroot/s1c33-none-piece/lib/` に一式が揃っていれば準備完了。

---

## 5. 動作確認

sysroot まで通ったら、`app/` 以下のアプリをビルドできる。アプリのビルドは
コンパイルとリンクだけで Python を使わないため、`BIN=` の指定だけでよい：

```
c:\msys64\usr\bin\make BIN=f:/src/piece-toolchain-llvm/build/Release/bin
```

`.pex` が生成されれば、ツールチェーンとして一通り動作している。

---

## 次のステップ

`docs/build-howto.md` を参照して、自分のアプリケーションをビルドする。
