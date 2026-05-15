#include "thread.h"
#include "simple_local.h"

/*
   指定位置にスプライトを表示
*/
void sppos(int num,int x,int y)
{
  if((num<0)||(num>=SIMPLE_SPRITE_MAX))
    return;
  thread_lock();
  simple_sprite_reg[num].x=x;
  simple_sprite_reg[num].y=y;
  thread_unlock();
}
