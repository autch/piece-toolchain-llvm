#include "thread.h"
#include "simple_local.h"
/*
   スプライト停止
*/
void spstop(int num)
{
  if((num<0)||(num>=SIMPLE_SPRITE_MAX))
    return;
  thread_lock();
  simple_sprite_reg[num].ctrl=0;
  thread_unlock();
}
