# fpkplay

P/ECE 標準音源ドライバの pmd ファイルを実機で再生します。pmd ファイルは拙作 [fpack](https://github.com/autch/funpack) でまとめて圧縮した fpk ファイルとして扱います。

## インストール

`fpkplay.pex` と `fpkplay.fpk` ファイルを P/ECE 実機に転送し、メニューから `FPACK Player` を起動してください。

### `fpkplay.fpk` の作り方

[fpack](https://github.com/autch/funpack) を入手し、pmd ファイルをまとめたディレクトリ内で以下のようにして作成します。

```shell-session 
> fpack -e fpkpack.fpk *.pmd
```

fpkplay に与えるには `-e` オプションが必須です。このオプションで pmd ファイルを圧縮します。

## 操作

起動できれば fpk ファイル内の最初のファイルを再生しているはずです。

`seek:` の行は方向キー左右で次に再生するファイルを選択します。Aボタンでそのファイルを再生します。

`playing:` の行は現在再生中のファイル名です。

その次の行には再生中のファイルのタイトル (`#Title`) 情報、さらにその下にはタイトル２ (`#Title2`) 情報が表示されます。

* 方向キー左右
  * 次に再生するファイルを選ぶ。`seek:` 行のファイル名が変化する
* Aボタン
  * `seek:` 行で選んだファイルを再生する。タイトル情報も変わる。
* Bボタン
  * 再生を停止する。表示は `seek:` 行だけになる。
* STARTボタン
  * 画面表示を停止する。もう一度STARTボタンを押すと元に戻る。
  * 液晶へのデータ転送が止まるため、節電とオーディオ出力に乗るノイズの削減になるかもしれない。
* START+SELECT 長押し
  * 終了してメニューに戻る。上記表示停止中にも有効。

## ビルドする

1. P/ECE 開発環境へのパスを通し、GNU make と `rm` を使えるようにしておきます。
1. music/wave の `mk.bat` を実行し、ドラム音色をビルドしておきます
1. music/ で `make` し、muslib をビルドします。
1. 最後にトップレベルディレクトリで `make pex` すると、 `fpkplay.pex` ができます。

## muslib への拡張

以下の点で P/ECE 開発環境添付の muslib から変更されています。

* 再生開始時のプチノイズ対策
  * `music/mus.c`
* エンベロープ、ビブラート、デチューンは毎回再生前に初期化する
  * `music/seq.c`
* ドラム音色末尾のノイズ削除
  * `music/wave/i_CYMBD.c`, `i_TOMH1.c`, `i_TOMM1.c`
* パート数拡張対応、26パートまで
  * `music/musdef.h`

## ライセンス

ソースディレクトリのうち、`music/` 以下は P/ECE 開発環境添付の muslib に由来するコードであり、前項の通り修正されています。このディレクトリについては P/ECE 公式サイトの [付属ソースを元にしたプログラムの再配布の扱いについて](https://aquaplus.jp/piece/intr.html) （ページ末尾）に従います。

トップレベルディレクトリのファイルは Autch が書いたものですが、CC0 とします。

fpkplay by Autch is marked CC0 1.0. To view a copy of this mark, visit https://creativecommons.org/publicdomain/zero/1.0/
