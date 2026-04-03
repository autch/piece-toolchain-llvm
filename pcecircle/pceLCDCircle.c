
#include "pcecircle.h"

/*
 * pceLCDCircle: 真円を描く
 * 
 * in  long color 線色、0: 白、3: 黒
 * in  long xc    中心の X 座標
 * in  long yc    中心の Y 座標
 * in  long r     半径
 * ret n/a
 *
 * Reference: 奥村晴彦「C 言語による最新アルゴリズム事典」
 *            1991 年、ISBN4-87408-414-1, 2400 円
 *            pp.65「グラフィックス」circle.c
 *            http://oku.edu.mie-u.ac.jp/~okumura/algo/circle.html
 *            のパッチ済み。
 */
void pceLCDCircle(long color, long xc, long yc, long r)
{
  int x, y;

  x = r;  y = 0;
  while(x >= y)
  {
    pceLCDPoint(color, xc + x, yc + y);
    pceLCDPoint(color, xc + x, yc - y);
    pceLCDPoint(color, xc - x, yc + y);
    pceLCDPoint(color, xc - x, yc - y);
    pceLCDPoint(color, xc + y, yc + x);
    pceLCDPoint(color, xc + y, yc - x);
    pceLCDPoint(color, xc - y, yc + x);
    pceLCDPoint(color, xc - y, yc - x);
    if((r -= (y++ << 1) + 1) <= 0)
      r += --x << 1;
  }
}
