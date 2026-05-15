#include <pclsprite.h>
/*
   Žw’èˆÊ’u‚Ì•¶Žš‚Ì“Ç‚ÝŽæ‚è
*/
int scan(int x,int y)
{
  return (pclSpriteBGGetCharacter(0,x,y)&0xff);
}
