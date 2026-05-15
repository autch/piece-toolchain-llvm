#include <pclsprite.h>
#include "thread.h"
#include "simple_local.h"
/*
   指定配列のキャラクタでパターンを再定義
*/
extern int *simple_pattern_adr;

void bgset(const unsigned char *ptr)
{
  if(((int)ptr)&3){
    simple_error("bgsetで指定のパターンのアドレスが不正です\n");
    thread_return();
  }
  if(ptr==0)
    ptr=(char *)simple_pattern_adr;
  thread_lock();
  pclSpriteSetPatternAdr((int*)ptr);
  pclSpriteBGFlush();
  thread_unlock();
}
