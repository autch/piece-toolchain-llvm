#include <piece.h>
#include "simple.h"
#include "simple_local.h"
static char printformat[6]="%1d";
/*
   printfnumの表示時、指定桁で右詰します
*/
void printsiz(int keta)
{
  if((keta<1)||(keta>99))
    return;
  pcesprintf(printformat+1,"%dd",keta);
}

/*
   数値の表示
*/
void printnum(int num)
{
  pcesprintf(simple_msgbuff,printformat,num);
  printstr(simple_msgbuff);
}
