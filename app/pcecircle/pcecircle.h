
#if !defined(PCECIRCLE_H)
#define PCECIRCLE_H

#include <piece.h>

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
void pceLCDCircle(long color, long xc, long yc, long r);

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
void pceLCDEllipse(long color, long xc, long yc, long rx, long ry);

#endif
