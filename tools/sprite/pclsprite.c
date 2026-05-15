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
//  v0.70 2001.11.26 とりあえず実装、未高速化、ラインスクロール未実装
//                   基本関数の仕様を確定
//  v0.75 2001.12.06 pclSpriteBGGetFrameAdr追加
//  v0.90 2002.01.16 ラインスクロール
//  v0.92 2002.02.26 プライオリティ
//  

#include <string.h>
#include "pclsprite.h"

/*
   pattern:パターンデータのアドレス
     <high><low><mask>
   sbuff:バッファのアドレス
   ssize:バッファのサイズ
  256*128dot bg
   32*16*2(byte)*2(double)=2048
   32*128*3(plane)*2(double)=24576
  obj
   reg_num*(1+1+2)

*/
#if 0
#define ror(r,ct) ((r)=((r)>>(ct)))
#define rol(r,ct) ((r)=((r)<<(ct)))
#else
#define ror(r,ct) ((r)=(((r)>>(ct)) | ((r)<<(32-(ct)))))
#define rol(r,ct) ((r)=(((r)<<(ct)) | ((r)>>(32-(ct)))))
#endif

#define srl(r,ct) ( (r)=((r)>>(ct)) )
#define sll(r,ct) ( (r)=((r)<<(ct)) )

#if 0
#define mirror(w) ((w)<<=1)
#else
#define mirror(w) {int i,a=0;for(i=0;i<8;i++){a<<=1;if(w&0x01000000)a|=0x01000000;if(w&0x00010000)a|=0x00010000;if(w&0x00000100)a|=0x00000100;if(w&0x00000001)a|=0x00000001;w>>=1;}(w)=a;}
#endif

static short ready=0;     /*ライブラリ使用可能フラグ*/
static unsigned int *pclsprite_pat; /*ソースパターン*/
static int disp;          /*表示コントロールレジスタ*/
static int *fram;         /*全体のフレームバッファ*/
static int *sreg;         /*スプライトレジスタ*/
static int sprmax=-1;     /*スプライト総数*/
static int *bg0f;         /*奥　背景 フレームバッファ*/
static int *bg0a;         /*奥　背景 キャラクタバッファ*/
static int *bg0b;         /*奥　背景 キャラクタバッファ(内部用)*/
static int *bg1f;         /*手前背景 フレームバッファ*/
static int *bg1a;         /*手前背景 キャラクタバッファ*/
static int *bg1b;         /*手前背景 キャラクタバッファ(内部用)*/
static unsigned char *line0xy,*line1xy;
                          /*ラインスクロールレジスタ*/
static unsigned char bg0x,bg0y,bg1x,bg1y; 
                          /*bgスクロールレジスタ*/
static unsigned char cursor_x,cursor_y,cur_lx,cur_rx,cur_uy,cur_dy;

static void write_bgcls();
static void write_bg0(int *frame,int *bg,int x,int y);
static void write_bg1(int *frame,int *bg,int x,int y);
static void write_bg0_line(int *frame,int *bg,unsigned char *linexy);
static void write_bg1_line(int *frame,int *bg,unsigned char *linexy);

void bg_check(int *frame,int *new,int *old);
void conv_fram(int *dst,int *src);
void write_raster0_a0(int *dst,int *src,int *ed,int h);
void write_raster0_a1(int *dst,int *src,int *ed,int h);
void write_raster0_a2(int *dst,int *src,int *ed,int h);
void write_raster0_a3(int *dst,int *src,int *ed,int h);

void write_raster1_a0(int *dst,int *src,int *ed,int h);
void write_raster1_a1(int *dst,int *src,int *ed,int h);
void write_raster1_a2(int *dst,int *src,int *ed,int h);
void write_raster1_a3(int *dst,int *src,int *ed,int h);

typedef void write_raster_func(int*,int*,int*,int);

write_raster_func *raster0func[4]={
write_raster0_a0,write_raster0_a1,write_raster0_a2,write_raster0_a3
};
write_raster_func *raster1func[4]={
write_raster1_a0,write_raster1_a1,write_raster1_a2,write_raster1_a3
};

void bg_write_1c(unsigned char *frame,int chr,int ofs,unsigned int *p);
void sp_write_1c(unsigned char *dst,unsigned char *src,int x,int chr);

/*
   表示モードの設定
   in INVALIDVALで現在の設定を取得
   bit 0  :bg0 奥　(1で表示)
   bit 1  :bg1 手前(1で表示)
   bit 2  :sprite  (1で表示)
   bit 3  :bg0と1のプライオリティ(1のとき、bg0とbg1を入れ替える)
   bit 4  :bg0 ラインスクロール(1でラインスクロールモード)
   bit 5  :bg1 ラインスクロール(1でラインスクロールモード)
   bit 8,9:backcolor(bg0が非表示の時の背景色)
   out 直前の表示モード
*/
int pclSpriteDispMode(int reg)
{
  int old;
  if(reg<0)
    return disp;
  old=disp;disp=reg;
  return(old);
}

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
void pclSpriteBGSetCharacter(int num,int x,int y,int chr)
{
  if(!ready)
    return;
  if((x<0)||(x>=BG_CX)||(y<0)||(y>=BG_CY)||(num<0)||(num>=2))
    return;
  if(num==0)
    *(((unsigned short*)bg0a)+y*BG_CX+x)=chr;
  else
    *(((unsigned short*)bg1a)+y*BG_CX+x)=chr;
}

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
int pclSpriteBGGetCharacter(int num,int x,int y)
{
  if(!ready)
    return ERROR_CHR;
  if((x<0)||(x>=BG_CX)||(y<0)||(y>=BG_CY)||(num<0)||(num>=2))
    return ERROR_CHR;
  if(num==0)
    return *(((unsigned short*)bg0a)+y*BG_CX+x);
  else
    return *(((unsigned short*)bg1a)+y*BG_CX+x);
}

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
unsigned short *pclSpriteBGGetAdr(int num)
{
  if(!ready)
    return (unsigned short*)ERROR_CHR;
  if(num==0)
    return (unsigned short*)bg0a;
  else
    return (unsigned short*)bg1a;
}

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
void pclSpriteSetCharacter(int num,int x,int y,int chr)
{
  if((num>=sprmax)||(num<0))
    return;
  sreg[num]=(chr<<16)|((y&0xff)<<8)|(x&0xff);
}

