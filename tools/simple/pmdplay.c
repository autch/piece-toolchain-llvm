#include <piece.h>
#include <muslib.h>
#include <musdef.h>
#include "thread.h"

extern INST i_square0;
extern INST i_saw0;
extern INST i_triangle0;
extern INST i_square;
extern INST i_saw;
extern INST i_triangle;

extern INST i_BD909;
extern INST i_CYMBD;
extern INST i_HANDCLAP;
extern INST i_HC909;
extern INST i_HO909;
extern INST i_SD909;
extern INST i_SDGATE;
extern INST i_TOMH1;
extern INST i_TOML1;
extern INST i_TOMM1;

INST *inst[] = {
	&i_square0,		// 0
	&i_saw0,		// 1
	&i_triangle0,	// 2
	&i_square,		// 3
	&i_saw,			// 4
	&i_triangle,	// 5
	&i_BD909, // 6  bdr
	&i_SDGATE, // 7  sdr
	&i_SD909, // 8  rim
	&i_HO909, // 9  ohh
	&i_HC909, // 10 chh
	&i_CYMBD, // 11 ccy
	&i_CYMBD, // 12 rcy
	&i_TOMH1, // 13 htm
	&i_TOMM1, // 14 mtm
	&i_TOML1, // 15 ltm
	&i_HANDCLAP, // 16 hcp
};

static char initflag=0;
/*
   pmd（音楽データ）を停止する
*/
void pmdstop()
{
  thread_lock();
  pceWaveAbort(0);
  thread_unlock();
}

/*
   pmd（音楽データ）を再生する
   pmdptr:配列の名前
*/
void pmdplay(unsigned char *pmdptr)
{
  if(initflag==0){
    thread_lock();
    InitMusic();
    pceWaveSetChAtt(0,0);
    initflag=1;
    thread_unlock();
  }
  pmdstop();
  thread_lock();
  PlayMusic(pmdptr);
  thread_unlock();
}

