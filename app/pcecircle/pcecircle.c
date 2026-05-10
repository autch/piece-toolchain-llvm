/* -*- mode: c; mode: abbrev; mode: auto-fill; -*- */

/*
 * pceLCD(Circle|Ellipse) のお試し
 * by Autch, 2006.10.19 *public domain*
 *
 * 起動後 A でランダムな色とサイズの真円描画、
 *        B でランダムな色とサイズの楕円描画、
 *        SELECT で画面消去
 */

#include <piece.h>
#include <stdlib.h>
#include <string.h>
#include "pcecircle.h"

static unsigned char vScreen[128 * 88];
int g_nDirty; /* ダーティフラグ */

#define MarkDirty() (g_nDirty = 1)   /* Refresh() が必要 */
#define ClearDirty() (g_nDirty = 0)  /* Refresh() 完了 */

int rnd(int n) { return rand() % n; }

/* 描画したときだけイメージ転送 */
void Refresh()
{
  if(!g_nDirty) return;
  pceLCDTrans();
  ClearDirty();
}

/* イメージ消去 */
void Cls()
{
  memset(vScreen, 0, 128 * 88);
  MarkDirty();
}

void pceAppInit()
{
  g_nDirty = 1;
  pceLCDDispStop();
  pceLCDSetBuffer(vScreen);
  pceAppSetProcPeriod(20);
  Cls();

  srand(pceTimerGetCount());

  Refresh();
  pceLCDDispStart();
}

void pceAppProc(int cnt)
{
  if(pcePadGet() & TRG_A)
  {
    /*
     * rnd(3) + 1: 白地に白い円を描くのを防ぐ
     * rnd(88 / 2): 半径を 44 までに制限すれば必ず画面内に見える
     */
    pceLCDCircle(rnd(3) + 1, rnd(128), rnd(88), rnd(88 / 2));
    MarkDirty();
  }
  if(pcePadGet() & TRG_B)
  {
    pceLCDEllipse(rnd(3) + 1, rnd(128), rnd(88), rnd(128 / 2), rnd(88 / 2));
    MarkDirty();
  }
  if(pcePadGet() & TRG_D)
    Cls();

  Refresh();
}

void pceAppExit()
{
}