/*
   カーソル位置のセット
   in
    x:Ｘキャラクタ位置
    y:Ｙキャラクタ位置
*/
void pclSpriteBGSetCursor(int x,int y)
{
  if((x>=cur_lx)&&(x<cur_rx))
    cursor_x=x;
  if((y>=cur_uy)&&(y<cur_dy))
    cursor_y=y;
}

/*
   １文字書き込み
   in:
    chr:文字コード
*/
void pclSpriteBGPutCharacter(int chr)
{
  if(!ready)
    return;
  if(cursor_x>=cur_rx){
    cursor_x=cur_lx;
    cursor_y++;
    if(cursor_y>=cur_dy){
      pclSpriteBGScroll(BG_SCROLL_UP);
      memset(((char*)bg0a)+(cur_lx+(cur_dy-1)*BG_CX)*2,0,(cur_rx-cur_lx)*2);
      cursor_y--;
    }
  }
  if(chr=='\n'){
    cursor_x=cur_lx;
    cursor_y++;
    if(cursor_y>=cur_dy){
      pclSpriteBGScroll(BG_SCROLL_UP);
      cursor_y--;
    }
    return;
  }else{
    pclSpriteBGSetCharacter(0,cursor_x,cursor_y,chr);
    cursor_x++;
  }
}

/*
   キャラクタ表示部分のクリア
*/
void pclSpriteBGClear()
{
  int y;
  for(y=cur_uy;y<cur_dy;y++)
    memset(((char*)bg0a)+(cur_lx+y*BG_CX)*2,0,(cur_rx-cur_lx)*2);
  cursor_x=cur_lx;
  cursor_y=cur_uy;
}

/*
   キャラクタ単位のスクロール
   in:
    dir:方向
     BG_SCROLL_UP:上へ
     BG_SCROLL_DN:下へ
     BG_SCROLL_LF:左へ
     BG_SCROLL_RI:右へ
*/
void pclSpriteBGScroll(int dir)
{
  int x,y;
  if(!ready)
    return;
  switch(dir){
  case BG_SCROLL_UP:
    for(y=cur_uy;y<(cur_dy-1);y++){
      memcpy(((char*)bg0a)+(cur_lx+(y+0)*BG_CX)*2,
	     ((char*)bg0a)+(cur_lx+(y+1)*BG_CX)*2,
	     (cur_rx-cur_lx)*2);
    }
    memset(((char*)bg0a)+(cur_lx+(cur_dy-1)*BG_CX)*2,0,(cur_rx-cur_lx)*2);
    break;
  case BG_SCROLL_DN:
    for(y=cur_dy-1;y>cur_uy;y--){
      memcpy(((char*)bg0a)+(cur_lx+(y-0)*BG_CX)*2,
	     ((char*)bg0a)+(cur_lx+(y-1)*BG_CX)*2,
	     (cur_rx-cur_lx)*2);
    }
    memset(((char*)bg0a)+(cur_lx+(cur_uy)*BG_CX)*2,0,(cur_rx-cur_lx)*2);
    break;
  case BG_SCROLL_LF:
    for(y=cur_uy;y<cur_dy;y++){
      for(x=cur_lx;x<(cur_rx-1);x++){
	*(((short*)bg0a)+(x+y*BG_CX))=*(((short*)bg0a)+(x+1+y*BG_CX));
      }
    }
    for(y=cur_uy;y<cur_dy;y++)
      *(((short*)bg0a)+(cur_rx-1+y*BG_CX))=0;
    break;
  case BG_SCROLL_RI:
    for(y=cur_uy;y<cur_dy;y++){
      for(x=cur_rx-1;x>cur_lx;x--){
	*(((short*)bg0a)+(x+y*BG_CX))=*(((short*)bg0a)+(x-1+y*BG_CX));
      }
    }
    for(y=cur_uy;y<cur_dy;y++)
      *(((short*)bg0a)+(cur_lx+y*BG_CX))=0;
    break;
  }
}

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
int *pclSpriteGetAdr()
{
  return sreg;
}

/*
   スプライトのX表示位置を取得する
   in
    num:スプライト番号（0～）
   out
    x座標
*/
int pclSpriteGetX(int num)
{
  if((num>=sprmax)||(num<0))
    return ERROR_CHR;
  return (sreg[num]&0xff);
}

/*
   スプライトのY表示位置を取得する
   in
    num:スプライト番号（0～）
   out
    y座標
*/
int pclSpriteGetY(int num)
{
  if((num>=sprmax)||(num<0))
    return ERROR_CHR;
  return ((sreg[num]>>8)&0xff);
}

