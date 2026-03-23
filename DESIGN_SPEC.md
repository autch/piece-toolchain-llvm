# S1C33000 LLVM Backend — 設計仕様書

## 1. プロジェクト概要

### 1.1 目的

アクアプラス P/ECE（S1C33209搭載）の開発環境を、LLVM/Clangベースで再構築する。P/ECE SDKの既存ライブラリおよびカーネルAPIとABI互換でリンク可能なバイナリを生成することが最終目標。

### 1.2 スコープ

| 項目 | 対応状況 |
|---|---|
| gcc33（S5U1C33000C）向けCソースのコンパイル | **対応**（ABI準拠、構造体/double引数渡し検証済み） |
| P/ECE SDK ライブラリとのリンク | **対応**（SRF→ELF変換 or ソースビルド） |
| P/ECE カーネルAPIの呼び出し | **対応**（ABI互換 + ヘッダ提供） |
| Clang → リンク → .pex のワンコマンドビルド | **対応**（sysroot + ToolChain + ppack） |
| 既存の純正アセンブリソース(.s)の互換性 | **asm33conv経由で対応**（§10.6準拠、一部カテゴリ未実装） |
| インラインasmの基本命令 | **対応**（LLVMのMCレイヤーでパース） |
| インラインasmの拡張命令（x付きニーモニック） | **対象外**（基本命令に書き下す必要あり） |
| 出力形式 | **ELF**（エプソン純正SRFではない） |
| .pex 生成 | **対応**（ppack ELF対応版） |
| デバッガ連携 | **DWARF**（db33の.def形式ではない） |
| ROMイメージ生成 | `llvm-objcopy -O binary` で対応 |

### 1.3 互換ABI

S5U1C33000C（gcc33）のABIに準拠する。S5U1C33001C（xgcc）のABIとは**異なる**ため混同しないこと。P/ECE SDKの既存バイナリ（.lib）はすべてS5U1C33000C ABIでコンパイルされているため、これとの互換は必須要件。

| | S5U1C33000C（本プロジェクト） | S5U1C33001C（参考） |
|---|---|---|
| Callee-saved | R0〜R3 | R0〜R3 |
| Scratch | R4〜R7 | R4〜R5（戻り値兼用） |
| GP/予約 | R8(カーネルテーブルベース=0x0), R9(予約) | — |
| 戻り値 | R10, R11 | R4, R5 |
| 引数 | R12〜R15 | R6〜R9 |
| データエリアポインタ | なし | R12〜R15 |

### 1.4 P/ECE SDK構成とLLVM側の対応

P/ECE SDKは以下の構成要素からなる。LLVMツールチェインで開発する際、それぞれどう扱うか。

**カーネル（BIOS）— ROM常駐:**

カーネルはP/ECE本体のROMに書き込まれており、固定アドレスでAPI関数を提供する。アプリケーションは `pceAppInit()` / `pceAppProc()` / `pceAppExit()` を実装し、カーネルからコールバックされる。`pceLCDTrans()`、`pcePadGet()` 等のシステムAPIもカーネルが提供する。

→ カーネル側の関数アドレスは `pcekn.sym` から得られる。LLVMツールチェインでは、これらのシンボルをリンカスクリプトまたはヘッダで宣言して利用する。コードの生成・リンクではなくABI互換のみが要件。

**SDKライブラリ（SRF形式バイナリ、ソース非提供）:**

スタートアップオブジェクト:

| オブジェクト | 内容 | LLVM側の対応 |
|---|---|---|
| `cstart.o` | スタートアップコード（pceAPPHEAD + pceAppInit00） | **srf2elfで変換済み** |
| `defnotify.o` | pceAppNotify デフォルト実装 | **srf2elfで変換済み** |

標準リンクライブラリ（リンク順序）:

| 順序 | ライブラリ | 内容 | LLVM側の対応 |
|---|---|---|---|
| 1 | `pceapi.lib` | P/ECE カーネルAPIスタブ（pceLCDTrans等） | **srf2elfで変換可能** |
| 2 | `io.lib` | I/O関数 | **srf2elfで変換可能** |
| 3 | `lib.lib` | 標準Cライブラリ（既知バグあり、errata.md参照） | **srf2elfで変換可能** |
| 4 | `math.lib` | 数学関数 | **srf2elfで変換可能** |
| 5 | `string.lib` | C文字列関数（memset, memcpy等） | **srf2elfで変換可能** |
| 6 | `ctype.lib` | 文字分類関数 | **srf2elfで変換可能** |
| 7 | `fp.lib` | 浮動小数点演算（`__addsf3`, `__divsf3`等） | **srf2elfで変換可能** / compiler-rtで再実装も可 |
| 8 | `idiv.lib` | 整数除算（`__divsi3`, `__modsi3`等） | **srf2elfで変換可能** / compiler-rtで再実装も可 |

上記以外のライブラリ（`muslib.lib` 等）はアプリケーション側で必要に応じて明示的にリンクする。

srf2elfは .o（SRFオブジェクト）と .lib（lib33アーカイブ）の両方に対応済み。lib33アーカイブのフォーマットは `docs/lib33_format.md` を参照。

リンクコマンド例:
```
ld.lld -T tools/piece.ld cstart.o defnotify.o hello.o \
  -Lbuild -lpceapi -lio -llib -lmath -lstring -lctype -lfp -lidiv \
  -o hello.elf
```

fp.libとidiv.libはcompiler-rtで再実装して置き換えることも可能。lib.libにはエラッタ（§6参照）があるため、バグのある関数を個別に差し替えるか、newlib等で代替する選択肢もある。

