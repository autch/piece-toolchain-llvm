/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              Library Programs
//
//
// PIECE sprite library : Ver 1.00
//
// Copyright (C)2001 AUQAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
//
// Coded by Katsumasa Tsuneyoshi
//
// Comments:
//
//  スプライト/ＢＧライブラリ
//  v0.70 2001.11.26 Katsumasa Tsuneyoshi
//                   とりあえず実装、未高速化、未ラインスクロール
//  v0.75 2001.12.06 pclSpriteBGGetFrameAdr追加
//  v0.90 2002.01.16 ラインスクロール
//  v0.92 2002.02.26 プライオリティ
//  v1.00 2002.03.04 変化チェックのスキップ
//

#define FR_CX (16)
#define FR_Y  (88)
#define BG_CX (32)
#define BG_CY (32)
#define BG_CBUFF_SIZE ((BG_CX*BG_CY)*2)
#define BG_FBUFF_SIZE ((BG_CX*BG_CY)*8*3)
#define FRAME_SIZE (FR_CX*FR_Y*2)
#define BG_LINE_SIZE (FR_Y*2)
#define ERROR_CHR (0x80000000)
#define DISP_BG0 (0x0001)
#define DISP_BG1 (0x0002)
#define DISP_SPR (0x0004)
#define PRIO_BG  (0x0008)
#define LINE_BG0 (0x0010)
#define LINE_BG1 (0x0020)
#define NOCHKBG0 (0x0040)
#define NOCHKBG1 (0x0080)
#define BG_SCROLL_UP (0)
#define BG_SCROLL_DN (1)
#define BG_SCROLL_LF (2)
#define BG_SCROLL_RI (3)

#define SPRITE_WORK_MIN (BG_CBUFF_SIZE*4+BG_FBUFF_SIZE*2+FRAME_SIZE+BG_LINE_SIZE*2)
#define pclSpriteWork(spr_num) ((SPRITE_WORK_MIN+spr_num*4)/4)

/*
   表示モードの設定
   in INVALIDVALで現在の設定を取得
   bit 0  :bg0 奥　(1で表示)
   bit 1  :bg1 手前(1で表示)
   bit 2  :sprite  (1で表示)
   bit 3  :bg0と1のプライオリティ(1のとき、bg0とbg1を入れ替える)
   bit 4  :bg0 ラインスクロール(1でラインスクロールモード)
   bit 5  :bg1 ラインスクロール(1でラインスクロールモード)
   bit 6  :bg0のキャラクタ番号変化チェックをしない
   bit 7  :bg1のキャラクタ番号変化チェックをしない
   bit 8,9:backcolor(bg0が非表示の時の背景色)
   out 直前の表示モード
*/
int pclSpriteDispMode(int reg);
/*
   ＢＧ画面へキャラクタを設定する
   in
    num:ＢＧ画面番号（0,1）
    x,y:キャラクタ座標（0～31）
    chr:キャラクタ番号
        bit0-13:パターン番号
	bit14  :水平反転
	bit15  :垂直反転
*/
void pclSpriteBGSetCharacter(int num,int x,int y,int chr);
/*
   ＢＧ画面のキャラクタを取得する
   in
    num:ＢＧ画面番号（0,1）
    x,y:キャラクタ座標（0～31）
   out
    chr:キャラクタ番号
        bit0-13:パターン番号
	bit14  :水平反転
	bit15  :垂直反転
*/
int pclSpriteBGGetCharacter(int num,int x,int y);
/*
   ＢＧ画面のキャラクタRAMアドレスを取得する
   in
    num:ＢＧ画面番号（0,1）
   out
    キャラクタRAMへのポインタ
    キャラクタの内容は以下のとおり
    chr:キャラクタ番号
        bit0-13:パターン番号
	bit14  :水平反転
	bit15  :垂直反転
*/
unsigned short *pclSpriteBGGetAdr(int num);
/*
   スプライトの表示レジスタアドレスを取得する
   in
    なし
   out
    表示レジスタへのポインタ
    レジスタの内容は以下のとおり
    +0.b:x座標
    +1.b:y座標
    +2.w:キャラクタ番号
        bit0-12:パターン番号
	bit13  :プライオリティ 0:BG1の奥 1:BG1の手前
	bit14  :水平反転
	bit15  :垂直反転
*/
int *pclSpriteGetAdr();
/*
   スプライトの表示位置/番号を設定する
   in
    num:スプライト番号（0～）
    x,y:表示座標
    chr:キャラクタ番号
        bit0-12:パターン番号
	bit13  :プライオリティ 0:BG1の奥 1:BG1の手前
	bit14  :水平反転
	bit15  :垂直反転
*/
void pclSpriteSetCharacter(int num,int x,int y,int chr);
/*
   スプライトのX表示位置を取得する
   in
    num:スプライト番号（0～）
   out
    x座標
*/
int pclSpriteGetX(int num);
/*
   スプライトのY表示位置を取得する
   in
    num:スプライト番号（0～）
   out
    y座標
*/
int pclSpriteGetY(int num);
/*
   スプライトのキャラクタ番号を取得する
   in
    num:スプライト番号（0～）
   out
    キャラクタ番号
*/
int pclSpriteGetCharacter(int num);
/*
   ライブラリのフラッシュ
   ライブラリ使用中にユーザ設定のパターンを書き換えた
   場合には、このフラッシュをコールすること。
   呼んだ瞬間はかなり負荷がかかります。
*/
void pclSpriteBGFlush();
/*
   ライブラリの初期化
   in
    pattern:キャラクタパターンへのポインタ
            キャラクタパターンの構成は以下のとおり
            +0.b chr1 8dot
	    +1.b chr0 8dot
	    +2.b mask 8dot
	     ...
	    8*8dot 1キャラクタ。3*8バイトで１キャラクタに相当
    sbuff  :ライブラリワーク
    　　　　必ずpclSpriteWorkで用意したワークのポインタをわたすこと
    ssize  :ライブラリワークのサイズ
    　　　　必ずpclSpriteWorkで用意したワークのサイズをわたすこと
   out
    負の数:ワーク不足などのエラー発生
*/
int pclSpriteInit(int *pattern,int *sbuff,int ssize);