/*
   スプライトのキャラクタ番号を取得する
   in
    num:スプライト番号（0～）
   out
    キャラクタ番号
*/
int pclSpriteGetCharacter(int num)
{
  if((num>=sprmax)||(num<0))
    return ERROR_CHR;
  return ((sreg[num]>>16)&0xffff);
}

/*
   ライブラリのフラッシュ
   ライブラリ使用中にユーザ設定のパターンを書き換えた
   場合には、このフラッシュをコールすること。
*/
void pclSpriteBGFlush()
{
  if(!ready)
    return;
  memset(bg0b,0xff,BG_CBUFF_SIZE);
  memset(bg1b,0xff,BG_CBUFF_SIZE);
}

/*
   パターンアドレスの変更
   ライブラリ使用中にパターンアドレスを変更した
   場合には、pclSpriteBGFlushをコールすること。
*/
int *pclSpriteSetPatternAdr(int *pattern)
{
  int *oldadr=pclsprite_pat;
  pclsprite_pat=(unsigned int*)pattern;
  return(oldadr);
}

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
    　　　　必ずcalcworkで用意したワークのポインタをわたすこと
    ssize  :ライブラリワークのサイズ
    　　　　必ずcalcworkで用意したワークのサイズをわたすこと
   out
    負の数:ワーク不足などのエラー発生
*/
int pclSpriteInit(int *pattern,int *sbuff,int ssize)
{
  int i;
  pclsprite_pat=(unsigned int*)pattern;
  sprmax=(ssize-SPRITE_WORK_MIN)/4;
  bg0a=sbuff;
  bg1a=bg0a+BG_CBUFF_SIZE/4;
  sreg=bg1a+BG_CBUFF_SIZE/4;
  bg0b=sreg+sprmax;
  bg1b=bg0b+BG_CBUFF_SIZE/4;
  bg0f=bg1b+BG_CBUFF_SIZE/4;
  bg1f=bg0f+BG_FBUFF_SIZE/4;
  line0xy=(unsigned char*)(bg1f+BG_FBUFF_SIZE/4);
  line1xy=line0xy+BG_LINE_SIZE;
  fram=(int*)(line1xy+BG_LINE_SIZE);
  /*fram=(int*)0x1000;*/
      /*描画テンポラリのフレームバッファをFRAMに持っていくと、
        若干ですがスピードがあがります。pclSpriteMakeFrameを
        コールしている間だけ必要です*/
  memset(bg0a,0x00,BG_CBUFF_SIZE);
  memset(bg0b,0xff,BG_CBUFF_SIZE);
  memset(bg1a,0x00,BG_CBUFF_SIZE);
  memset(bg1b,0xff,BG_CBUFF_SIZE);
  for(i=0;i<sprmax;i++)
    sreg[i]=0x0000f8f8; /*x,y=-8,-8 非表示*/
  disp=DISP_SPR|DISP_BG0|DISP_BG1;/*すべて表示*/
  cursor_x=0;cursor_y=0;
  cur_lx=0;cur_rx=16;cur_uy=0;cur_dy=11;

  if(ssize<SPRITE_WORK_MIN)
    return -1;
  ready=-1;
  return 0;
}

/*
   ＢＧ画面のスクロール位置設定
   in
    num:ＢＧ画面番号（0,1）
    x,y:スクロール位置
*/
void pclSpriteBGSetPosition(int num,int x,int y)
{
  if(num==0){
    bg0x=x&(BG_CX*8-1);
    bg0y=y&(BG_CY*8-1);
  }else{
    bg1x=x&(BG_CX*8-1);
    bg1y=y&(BG_CY*8-1);
  }
}

/*
   ＢＧ画面のXスクロール位置取得
   in
    num:ＢＧ画面番号（0,1）
   out
    xスクロール位置
*/
int pclSpriteBGGetX(int num)
{
  if(num==0)
    return bg0x;
  else
    return bg1x;
}

/*
   ＢＧ画面のYスクロール位置取得
   in
    num:ＢＧ画面番号（0,1）
   out
    yスクロール位置
*/
int pclSpriteBGGetY(int num)
{
  if(num==0)
    return bg0y;
  else
    return bg1y;
}

/*
   ＢＧ画面のラインスクロール位置設定
   in
    num:ＢＧ画面番号（0,1）
    line:ライン番号(0～87)
    x,y:スクロール位置
*/
void pclSpriteBGSetLinePosition(int num,int line,int x,int y)
{
  if((line<0)||(line>=FR_Y))
    return;
  if(num==0){
    line0xy[line*2+0]=x;
    line0xy[line*2+1]=y;
  }else{
    line1xy[line*2+0]=x;
    line1xy[line*2+1]=y;
  }
}

/*
   ＢＧ画面のXラインスクロール位置取得
   in
    num:ＢＧ画面番号（0,1）
    line:ライン番号(0～87)
   out
    xスクロール位置(不正なライン番号では負の数を返します)
*/
int pclSpriteBGGetLineX(int num,int line)
{
  if((line<0)||(line>=FR_Y))
    return -1;
  if(num==0)
    return line0xy[line*2+0];
  else
    return line1xy[line*2+0];
}

