#include <pclsprite.h>
/*
   •¶Žš—ñ‚Ì•\Ž¦
*/
void printstr(char *ptr)
{
  int i;
  for(i=0;i<256;i++){
    if(*ptr==0)
      break;
    pclSpriteBGPutCharacter((*ptr++)&0xff);
  }
}
