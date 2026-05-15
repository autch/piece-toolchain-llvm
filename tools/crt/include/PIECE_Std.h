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
//      ★PIECEのハードウエアのカバー
//


#ifndef	_PIECE_STD_H_
#define _PIECE_STD_H_

#include <piece.h>
#include <stdlib.h>


typedef struct{
	unsigned char	left;
	unsigned char	top;
	unsigned char	right;
	unsigned char	bottom;
}RECTP;


typedef unsigned long       DWORD;
typedef int                 BOOL;
//typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef FLOAT               *PFLOAT;

typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned int        *PUINT;

#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#endif



#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif


#define BIT4_HI(BIT8)	(((BIT8)>>4)&0x0f)
#define BIT4_LO(BIT8)	((BIT8)&0x0f)


#endif	//_PIECE_STD_H_