/*
   ＢＧ画面のYラインスクロール位置取得
   in
    num:ＢＧ画面番号（0,1）
    line:ライン番号(0～87)
   out
    yスクロール位置(不正なライン番号では負の数を返します)
*/
int pclSpriteBGGetLineY(int num,int line)
{
  if((line<0)||(line>=FR_Y))
    return -1;
  if(num==0)
    return line0xy[line*2+1];
  else
    return line1xy[line*2+1];
}

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
unsigned char *pclSpriteBGGetLineAdr(int num)
{
  if(num==0)
    return line0xy;
  else
    return line1xy;
}

/*
   現在の表示レジスタ内容にしたがって画面を構成する
   in
    gbuf:128*88/4バイトのワークへのポインタ

   gbufはユーザでpceLCDTransDirectしてください
*/
void pclSpriteMakeFrame(char *gbuf)
{
  if(!ready)
    return;
  if((disp&NOCHKBG0)==0)
	bg_check(bg0f,bg0a,bg0b);
  if((disp&NOCHKBG1)==0)
	bg_check(bg1f,bg1a,bg1b);

  if(disp&PRIO_BG){
    if(disp&DISP_BG1){
      if(disp&LINE_BG1)
        write_bg0_line(fram,bg1f,line1xy);
      else
        write_bg0(fram,bg1f,bg1x,bg1y);
    }else{
      write_bgcls();
    }
  }else{
    if(disp&DISP_BG0){
      if(disp&LINE_BG0)
        write_bg0_line(fram,bg0f,line0xy);
      else
        write_bg0(fram,bg0f,bg0x,bg0y);
    }else{
      write_bgcls();
    }
  }
  if(disp&DISP_SPR){
    unsigned short *ptr=((unsigned short*)sreg)+1;
    int i;
    for(i=0;i<sprmax;i++){
      if(!(*ptr&0x2000))
	sp_write_1c((unsigned char*)fram,(unsigned char*)pclsprite_pat,*(ptr-1),*ptr);
      ptr+=2;
    }
  }
  if(disp&PRIO_BG){
    if(disp&DISP_BG0){
      if(disp&LINE_BG0){
	write_bg1_line(fram,bg0f,line0xy);
      }else{
	write_bg1(fram,bg0f,bg0x,bg0y);
      }
    }
  }else{
    if(disp&DISP_BG1){
      if(disp&LINE_BG1){
	write_bg1_line(fram,bg1f,line1xy);
      }else{
	write_bg1(fram,bg1f,bg1x,bg1y);
      }
    }
  }
  if(disp&DISP_SPR){
    unsigned short *ptr=((unsigned short*)sreg)+1;
    int i;
    for(i=0;i<sprmax;i++){
      if((*ptr&0x2000))
	sp_write_1c((unsigned char*)fram,(unsigned char*)pclsprite_pat,*(ptr-1),*ptr);
      ptr+=2;
    }
  }
  conv_fram((int*)gbuf,fram);
}

static void write_bgcls()
{
  int p0,p1,ct=FR_Y,*ptr=fram;
  if(disp&0x200) p0=-1; else p0=0;
  if(disp&0x100) p1=-1; else p1=0;
  do{
    *ptr++=p0;*ptr++=p0;*ptr++=p0;*ptr++=p0;
    *ptr++=p1;*ptr++=p1;*ptr++=p1;*ptr++=p1;
  }while(--ct);
}

static void write_bg1(int *frame,int *bg,int x,int y)
{
  int h=x&31,*bge=bg+(BG_FBUFF_SIZE/4),rofs,ct=FR_Y;
  write_raster_func *cur_func;

  bg+=((y&(BG_CY*8-1))*BG_CX*3+BG_CX)>>2;
  rofs=-((x>>3)&0x1c)+BG_CX;
  cur_func=raster1func[h>>3];
  do{
    cur_func(frame,bg-(rofs>>2),bg,h);frame+=FR_CX*2/4;bg+=BG_CX*3/4;
    if(bg>bge) bg-=BG_FBUFF_SIZE/4;
  }while(--ct);
}

static void write_bg0(int *frame,int *bg,int x,int y)
{
  int h=x&31,*bge=bg+(BG_FBUFF_SIZE/4),rofs,ct=FR_Y;
  write_raster_func *cur_func;

  bg+=((y&(BG_CY*8-1))*BG_CX*3+BG_CX)>>2;
  rofs=-((x>>3)&0x1c)+BG_CX;
  cur_func=raster0func[h>>3];
  do{
    cur_func(frame,bg-(rofs>>2),bg,h);frame+=FR_CX/4;bg+=BG_CX/4;
    cur_func(frame,bg-(rofs>>2),bg,h);frame+=FR_CX/4;bg+=BG_CX*2/4;
    if(bg>bge) bg-=BG_FBUFF_SIZE/4;
  }while(--ct);
}

static void write_bg1_line(int *frame,int *bg,unsigned char *linexy)
{
  int ct=FR_Y;
  do{
    int h,lofs,rofs;
    {
      int x,y;
      x=*linexy++;y=*linexy++;
      h=x&31;
      lofs=((x-h)>>3)-BG_CX;
      rofs=y*BG_CX*3+BG_CX;
    }
    raster1func[h/8](frame,bg+lofs/4+rofs/4,bg+rofs/4,h);
    frame+=FR_CX*2/4;
  }while(--ct);
}

