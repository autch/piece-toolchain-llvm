#include <pclsprite.h>
/*
   グラフィック面のスクロール位置指定
   x,y:スクロール座標 0-255
*/
void home(int x,int y)
{
  pclSpriteBGSetPosition(1,x,y);
}
