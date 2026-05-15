#include "simple.h"
#include "thread.h"
#include <pclsprite.h>
/*
   グラフィック面に絵を表示する
   num:0:左上 1:右上 2:左下 3:右下
   patptr:配列の名前
*/
static void write_frame(char *dst,char *src);

static short frame_ofs[]={  0,16,32*3*128,32*3*128+16};

void pspdisp(int num,unsigned char *patptr)
{
  thread_lock();
  write_frame(pclSpriteBGGetFrameAdr(1)+frame_ofs[num],patptr);
  thread_unlock();
}

static void write_frame(char *dst,char *src)
{
  int y,x,*p=(int*)src;
  for(y=0;y<16;y++){
    for(x=0;x<16;x++){
      char *frame=dst;
      int ct=6;
      do{
	int w=*p++;
	*frame=w;frame+=32;w>>=8;*frame=w;frame+=32;w>>=8;
	*frame=w;frame+=32;w>>=8;*frame=w;frame+=32;
      }while(--ct);
      dst++;
    }
    dst=dst-16+32*3*8;
  }
}