static void write_bg0_line(int *frame,int *bg,unsigned char *linexy)
{
  int ct=FR_Y;
  do{
    int h,lofs,rofs;
    {
      int x,y;
      x=*linexy++;y=*linexy++;
      h=x&31;
      lofs=((x-h)>>3)-BG_CX;
      rofs=y*BG_CX*3+BG_CX;
    }
    raster0func[h/8](frame,bg+lofs/4+rofs/4,bg+rofs/4,h);
    frame+=FR_CX/4;rofs+=BG_CX;
    raster0func[h/8](frame,bg+lofs/4+rofs/4,bg+rofs/4,h);
    frame+=FR_CX/4;
  }while(--ct);
}


void bg_check_in(int *frame,int *pn,int *po,int *base)
{
  unsigned short w0,w1;
  pn--;po--;
  w0=*(((unsigned short*)pn)+0);w1=*(((unsigned short*)po)+0);
  if(w0!=w1){
    *(((unsigned short*)po)+0)=w0;
    bg_write_1c((unsigned char*)frame,w0,(pn-base)*4+0,pclsprite_pat);
  }
  w0=*(((unsigned short*)pn)+1);w1=*(((unsigned short*)po)+1);
  if(w0!=w1){
    *(((unsigned short*)po)+1)=w0;
    bg_write_1c((unsigned char*)frame,w0,(pn-base)*4+2,pclsprite_pat);
  }
}

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
unsigned char *pclSpriteBGGetFrameAdr(int num)
{
  if(!ready)
    return (unsigned char*)ERROR_CHR;
  if(num==0)
    return (unsigned char*)bg0f;
  else
    return (unsigned char*)bg1f;
}

#if 0
static void bg_check(int *frame,int *new,int *old)
{
  int *pn=new,*po=old,*endcheck=new+(BG_CBUFF_SIZE/4);
  do{
    if(*pn++!=*po++) bg_check_in(frame,pn,po,new);
    if(*pn++!=*po++) bg_check_in(frame,pn,po,new);
    if(*pn++!=*po++) bg_check_in(frame,pn,po,new);
    if(*pn++!=*po++) bg_check_in(frame,pn,po,new);
  }while(pn<endcheck);
}

void bg_write_1c(unsigned char *frame,int chr,int ofs,unsigned int *pat)
{
  unsigned int *p=pat+(chr&0x3fff)*24/4;
  int ct=6,w;
  frame+=((ofs&(~0x3f))*12)+((ofs&0x3f)>>1);
  if(chr&0x8000){  /*vrev*/
    frame+=32*3*8+32*2;
    ct=2;
    if(chr&0x4000){ /*hrev*/
      do{
	w=*p++;
	mirror(w);
	frame-=32*5;*frame=w;w>>=8;frame+=32;*frame=w;w>>=8;
	frame+=32;*frame=w;w>>=8;frame-=32*5;*frame=w;
	w=*p++;
	mirror(w);
	frame+=32;*frame=w;w>>=8;frame+=32;*frame=w;w>>=8;
	frame-=32*5;*frame=w;w>>=8;frame+=32;*frame=w;
	w=*p++;
	mirror(w);
	frame+=32;*frame=w;w>>=8;frame-=32*5;*frame=w;w>>=8;
	frame+=32;*frame=w;w>>=8;frame+=32;*frame=w;
      }while(--ct);
    }else{          /*hnom*/
      do{
	w=*p++;
	frame-=32*5;*frame=w;w>>=8;frame+=32;*frame=w;w>>=8;
	frame+=32;*frame=w;w>>=8;frame-=32*5;*frame=w;
	w=*p++;
	frame+=32;*frame=w;w>>=8;frame+=32;*frame=w;w>>=8;
	frame-=32*5;*frame=w;w>>=8;frame+=32;*frame=w;
	w=*p++;
	frame+=32;*frame=w;w>>=8;frame-=32*5;*frame=w;w>>=8;
	frame+=32;*frame=w;w>>=8;frame+=32;*frame=w;
      }while(--ct);
    }
  }else{           /*vnom*/
    if(chr&0x4000){ /*hrev*/
      do{
	w=*p++;
	mirror(w);
	*frame=w;frame+=32;w>>=8;*frame=w;frame+=32;w>>=8;
	*frame=w;frame+=32;w>>=8;*frame=w;frame+=32;
      }while(--ct);
    }else{          /*hnom*/
      do{
	w=*p++;
	*frame=w;frame+=32;w>>=8;*frame=w;frame+=32;w>>=8;
	*frame=w;frame+=32;w>>=8;*frame=w;frame+=32;
      }while(--ct);
    }
  }
}
static void write_raster0_a0(int *dst,int *src,int *ed,int h)
{/*h(0~7)*/
  unsigned int w0,w1,mask0,mask1;
  mask1=0xffffffff;srl(mask1,h);mask0=~mask1;
  /*mask0=sll(0xff000000,(8-h)),mask1=~mask0;*/
  w0=*src++;if(src==ed)src-=8;srl(w0,h);
            w1=*src++;if(src==ed)src-=8;
  ror(w1,h);
  w0|=(w1&mask0);*dst++=w0;
  w1&=mask1;w0=*src++;if(src==ed)src-=8;
  ror(w0,h);
  w1|=(w0&mask0);*dst++=w1;
  w0&=mask1;w1=*src++;if(src==ed)src-=8;
  ror(w1,h);
  w0|=(w1&mask0);*dst++=w0;
  w1&=mask1;w0=*src++;if(src==ed)src-=8;
  ror(w0,h);
  w1|=(w0&mask0);*dst++=w1;
}

