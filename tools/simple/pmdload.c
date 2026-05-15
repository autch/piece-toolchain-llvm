#include <string.h>
#include <piece.h>
#include "thread.h"
#include "simple_local.h"
#include "simple.h"
/*
   pmd（音楽データ）を読み込み、再生する
   filename:ファイル名(拡張子なし)
*/
static unsigned char pmdbuff[8*1024]; /*pmd読み込みバッファ*/

void pmdload(char *filename)
{
  FILEACC fa;
  int ret;
  char fname[16];
  strncpy(fname,filename,8);
  fname[8]=0;
  strcat(fname,".pmd");
  thread_lock();
  if(pceFileOpen(&fa,fname,FOMD_RD)){
    simple_error("pmdloadで指定のファイル\n%s\nが見つかりません\n",fname);
    thread_unlock();
    thread_return();
  }
  if(fa.fsize>sizeof(pmdbuff)){
    simple_error("pmdloadで指定されたファイル\n%s\nのサイズが8192バイト以上で読み込めません\n",fname);
    thread_unlock();
    thread_return();
  }
  pmdstop();
  ret=pceFileReadSct(&fa,pmdbuff     ,0,4096);
  pceFileReadSct(&fa,pmdbuff+4096,1,4096);
  pceFileClose(&fa);
  thread_unlock();
  pmdplay(pmdbuff);
}
