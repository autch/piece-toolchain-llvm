
#include "pcecircle.h"

/*
 * pceLCDEllipse: 楕円を描く
 * 
 * in  long color 線色、0: 白、3: 黒
 * in  long xc    中心の X 座標
 * in  long yc    中心の Y 座標
 * in  long rx    X 軸方向の半径
 * in  long ry    Y 軸方向の半径
 * ret n/a
 *
 * Reference: 奥村晴彦「C 言語による最新アルゴリズム事典」
 *            1991 年、ISBN4-87408-414-1, 2400 円
 *            pp.65-66「グラフィックス」ellipse.c
 *            http://oku.edu.mie-u.ac.jp/~okumura/algo/circle.html
 *            のパッチ済み。
 */
void pceLCDEllipse(long color, long xc, long yc, long rx, long ry)
{
  int x, x1, y, y1, r;

  if(rx > ry)
  {
    x = r = rx;  y = 0;
    while(x >= y)
    {
      x1 = x * ry / rx;
      y1 = y * ry / rx;
      pceLCDPoint(color, xc + x, yc + y1);
      pceLCDPoint(color, xc + x, yc - y1);
      pceLCDPoint(color, xc - x, yc + y1);
      pceLCDPoint(color, xc - x, yc - y1);
      pceLCDPoint(color, xc + y, yc + x1);
      pceLCDPoint(color, xc + y, yc - x1);
      pceLCDPoint(color, xc - y, yc + x1);
      pceLCDPoint(color, xc - y, yc - x1);
      if((r -= (y++ << 1) + 1) <= 0)
        r += --x << 1;
    }
  }
  else
  {
    x = r = ry;  y = 0;
    while(x >= y)
    {
      x1 = x * rx / ry;
      y1 = y * rx / ry;
      pceLCDPoint(color, xc + x1, yc + y);
      pceLCDPoint(color, xc + x1, yc - y);
      pceLCDPoint(color, xc - x1, yc + y);
      pceLCDPoint(color, xc - x1, yc - y);
      pceLCDPoint(color, xc + y1, yc + x);
      pceLCDPoint(color, xc + y1, yc - x);
      pceLCDPoint(color, xc - y1, yc + x);
      pceLCDPoint(color, xc - y1, yc - x);
      if((r -= (y++ << 1) + 1) <= 0)
        r += --x << 1;
    }
  }
}
