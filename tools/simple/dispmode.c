#include "thread.h"
#include "simple_local.h"
#include <pclsprite.h>
/*
   •\Ž¦ƒ‚[ƒh‚ðÝ’è
*/
void dispmode(int spr,int bg0,int bg1,int prio,int color)
{
  int mode=NOCHKBG1;
  if(spr) mode|=DISP_SPR;
  if(bg0) mode|=DISP_BG0;
  if(bg1) mode|=DISP_BG1;
  if(prio)mode|=PRIO_BG;
  if((color>=0)&&(color<=3))
    mode|=(color<<8);
  pclSpriteDispMode(mode);
}
