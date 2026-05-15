#include "thread.h"
#include "simple_local.h"
/*
   指定キャラクタに変更
*/
void spchr(int num,int chr)
{
  if((num<0)||(num>=SIMPLE_SPRITE_MAX))
    return;
  simple_sprite_reg[num].chr=chr;
}
