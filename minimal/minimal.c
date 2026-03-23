#include <piece.h>

void* memset(void*, int, size_t);

unsigned char vbuff[128*88];

void pceAppInit(void)
{
  memset(vbuff, 3, sizeof(vbuff));
  pceLCDDispStop();
  pceLCDSetBuffer(vbuff);
  pceLCDDispStart();
  pceLCDTrans();
  pceAppSetProcPeriod(50);
}

void pceAppProc(int cnt)
{
}

void pceAppExit(void)
{
}

