#include <pclsprite.h>
#include "simple_local.h"
/*
   pset
   -1:“§‰ßF 0-3:’ÊíF
*/
void pset(int x,int y,int color)
{
  if((x<0)||(x>255)||(y<0)||(y>255))
    return;
  if((color<-1)||(color>3))
    return;
  simple_pset(x,y,color);
}