**SRF→ELF変換ツール（srf2elf）:**

srf2elfはSRFオブジェクトファイル(.o)のELF変換に対応済み。SRFフォーマットの仕様はCコンパイラマニュアルの Appendix「srf33ファイルの構造」に記載（ただしe_scnndxのサイズに誤記あり、errata.md参照）。

変換で対処すべき主な要素:
- セクション情報（コード/データ/BSS）のELFセクションへの写像
- シンボルテーブルの変換
- リロケーション情報の変換（SRF固有のリロケーション型→ELF定義のリロケーション型）

**ビルドフロー比較:**

```
[純正SDK]
  pcc33 hello.c
    → gcc33 → ext33 → as33 → lk33(+boot.o +pceapi.lib +lib.lib +fp.lib +idiv.lib ...)
    → hello.srf → ppack → hello.pex

[LLVMツールチェイン]
  clang --target=s1c33-none-elf hello.c
    → LLVM backend → ELF .o
  srf2elf boot.o pceapi.lib io.lib lib.lib math.lib string.lib ctype.lib fp.lib idiv.lib ...
    → ELF .o / .a に変換
  ld.lld -T piece.ld hello.o boot.o -lfp -lidiv -lsimple
    → hello.elf → llvm-objcopy -O binary → hello.bin
    → (別途pex化が必要な場合はppackまたは互換ツール)
```

**P/ECE メモリマップ:**

```
0x000000  内蔵RAM 8KB（システム使用）
0x001000  ユーザ使用可
0x002000  スタックの底
0x100000  外付けSRAM 256KB — アプリのユーザエリア
0x13D000  システムワーク
0x140000  (SRAM末尾)
0xC00000  外付けフラッシュ（512KB〜2MB）割込みベクタ＋緊急カーネル
0xC02000  通常カーネル開始
0xC0C000  フォント
0xC28000〜 ファイルシステム
```

**アプリケーション実行モデル:**

P/ECEのアプリファイル `.pex` はフラッシュ上のファイルシステムに圧縮格納されている。アプリ起動時にカーネルがSRAMの 0x100000 に展開し、エントリポイントを呼び出す。

このため:
- ユーザアプリのエントリポイントは **0x100000**
- .text, .rodata, .data, .bss すべてがSRAM上に配置される
- ROM→RAMコピー（LMA/VMA分離）は不要
- リンカスクリプトは全セクションを 0x100000 から素直に配置するだけでよい
- アプリが使えるSRAM領域は 0x100000〜0x13D000（244KB）

**アプリケーションヘッダ (pceAPPHEAD):**

0x100000 の先頭にはアプリケーションヘッダ構造体が配置されなければならない。カーネルはこの構造体を読んでアプリの各コールバック関数を呼び出す。

```c
typedef struct _pceAPPHEAD {
    unsigned long signature;                     // +0  シグネチャ 'pCeA' (0x41654370 LE)
    unsigned short sysver;                       // +4  システムバージョン
    unsigned short resv1;                        // +6  予約(=0)
    void (*initialize)( void );                  // +8  初期化関数 → pceAppInit
    void (*periodic_proc)( int cnt );            // +12 処理関数 → pceAppProc
    void (*pre_terminate)( void );               // +16 終了前関数 → pceAppExit
    int (*notify_proc)( int type, int param );   // +20 通知関数
    unsigned long stack_size;                    // +24 スタックサイズ
    unsigned char *bss_end;                      // +28 BSSの終了アドレス
} pceAPPHEAD;
```

リンカスクリプトへの要件:
- pceAPPHEAD（cstart.o の pceAppHead セクション）が必ず 0x100000 の先頭に配置されること
- bss_end フィールドはリンク時に .bss セクションの末尾アドレスで解決される
- pceAPPHEAD の直後に実際のコード（pceAppInit00 等のSDKラッパー）が続く

**lk33 互換ブロックシンボル:**

lk33 は各ブロック（セクション）に対して `__START_<name>`, `__END_<name>`, `__SIZEOF_<name>` の3シンボルを自動生成する（§12.6.5）。lld にはこの自動生成機能がないため、piece.ld で明示的に定義する。

デフォルトで定義するシンボル（cstart.c 等が参照）:
```
__START_DEFAULT_CODE / __END_DEFAULT_CODE / __SIZEOF_DEFAULT_CODE
__START_DEFAULT_DATA / __END_DEFAULT_DATA / __SIZEOF_DEFAULT_DATA
__START_DEFAULT_BSS  / __END_DEFAULT_BSS  / __SIZEOF_DEFAULT_BSS
```

**内蔵RAM転送パターン（応用）:**

S1C33209の内蔵RAM（0x001000〜0x002000）はSRAMより高速にアクセスできる。高速処理が必要なコードを実行時にSRAMから内蔵RAMに転送して実行するパターンが、P/ECEアプリケーションで見られる。

lk33ではリンカコマンドラインオプションでブロック名を都度命名し、仮想ブロック（`@`付き）として配置する。lld で同等のことを行うには、リンカスクリプトで LMA/VMA 分離とブロックシンボルを定義する。

例: `.fast_code` セクションを内蔵RAM（VMA=0x001000）に配置し、ロードイメージはSRAM内（LMA は .bss の直後）に置く場合:

