#include "thread.h"
#include "simple_local.h"
/*
   スプライト移動指定
*/
void spmove(int num,int dir,int speed,int dist)
{
  if((num<0)||(num>=SIMPLE_SPRITE_MAX))
    return;
  if(speed<0) speed=0;
  else if(speed>=1000) speed=1000;
  if((dir<0)||(dir>128)){
    dir=0;
  }else if(dir>=64){
    dir+=9-64;
  }else if(dir>8){
    dir=0;
  }
  if(dist<0) dist=0;
  else if(dist>=255) dist=255;
  thread_lock();
  simple_sprite_reg[num].sx=simple_sprite_reg[num].x;
  simple_sprite_reg[num].sy=simple_sprite_reg[num].y;
  simple_sprite_reg[num].dir=dir;
  simple_sprite_reg[num].sp=speed;
  simple_sprite_reg[num].si=0;
  simple_sprite_reg[num].ctrl=1;
  simple_sprite_reg[num].ed=dist;
  thread_unlock();
}

