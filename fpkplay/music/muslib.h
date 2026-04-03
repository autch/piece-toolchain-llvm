
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

/* Debug: per-channel sequencer trace; MAXSEQ = MAXCH = 26 */
extern unsigned short seqdbg_end_offset[26];
extern unsigned char  seqdbg_pops[26];
extern unsigned char  seqdbg_pushes[26];
extern unsigned char  seqdbg_reptruns[26];
extern unsigned char  seqdbg_reptexits[26];
/* Bad-SeqEnd detection */
extern unsigned short seqdbg_bad_offset[26];  /* 0xFFFF = no bad SeqEnd yet */
extern unsigned char  seqdbg_bad_np[26];
extern unsigned short seqdbg_bad_nest0[26];
extern unsigned short seqdbg_bad_nest1[26];
/* SeqRept readback */
extern unsigned char  seqdbg_rept_idx[26];
extern unsigned short seqdbg_rept_times[26][4];
extern unsigned short seqdbg_rept_lstart[26][4];
/* SeqCall readback */
extern unsigned char  seqdbg_call_idx[26];
extern unsigned short seqdbg_call_times[26][4];
extern unsigned short seqdbg_call_retaddr[26][4];
/* SeqNext tracing */
extern unsigned char  seqdbg_nexts[26];
extern unsigned char  seqdbg_next_loops[26];
extern unsigned short seqdbg_next_lstart0[26];
extern unsigned short seqdbg_next_badlst[26];
extern unsigned char  seqdbg_next_np[26];
extern unsigned short seqdbg_next_nd9[26];
extern unsigned short seqdbg_next_nd10[26];
extern unsigned char  seqdbg_rest_idx[26];
extern unsigned char  seqdbg_rest_np[26][4];
extern unsigned short seqdbg_rest_nestd[26][4];
extern volatile unsigned short *seqdbg_probe_ptr[26];
extern unsigned short seqdbg_probe_val[26];
extern unsigned short seqdbg_probe_computed[26];

#endif //ifndef _MUSLIB_H
