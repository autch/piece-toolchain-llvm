#include <pclsprite.h>
#include "simple_local.h"
/*
   point
   -1:“§‰ßF 0-3:’ÊíF
*/
int point(int x,int y)
{
  char *p=simple_frame_ptr;
  int retcolor=0;
  if((x<0)||(x>255)||(y<0)||(y>255))
    return -1;
  p+=y*32*3+(x>>3);
  x=1<<(x&7);
  if(p[32*2]&x)
	return -1;
  if(p[32*0]&x)
	retcolor+=2;
  if(p[32*1]&x)
	retcolor+=1;
  return retcolor;
}
