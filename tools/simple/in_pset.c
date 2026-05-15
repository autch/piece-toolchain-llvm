#include <pclsprite.h>
#include "simple_local.h"

#ifndef SIMPLE_USE_MACRO
void simple_pset(int x,int y,int color)
{
  char *p=simple_frame_ptr;
  int nextline=32;
  p+=y*32*3+(x>>3);
  x=1<<(x&7);
  y=~x;
  if(color<2){
    if(color<1){
      if(color<0){
	*p&=y;p+=nextline; *p&=y;p+=nextline; *p|=x; /*-1*/
      }else{
	*p&=y;p+=nextline; *p&=y;p+=nextline; *p&=y; /* 0*/
      }
    }else{
      *p&=y;p+=nextline; *p|=x;p+=nextline; *p&=y;   /* 1*/
    }
  }else if(color==2){
    *p|=x;p+=nextline;*p&=y;p+=nextline;*p&=y;       /* 2*/
  }else{
    *p|=x;p+=nextline;*p|=x;p+=nextline;*p&=y;       /* 3*/
  }
}
#endif
