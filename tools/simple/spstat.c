#include "simple_local.h"
/*
   スプライトの動作状態を取得
   0:停止中 1:動作中
*/
int spstat(int num)
{
  if((num<0)||(num>=SIMPLE_SPRITE_MAX))
    return 0;
  return simple_sprite_reg[num].ctrl;
}
