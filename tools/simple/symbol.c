#include <piece.h>
#include <pclsprite.h>
#include <simple.h>
#include "simple_local.h"
#include "thread.h"

#define SY_FONT_MASK (0xf)
#define SY_FONT_5 (0)
#define SY_FONT_8 (1)
#define SY_FONT_4 (2)
/*
   symbol
   -1:透過色 0-3:通常色
*/
const static char fontsizex[]={ 5,8,4 };
const static char fontsizey[]={ 10,16,6 };
enum {HANKAKU,ZENKAKU,NOT_PUT};
static void put_1char(int x,int y,int chr,int type,int color)
{
  const unsigned char *fontptr=NULL;
  int xsize,ysize,wflag=HANKAKU;
  int font=type & SY_FONT_MASK;

  thread_lock();
  xsize=fontsizex[font];
  ysize=fontsizey[font];
  switch(font){
  case SY_FONT_5:
    fontptr=pceFontGetAdrs(chr);
    if(((int)fontptr & 0x80000000)==0) /*全角*/
      wflag=ZENKAKU;
    break;
  case SY_FONT_8:
  case SY_FONT_4:
    pceFontSetType(font);
    fontptr=pceFontGetAdrs(chr);
    pceFontSetType(0);
    if(((int)fontptr & 0x80000000)==0) /*全角*/
      wflag=NOT_PUT;
    break;
  default:
    wflag=NOT_PUT;
    break;
  }
  thread_unlock();

  if(wflag==HANKAKU){
    int yct,xct;
    for(yct=y;yct<y+ysize;yct++){
      int fontdata=*fontptr++;
      for(xct=x;xct<x+xsize;xct++){
	if(fontdata&0x80) pset(xct,yct,color);
	fontdata<<=1;
      }
    }
  }else if(wflag==ZENKAKU){
    int yct,xct;
    for(yct=y;yct<y+ysize;yct+=2){
      int fontdata=(*fontptr++)<<8;
      fontdata=(fontdata+(*fontptr++))<<8;
      fontdata+=*fontptr++;
      for(xct=x;xct<x+xsize*2;xct++){
	if(fontdata & 0x800000) pset(xct,yct,color);
	if(fontdata &    0x800) pset(xct,yct+1,color);
	fontdata<<=1;
      }
    }
  }
}

void symbol(int x,int y,char *str,int type,int color)
{
  unsigned char c1;
  int font=type&SY_FONT_MASK;
  while((c1=*str++)){
    if ( (c1>=0x81 && c1<=0x9f) || (c1>=0xe0 && c1<=0xfc) ) {
      if ( *str ) {
	unsigned char c2 = *str++;
	put_1char( x, y,(c1<<8)+c2,type,color);
      } else {
	break;
      }
      x += fontsizex[font]*2;
    } else if(c1>=' '){
      put_1char( x, y,c1,type,color);
      x += fontsizex[font];
    }
  }
}
