# lib33 アーカイブフォーマット仕様

lib33.exe が生成する SRF ライブラリファイル (.lib) のバイナリフォーマット。

フォーマットの公式資料は存在しない。本ドキュメントは P/ECE SDK の lib.lib バイナリの直接観察と、
piece-lab 作者によるリバースエンジニアリング資料 (`docs/lib33_format-20060623.txt`)、および
idiv.lib / fp.lib / lib.lib の 3 ファイルを用いたバイナリ検証に基づいて記述する。

## 全体構造

```
+---------------------------------------+  0x00
| ファイルヘッダ (20 バイト、SYMDEF の場合) |  magic(9) + symtab_size(4) + sig(7)
+---------------------------------------+  0x14
| シンボルテーブル (symtab_size バイト)    |
|   シンボル情報テーブルサイズ (4)         |
|   シンボル情報テーブル (N × 8)           |
|   シンボル名称テーブルサイズ (4)         |
|   シンボル名称テーブル (NUL 終端文字列)   |
+---------------------------------------+  0x14 + symtab_size
| モジュールエントリ #1                   |  (srf_size + name_len + filename + SRF data)
+---------------------------------------+
| モジュールエントリ #2                   |
+---------------------------------------+
| ...                                   |
+---------------------------------------+  EOF
```

## ファイルヘッダ

| オフセット | サイズ | フィールド | 内容 |
|-----------|--------|-----------|------|
| 0x00 | 9 | magic | `"!<lib33>"` + NUL (0x21 0x3C 0x6C 0x69 0x62 0x33 0x33 0x3E 0x00) |
| 0x09 | 4 | symtab_size | シンボルテーブル全体のバイト数（ヘッダ自身とシグネチャは含まない） |
| 0x0D | 1 | name_len | セクション名の長さ = 6 |
| 0x0E | 6 | name | セクション名 = `"SYMDEF"` |

ヘッダ末尾は `0x0E + name_len = 0x14`。シンボルテーブルはここから始まる。

すべての多バイトフィールドはビッグエンディアン。

## シンボルテーブル (0x14 から symtab_size バイト)

シンボルテーブルは以下の 4 つの要素で構成される。

### シンボル情報テーブルサイズ (4 バイト)

```
syminfo_size = N × 8  (N = シンボル数)
```

### シンボル情報テーブル (syminfo_size バイト、各エントリ 8 バイト)

| オフセット | サイズ | フィールド | 内容 |
|-----------|--------|-----------|------|
| 0 | 4 | module_pos | このシンボルが属するモジュールの、ファイル先頭からの絶対位置 |
| 4 | 4 | name_off | シンボル名称テーブル先頭からのバイトオフセット |

一つの .o ファイルが複数のグローバルシンボルをエクスポートする場合、
同一の `module_pos` が複数エントリに出現する。

### シンボル名称テーブルサイズ (4 バイト)

シンボル名称テーブルの合計バイト数。

### シンボル名称テーブル (symname_size バイト)

NUL 終端された文字列の並び。`name_off` はこのテーブルの先頭からのオフセット。

```
例 (idiv.lib):
  0x00: "__umodsi3\0"  (10 bytes)
  0x0A: "__modsi3\0"   ( 9 bytes)
  0x13: "__udivsi3\0"  (10 bytes)
  0x1D: "__divsi3\0"   ( 9 bytes)
  合計: 38 bytes = symname_size
```

**symtab_size の検算:** `symtab_size = 4 + syminfo_size + 4 + symname_size`

## モジュールエントリ

シンボルテーブルの直後 (`0x14 + symtab_size`) からファイル末尾まで連続する。

| オフセット | サイズ | フィールド | 内容 |
|-----------|--------|-----------|------|
| 0 | 4 | srf_size | SRF オブジェクトデータのバイトサイズ |
| 4 | 1 | name_len | ファイル名の長さ（**NUL を含まない**） |
| 5 | name_len | filename | ファイル名（**NUL 終端なし**） |
| 5 + name_len | srf_size | srf_data | SRF オブジェクトの実体 |

次のモジュールエントリは `5 + name_len + srf_size` の位置から始まる。

`module_pos` は各モジュールの `srf_size` フィールド（エントリ先頭）を指す。

## 検証データ

3 ファイルの実バイナリで `symtab_size = 4 + syminfo_size + 4 + symname_size` が成立することを確認済み。

| ファイル | symtab_size | syminfo_size | N | symname_size | 検算 |
|---------|------------|-------------|---|-------------|------|
| idiv.lib | 78 | 32 | 4 | 38 | 4+32+4+38=78 ✓ |
| fp.lib | 388 | 168 | 21 | 212 | 4+168+4+212=388 ✓ |
| lib.lib | 746 | 368 | 46 | 370 | 4+368+4+370=746 ✓ |

**lib.lib 詳細 (22636 バイト):**

```
0x00: "!<lib33>\0"      magic (9 bytes)
0x09: 0x000002EA (746)  symtab_size
0x0D: 0x06              name_len
0x0E: "SYMDEF"          name
0x14: 0x00000170 (368)  syminfo_size → 46 entries
0x18: {0x5784, 0x0000}  syminfo[0] = module:0x5784 ("seed.o"), sym:"seed"
0x20: {0x5647, 0x0005}  syminfo[1] = module:0x5647,           sym:"longjmp"
...
0x188: 0x00000172 (370) symname_size
0x18C: "seed\0longjmp\0setjmp\0ANSI_CLRZ\0..."
0x2FE: module table starts (abort.o の srf_size フィールド)
```

**モジュールエントリ例 (idiv.lib, module_pos=0x62):**

```
0x62: 0x00000164 (356)  srf_size
0x66: 0x08              name_len
0x67: "divsi3.o"        filename (8 bytes, NUL なし)
0x6F: [SRF data]        先頭 2 bytes = 0x0001 = c_fatt (RELOC)
```

## 旧 lib33_format.md (初期バイナリ解析) との差異

初期解析では magic を 8 バイトとし、1 バイトのずれにより以下の誤認識が生じていた:

| 旧記述 | 正しい解釈 |
|--------|-----------|
| `unknown_1` (4バイト, 0x08) = 0x00000002 | magic 末尾の NUL + symtab_size 上位3バイト |
| `unknown_2` (1バイト, 0x0C) = 0xEA | symtab_size の最下位バイト |
| `symtab_size` (4バイト, 0x14) | 実際は syminfo_size（情報テーブルのみの長さ） |
| `last_module_off` (4バイト, 0x18) | 実際は syminfo[0].module_pos |
| エントリのフィールド順: {name_off, module_off} | 正しくは {module_pos, name_off} |
| ファイル名: NUL 含む | 正しくは NUL を含まない |
| 文字列テーブルサイズ: 明示なし (不明) | symname_size フィールドとして明示されている |

## C コンパイラマニュアル (Appendix A-2) との差異

マニュアルでは `l_att`, `l_size`, `l_ver`, `l_objptr` 等が定義されているが、
実バイナリの構造とは一致しない。別バージョンの lib33 の仕様と考えられる。

## 実装状況

srf2elf はこのフォーマットに基づいて `.lib` → `.a` (Unix ar 形式 ELF アーカイブ) 変換に対応済み。
P/ECE SDK の fp.lib, idiv.lib, lib.lib, simple.lib, muslib.lib の変換が可能。
