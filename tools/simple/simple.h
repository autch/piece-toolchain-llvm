/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              Library Programs
//
//
// PIECE シンプルライブラリ : Ver 1.00
//
// Copyright (C)2001 AUQAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
//
// Coded by Katsumasa Tsuneyoshi
//
// Comments:
//
//  v0.50 2001.11.28 bug fix
//  v0.80 2001.11.28 add sp___ func
//  v0.90 2001.11.28 add pic/sound func
//  v0.92 2002.01.15 add pset/pspdisp/ソース分割
//  v0.93 2002.02.04 siprintf ソース分割時のbug fix
//  v0.97 2002.02.20 add line/fill/symbol
//  v0.98 2002.02.25 add dispmode/point/circle
//  v1.00 2002.03.02 add ppdlaod/ppdplay/msec
//

void printstr(char *str);
void printnum(int num);
void printsiz(int keta);
/*
   改行
*/
void printnl();
void wait(int count);
void siprintf( const char *, ...  );
/*
   カーソル位置の設置
*/
void locate(int x,int y);
/*
   指定位置の文字の読み取り
*/
int scan(int x,int y);
/*
   パッド入力
*/
int pad();
/*
   指定数-1までのの乱数を返す
*/
int rnd(int max);
/*
   画面のクリア
*/
void cls();
/*
   文字単位スクロール
   0:上 1:下 2:左 3:右
*/
void scroll(int dir);
/*
   ２つの文字列に同じ文字が存在するかチェック
   発見すれば、その文字を返す
*/

/*
   指定位置にスプライトを表示
*/
void spdisp(int num,int x,int y,int chr);
/*
   指定位置にスプライトを表示
*/
void sppos(int num,int x,int y);
/*
   指定キャラクタに変更
*/
void spchr(int num,int chr);
/*
   スプライト移動指定
*/
void spmove(int num,int dir,int speed,int dist);
/*
   スプライト停止
*/
void spstop(int num);
/*
   スプライト移動再開
*/
void spcont(int num);
/*
   スプライト表示位置取得
*/
int spposx(int num);
int spposy(int num);
/*
   スプライト重なり取得
   指定スプライトの中心からsizeドット分に重なるキャラクタを
   全て取得して返す
*/
char *spscan(int num,int size);
/*
   スプライトの動作状態を取得
   0:停止中 1:動作中
*/
int spstat(int num);


int inchr(char *str1,char *str2);
/*
   内蔵サウンド再生
*/
void sound(int num);
/*パッド関連定義*/
#ifndef PAD_RI
#define PAD_RI 0x01
#define PAD_LF 0x02
#define PAD_DN 0x04
#define PAD_UP 0x08
#define PAD_B  0x10
#define PAD_A  0x20
#define PAD_D  0x40
#define PAD_C  0x80
#define PAD_START PAD_C
#define PAD_SELECT PAD_D
#endif

/*
   グラフィック面のスクロール位置指定
   x,y:スクロール座標 0-255
*/
void home(int x,int y);
/*
   グラフィック面のXスクロール位置取得
*/
int homex();
/*
   グラフィック面のYスクロール位置取得
*/
int homey();
/*
   グラフィック面に絵を表示する
   num:0:左上 1:右上 2:左下 3:右下
   patptr:配列の名前
*/
void pspdisp(int num,unsigned char *patptr);
/*
   グラフィック面に絵を読み込む
   num:0:左上 1:右上 2:左下 3:右下
   filename:ファイル名(拡張子なし)
*/
void pspload(int num,char *filename);
/*
   グラフィック面に点を打つ
   x,y:座標(0～255)
   color:-1:透過色 0-3:通常色
*/
void pset(int x,int y,int color);
/*
   セレクトボタンを押している間、
   変数を表示するモードにします
   mode:0:変数を表示しない（標準）
        1:セレクトボタンを押している間変数表示モードにする
*/
void selectdebug(int mode);
/*
   pmd（音楽データ）を読み込み、再生する
   filename:ファイル名(拡張子なし)
*/
void pmdload(char *filename);
/*
   pmd（音楽データ）を再生する
   pmdptr:配列の名前
*/
void pmdplay(unsigned char *pmdptr);
/*
   pmd（音楽データ）を停止する
*/
void pmdstop();
/*
   line
   -1:透過色 0-3:通常色
*/
void line(int x1,int y1,int x2,int y2,int color);
/*
   fill
   -1:透過色 0-3:通常色
*/
void fill(int x1,int y1,int x2,int y2,int color);
/*
   symbol
   type:フォントサイズ 0:5x10 1:8x16 2:4x6
   color:-1:透過色 0-3:通常色
*/
void symbol(int x,int y,char *str,int type,int color);
/*
   表示モードを設定
*/
void dispmode(int spr,int bg0,int bg1,int prio,int color);
/*
   表示色を取得
*/
int point(int x,int y);
/*
   circle
   -1:透過色 0-3:通常色
*/
void circle(int x,int y,int r,int hv,int color);
/*
   効果音を読み込み、再生する
   filename:ファイル名(拡張子なし)
*/
void ppdload(char *filename);
/*
   サウンド再生
*/
void ppdplay(const unsigned char *wp);
/*
   1ms単位の時間を取得
*/
unsigned int msec();
/*
   指定配列のキャラクタでパターンを再定義
*/
void bgset(const unsigned char *ptr);
/*
   指定ファイルでキャラクタを再定義
   filename:ファイル名(拡張子なし)
*/
void bgload(char *filename);
