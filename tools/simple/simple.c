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
#include <smcvals.h>
#include <stdarg.h>
#include <string.h>
#include <piece.h>
#include <pclsprite.h>
#include "thread.h"
#include "simple_local.h"
#include "simple.h"

#define XW (128)
#define YW (88)

extern int pattern[],PAT[];

static unsigned char pbuff[128*88];
static int sprwork[pclSpriteWork(SIMPLE_SPRITE_MAX)];
static unsigned char direct[128*88/4];
static int smain_id;
static char errorflag=0;
static char selectdebug_flag=0;
static char errorstr[512];

char simple_msgbuff[512];
SPR_WORK simple_sprite_reg[64];
char *simple_frame_ptr;
int *simple_pattern_adr;

extern void smain();


/*
   simpleライブラリ用エラー表示
*/
void simple_error(char *format,...)
{
  va_list arglist;
  va_start( arglist, format );
  pcevsprintf( errorstr, format, arglist );
  va_end( arglist );
  errorflag=1;
}

void selectdebug(int mode)
{
  selectdebug_flag=mode;
}

/*
   バックグラウンドでスプライトを動かします
   812 + 64方向
   7*3
   654
*/
static short dir2x[]={
 0*256,0*256,1*256,1*256,1*256,0*256,-1*256,-1*256,-1*256,
 256, 255, 251, 245, 237, 226, 213, 198, 181, 162, 142, 121,  98,  74,  50,  25,
   0, -25, -50, -74, -98,-121,-142,-162,-181,-198,-213,-226,-237,-245,-251,-255,
-256,-255,-251,-245,-237,-226,-213,-198,-181,-162,-142,-121, -98, -74, -50, -25,
   0,  25,  50,  74,  98, 121, 142, 162, 181, 198, 213, 226, 237, 245, 251, 255,
};
static short dir2y[]={
 0*256,-1*256,-1*256,0*256,1*256,1*256,1*256,0*256,-1*256,
   0, -25, -50, -74, -98,-121,-142,-162,-181,-198,-213,-226,-237,-245,-251,-255,
-256,-255,-251,-245,-237,-226,-213,-198,-181,-162,-142,-121, -98, -74, -50, -25,
   0,  25,  50,  74,  98, 121, 142, 162, 181, 198, 213, 226, 237, 245, 251, 255,
 256, 255, 251, 245, 237, 226, 213, 198, 181, 162, 142, 121,  98,  74,  50,  25,
};

int oldct;

static void init_back_sprite()
{
  oldct=pceTimerGetCount();
}

static void back_sprite()
{
  int i,newct,difct;
  newct=pceTimerGetCount();
  difct=newct-oldct;
  oldct=newct;
  thread_lock();
  for(i=0;i<SIMPLE_SPRITE_MAX;i++){
    if(simple_sprite_reg[i].ctrl){
      if(simple_sprite_reg[i].dir){
	int speed=simple_sprite_reg[i].sp*difct*6;
	int c_dir=simple_sprite_reg[i].dir;
	simple_sprite_reg[i].si+=speed;
	if(simple_sprite_reg[i].si>=(simple_sprite_reg[i].ed<<16)){
	  simple_sprite_reg[i].ctrl=0;
	  simple_sprite_reg[i].x=simple_sprite_reg[i].sx+simple_sprite_reg[i].ed*dir2x[c_dir]/256;
	  simple_sprite_reg[i].y=simple_sprite_reg[i].sy+simple_sprite_reg[i].ed*dir2y[c_dir]/256;
	}else{
	  speed=simple_sprite_reg[i].si>>8;
	  simple_sprite_reg[i].x=simple_sprite_reg[i].sx
	    +speed*dir2x[c_dir]/65536;
	  simple_sprite_reg[i].y=simple_sprite_reg[i].sy
	    +speed*dir2y[c_dir]/65536;
	}
      }
    }
  }
  thread_unlock();
}

static void gcls()
{
  char *p=simple_frame_ptr;
  int i=256;
  do{
    memset(p,0,64);
    p+=64;
    memset(p,0xff,32);
    p+=32;
  }while(--i);
}