/*
   ＢＧ画面のスクロール位置設定
   in
    num:ＢＧ画面番号（0,1）
    x,y:スクロール位置
*/
void pclSpriteBGSetPosition(int num,int x,int y);
/*
   ＢＧ画面のXスクロール位置取得
   in
    num:ＢＧ画面番号（0,1）
   out
    xスクロール位置
*/
int pclSpriteBGGetX(int num);
/*
   ＢＧ画面のYスクロール位置取得
   in
    num:ＢＧ画面番号（0,1）
   out
    yスクロール位置
*/
int pclSpriteBGGetY(int num);
/*
   現在の表示レジスタ内容にしたがって画面を構成する
   in
    gbuf:128*88/4バイトのワークへのポインタ

   gbufはユーザでpceLCDTransDirectしてください
*/
void pclSpriteMakeFrame(char *gbuf);


/*
   カーソル位置のセット
   in
    x:Ｘキャラクタ位置
    y:Ｙキャラクタ位置
*/
void pclSpriteBGSetCursor(int x,int y);
/*
   １文字書き込み
   in:
    chr:文字コード
*/
void pclSpriteBGPutCharacter(int chr);
/*
   キャラクタ単位のスクロール
   in:
    dir:方向
     BG_SCROLL_UP:上へ
     BG_SCROLL_DN:下へ
     BG_SCROLL_LF:左へ
     BG_SCROLL_RI:右へ
*/
void pclSpriteBGScroll(int dir);

/*
   キャラクタ表示部分のクリア
*/
void pclSpriteBGClear();

/*
   ＢＧ画面のフレームバッファアドレスを取得する
   in
    num:ＢＧ画面番号（0,1）
   out
    フレームバッファへのポインタ
    フレームバッファの内容は以下のとおり
    <32byte plane 1><32byte plane 2><32byte mask>
    32*3バイト１ライン、96×128バイトで１画面
*/
unsigned char *pclSpriteBGGetFrameAdr(int num);

/*
   ＢＧ画面のラインスクロール位置設定
   in
    num:ＢＧ画面番号（0,1）
    line:ライン番号(0～87)
    x,y:スクロール位置
*/
void pclSpriteBGSetLinePosition(int num,int line,int x,int y);

/*
   ＢＧ画面のXラインスクロール位置取得
   in
    num:ＢＧ画面番号（0,1）
    line:ライン番号(0～87)
   out
    xスクロール位置(不正なライン番号では負の数を返します)
*/
int pclSpriteBGGetLineX(int num,int line);

/*
   ＢＧ画面のYラインスクロール位置取得
   in
    num:ＢＧ画面番号（0,1）
    line:ライン番号(0～87)
   out
    yスクロール位置(不正なライン番号では負の数を返します)
*/
int pclSpriteBGGetLineY(int num,int line);


/*
   ＢＧ画面のラインスクロールレジスタアドレスを取得する
   in
    num:ＢＧ画面番号（0,1）
   out
    ラインスクロールレジスタへのポインタ
    レジスタの内容は以下のとおり
    +0.b:０ライン目（表示上端）のxスクロール座標
    +1.b:０ライン目（表示上端）のyスクロール座標
    +2.b:１ライン目のxスクロール座標
    +3.b:１ライン目のyスクロール座標
    ～１画面で８８ライン分（176バイト）
*/
unsigned char *pclSpriteBGGetLineAdr(int num);
/*
   パターンアドレスの変更
   ライブラリ使用中にパターンアドレスを変更した
   場合には、pclSpriteBGFlushをコールすること。
*/
int *pclSpriteSetPatternAdr(int *pattern);
