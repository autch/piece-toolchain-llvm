
/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              System Programs
//
//
// PIECE LIBRARY : muslib : Ver 1.00
//
// Copyright (C)2001 AUQAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
//
// Coded by MIO.H (OeRSTED)
//
// Comments:
//
// PIECE �W�� ���y�h���C�o�[
//
// �A�v���P�[�V�����p�w�b�_�[�t�@�C��
//
//  v1.00 2001.11.09 MIO.H
//



#ifndef _MUSLIB_H
#define _MUSLIB_H

void InitMusic( void );
void PlayMusic( unsigned char *seq );
void StopMusic( void );
int MusicCheck( void );
extern char *title;
extern char *title2;

extern unsigned char music_wch;

#endif //ifndef _MUSLIB_H
