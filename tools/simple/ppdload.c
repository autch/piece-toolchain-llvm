#include <string.h>
#include <piece.h>
#include "thread.h"
#include "simple_local.h"
#include "simple.h"
/*
   効果音を読み込み、再生する
   filename:ファイル名(拡張子なし)
*/
static unsigned char ppdbuff[20*1024]; /*サウンド読み込みバッファ*/

void ppdload(char *filename)
{
  FILEACC fa;
  int i,ret;
  char fname[16];
  strncpy(fname,filename,8);
  fname[8]=0;
  strcat(fname,".ppd");
  thread_lock();
  if(pceFileOpen(&fa,fname,FOMD_RD)){
    simple_error("ppdloadで指定のファイル\n%s\nが見つかりません\n",fname);
    thread_unlock();
    thread_return();
  }
  if(fa.fsize>sizeof(ppdbuff)){
    simple_error("ppdloadで指定のファイル\n%s\nのサイズが大きすぎます\n",fname);
    thread_unlock();
    thread_return();
  }
  ret=pceFileReadSct(&fa,ppdbuff     ,0,4096);
  for(i=1;i<sizeof(ppdbuff)/4096;i++){
    if(fa.fsize<=i*4096)
      break;
    pceFileReadSct(&fa,ppdbuff+i*4096,i,4096);
  }
  pceFileClose(&fa);
  thread_unlock();
  ppdplay(ppdbuff);
}
