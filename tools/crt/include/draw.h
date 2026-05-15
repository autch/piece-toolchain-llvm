
/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              System Programs
//
//
// PIECE KERNEL : 
//
// Copyright (C)2001 AUQAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
//
// Coded by Miyakusa Masakazu (AQUAPLUS)
//
// Comments:
//
//  v1.19 2003.02.14 N.SAWA PDW_Draw*()補助関数群を削除
//

#ifndef	_PIECE_DRAW_H_
#define _PIECE_DRAW_H_

#include <PIECE_Std.h>
#include <PIECE_Bmp.h>

#define	DISP_X		(128)
#define	DISP_Y		(88)

#define TYPE_1B		0	// 1BIT
#define TYPE_2B		1	// 2BIT

#define DRW_PARAM	0x3f	// パラメータマスク
#define DRW_REV		0xc0	// 反転ビットマスク

//2BIT時のパラメータ
#define DRW_NOMAL	0		// ベタ転送
#define DRW_ADD		1		// 飽和加算転送
#define DRW_SUB		2		// 飽和減算転送
#define DRW_HIGH	3		// HIGH 転送
#define DRW_LOW		4		// LOW 転送
#define DRW_NOT		5		// NOT 転送

#define DRW_OR		6		// OR 転送(未対応)
#define DRW_AND		7		// AND 転送(未対応)
#define DRW_XOR		8		// XOR 転送(未対応)

#define DRW_HALF	9		// 暗く（半調）
#define DRW_LIGHT	10		// 明るく（明半調）

// 1BIT時のパラメータ、色情報
#define DRW_CLR(C1,C2)	(((C1)&0x07)|(((C2)&0x07)<<3))	//C1:1の色  C2:0の色

#define COLOR_BLACK		0x03	//黒
#define COLOR_GLAY_B	0x02	//灰（濃）
#define COLOR_GLAY_W	0x01	//灰（淡）
#define COLOR_WHITE		0x00	//白
#define COLOR_MASK		0x04	//マスクとして扱う



#define DRW_REVX	0x80	// 横反転フラグ
#define DRW_REVY	0x40	// 縦反転フラグ

#define TYPE_BMP	0	// BMP描画
#define TYPE_PNT	1	// 塗りつぶし
#define TYPE_LIN	2	// ライン描画
#define TYPE_PIX	3	// ピクセル描画
#define TYPE_TXT	4	// テキスト描画

typedef struct{
	PIECE_VRAM	*dest;
	short	dx,  dy;	//描画開始座標（左上座標）
	short	dw,  dh;	//描画幅

	PIECE_BMP	*src;
	short	sx,  sy;	//画像開始座標（左上座標）

	RECTP	clip;		//クリップ範囲（画面端の場合は、NULL）

	BYTE	disp;		//描画フラグ
	BYTE	param;		//転送パラメータ
			// 下位４BIT
			//	(0)DRW_NOMAL : ベタ転送～(15)未設定まで
			// 上位４BIT
			// 0BIT 横反転フラグ
			// 1BIT 縦反転フラグ
	BYTE	type;		//描画タイプ
			//	(0)TYPE_BMP : BMP描画
	BYTE	layer;		//転送順番
}DRAW_OBJECT;

#endif	//_PIECE_DRAW_H_

extern void pceLCDPoint(long color, long x, long y);
extern void pceLCDLine(long color, long x1, long y1, long x2, long y2);
extern void pceLCDPaint(long color, long x1, long y1, long x2, long y2);
extern void pceLCDSetObject(DRAW_OBJECT *obj, PIECE_BMP *src, int dx, int dy, int sx, int sy, int w, int h, int param );
extern int pceLCDDrawObject(DRAW_OBJECT dobj );

//2003.02.14 N.SAWA 削除しました。
//extern BOOL PDW_Draw_1B_NOMAL( PIECE_VRAM *dest, int dx, int dy, int dw, int dh, PIECE_BMP *src, int sx, int sy, int param, int cl1, int cl2 );
//extern BOOL PDW_Draw_2B_NOMAL( PIECE_VRAM *dest, int dx, int dy, int dw, int dh, PIECE_BMP *src, int sx, int sy, int param );
//extern BOOL PDW_Draw_2B_ADD( PIECE_VRAM *dest, int dx, int dy, int dw, int dh, PIECE_BMP *src, int sx, int sy, int param );
//extern BOOL PDW_Draw_2B_SUB( PIECE_VRAM *dest, int dx, int dy, int dw, int dh, PIECE_BMP *src, int sx, int sy, int param );
//extern BOOL PDW_Draw_2B_NOT( PIECE_VRAM *dest, int dx, int dy, int dw, int dh, PIECE_BMP *src, int sx, int sy, int param );
//extern BOOL PDW_Draw_2B_HIGH( PIECE_VRAM *dest, int dx, int dy, int dw, int dh, PIECE_BMP *src, int sx, int sy, int param );
//extern BOOL PDW_Draw_2B_LOW( PIECE_VRAM *dest, int dx, int dy, int dw, int dh, PIECE_BMP *src, int sx, int sy, int param );