```
MEMORY
{
    SRAM : ORIGIN = 0x100000, LENGTH = 0x3D000
    IRAM : ORIGIN = 0x001000, LENGTH = 0x1000
}

SECTIONS
{
    /* ... 通常セクション（SRAM上）... */

    .fast_code :
    {
        __START_FAST_CODE = .;
        *(.fast_code .fast_code.*)
        __END_FAST_CODE = .;
    } > IRAM AT> SRAM
    __SIZEOF_FAST_CODE = __END_FAST_CODE - __START_FAST_CODE;
    __LOAD_FAST_CODE = LOADADDR(.fast_code);
}
```

ユーザコードで転送:
```c
extern char __START_FAST_CODE[], __LOAD_FAST_CODE[], __SIZEOF_FAST_CODE[];
memcpy(__START_FAST_CODE, __LOAD_FAST_CODE, (size_t)__SIZEOF_FAST_CODE);
```

対象関数を `.fast_code` セクションに配置する属性:
```c
__attribute__((section(".fast_code"))) void hot_loop(void) { ... }
```

ブロック名とシンボル名はアプリケーションごとに自由に決めてよい。piece.ld のデフォルトにはIRAM転送用セクションを含めない。必要なアプリケーションが独自のリンカスクリプトで定義する。

---

## 2. ターゲットアーキテクチャ仕様

### 2.1 基本特性

- **アーキテクチャ**: S1C33000 — 32ビットRISC、ロード・ストア型
- **命令長**: 16ビット固定（ext命令による即値拡張あり）
- **エンディアン**: リトルエンディアン
- **アドレス空間**: 28ビット（256MB）、上位4ビットは無視される
- **パイプライン**: 5段（Fetch → Decode → Execute → Memory → Writeback）
- **ディレイドブランチ**: あり（分岐命令の直後1命令がディレイスロット）

### 2.2 レジスタセット

**汎用レジスタ（32ビット × 16本）:**

| レジスタ | ABI上の用途 | LLVM分類 |
|---|---|---|
| R0〜R3 | Callee-saved | CalleeSavedRegs |
| R4〜R7 | Scratch（caller-saved） | 一時レジスタ |
| R8 | カーネルテーブルベースポインタ（常に 0x0） | Reserved |
| R9 | 予約（純正ツールではext33スクラッチ） | Reserved |
| R10 | 戻り値（32bit以下 / doubleの下位ワード） | RetValReg |
| R11 | 戻り値（doubleの上位ワード） | RetValRegHi |
| R12〜R15 | 引数渡し（最大4ワード） | ArgRegs |

**特殊レジスタ:**

| レジスタ | 用途 |
|---|---|
| PC | プログラムカウンタ（28ビット有効） |
| SP | スタックポインタ |
| PSR | プロセッサステータスレジスタ（N, Z, V, C フラグ） |
| ALR | 算術演算ローレジスタ（乗除算・MAC結果の下位32ビット） |
| AHR | 算術演算ハイレジスタ（乗除算・MAC結果の上位32ビット / 除算の余り） |

### 2.3 データ型

| C型 | サイズ | アライメント |
|---|---|---|
| char | 1バイト | 1バイト |
| short | 2バイト | 2バイト |
| int / long | 4バイト | 4バイト |
| pointer | 4バイト | 4バイト |
| float | 4バイト（IEEE754） | 4バイト |
| double | 8バイト（IEEE754） | 4バイト |
| long long | 8バイト | 4バイト |

DataLayout文字列: `e-m:e-p:32:32-i1:8-i8:8-i16:16-i32:32-i64:32-f32:32-f64:32-n32-S32`

---

## 3. 呼び出し規約

### 3.1 引数渡し

- 引数はR12→R13→R14→R15の順に格納（最大4ワード）
- 4ワードを超える引数はスタック渡し
- double型は2レジスタを消費し、空きが2つなければスタックに回す
- 構造体の値渡しはすべてスタック経由
- 構造体を返す関数は、結果格納先のポインタがR12に暗黙の第1引数（sret）

### 3.2 戻り値

- 32ビット以下: R10
- 64ビット（double等）: R10（下位）+ R11（上位）
- 構造体: R12に渡されたポインタ経由で書き込み（sret）

### 3.3 スタックフレームレイアウト

高アドレスから低アドレス方向に:
```
+------------------------+ ← 呼び出し元のSP
| リターンアドレス       |    (call命令がスタックにプッシュ)
+------------------------+
| レジスタ退避領域       |    (R3, R2, R1, R0 — 使用分のみ、pushn/popnで操作)
+------------------------+
| ローカル変数領域       |
+------------------------+
| 引数領域（callee用）   |    (4ワード超の引数)
+------------------------+ ← 現在のSP
```

- フレームポインタは使用しない（SPベースのアドレッシング）
- call命令がリターンアドレスをスタックにプッシュし、ret命令でポップして復帰

### 3.4 可変引数関数（Variadic Functions）

**EPSONマニュアルに未記載のABI仕様。gcc33の実バイナリ出力から解析。**

可変引数関数（`printf`、`scanf`等）では、通常の関数と**まったく異なる引数渡し規約**が適用される:

**呼び出し側（caller）:**
- R12〜R15を**一切使用しない**
- 固定引数・可変引数を問わず**全引数をスタックに積む**
- 第1引数が `[%sp+0]`、第2引数が `[%sp+4]`、第3引数が `[%sp+8]`...と連続配置
- 引数領域のサイズ分だけ事前に `sub %sp, N` でスタックを確保し、呼び出し後に `add %sp, N` で復帰

**callee側:**
- 全引数がスタック上に連続して配置されているため、レジスタからのスピル処理は不要
- `va_start`は最後の固定引数のスタックアドレスから `_SIZEOF(lastparm)` 分進めたポインタを返す

