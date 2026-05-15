#include <piece.h>
#include "pclsprite.h"

#define XW (128)
#define YW (88)
#define SPR_MAX (64)

unsigned char dummy[128*88];

char sintbl[]={
   0,  12,  24,  37,  48,  60,  71,  81,  90,  98, 106, 112, 118, 122, 125, 127,
 127, 127, 125, 122, 118, 112, 106,  98,  90,  81,  71,  60,  48,  37,  24,  12,
   0, -12, -24, -37, -48, -60, -71, -81, -90, -98,-106,-112,-118,-122,-125,-127,
-127,-127,-125,-122,-118,-112,-106, -98, -90, -81, -71, -60, -48, -37, -24, -12,
};

int sprwork[pclSpriteWork(SPR_MAX)];
unsigned char direct[128*88/4];

struct sprite_local_work {
  short x,dx,y,dy,chr;
} move_work[SPR_MAX];

extern int pattern[];

short aqua[]={
  0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,
  0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,
  0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,
  0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,
  0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,
};  

short num[]={
  0x20c6,0x20c7,0x20c8,0x20c9,0x20ca,0x20cb,0x20cc,0x20cd,0x20ce,0x20cf,
};

int x0,x1,y0,y1;

static int rand_num=0x5555;
/* óêêîéÊìæ */
short rand_get()
{
  rand_num<<=1;
  if(((rand_num&0x8000)!=0)^((rand_num&0x0002)!=0))
    rand_num|=1;
  return rand_num;
}

void pceAppInit( void )
{
  int x,y,i,j;
  pclSpriteInit(pattern,sprwork,sizeof(sprwork));
  pclSpriteDispMode(LINE_BG0|LINE_BG1|DISP_SPR|DISP_BG0|DISP_BG1);
//  pclSpriteDispMode(LINE_BG0|LINE_BG1|DISP_SPR|DISP_BG0|DISP_BG1|PRIO_BG);
  for(y=0;y<YW/8;y++){
    for(x=0;x<XW/8;x++){
      pclSpriteBGSetCharacter(0,x,y,x+y*XW/8);
      pclSpriteBGSetCharacter(0,x+16,y,x+y*XW/8);
    }
  }
  for(y=0;y<32;y++)
    for(x=0;x<32;x++){
//      pclSpriteBGSetCharacter(0,x,y,0xb0); /*ìßñæÉLÉÉÉâÉNÉ^Ç≈fill*/
      pclSpriteBGSetCharacter(1,x,y,0xb0); /*ìßñæÉLÉÉÉâÉNÉ^Ç≈fill*/
    }
  for(i=0;i<32;i+=8)
    for(j=0;j<32;j+=8)
      for(y=0;y<5;y++)
	for(x=0;x<6;x++){
//	  pclSpriteBGSetCharacter(0,x+j,y+i,aqua[x+y*6]); /*éËëOÇÃÇaÇfÇÃÉçÉSÇ*/
	  pclSpriteBGSetCharacter(1,x+j,y+i,aqua[x+y*6]); /*éËëOÇÃÇaÇfÇÃÉçÉSÇ*/
	}
  for(i=0;i<SPR_MAX;i++){
    move_work[i].x=64*0x80;
    move_work[i].y=44*0x80;
    move_work[i].dx=(rand_get()&0xff)-0x80;
    move_work[i].dy=(rand_get()&0xff)-0x80;
    move_work[i].chr=(rand_get()&0x2007)+0xb6;
  }
  x0=x1=y0=y1=0;

  pceLCDDispStop();
  pceLCDSetBuffer( dummy );
  pceLCDDispStart();

  pceAppSetProcPeriod( 25 );
  pclSpriteMakeFrame(direct);
  pclSpriteDispMode(LINE_BG0|LINE_BG1|DISP_SPR|DISP_BG0|DISP_BG1|NOCHKBG1|NOCHKBG0);
  for(x=0;x<32;x++){
      pclSpriteBGSetCharacter(0,x,0,0xd6);
      pclSpriteBGSetCharacter(1,x,0,0xd6);
  }
}

int time=0;

void pceAppProc( int cnt )
{
  int i;
  int st,ed;
  int  yadd=0;
  x0++;
  x1++;y1++;
  
  yadd=sintbl[(x1/2)&63]+128;
  for(i=0;i<88;i++){
    pclSpriteBGSetLinePosition(0,i,x0+sintbl[(x0+i)&63]/8,i);
    pclSpriteBGSetLinePosition(1,i,x1,y1+yadd*i/64);
  }
  for(i=0;i<SPR_MAX;i++){
    move_work[i].x+=move_work[i].dx;
    move_work[i].y+=move_work[i].dy;
    if((move_work[i].x<=-8*0x80)||(move_work[i].x>=128*0x80)||
       (move_work[i].y<=-8*0x80)||(move_work[i].y>=88*0x80)){
      move_work[i].x=64*0x80;
      move_work[i].y=44*0x80;
    }
    pclSpriteSetCharacter(i,move_work[i].x>>7,move_work[i].y>>7,move_work[i].chr);
  }
  {
    int temp=time;
    pclSpriteSetCharacter(0,8*0,0,num[temp/10000]);temp=temp%10000;
    pclSpriteSetCharacter(1,8*1,0,num[temp/1000]); temp=temp%1000;
    pclSpriteSetCharacter(2,8*2,0,num[temp/100]);  temp=temp%100;
    pclSpriteSetCharacter(3,8*3,0,num[temp/10]);   temp=temp%10;
    pclSpriteSetCharacter(4,8*4,0,num[temp]);
  }
  st=pceTimerGetPrecisionCount();
  pclSpriteMakeFrame(direct);
  pceLCDTransDirect(direct);
  ed=pceTimerGetPrecisionCount();
  time=pceTimerAdjustPrecisionCount(st,ed);
}


void pceAppExit( void )
{
}

