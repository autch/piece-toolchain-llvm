#include <pclsprite.h>
#include <simple.h>
#include "simple_local.h"
/*
   line
   -1:“§‰ßF 0-3:’ÊíF
*/
void line(int x1,int y1,int x2,int y2,int color)
{
  int dx,dy,s,step;
  dx = iabs(x2-x1);
  dy = iabs(y2-y1);
  if(dx>dy){
    if(x1>x2){
      step = (y1 > y2)?1:-1;
      s = x1; x1 = x2; x2 = s;
      y1 = y2;
    }else{
      step = (y1 < y2)?1:-1;
    }
    pset(x1,y1,color);
    s = dx>>1;
    while(++x1 <= x2){
      if((s -= dy) < 0){
	s+=dx;
	y1+=step;
      }
      pset(x1,y1,color);
    }
  }else{
    if(y1>y2){
      step = (x1 > x2)?1:-1;
      s = y1; y1 = y2; y2 = s;
      x1 = x2;
    }else{
      step = (x1 < x2)?1:-1;
    }
    pset(x1,y1,color);
    s = dy>>1;
    while(++y1 <= y2){
      if((s -= dx) < 0){
	s+=dy;
	x1+=step;
      }
      pset(x1,y1,color);
    }
  }
}