**根拠（gcc33 -O0 出力から確認）:**

```
; pceFontPrintf("Hello, %s! %d", "Alice", 5) の呼び出し
sub  %sp, 0x3          ; 3ワード = 12バイト確保
xld.w %r10, __LC0      ; format string
xld.w [%sp+0], %r10    ; 第1引数 → スタック（R12ではない！）
xld.w %r10, __LC1      ; "Alice"
xld.w [%sp+4], %r10    ; 第2引数 → スタック（R13ではない！）
xld.w %r10, 0x5        ; 5
xld.w [%sp+8], %r10    ; 第3引数 → スタック（R14ではない！）
xcall pceFontPrintf
add  %sp, 0x3          ; スタック復帰
```

**`stdarg.h` の実装（SDK実物より）:**

```c
typedef char *va_list;
#define _BOUNDARY   4
#define _SIZEOF(type) (((sizeof(type) + _BOUNDARY - 1) / _BOUNDARY) * _BOUNDARY)
#define va_start(tdArgP, lastparm) \
    (void)(tdArgP = (va_list)&(lastparm) + _SIZEOF(lastparm))
#define va_arg(tdArgP, type) \
    *((type *)((tdArgP = tdArgP + _SIZEOF(type)) - \
      ((sizeof(type) + _BOUNDARY - 1) / _BOUNDARY) * _BOUNDARY))
#define va_end(tdArgP) (void)0
```

`va_start`が `&(lastparm)` を取るため、固定引数がスタック上に存在することが前提。R12〜R15に格納されていたら `&` 演算子でアドレスが取れないので、この設計は**全引数スタック渡し**を前提としている。

**LLVM実装への影響:**

1. **`LowerCall`（呼び出し側）:** calleeがvariadicの場合、`CCAssignToReg`をスキップし全引数を`CCAssignToStack`で処理する
2. **`LowerFormalArguments`（callee側）:** isVarArgの場合も全引数がスタック上にある前提で処理。ARM/MIPSのような「プロローグでレジスタ引数をスタックにスピルしてVarArgsSaveAreaを作る」パターンは**不要**
3. **`LowerVASTART`:** `VarArgsFrameIndex`を、固定引数領域の直後を指すフレームインデックスとして計算
4. **`LowerVACOPY`:** 単純なポインタコピー（`va_list`は`char*`）

### 3.5 構造体引数の未記載規約（piece-lab-2019調査）

EPSONマニュアル（6.5.4節）では「構造体引数はすべてスタック渡し」と記載されているが、gcc33の実際の動作では**32ビット以内で1要素のみの構造体はレジスタ渡し**される:

```c
struct _foo { int a; };          // → R12にレジスタ渡し（マニュアルと異なる）
struct _foo { int a; int b; };   // → スタック渡し（マニュアルどおり）
struct _foo { long long a; };    // → スタック渡し（マニュアルどおり）
```

さらに、8ビット/16ビット構造体がレジスタ渡しされる場合、値はレジスタの**上位ビット側に詰めて格納**される（リトルエンディアンでは通常不要な配置で、MIPSビッグエンディアンのコード起源と推測される）。

**LLVM実装方針:** 初期実装ではマニュアル記載どおり（全構造体スタック渡し）で実装する。小構造体のレジスタ渡し最適化はSDK互換テストで問題が出た場合に対応する。

---

## 4. 命令エンコーディング

### 4.1 命令クラス

全命令は16ビット固定長。上位3ビット（bit[15:13]）がクラスを示す。

| クラス | bit[15:13] | 主な命令 |
|---|---|---|
| Class 0 | 000 | nop, slp, halt, ret, reti, retd, int, brk, ext, pushn, popn, jpr, call, jp |
| Class 1 | 001 | ld.b, ld.ub, ld.h, ld.uh, ld.w（レジスタ間接） |
| Class 2 | 010 | add, sub, cmp, and, or, xor, not, srl, sll, sra, sla, rr, rl |
| Class 3 | 011 | add/sub/cmp with immediate |
| Class 4 | 100 | ld.b, ld.h, ld.w（SP相対）, ld immediate |
| Class 5 | 101 | jr（条件/無条件相対ジャンプ）, call相対 |
| Class 6 | 110 | ext（即値拡張命令） |
| Class 7 | 111 | MAC/乗算/除算、コプロセッサ命令 |

### 4.2 ext命令（即値拡張）— 最重要機構

16ビット命令の即値フィールドは通常6〜13ビットしかないが、ext命令を前置することで拡張する。

- ext 1個: 即値を13ビット拡張
- ext 2個: さらに13ビット拡張（合計最大32ビットの即値が表現可能）

**ハードウェア保証**: ext命令と拡張対象命令の間は、リセットとアドレス不整例外を除くすべてのトラップがハードウェアによりマスクされる。割り込みは発生しない。

**ext非対応命令**: シフト・ローテート命令（srl, sll, sra, sla, rr, rl）は ext による即値拡張ができない。即値フィールドは imm4 だが、シフト量のマッピングが特殊で最大8ビットまで（0000=0, 0001=1, ... 0111=7, 1xxx=8）。9ビット以上のシフトは複数命令に分割する必要がある（§10.6.4 参照）。LLVMバックエンドの ISelLowering でシフト量 > 8 のカスタム lowering が必要。

### 4.3 ディレイドブランチ

分岐命令（jp, jr, call, ret, reti, retd）の直後1命令がディレイスロット。

