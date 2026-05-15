#include "thread.h"
#include "simple_local.h"
#include <piece.h>

/*
   1ms’PˆÊ‚ÌŠÔ‚ğæ“¾
*/
unsigned int msec()
{
  int rettime;
  thread_lock();
  rettime=pceTimerGetCount();
  thread_unlock();
  return rettime;
}
