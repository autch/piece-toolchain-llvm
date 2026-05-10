#include <piece.h>

unsigned char vbuff[128*88];

extern void* memset(void*, int, size_t);

void pceAppInit(void)
{
#ifndef NO_FILLOUT_VBUF
  unsigned char* p = vbuff;
  unsigned char* pe = vbuff + 128*88;
  while(p != pe)
    *p++ = 2;
#endif

  pceLCDDispStop();
  pceLCDSetBuffer(vbuff);
  pceLCDDispStart();
  pceLCDTrans();
  pceAppSetProcPeriod(50);
}

void pceAppProc(int cnt) { (void)cnt; }
void pceAppExit(void) {}