**ディレイスロットに置ける命令の制約:**
- 1サイクル命令であること
- メモリアクセス命令は不可
- ext拡張付き命令は不可
- 分岐命令は不可

**ハードウェアバグ: `jp.d %rb` は使用禁止。** レジスタ間接のディレイド分岐 `jp.d %rb` には2件のハードウェアバグがある。(A) 直前にメモリアクセス命令があるとディレイスロットの命令が実行されない。(B) DMA転送と重なった場合にも同じ誤動作が発生し、プログラムで回避不可能（サウンド再生中等のDMA常時動作時に顕在化）。`call.d %rb` と `ret.d` には同問題なし。バックエンドは `jp.d %rb` を**絶対に生成してはならない**。代わりに `jp %rb`（非ディレイド）を使用すること。

実装ではMIPSバックエンドのDelaySlotFillerパスを参考にする。候補がなければnopを挿入。

---

## 5. 設計上の重要決定

### 5.1 ext命令の生成方針

**純正ツールチェインとの違い:**

```
[純正] gcc33 → .ps(拡張命令) → ext33(展開+最適化) → as33 → .o → lk33
[LLVM] Clang → SelectionDAG → MachineInstr → MCInst(+relaxation) → ELF
```

純正ツールチェインではテキストベースで各段階がソースを書き換えるが、LLVMでは内部表現を一貫保持する。ext33/pp33/as33の機能はMCレイヤーに統合される。

**コード生成段階（SelectionDAG → MachineInstr）:**
- 最大サイズ（悲観的）で命令を出力する
- グローバル変数アクセス → ext+ext+ld.w の擬似命令
- 関数呼び出し → ext+ext+call の擬似命令
- GP最適化有効時 → ext+ext+ld.w [%r8] の擬似命令

**MCリラクゼーション（S1C33AsmBackend）:**
- `relaxInstruction()`: ext+ext+op → ext+op → op への縮小判定
- `fixupNeedsRelaxation()`: 距離/値でext不要か判定
- 反復収束アルゴリズム（ext33の2パス最適化に相当）

### 5.2 R9を使わない設計

純正ext33はR9をアドレス計算用スクラッチとして暗黙に使用する（割り込み安全性の問題あり）。

**LLVMでの方針**: R9を特別扱いせず、Reserved指定とする。アドレスのマテリアライズには通常のレジスタアロケータが管理するレジスタを使う。

```asm
; 純正ext33（R9を暗黙使用、割り込み時に危険）
ext symbol@h
ext symbol@m
ld.w %r9, symbol@l
ld.w %r1, [%r9]

; LLVM（レジスタアロケータがR4を選択、安全）
ext symbol@h
ext symbol@m
ld.w %r4, symbol@l
ld.w %r1, [%r4]
```

### 5.3 R8 の役割（カーネルテーブルベースポインタ）

**P/ECE カーネルの呼び出し規約**: P/ECE カーネルはアプリケーションコールバック（pceAppInit等）を呼び出す前に R8 = 0x0 をセットする。pceapi スタブはこの慣習を利用して:

```asm
ext  N          ; N = バイトオフセット（例: pceLCDTrans = 0x68）
ld.w %r9, [%r8] ; %r9 = [0x0 + N] = カーネルジャンプテーブル[N]
jp   %r9        ; カーネル関数へジャンプ
```

アドレス 0x0 からのワードアレイがカーネルのジャンプテーブル（関数ポインタの配列）として機能する。

**ユーザアプリでの扱い**: R8 を Reserved として登録し、レジスタアロケータが決して割り付けないようにする。ユーザコンパイルコードは R8 を変更してはならない。

**MIPSスタイルの GP 最適化との違い**: ユーザアプリ向けに R8 をユーザの `.sdata` セクション先頭に向ける「GP最適化」は**実装しない**。R8 は常にカーネルが設定する 0x0 であり、ユーザのグローバルデータポインタとしては使わない。

**将来のカーネルコンパイル対応**: カーネル自体を Clang でコンパイルする場合、カーネルのブートコード（リセットハンドラ）がアセンブリで `ld.w %r8, 0` を明示的に実行する必要がある。R8 は Reserved なので通常の C コードには影響しない。

### 5.4 割り込みハンドラ

`__attribute__((interrupt_handler))`を実装:
- プロローグで`pushn %r15`（全レジスタ退避）を生成
- エピローグで`popn %r15` + `reti`を生成
- 通常関数のprologue/epilogueとは別パスで処理

純正ツールのP/ECEコードでは `INT_BEGIN` / `INT_END` マクロ（`pushn %r15` / `popn %r15; reti`）が使われていた。

### 5.5 ステップ除算

S1C33000にはハードウェア除算命令がない。div0s/div1/div2s/div3sの命令列で実現する。

- 符号なし32ビット除算: `div0u` + `div1`×32 + `div2s` + `div3s`
- 符号付き32ビット除算: `div0s` + `div1`×32 + `div2s` + `div3s`

この命令列は35命令に及ぶため、各所でインライン展開するとコードサイズが爆発する。P/ECE SDKではidiv.lib（`__divsi3`, `__modsi3`, `__udivsi3`, `__umodsi3`）としてライブラリ化されていた。

LLVMバックエンドでは以下の選択肢がある:
- **ライブラリコール（デフォルト推奨）**: SDIVノード→`__divsi3`呼び出し。コードサイズ優先。
- **インライン展開（-Ofast等）**: SDIVノード→div命令列の直接展開。速度優先。
- SDKのidiv.libを使用する場合は関数名・引数規約を一致させること。compiler-rtで自前実装も可。