static void write_raster0_a1(int *dst,int *src,int *ed,int h)
{/*h(8~15)*/
  unsigned int w0,w1,mask0,mask1;
  h-=8;
  mask1=0x00ffffff;srl(mask1,h);mask0=~mask1;
  /*mask0=sll(0xffff0000,(8-h)),mask1=~mask0;*/
  w0=*src++;if(src==ed)src-=8;srl(w0,8);srl(w0,h);
            w1=*src++;if(src==ed)src-=8;
  ror(w1,8);ror(w1,h);
  w0|=(w1&mask0);*dst++=w0;
  w1&=mask1;w0=*src++;if(src==ed)src-=8;
  ror(w0,8);ror(w0,h);
  w1|=(w0&mask0);*dst++=w1;
  w0&=mask1;w1=*src++;if(src==ed)src-=8;
  ror(w1,8);ror(w1,h);
  w0|=(w1&mask0);*dst++=w0;
  w1&=mask1;w0=*src++;if(src==ed)src-=8;
  ror(w0,8);ror(w0,h);
  w1|=(w0&mask0);*dst++=w1;
}

static void write_raster0_a2(int *dst,int *src,int *ed,int h)
{/*h(16~23)*/
  unsigned int w0,w1,mask0,mask1;
  h=24-h;
  mask0=0xffffff00;sll(mask0,h),mask1=~mask0;
  w0=*src++;if(src==ed)src-=8;
  rol(w0,8);rol(w0,h);
  w0&=mask1;w1=*src++;if(src==ed)src-=8;
  rol(w1,8);rol(w1,h);
  w0|=(w1&mask0);*dst++=w0;
  w1&=mask1;w0=*src++;if(src==ed)src-=8;
  rol(w0,8);rol(w0,h);
  w1|=(w0&mask0);*dst++=w1;
  w0&=mask1;w1=*src++;if(src==ed)src-=8;
  rol(w1,8);rol(w1,h);
  w0|=(w1&mask0);*dst++=w0;
  w1&=mask1;w0=*src++;if(src==ed)src-=8;
  rol(w0,8);rol(w0,h);
  w1|=(w0&mask0);*dst++=w1;
}

static void write_raster0_a3(int *dst,int *src,int *ed,int h)
{/*h(24~31)*/
  unsigned int w0,w1,mask0,mask1;
  h=32-h;
  mask0=0xffffffff;sll(mask0,h),mask1=~mask0;
  w0=*src++;if(src==ed)src-=8;
  rol(w0,h);
  w0&=mask1;w1=*src++;if(src==ed)src-=8;
  rol(w1,h);
  w0|=(w1&mask0);*dst++=w0;
  w1&=mask1;w0=*src++;if(src==ed)src-=8;
  rol(w0,h);
  w1|=(w0&mask0);*dst++=w1;
  w0&=mask1;w1=*src++;if(src==ed)src-=8;
  rol(w1,h);
  w0|=(w1&mask0);*dst++=w0;
  w1&=mask1;w0=*src++;if(src==ed)src-=8;
  rol(w0,h);
  w1|=(w0&mask0);*dst++=w1;
}

static void write_raster1_a0(int *dst,int *src,int *ed,int h)
{/*h(0~7)*/
  unsigned int w0,w1,x0,x1,m0,m1,mask0,mask1;
  mask0=0xff000000<<(8-h),mask1=~mask0;
  w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;srl(w0,h);srl(x0,h);srl(m0,h);
   w1=src[0];x1=src[8];m1=src[16];src++;if(src==ed)src-=8;
  ror(w1,h);ror(x1,h);ror(m1,h);
    w0|=(w1&mask0);x0|=(x1&mask0);m0|=(m1&mask0);dst[0]=(dst[0]&m0)|w0;dst[4]=(dst[4]&m0)|x0;
  dst++;w1&=mask1;x1&=mask1;m1&=mask1;
   w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;
  ror(w0,h);ror(x0,h);ror(m0,h);
    w1|=(w0&mask0);x1|=(x0&mask0);m1|=(m0&mask0);dst[0]=(dst[0]&m1)|w1;dst[4]=(dst[4]&m1)|x1;
  dst++;w0&=mask1;x0&=mask1;m0&=mask1;
   w1=src[0];x1=src[8];m1=src[16];src++;if(src==ed)src-=8;
  ror(w1,h);ror(x1,h);ror(m1,h);
    w0|=(w1&mask0);x0|=(x1&mask0);m0|=(m1&mask0);dst[0]=(dst[0]&m0)|w0;dst[4]=(dst[4]&m0)|x0;
  dst++;w1&=mask1;x1&=mask1;m1&=mask1;
   w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;
  ror(w0,h);ror(x0,h);ror(m0,h);
    w1|=(w0&mask0);x1|=(x0&mask0);m1|=(m0&mask0);dst[0]=(dst[0]&m1)|w1;dst[4]=(dst[4]&m1)|x1;
}

