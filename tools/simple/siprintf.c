//  v0.93 2002.02.04 siprintf ソース分割時のbug fix

#include <smcvals.h> //add 02.04
#include <stdarg.h>  //add 02.04
#include <piece.h>
#include "simple.h"
#include "simple_local.h"
/*
   simpleライブラリ用printf
*/
void siprintf(const char *format,...)
{
  va_list arglist;
  va_start( arglist, format );
  pcevsprintf( simple_msgbuff, format, arglist );
  va_end( arglist );
  printstr(simple_msgbuff);
}