S1C33209はオプション乗算器を内蔵するため、mlt.h/mltu.h/mlt.w/mltu.w/MAC命令もサポートする。SubtargetFeatureで有無を切り替え。

### 5.6 memcpy/memsetのlowering

gcc33はmemcpy()をポインタ型に基づいてワード単位のロード/ストアにインライン展開するが、実引数が非アラインドの場合にアドレス不整例外が発生するバグがあった。

LLVMバックエンドでは:
- memcpy/memsetのインライン展開時、アライメント情報を正しく伝搬すること
- アライメントが保証されない場合はバイト単位のロード/ストアを生成すること
- S1C33000はワード/ハーフワードアクセスにアライメントが必要（不整アクセスは例外）

### 5.7 `__attribute__((packed))` の正しいサポート

gcc33とas33の間でpackedの解釈が整合しておらず、P/ECE開発では事実上使用不可だった。LLVMバックエンドではpacked構造体のフィールドアクセスに対し、アライメントが満たされない場合にバイト単位のロード/ストアを正しく生成すること。

### 5.8 Cライブラリに関する注記

エプソン純正Cライブラリ（lib.lib）にはsin()、strtok()、strtod()、pow()、ispunct()等に既知のバグがある（特にsin()はPSRのVフラグ残留で結果が反転するバグ）。ただしP/ECE SDKの他のコンポーネントがlib.libの関数に依存している可能性があるため、完全な除去は慎重に行う必要がある。バグのある関数のみを個別に差し替え（例: 自前のsin()をリンク順で優先させる）するのが現実的。

### 5.9 PSRフラグの生存区間管理

S1C33000のほぼ全てのALU命令（add, sub, cmp, and, or, xor, not, srl, sll, sra, rr, rl、さらにld即値含む）がPSRフラグ（N, Z, V, C）を暗黙に更新する。

これは gcc33 バグ#2/#3（0比較cmp欠落）の根本原因でもある。gcc33はフラグ生存区間の追跡が不十分で、0比較のcmpを「不要」と誤判断して削除した。

LLVMバックエンドでは:
- TableGenの命令記述で、フラグを更新する全命令に `Defs = [PSR]` を付与
- フラグを消費する命令（条件分岐jr系）には `Uses = [PSR]` を付与
- LLVMの暗黙レジスタモデルにより、命令スケジューラがフラグの定義-使用チェーンを正しく追跡する

### 5.10 64ビット整数ランタイム

S1C33000は32ビットアーキテクチャのため、64ビット整数演算（long long）はライブラリ関数で実現する。

gcc33互換の引数規約: 64ビット値はレジスタペアで渡す。通常ABI（R12〜R15引数）に従い:
- 第1引数(64bit) = R12(下位) + R13(上位)
- 第2引数(64bit) = R14(下位) + R15(上位)

必要なランタイム関数: `__adddi3`, `__subdi3`, `__muldi3`, `__divdi3`, `__moddi3`, `__udivdi3`, `__umoddi3`, `__cmpdi2`, `__ucmpdi2`, `__fixsfdi`, `__fixunssfdi`, `__floatdisf`

エプソン純正では `__fixsfdi`, `__fixunssfdi`, `__floatdisf`（float⇔64bit変換）と `__cmpdi2` が未実装だった。compiler-rtで完全に実装する必要がある。

---

## 6. エラッタ・既知のハードウェア制約

純正ツールチェイン（gcc33/as33/ext33/pp33）および S1C33209 CPU で確認されている既知の問題のうち、LLVMバックエンド実装に影響するものを記載する。詳細は `docs/errata.md` を参照。

### 6.1 LLVMバックエンドで対処が必要なもの

| エラッタ/制約 | 影響 | 対処 |
|---|---|---|
| `jp.d %rb` ハードウェアバグ | DMA動作中にディレイスロットが実行されない | `jp.d %rb` を生成禁止。`jp %rb` を使用。§4.3に詳述 |
| 非アラインドアクセス例外 | memcpyインライン展開でワードアクセスが不正 | アライメント不明時はバイト単位アクセスを生成 |
| packed構造体 | gcc33/as33間の解釈不整合 | LLVMの標準的なpacked lowering（バイトアクセス）で正しく動作 |
| PSRフラグ暗黙更新 | ほぼ全ALU命令がフラグ更新 | TableGenで全命令に`Defs=[PSR]`を付与。§5.9に詳述 |
| 即値範囲チェック | as33は範囲外でも無警告 | MC層で命令ごとの正確な即値範囲を検証、範囲外は自動ext化 |
| 64bit整数ランタイム不完全 | エプソン純正で一部未実装 | compiler-rtで完全実装が必要。§5.10に詳述 |
| 除算ランタイム | 35命令のインライン展開はサイズ爆発 | デフォルトはライブラリコール。§5.5に詳述 |

### 6.2 gcc33固有（LLVMでは発生しない）

以下はgcc33の最適化バグであり、LLVMの最適化パスには存在しない問題。参考情報として記録する。

- **0比較の条件式ネスト**: gcc33の-O1/-O2でcmp命令が欠落する。abs()のインライン展開でも同根のバグが発生。
- **forループのポインタ最適化**: 符号付き→符号なし比較変換の誤り。
- **switch/caseの巨大ジャンプテーブル**: gcc33は常にテーブル展開を選択。LLVMは独自のヒューリスティクスで判断。

### 6.3 純正ツール固有（LLVMでは無関係）

