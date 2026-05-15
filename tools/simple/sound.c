#include  <piece.h>
#include "simple.h"

/*
   ƒTƒEƒ“ƒhÄ¶
*/
extern unsigned char S1[],S2[],S3[],S4[];

void sound(int num)
{
  unsigned char *wp;
  switch(num){
  case 0:    wp=S1;break;
  case 1:    wp=S2;break;
  case 2:    wp=S3;break;
  case 3:    wp=S4;break;
  default:   return;
  }
  ppdplay(wp);
}