static void write_raster1_a1(int *dst,int *src,int *ed,int h)
{/*h(8~15)*/
  unsigned int w0,w1,x0,x1,m0,m1,mask0,mask1;
  h-=8;
  mask0=0xffff0000<<(8-h),mask1=~mask0;
  w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;
      srl(w0,8);srl(w0,h);srl(x0,8);srl(x0,h);srl(m0,8);srl(m0,h);
   w1=src[0];x1=src[8];m1=src[16];src++;if(src==ed)src-=8;
      ror(w1,8);ror(w1,h);ror(x1,8);ror(x1,h);ror(m1,8);ror(m1,h);
    w0|=(w1&mask0);x0|=(x1&mask0);m0|=(m1&mask0);dst[0]=(dst[0]&m0)|w0;dst[4]=(dst[4]&m0)|x0;
  dst++;w1&=mask1;x1&=mask1;m1&=mask1;
   w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;
      ror(w0,8);ror(w0,h);ror(x0,8);ror(x0,h);ror(m0,8);ror(m0,h);
    w1|=(w0&mask0);x1|=(x0&mask0);m1|=(m0&mask0);dst[0]=(dst[0]&m1)|w1;dst[4]=(dst[4]&m1)|x1;
  dst++;w0&=mask1;x0&=mask1;m0&=mask1;
   w1=src[0];x1=src[8];m1=src[16];src++;if(src==ed)src-=8;
      ror(w1,8);ror(w1,h);ror(x1,8);ror(x1,h);ror(m1,8);ror(m1,h);
    w0|=(w1&mask0);x0|=(x1&mask0);m0|=(m1&mask0);dst[0]=(dst[0]&m0)|w0;dst[4]=(dst[4]&m0)|x0;
  dst++;w1&=mask1;x1&=mask1;m1&=mask1;
   w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;
      ror(w0,8);ror(w0,h);ror(x0,8);ror(x0,h);ror(m0,8);ror(m0,h);
    w1|=(w0&mask0);x1|=(x0&mask0);m1|=(m0&mask0);dst[0]=(dst[0]&m1)|w1;dst[4]=(dst[4]&m1)|x1;
}

static void write_raster1_a2(int *dst,int *src,int *ed,int h)
{/*h(16~23)*/
  unsigned int w0,w1,x0,x1,m0,m1,mask0,mask1;
  h=24-h;
  mask0=0xffffff00<<h,mask1=~mask0;

  w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;
      rol(w0,8);rol(w0,h);rol(x0,8);rol(x0,h);rol(m0,8);rol(m0,h);
  w0&=mask1;x0&=mask1;m0&=mask1;
   w1=src[0];x1=src[8];m1=src[16];src++;if(src==ed)src-=8;
      rol(w1,8);rol(w1,h);rol(x1,8);rol(x1,h);rol(m1,8);rol(m1,h);
    w0|=(w1&mask0);x0|=(x1&mask0);m0|=(m1&mask0);dst[0]=(dst[0]&m0)|w0;dst[4]=(dst[4]&m0)|x0;
  dst++;w1&=mask1;x1&=mask1;m1&=mask1;
   w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;
      rol(w0,8);rol(w0,h);rol(x0,8);rol(x0,h);rol(m0,8);rol(m0,h);
    w1|=(w0&mask0);x1|=(x0&mask0);m1|=(m0&mask0);dst[0]=(dst[0]&m1)|w1;dst[4]=(dst[4]&m1)|x1;
  dst++;w0&=mask1;x0&=mask1;m0&=mask1;
   w1=src[0];x1=src[8];m1=src[16];src++;if(src==ed)src-=8;
      rol(w1,8);rol(w1,h);rol(x1,8);rol(x1,h);rol(m1,8);rol(m1,h);
    w0|=(w1&mask0);x0|=(x1&mask0);m0|=(m1&mask0);dst[0]=(dst[0]&m0)|w0;dst[4]=(dst[4]&m0)|x0;
  dst++;w1&=mask1;x1&=mask1;m1&=mask1;
   w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;
      rol(w0,8);rol(w0,h);rol(x0,8);rol(x0,h);rol(m0,8);rol(m0,h);
    w1|=(w0&mask0);x1|=(x0&mask0);m1|=(m0&mask0);dst[0]=(dst[0]&m1)|w1;dst[4]=(dst[4]&m1)|x1;
}

static void write_raster1_a3(int *dst,int *src,int *ed,int h)
{/*h(24~31)*/
  unsigned int w0,w1,x0,x1,m0,m1,mask0,mask1;
  h=32-h;
  mask0=0xffffffff<<h,mask1=~mask0;

  w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;
  rol(w0,h);rol(x0,h);rol(m0,h);
  w0&=mask1;x0&=mask1;m0&=mask1;
   w1=src[0];x1=src[8];m1=src[16];src++;if(src==ed)src-=8;
  rol(w1,h);rol(x1,h);rol(m1,h);
    w0|=(w1&mask0);x0|=(x1&mask0);m0|=(m1&mask0);dst[0]=(dst[0]&m0)|w0;dst[4]=(dst[4]&m0)|x0;
  dst++;w1&=mask1;x1&=mask1;m1&=mask1;
   w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;
  rol(w0,h);rol(x0,h);rol(m0,h);
    w1|=(w0&mask0);x1|=(x0&mask0);m1|=(m0&mask0);dst[0]=(dst[0]&m1)|w1;dst[4]=(dst[4]&m1)|x1;
  dst++;w0&=mask1;x0&=mask1;m0&=mask1;
   w1=src[0];x1=src[8];m1=src[16];src++;if(src==ed)src-=8;
  rol(w1,h);rol(x1,h);rol(m1,h);
    w0|=(w1&mask0);x0|=(x1&mask0);m0|=(m1&mask0);dst[0]=(dst[0]&m0)|w0;dst[4]=(dst[4]&m0)|x0;
  dst++;w1&=mask1;x1&=mask1;m1&=mask1;
   w0=src[0];x0=src[8];m0=src[16];src++;if(src==ed)src-=8;
  rol(w0,h);rol(x0,h);rol(m0,h);
    w1|=(w0&mask0);x1|=(x0&mask0);m1|=(m0&mask0);dst[0]=(dst[0]&m1)|w1;dst[4]=(dst[4]&m1)|x1;
}

