#include <pclsprite.h>
#include <simple.h>
#include "simple_local.h"
/*
   circle
   -1:ìßâﬂêF 0-3:í èÌêF
*/
void circle(int x,int y,int r,int hv,int color)
{
  int f,xi=r,yi=0,xt=256,yt=256;
  if((r<0)||(r>32767))
    return;
  if(hv<0)
    return;
  if(hv<=256)
    yt=hv;
  else if(hv<=512)
    xt=512-hv;
  else
    return;
  f=-2*r+3;
  while(xi>=yi){
    int xh=xi*xt>>8;
    int yh=yi*yt>>8;
    pset(x+xh,y+yh,color);
    pset(x-xh,y+yh,color);
    pset(x+xh,y-yh,color);
    pset(x-xh,y-yh,color);
    xh=yi*xt>>8;
    yh=xi*yt>>8;
    pset(x+xh,y+yh,color);
    pset(x-xh,y+yh,color);
    pset(x+xh,y-yh,color);
    pset(x-xh,y-yh,color);

    if(f>=0){
      xi--;
      f-=4*xi;
    }
    yi++;
    f+=4*yi+2;
  }
}
