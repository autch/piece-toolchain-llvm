// This file was automatically converted by asm33conv.py
// from: musfast.s
// on:   2026-03-28 05:41:39
// Do not edit — re-run asm33conv to regenerate.
; ----------------------------------------------------------
	.global	MakeWaveLP_fast

MakeWaveLP_fast:
	ld.w	%r7, [%r12]	; tbl = mp->pData
	ext	4
	ld.w	%r5, [%r12]	; cc = mp->freqwk

	ext	255
	ld.w	%r9, -1	; 16383

__LX16:

	ld.w	%r10,%r5	; o = ( cc >> 14 );
	srl	%r10, 8
	srl	%r10, 6

	ld.w	%r6,%r5		; x = ( cc & 0x3fff );
	and	%r6,%r9

	add	%r10,%r7
	ld.b	%r4, [%r10]	; d1 = tbl[o];
	ext	1
	ld.b	%r11, [%r10]	; d2 = tbl[o+1];

	ext	8
	ld.w	%r10, [%r12]	; cc += mp->freq
	add	%r5,%r10

	ext	12
	ld.w	%r10, [%r12]	; >= mp->loop_end
	cmp	%r5,%r10
	jrult.d	__LX19

	sub	%r11,%r4	; d2 -= d1 (ディレイド対応!!)

	ext	16
	ld.w	%r10, [%r12]	; cc -= mp->loop_w
	sub	%r5,%r10
__LX19:	

	mlt.h	%r11,%r6	; d2 += x
	ld.w	%r11,%alr

	sra	%r11, 8
	sra	%r11, 6	; d2 >>= 14

	add	%r4,%r11	; d1 += d2

	mlt.h	%r4,%r14	; d1 *= vv
	ld.w	%r4,%alr

	sra	%r4, 8	; *p++ = (d1>>=8)
	ld.uh	%r10, [%r13]
	add	%r10,%r4
	ld.h	[%r13]+,%r10

	sub	%r15,0x1	; while ( --cnt )
	jrne	__LX16

	ext	4
	ld.w	[%r12], %r5	; mp->freqwk = cc

	ret


; ----------------------------------------------------------
	.global	MakeWaveNL_fast

MakeWaveNL_fast:
	ld.w	%r7, [%r12]	; tbl = mp->pData
	ext	4
	ld.w	%r5, [%r12]	; cc = mp->freqwk

	ext	255
	ld.w	%r9, -1	; 16383

__LX216:

	ld.w	%r10,%r5	; o = ( cc >> 14 );
	srl	%r10, 8
	srl	%r10, 6

	ld.w	%r6,%r5		; x = ( cc & 0x3fff );
	and	%r6,%r9

	add	%r10,%r7
	ld.b	%r4, [%r10]	; d1 = tbl[o];
	ext	1
	ld.b	%r11, [%r10]	; d2 = tbl[o+1];

	ext	8
	ld.w	%r10, [%r12]	; cc += mp->freq
	add	%r5,%r10

	ext	12
	ld.w	%r10, [%r12]	; >= mp->loop_end
	cmp	%r5,%r10
	jrult.d	__LX219

	sub	%r11,%r4	; d2 -= d1 (ディレイド対応!!)

	ld.w	%r5, 0
	ext	8
	ld.w	[%r12], %r5	; mp->freqwk = 0
	ret

__LX219:

	mlt.h	%r11,%r6	; d2 += x
	ld.w	%r11,%alr

	sra	%r11, 8
	sra	%r11, 6	; d2 >>= 14

	add	%r4,%r11	; d1 += d2

	mlt.h	%r4,%r14	; d1 *= vv
	ld.w	%r4,%alr

	sra	%r4, 8	; *p++ = (d1>>=8)
	ld.uh	%r10, [%r13]
	add	%r10,%r4
	ld.h	[%r13]+,%r10

	sub	%r15,0x1	; while ( --cnt )
	jrne	__LX216

	ext	4
	ld.w	[%r12], %r5	; mp->freqwk = cc

	ret


; ----------------------------------------------------------
	.global	MakeWaveSQR_fast

MakeWaveSQR_fast:

	ext	4
	ld.w	%r4, [%r12]	; cc = mp->freqwk
	ext	8
	ld.uh	%r6, [%r12]	; ff = mp->freq

	sra	%r14,1		; vv >>= 1

__LX18:

	ld.h	%r11,[%r13]	; tmp = *p
	ld.h	%r10,%r4
	cmp	%r10,0
	jrlt.d	__LX21
	add	%r4,%r6		; cc += ff (ディレイド対応!!)
	sub	%r11,%r14
	jp	__LX22
__LX21:
	add	%r11,%r14
__LX22:
	ld.h	[%r13]+,%r11	; *p++ = tmp

	sub	%r15,1
	jrne	__LX18

	ext	4
	ld.w	[%r12], %r4	; mp->freqwk = cc

	ret

; ----------------------------------------------------------
	.global	MakeWaveSAW_fast

MakeWaveSAW_fast:

	ext	4
	ld.w	%r4, [%r12]	; cc = mp->freqwk
	ext	8
	ld.uh	%r6, [%r12]	; ff = mp->freq

__LX24:

	ld.h	%r11,[%r13]	; tmp = *p
	ld.h	%r10,%r4
	mlt.h	%r10,%r14
	ld.w	%r11,%alr

	sra	%r11, 8
	sra	%r11, 8

	ld.uh	%r10,[%r13]
	add	%r10,%r11
	ld.h	[%r13]+,%r10	; *p++ = tmp

	add	%r4,%r6		; cc += ff

	sub	%r15,1
	jrne	__LX24

	ext	4
	ld.w	[%r12], %r4	; mp->freqwk = cc

	ret

; ----------------------------------------------------------
	.global	MakeWaveTRI_fast

MakeWaveTRI_fast:
	ext	4
	ld.uh	%r4, [%r12]	; cc = mp->freqwk
	ext	8
	ld.uh	%r6, [%r12]

	sra	%r14,1		; vv >= 1

	ld.w	%r5,%r14
	sll	%r5, 8
	sll	%r5, 6	; vv2 = (vv<<14)
__LX25:	

	ld.h	%r10,%r4
	mlt.h	%r10,%r14
	ld.w	%r11,%alr

	cmp	%r10,0
	jrge.d	__LX28
	add	%r4,%r6		; cc += ff (ディレイド対応!!)
	not	%r11,%r11
	add	%r11,1
__LX28:
	sub	%r11,%r5
	sra	%r11, 8
	sra	%r11, 6

	ld.uh	%r10,[%r13]
	add	%r10,%r11
	ld.h	[%r13]+,%r10


	sub	%r15,1
	jrne	__LX25

	ext	4
	ld.w	[%r12], %r4	; mp->freqwk = cc

	ret