static void sp_write_1c(unsigned char *dst,unsigned char *src,int x,int chr)
{
  int ct,nextraster;
  {
    int y;
    y=((signed short)x)>>8;
    x=(int)((signed char)x);
    src+=(chr&0x1fff)*24;
    dst+=(x>>3)+(y<<5);
    if(chr&0x8000){ /*vrev*/
      nextraster=-48;
      dst+=7<<5;
      if(((unsigned int)y)<=80){ /*yクリップ無し*/
	ct=8;
      }else{
	if(y>=0){
	  if(y<88){             /*画面下端*/
	    ct=88-y;
	    src+=(8-ct)*3;dst-=(8-ct)<<5;
	  }else{
	    return;
	  }
	}else{
	  if(y>=-7){            /*画面上端*/
	    ct=8+y;
	  }else{
	    return;
	  }
	}
      }
    }else{          /*vnom*/
      nextraster=16;
      if(((unsigned int)y)<=80){ /*yクリップ無し*/
	ct=8;
      }else{
	if(y>=0){
	  if(y<88){             /*画面下端*/
	    ct=88-y;
	  }else{
	    return;
	  }
	}else{
	  if(y>=-7){            /*画面上端*/
	    ct=8+y;
	    src+=-y*3;dst+=(-y)<<5;
	  }else{
	    return;
	  }
	}
      }
    }
  }
  if(chr&0x4000){/*hrev*/
    if(((unsigned int)x)<=120){ /*xクリップ無し*/
      nextraster--;
      x&=7;
      do{
	unsigned int p0,p1,p2;
	p0=*src++;p1=*src++;p2=(*src++)|0xffffff00;
	mirror(p0);mirror(p1);mirror(p2);
	sll(p0,x);sll(p1,x);
	rol(p2,x);
	*dst=(*dst&p2)|p0;dst+=16;*dst=(*dst&p2)|p1;dst+=-16+1;
	srl(p0,8);srl(p1,8);srl(p2,8);
	*dst=(*dst&p2)|p0;dst+=16;*dst=(*dst&p2)|p1;dst+=nextraster;
      }while(--ct);
    }else{
      if(x>=0){               /*画面右端*/
	x&=7;
	do{
	  unsigned int p0,p1,p2;
	  p0=*src++;p1=*src++;p2=(*src++)|0xffffff00;
	  mirror(p0);mirror(p1);mirror(p2);
	  sll(p0,x);sll(p1,x);
	  rol(p2,x);
	  *dst=(*dst&p2)|p0;dst+=16;*dst=(*dst&p2)|p1;dst+=nextraster;
	}while(--ct);
      }else if(x>=-7){        /*画面左端*/
	x=-x;dst++;
	do{
	  unsigned int p0,p1,p2;
	  p0=*src++;p1=*src++;p2=(*src++)|0xffffff00;
	  mirror(p0);mirror(p1);mirror(p2);
	  srl(p0,x);srl(p1,x);srl(p2,x);
	  *dst=(*dst&p2)|p0;dst+=16;*dst=(*dst&p2)|p1;dst+=nextraster;
	}while(--ct);
      }
    }
  }else{       /*hnom*/
    if(((unsigned int)x)<=120){ /*xクリップ無し*/
      nextraster--;
      x&=7;
      do{
	unsigned int p0,p1,p2;
	p0=*src++;p1=*src++;p2=(*src++)|0xffffff00;
	sll(p0,x);sll(p1,x);
	rol(p2,x);
	*dst=(*dst&p2)|p0;dst+=16;*dst=(*dst&p2)|p1;dst+=-16+1;
	srl(p0,8);srl(p1,8);srl(p2,8);
	*dst=(*dst&p2)|p0;dst+=16;*dst=(*dst&p2)|p1;dst+=nextraster;
      }while(--ct);
    }else{
      if(x>=0){               /*画面右端*/
	x&=7;
	do{
	  unsigned int p0,p1,p2;
	  p0=*src++;p1=*src++;p2=(*src++)|0xffffff00;
	  sll(p0,x);sll(p1,x);
	  rol(p2,x);
	  *dst=(*dst&p2)|p0;dst+=16;*dst=(*dst&p2)|p1;dst+=nextraster;
	}while(--ct);
      }else if(x>=-7){        /*画面左端*/
	x=-x;dst++;
	do{
	  unsigned int p0,p1,p2;
	  p0=*src++;p1=*src++;p2=(*src++)|0xffffff00;
	  srl(p0,x);srl(p1,x);srl(p2,x);
	  *dst=(*dst&p2)|p0;dst+=16;*dst=(*dst&p2)|p1;dst+=nextraster;
	}while(--ct);
      }
    }
  }
}

static void conv_fram(int *dst,int *src)
{
  char *d,*s;
  int x,y;
  d=(char*)dst;s=(char*)src;
  for(x=0;x<16;x++){
    for(y=0;y<88;y++){
      *d++=*s;s+=16;
      *d++=*s;s+=16;
    }
    s+=-88*16*2+1;
  }
}

#endif
