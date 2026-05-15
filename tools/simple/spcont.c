#include "thread.h"
#include "simple_local.h"
/*
   スプライト移動再開
*/
void spcont(int num)
{
  if((num<0)||(num>=SIMPLE_SPRITE_MAX))
    return;
  thread_lock();
  simple_sprite_reg[num].ctrl=1;
  thread_unlock();
}
