#include "thread.h"
#include "simple_local.h"
/*
   スプライト重なり取得
   指定スプライトの中心からsizeドット分に重なるキャラクタを
   全て取得して返す
*/
static char dummy_nullstr[]="";
char *spscan(int num,int size)
{
  int x,y,i;
  char *p;
  if((num<0)||(num>=SIMPLE_SPRITE_MAX)||(size<=0))
    return dummy_nullstr;
  thread_lock();
  x=simple_sprite_reg[num].x;y=simple_sprite_reg[num].y;
  p=simple_sprite_reg[num].atari;
  for(i=0;i<SIMPLE_SPRITE_MAX;i++){
    if(i==num)
      continue;
    if((iabs(simple_sprite_reg[i].x-x)<=size)&&(iabs(simple_sprite_reg[i].y-y)<=size))
      *p++=simple_sprite_reg[i].chr & 0xff;
  }
  thread_unlock();
  *p=0;
  return simple_sprite_reg[num].atari;
}
