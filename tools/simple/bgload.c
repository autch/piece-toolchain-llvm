#include <string.h>
#include <piece.h>
#include <pclsprite.h>
#include "thread.h"
#include "simple_local.h"
#include "simple.h"

/*
   指定ファイルでキャラクタを再定義
   filename:ファイル名(拡張子なし)
*/
static unsigned char simple_picbuff[6*1024]; /*一枚絵読み込みバッファ*/

void bgload(char *filename)
{
  FILEACC fa;
  int ret;
  char fname[16];
  strncpy(fname,filename,8);
  fname[8]=0;
  strcat(fname,".psp");
  thread_lock();
  if(pceFileOpen(&fa,fname,FOMD_RD)){
    simple_error("bgloadで指定のファイル\n%s\nが見つかりません\n",fname);
    thread_unlock();
    thread_return();
  }
  if(fa.fsize!=6*1024){
    simple_error("bgloadで指定されたファイル\n%s\nのサイズが6144以外です\n",fname);
    thread_unlock();
    thread_return();
  }
  ret=pceFileReadSct(&fa,simple_picbuff     ,0,4096);
  pceFileReadSct(&fa,simple_picbuff+4096,1,2048);
  pceFileClose(&fa);
  thread_unlock();
  bgset(simple_picbuff);
}