void pceAppInit( void )
{
  int x,y;
  if(*PAT == 0x00555555)
    simple_pattern_adr=pattern;
  else
    simple_pattern_adr=PAT;
  pclSpriteInit(simple_pattern_adr,sprwork,sizeof(sprwork));
  for(y=0;y<32;y++){
    for(x=0;x<32;x++){
      pclSpriteBGSetCharacter(0,x,y,0x00); /*0キャラクタでfill*/
      pclSpriteBGSetCharacter(1,x,y,0x00); /*0キャラクタでfill*/
    }
  }
  for(x=0;x<SIMPLE_SPRITE_MAX;x++){
    simple_sprite_reg[x].x=-8;
    simple_sprite_reg[x].y=-8;
    simple_sprite_reg[x].sx=-8;
    simple_sprite_reg[x].sy=-8;
    simple_sprite_reg[x].dir=0;
    simple_sprite_reg[x].ctrl=0;
    simple_sprite_reg[x].chr=0;
    simple_sprite_reg[x].ed=0;
    simple_sprite_reg[x].si=0;
    simple_sprite_reg[x].sp=0;
  }
  pceLCDDispStop();
  pclSpriteMakeFrame(direct);
  memset(pbuff,0,sizeof(pbuff));
  pceLCDSetBuffer(pbuff);
  pceLCDTrans();
  pceLCDDispStart();

  pceAppSetProcPeriod( 12 );
  simple_frame_ptr=pclSpriteBGGetFrameAdr(1);
  gcls();
  init_back_sprite();
  thread_gr_init(); /*簡易スレッド起動*/
  dispmode(1,1,1,0,0); /*dispmodeの設定は、smain実行前*/
  smain_id=thread_create(smain);
  thread_lock();
}

static void disp_allreg()
{
  extern int a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z;

  memset(pbuff,0,sizeof(pbuff));
  pceFontSetType(0x82);
  pceFontSetBkColor(0);
  pceFontSetTxColor(3);
  pceFontSetPos(0,0);
  pceFontPrintf("  A %12d  B %12d\n",a,b);
  pceFontPrintf("  C %12d  D %12d\n",c,d);
  pceFontPrintf("  E %12d  F %12d\n",e,f);
  pceFontPrintf("  G %12d  H %12d\n",g,h);
  pceFontPrintf("  I %12d  J %12d\n",i,j);
  pceFontPrintf("  K %12d  L %12d\n",k,l);
  pceFontPrintf("  M %12d  N %12d\n",m,n);
  pceFontPrintf("  O %12d  P %12d\n",o,p);
  pceFontPrintf("  Q %12d  R %12d\n",q,r);
  pceFontPrintf("  S %12d  T %12d\n",s,t);
  pceFontPrintf("  U %12d  V %12d\n",u,v);
  pceFontPrintf("  W %12d  X %12d\n",w,x);
  pceFontPrintf("  Y %12d  Z %12d\n",y,z);
  pceFontSetType(0);
  pceLCDTrans();
}

void pceAppProc( int cnt )
{
  int i;
  thread_unlock();
  back_sprite();
  switch(errorflag){
  case 0:
    if(thread_status(smain_id)==TH_NULL)
      if(errorflag==0)
	pceAppReqExit(0);
    thread_wait(10);
    thread_lock();
    if(selectdebug_flag&&(pcePadGet()&PAD_SELECT)){
      disp_allreg();
    }else{
      for(i=0;i<SIMPLE_SPRITE_MAX;i++)
	pclSpriteSetCharacter(i,simple_sprite_reg[i].x,simple_sprite_reg[i].y,simple_sprite_reg[i].chr);
      pclSpriteMakeFrame(direct);
      pceLCDTransDirect(direct);
    }
    thread_unlock();
    break;
  case 1:
    thread_lock();
    pceFontSetPos(0,0);
    pceFontPutStr(errorstr);
    pceFontPutStr("スタートボタンでメニューに戻ります");
    pceLCDTrans();
    errorflag++;
    thread_unlock();
  case 2:
    thread_lock();
    if(pcePadGet()&PAD_START)
      pceAppReqExit(0);
    thread_unlock();
    break;
  }
  thread_lock();
}


void pceAppExit( void )
{
  thread_unlock();
  thread_gr_stop();
  pceWaveStop(0);
}


