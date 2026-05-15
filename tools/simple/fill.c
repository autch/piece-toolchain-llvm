#include <pclsprite.h>
#include "simple_local.h"
/*
   fill
   -1:“§‰ßF 0-3:’ÊíF
*/
void fill(int x1,int y1,int x2,int y2,int color)
{
  if(x1>x2){
    int s;
    s = x1; x1 = x2; x2 = s;
  }
  if(y1>y2){
    int s;
    s = y1; y1 = y2; y2 = s;
  }
  if((x1>255)||(y1>255)||(x2<0)||(y2<0))
    return;
  if((color<-1)||(color>3))
    return;
  if(x1<0)   x1=0;
  if(x2>255) x2=255;
  if(y1<0)   y1=0;
  if(y2>255) y2=255;
  do{
    int x=x1;
    do{
      simple_pset(x,y1,color);
      x++;
    }while(x<=x2);
	y1++;
  }while(y1<=y2);
}
