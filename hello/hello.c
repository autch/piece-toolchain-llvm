#include <piece.h>
#include <string.h>

#ifndef FROM
#define FROM "gcc33"
#endif

unsigned char vbuff[128 * 88];
int update = 0;

void pceAppInit(void)
{
  memset(vbuff, 0, sizeof vbuff);
  
  pceLCDDispStop();
  pceLCDSetBuffer(vbuff);
  pceFontSetPos(0, 0);
  pceFontPutStr("Hello, world\nfrom " FROM " toolchain");
  update = 1;
  
  pceAppSetProcPeriod(50);
  pceLCDDispStart();
}

void pceAppProc(int cnt)
{
  if(update) {
    pceLCDTrans();
    update = 0;
  }
  if(pcePadGet() & TRG_SELECT)
    pceAppReqExit(0);
}

void pceAppExit(void)
{
}
