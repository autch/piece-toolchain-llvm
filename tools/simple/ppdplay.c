#include  <piece.h>
#include "thread.h"

/*
   ÉTÉEÉìÉhçƒê∂
*/
static PCEWAVEINFO wi;

void ppdplay(const unsigned char *wp)
{
  thread_lock();
  pceWaveAbort(1);
  pceWaveSetChAtt(1,0);
  wi=*((PCEWAVEINFO*)(wp+8));
  wi.pData=wp+8+sizeof(PCEWAVEINFO);
  pceWaveDataOut(1,&wi);
  thread_unlock();
}