- as33の即値範囲チェック不備（負の即値で不正コード生成）
- ext33の `.ascii` 末尾バックスラッシュ問題
- pp33の負数マクロ展開問題
- **srf33フォーマットのマニュアル記載ミス**: エクスターン情報の`e_scnndx`が4Byteと記載されているが実際は2Byte。srf2elf実装時に要注意（errata.md参照）

---

## 7. ELFリロケーション型

純正as33のシンボルマスクに対応するリロケーション型を定義する。

### 7.1 絶対アドレス分割

| as33記法 | リロケーション型 | ビットフィールド |
|---|---|---|
| `@h` | `R_S1C33_ABS_H` | bits 31:19 |
| `@m` | `R_S1C33_ABS_M` | bits 18:6 |
| `@l` | `R_S1C33_ABS_L` | bits 5:0 |

### 7.2 相対アドレス分割（分岐用）

| as33記法 | リロケーション型 | ビットフィールド |
|---|---|---|
| `@rh` | `R_S1C33_REL_H` | bits 31:22 (<<3) |
| `@rm` | `R_S1C33_REL_M` | bits 21:9 |
| `@rl` | `R_S1C33_REL_L` | bits 8:1 |

**PC基準**: ext+ext+call/jp のペアでは、3つのリロケーション（REL_H, REL_M, REL_L）は全て分岐命令自身（ext ではなく call/jp）のアドレスを PC 基準とする。各リロケーションが自分自身のアドレスを基準にするのではない。これを間違えるとビットフィールド間のキャリー伝播で分岐先がずれる。

R_S1C33_REL21（ext+call/jp の21ビット版）も同じ基準。

### 7.3 GP相対アドレス分割

| as33記法 | リロケーション型 | ビットフィールド |
|---|---|---|
| `@ah` | `R_S1C33_GP_H` | bits 25:13 |
| `@al` | `R_S1C33_GP_L` | bits 12:0 |

---

## 8. 実装フェーズ計画

### Phase 1: 基盤（TableGen + 基本命令生成） — **完了**

- `S1C33.td` — ターゲット定義
- `S1C33RegisterInfo.td` — レジスタ定義（R0〜R15, SP, PSR, ALR, AHR）
- `S1C33InstrInfo.td` — 命令定義（Class 0〜7のエンコーディング）
- `S1C33InstrFormats.td` — 命令フォーマットクラス
- `S1C33Subtarget.td` — サブターゲット特性（乗算器有無等）
- `S1C33TargetMachine.cpp` — ターゲットマシン登録
- `S1C33AsmPrinter.cpp` — アセンブリ出力

### Phase 2: 呼び出し規約 + フレーム生成 — **完了**

- `S1C33CallingConv.td` — 呼び出し規約定義（double スプリット禁止含む）
- `S1C33FrameLowering.cpp` — スタックフレーム生成
- `S1C33ISelLowering.cpp` — SelectionDAGのlowering（byval/sret対応済み）
- `S1C33RegisterInfo.cpp` — レジスタ情報（eliminateFrameIndex でバイト/ハーフワードSP相対対応済み）

### Phase 3: MCレイヤー（アセンブラ + リラクゼーション） — **完了**

- `S1C33AsmBackend.cpp` — リラクゼーション実装（FK_Data_4 addend修正済み）
- `S1C33MCCodeEmitter.cpp` — 命令エンコーディング
- `S1C33ELFObjectWriter.cpp` — ELFリロケーション処理（PC基準修正済み）
- `S1C33FixupKinds.h` — カスタムフィクスアップ型定義
- `S1C33MCTargetDesc.cpp` — MCターゲット記述

### Phase 4: 最適化 + ディレイスロット — **完了**

- `S1C33DelaySlotFiller.cpp` — ディレイスロット充填パス（jp.d/ret.d/call.d のバンドル化対応）
- `S1C33AsmPrinter.cpp` — バンドル内サブ命令（ディレイスロット）の正しいエミット対応
  - `MIBundleBuilder` が付与する `InsideBundle` フラグにより `emitFunctionBody()` がスキップしていたバグを修正
  - MIPS と同様の `do { emit(*I) } while (++I != E && I->isInsideBundle())` パターンに変更
- 割り込みハンドラ属性の実装
- ステップ除算のカスタムlowering
- MAC/乗算命令のサポート（SubtargetFeature）
- シフト量>8の複数命令分割（ISelDAGToDAG）
- AND/OR/XOR/SUB の大即値対応（`_ri32` 疑似命令 + `expandPostRAPseudo` 展開）
- デクリメントイディオム（`add %r, -1` → `sub %r, 1`）パターン追加
- 分岐最適化（`analyzeBranch` / `insertBranch` / `removeBranch` / `reverseBranchCondition` 実装）
- ユーザアプリ向け GP 最適化は実装しない（R8 = カーネルテーブルベース 0x0 として Reserved のみで対応）

### Phase 5: SRF→ELF変換 + ランタイム + リンカスクリプト — **完了**

- `srf2elf` — SRFオブジェクト(.o)とlib33アーカイブ(.lib)の両方に対応
- lld に S1C33 リロケーションサポート追加（REL_H/M/L PC基準修正済み）
- piece.ld — lk33互換ブロックシンボル定義（DEFAULT_CODE/DATA/BSS）
- cstart.c からソースビルドの crt0.o（pceAPPHEAD addend修正済み）
- sysroot 構築（ヘッダ + 変換済みライブラリ + リンカスクリプト）
- Clang ToolChain（BareMetal ベース、デフォルトリンク順序設定済み）

