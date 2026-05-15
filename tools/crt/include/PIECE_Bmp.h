
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
//      ★描画管理ライブラリ ヘッダ
//

#ifndef	_PIECE_BMP_H_
#define _PIECE_BMP_H_

#include <PIECE_Std.h>

//
//

#define	SCREEN_W	128	//バックバッファ Xサイズ
#define	SCREEN_H	88	//バックバッファ Yサイズ

typedef struct{	//8BIT BMP(下位２BITのみ有効)
	short	w;
	short	h;
	BYTE	*buf;	//1BYTE １ピクセル
}PIECE_VRAM;

//
//

typedef struct {
	DWORD	head;		//	HEADER   'PBMP'
	DWORD	fsize;		//	ファイル長 （BYTE単位）
	BYTE	bpp;		//	bit深度  （2）
	BYTE	mask;		//	マスクのbit深度  （1）
	short	w;			//	X幅		４ピクセル単位厳守
	short	h;			//	Y高さ		
	DWORD	buf_size;	//	BMPサイズ	（BYTE単位）
}PBMP_FILEHEADER;

//
/*
typedef struct {
	unsigned char	b;
	unsigned char	g;
	unsigned char	r;
}RGB24;



typedef struct{	//FullカラーBMP構造体（24BIT データ）
	PBMP_FILEHEADER	header;
	RGB24			*buf;	//３char(24BIT) １ピクセル
}BMP_F;

//
*/
typedef struct{	//2BIT BMP + 1BIT MASK
	PBMP_FILEHEADER	header;
	BYTE			*buf;	//2BIT １ピクセル
	BYTE			*mask;	//1BIT １ピクセル
}PIECE_BMP;

extern BOOL PBM_CreateVram( PIECE_BMP *pbmp, PBMP_FILEHEADER *pbhead );
extern BOOL PBM_ReleaseVram( PIECE_BMP *pbmp );
extern BOOL PBM_Load_2B( PIECE_BMP *pbmp, char *fname );
extern BOOL PBM_Release_2B( PIECE_BMP *pbmp );

#endif	//_PIECE_BMP_H_

