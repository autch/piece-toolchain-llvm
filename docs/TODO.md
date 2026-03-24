# 未解決・後回し項目一覧

Phase 1〜6 の実装および正確性検証を経た残存項目。

## 解決済み（記録）

以下の項目は検証・修正完了。

- **PSRフラグの Defs/Uses**: 全ALU命令を確認済み。ADDSP_i/SUBSP_i に Defs=[PSR] 追加。ld系命令はPSR非更新で正しい。
- **即値フィールドの符号付き/符号なし区別**: 全命令について確認済み、問題なし。
- **callee-saved レジスタの pushn/popn**: R0単独、R0-R3全退避、割り込みハンドラの全境界ケースで正常動作確認。
- **構造体の引数渡し/戻り値のABI互換性**: byval/sret のクラッシュ修正、post-call FrameIndex展開修正。
- **double型の引数渡し**: スプリット禁止ルール実装、CCCustom戻り値修正、fixed frame indexオフセット修正。
- **pceAPPHEAD の R_S1C33_32 暗黙addend**: SHT_REL の FK_Data_4 で applyFixup がデータ書き込み前に return していた問題を修正。
- **srf2elf.py の r_symoff 無視バグ**: SHT_REL → SHT_RELA に切り替え、r_symoff を explicit r_addend として出力するよう修正。math.lib/acos 等の `NAN + 4` 参照や muslib/seq の負オフセット参照が正しくリンクされる。詳細: `docs/investigation-2026-03-srf2elf-and-objdump.md`
- **llvm-objdump の ELF 自動認識 (TODO #2)**: ELFObjectFile.h に EM_SE_C33 → `"elf32-s1c33"` / `Triple::s1c33` のマッピングを追加。`--triple` なしで逆アセ可能に。
- **PC相対リロケーションの基準アドレス**: REL_H/M/L 全てが call 命令自身のアドレス基準に統一。
- **SP相対 ld.b/ld.ub/ld.h/ld.uh の命令選択**: eliminateFrameIndex でオペコード差し替えを追加。
- **シフト量 > 8 の不正な1命令生成**: ISelDAGToDAG で MachineNode として複数 shift に分割。ext は使えない。
- **memcpy/memset の libcall シンボル名 null**: ライブラリコール名の登録漏れ修正。
- **GlobalAddress/ExternalSymbol callee の判別**: LowerCall で Wrapper 誤適用と間接呼び出し対応。
- **AsmPrinter バンドルエミットバグ（TODO #コード生成）**: `MIBundleBuilder` で `InsideBundle` フラグが付いたディレイスロット命令（NOP等）を `emitFunctionBody()` がスキップしていた。MIPS と同様の `do { emit(*I) } while (++I != E && I->isInsideBundle())` パターンに変更。13本の lit テスト全通過確認。
- **AND/OR/XOR/SUB 大即値対応 (TODO #3)**: `_ri32` 疑似命令 + `expandPostRAPseudo` でext展開。即値範囲に応じて noext/ext/ext+ext の3形式。
- **分岐最適化 (TODO #4)**: `analyzeBranch` / `insertBranch` / `removeBranch` / `reverseBranchCondition` を実装。ダイヤモンドパターンが条件反転+フォールスルーに最適化される。
- **jp.d imm ディレイドブランチ (TODO #6)**: DelaySlotFiller が `jp imm` をディレイド版 `jp.d imm` に変換。ハードウェアバグ対象の `jp.d %rb` は引き続き禁止。
- **デクリメントイディオム (TODO #7)**: `add %r, -1` → `sub %r, 1` のパターン追加。
- **不要な ext 0 の除去 (TODO #8)**: MCリラクゼーション + ISelDAGToDAG のフィックスアップで冗長 ext 0 を削除。
- **mini_nocrt エミュレータ動作確認 (2026-03)**: Cランタイムなし最小構成で ST+SL 長押し→メニュー遷移を確認。カーネルコール（pceapi スタブ経由）正常動作を確認。

## アセンブリ表示の不整合

### 1. sub %sp のオペランド表示（解決済み・対応不要）
~~CPUマニュアルの記法は `sub %sp, 17`（17×4=68バイト）だが、
バックエンドは `sub %sp, 68`（バイト数直接）で表示している。~~

**調査結果**: `sub %sp, 68` のバイナリエンコーディングは `0x8444` であり、
imm10 フィールドには 68 (byte count) が直接格納される。ハードウェアは
imm10 をそのまま SP に加減算する（×4 乗算はない）。
as33 アセンブラがワード数表記を受け付けて×4して格納する「アセンブラ側の
慣習」であり、ハードウェア側の話ではない。
現在の `sub %sp, 68` 表示はエンコーディングとセマンティクスが一致しており
正しい。表示をワード数に変えると AsmParser 側も変更が必要となり、
round-trip が壊れるため対応不要。

## コード生成品質の改善

### 3. 即値ALU命令の活用不足 — **解決済み**

~~and/or/xor/sub の即値版（ext 付き含む）を使わず、即値をレジスタにロードしてからレジスタ間演算している。~~

`AND_ri32` / `OR_ri32` / `XOR_ri32` / `SUB_ri32` 疑似命令を追加し、`expandPostRAPseudo` で3形式（noext/ext/ext+ext）に展開。即値範囲（isInt<6>/isInt<19>/その他）に応じて自動選択。

### 4. 分岐構造の最適化不足 — **解決済み**

~~全ての条件分岐後に無条件 jp を挿入する「ダイヤモンドパターン」。~~

`analyzeBranch` / `insertBranch` / `removeBranch` / `reverseBranchCondition` を実装。BranchFolder パスが条件反転+フォールスルーに変換するようになった。

### 5. GP最適化（対応方針確定）

**ユーザアプリ向け GP 最適化は実装しない。** R8 はカーネルが 0x0 にセットする「カーネルジャンプテーブルベースポインタ」として使用される。pceapi スタブは `ext N / ld.w %r9, [%r8]` でカーネル関数ポインタを取得する。

- R8 は Reserved（レジスタアロケータが割り付けない）として実装済み ✓
- ユーザコンパイルコードが R8 を変更しないことはこれで保証される ✓
- MIPSスタイルの「R8 = ユーザ .sdata ポインタ + [R8+offset] で GlobalAddress を解決する」最適化は**不要かつ実装しない**
- **将来のカーネルコンパイル対応**: カーネルのブートコード（リセットハンドラ）がアセンブリで `ld.w %r8, 0`（または `ld.w %r8, kernel_table_addr`）を明示的に実行する必要がある。通常の C コードのコンパイルには影響しない。

### 6. jp.d imm（即値オフセット版ディレイドブランチ） — **解決済み**

~~現在のディレイスロットフィラーは `jp.d` 自体を生成しない安全側の設計。~~

DelaySlotFiller が `jp imm` を `jp.d imm` に変換するよう実装。`jp.d %rb`（ハードウェアバグ対象）は引き続き禁止。

### 7. デクリメントのイディオム — **解決済み**

~~`ld.w %r5, -1 / add %r4, %r5` ではなく `sub %r4, 1` を生成すべき。~~

TableGen パターン追加により `add %r, -1` → `sub %r, 1` が正しく生成されるようになった。

### 8. 不要な ext 0 の頻出 — **解決済み**

~~SP相対アクセスでオフセットが imm6 に収まるのに ext 0 が付く。~~

MCリラクゼーション + ISelDAGToDAG のフィックスアップで冗長 ext 0 を除去。

## 未実装の機能

### 9. データアクセス命令のリラクゼーションの網羅性 — **解決済み**

~~Phase 3 で分岐とデータアクセスの基本パターンは実装したが、
全てのリラクゼーションパターンが網羅されているかは未検証。
ext33 は「分岐」「データアクセス」「GP有無」の3軸でパターンを持つ。~~

**検証結果（2026-03）**: 3軸すべてカバー済み。
- **分岐**: `branch-relaxation.ll` でテスト済み（近傍2バイト/遠方ext+4バイト）
- **データアクセス**: `global-relocs.ll` で EXT2 パス（非絶対シンボル → ext+ext+ld.w）をテスト済み。  
  EXT0（絶対値が6ビット以内）・EXT1（絶対値が19ビット以内）パスのコードも正しく実装されており、
  `asm-data-access.s` で全アドレッシングモードの encoding をテスト済み。
  なお、通常のリンク済みバイナリでは非絶対シンボルが支配的なため、EXT2 が常用される。
- **GP最適化**: 「GP最適化#5」参照。ユーザアプリには不要、設計上実装しない。

### 10. 全命令の AsmParser 対応 — **解決済み**

~~手書きアセンブリサポートのため全命令・全アドレッシングモードの
AsmParser 対応を進行中。musfast.s で必要な命令は対応済み。
残存: swap、一部の特殊レジスタ転送、btst/bclr/bset/bnot の [%rb] 構文。~~

**検証結果（2026-03）**: すべて実装・動作確認済み。
- `swap %rd, %rs` — Class 4 命令として定義済み、AsmMatcher で正常認識
- `btst/bclr/bset/bnot [%rb], imm3` — `[%rb]` をメモリアドレスとする構文が正しく動作
- SP/PSR/ALR/AHR の読み書き (`ld.w %rd, %sp` 等) — 全4レジスタ実装済み
- 注: PC 直接転送 (`ld.w %rd, %pc`) はS1C33 CPUマニュアルのClass 5特殊レジスタ表  
  (sp=0, psr=1, alr=2, ahr=3) に PC エントリがないため未定義。PC への分岐は `jp %rb` を使う。

`asm-special-regs.s` で全項目のエンコーディングをテスト済み。

## ランタイム・ライブラリ

### 11. compiler-rt の S1C33 向けビルド（任意）
fp.lib/idiv.lib の代替。SDKライブラリを srf2elf で変換して使えるため
必須ではないが、SDKへの依存を減らしたい場合に必要。

### 12. newlib の S1C33 向けポーティング（任意）
lib.lib の代替。lib.lib にはエラッタがあるため、
newlib で置き換えれば既知バグを回避できる。

### 13. 64ビット整数ランタイムの完全性
DESIGN_SPEC.md §5.10 に規定。__fixsfdi, __fixunssfdi, __floatdisf,
__cmpdi2 がエプソン純正では未実装だった。compiler-rt で補う必要あり。
現状 SDKの fp.lib/idiv.lib を使っている限りは問題にならないが、
float⇔long long 変換を使うコードでリンクエラーになる。

## ツール・周辺機能

### 14. アセンブリソーストランスレータ asm33conv（§10.6 準拠の拡張命令展開）

修正済みのパターン:
- xld（ロード/ストア）: ext+ext+ld.w [%rb] 形式に展開
- xshift（シフト/ローテート）: 複数の基本シフト命令に分割（ext不使用）

未実装のパターン（§10.6 準拠で必要）:
- §10.6.1 算術演算（xadd/xsub）: 6展開形式、3オペランド、SP操作でR9使用
- §10.6.2 比較（xcmp）: 3展開形式、SP比較でR9使用
- §10.6.3 論理演算（xand/xoor/xxor/xnot）: 3展開形式
- §10.6.5 SP相対転送: 4展開形式、SP→R9経由
- §10.6.6 メモリ転送: 8展開形式、R9スクラッチ、GP分岐
- §10.6.7 即値ロード: 2展開形式、シンボル±即値
- §10.6.8 ビット操作: 5展開形式、R9でアドレス構築
- §10.6.9 分岐: 2展開形式、1/2/3命令展開

設計方針（確定）:
- AsmParser は基本命令のみ受け付ける（拡張命令サポート削除済み）
- asm33conv がスタンドアロンツールとして §10.6 を忠実に実装
- インラインアセンブリでは拡張命令は使用不可（制約事項）

### 15. pceapi のソースビルド体制

完了済み:
- cstart.c からの crt0.o ビルド
- defnotify.c からの crti.o ビルド

未完了:
- gen_pceapi.py によるカーネルAPIスタブの自動生成（vector.h から）
- ユーティリティソース（memset.s, memcpy.s, stacklen.s, iodef.s,
  version_check.c, def_vbuff.c）のビルドと libpceapi.a への統合
- 上記が完了すれば srf2elf 変換の libpceapi.a への依存を完全に排除可能

### 16. lib33 フォーマットの未確定事項
docs/lib33_format.md に記載の通り:
- unknown_1 (ヘッダ+0x08、4バイト) の正体
- unknown_2 (ヘッダ+0x0C、1バイト) の正体（234=シンボル数？）
- 文字列テーブルの正確な終端位置
- fp.lib, idiv.lib 等の他 .lib での構造検証

### 17. SDK ヘッダの Clang 互換性
fpkplay.c で確認された問題:
- Shift-JIS 文字列リテラルの警告 (-Winvalid-source-encoding)
- memset 等のプロトタイプ未宣言警告 (-Wdeprecated-non-prototype)
- -Wpointer-sign（unsigned char* ↔ char* 混在）
- -Wunsequenced（`p += *p++ << 1` のような未定義動作）

現状は -W オプションで抑制しているが、SDKヘッダの互換ラッパーを
作る選択肢もある。

### 18. ppack の decode モード
ppack.c の decode_pack() は空実装。ELF対応版でも未実装のまま。
.pex → ELF への逆変換が必要になる場面は限られるが、
デバッグ時にあると便利。