### Phase 6: P/ECE SDK統合テスト — **完了**

- hello.c のコンパイル→リンク→pex生成: **完了**（エミュレータでアプリ認識確認）
- fpkplay.c のコンパイル: **完了**（LowerCall / 間接呼び出し修正後）
- decode.c のコンパイル: **完了**（SP相対ld.b/シフト分割修正後）
- mus.c のコンパイル: **完了**（間接呼び出し修正後）
- ppack ELF対応版: **完了**
- 正確性検証（PSR/即値/pushn/構造体/double）: **全5項目完了**
- mini_nocrt（Cランタイムなし最小構成）エミュレータ動作確認: **完了**（2026-03）
  - ST+SL長押しでメニューへの戻り確認（カーネルへの制御移行正常）
  - AsmPrinter バンドルエミットバグ修正後の最新バイナリで確認済み
- 実機での動作確認: **完了**（2026-03）
  - mini_nocrt/mini1.pex・mini2.pex: vbuff塗りつぶし・ST+SLメニュー遷移すべて正常
  - minimal/mini_l.pex: sysroot crt0.o + libpceapi.a リンクで正常動作
  - hello/hello_l.pex: EPSON SDK CRT 関数（printf等）・システムメニュー呼び出し・メニュー復帰すべて正常
- crt0.o + libpceapi.a による hello/ 動作確認: **完了**（2026-03）

---

## 9. LLVMソースツリー上の配置

```
llvm/lib/Target/S1C33/
├── S1C33.td
├── S1C33.h
├── S1C33RegisterInfo.td
├── S1C33InstrInfo.td
├── S1C33InstrFormats.td
├── S1C33CallingConv.td
├── S1C33Subtarget.td
├── S1C33Subtarget.h / .cpp
├── S1C33TargetMachine.h / .cpp
├── S1C33ISelLowering.h / .cpp
├── S1C33ISelDAGToDAG.h / .cpp
├── S1C33InstrInfo.h / .cpp
├── S1C33RegisterInfo.h / .cpp
├── S1C33FrameLowering.h / .cpp
├── S1C33AsmPrinter.cpp
├── S1C33DelaySlotFiller.cpp
├── S1C33TargetObjectFile.h / .cpp
├── MCTargetDesc/
│   ├── S1C33MCTargetDesc.h / .cpp
│   ├── S1C33MCCodeEmitter.cpp
│   ├── S1C33AsmBackend.cpp
│   ├── S1C33ELFObjectWriter.cpp
│   ├── S1C33FixupKinds.h
│   ├── S1C33InstPrinter.h / .cpp
│   └── S1C33MCAsmInfo.h / .cpp
├── TargetInfo/
│   └── S1C33TargetInfo.h / .cpp
└── CMakeLists.txt
```

---

## 10. 参考にすべき既存バックエンド

| 参考対象 | 参考理由 |
|---|---|
| **RISC-V** | MCリラクゼーション、ELFリロケーション分割の実装 |
| **AVR** | 16ビット命令のエンコーディング、小規模レジスタセット |
| **MIPS** | ディレイドブランチ処理、GPレジスタの扱い |
| **Lanai** | 小規模で読みやすいバックエンド構成の参考 |
| **ARM (Thumb)** | 16ビット/32ビット混在命令の処理（構造的に参考） |

---

## 11. 一次資料一覧

以下のファイルを `docs/` ディレクトリに配置すること:

| ファイル名 | 内容 | 主な参照セクション |
|---|---|---|
| `S1C33000_コアCPUマニュアル_2001-03.pdf` | CPU仕様、命令セット、エンコーディング | 第3章(レジスタ), 第4章(命令コード), 第5章(命令リファレンス), 第7章(割り込み) |
| `S1C33_Family_Cコンパイラパッケージ.pdf` | ABI、ext33/pp33/as33仕様 | 6.5節(呼び出し規約), 第10章(ext33), 第8章(as33), 第9章(pp33) |
| `S1C33209_201_222テクニカルマニュアル_PRODUCT_FUNCTION.pdf` | S1C33209メモリマップ、周辺回路 | メモリマップ（リンカスクリプト用）、乗算器仕様 |
| `S1C33_family_スタンダードコア用アプリケーションノート.pdf` | 割り込みハンドラ、ブート処理 | 割り込み処理パターン、INT_BEGIN/INT_ENDマクロ |
| `errata.md` | CPU・コンパイラ・ライブラリのエラッタ集 | §6のエラッタ情報の詳細版。piece-lab記事群からの調査結果 |
| `lib33_format.md` | lib33アーカイブ(.lib)のバイナリフォーマット | srf2elfの.lib対応時。マニュアルA-2とは異なる実フォーマット |

---

## 12. 用語対応表

LLVM実装時に混同しやすい用語の対応:

| エプソン純正用語 | LLVM対応概念 |
|---|---|
| 拡張命令（xcall, xld.w等） | 擬似命令（MachineInstrレベル） |
| ext33の命令展開 | MCリラクゼーション |
| ext33の2パスシンボル最適化 | MCAssembler反復収束 |
| as33のシンボルマスク（@h/@m/@l） | ELFリロケーション型 |
| pp33のビットフィールド演算子（^H/^M/^L） | 不要（コンパイラが直接生成） |
| lk33のセクション配置 | lldリンカスクリプト |
| SRF形式 | ELF形式（llvm-objcopyでバイナリ化） |
| .def形式（デバッグ情報） | DWARF |
| GP（グローバルポインタ） | R8 Reserved（カーネル規約 R8=0x0 を尊重） |
