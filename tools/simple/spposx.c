#include "thread.h"
#include "simple_local.h"
/*
   スプライト表示位置取得
*/
int spposx(int num)
{
  if((num<0)||(num>=SIMPLE_SPRITE_MAX))
    return -8;
  return simple_sprite_reg[num].x;
}

