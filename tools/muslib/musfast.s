; ----------------------------------------------------------
	.global	MakeWaveLP_fast

MakeWaveLP_fast:
	xld.w	%r7,[%r12]	; tbl = mp->pData
	xld.w	%r5,[%r12+4]	; cc = mp->freqwk

	xld.w	%r9,0x00003fff		; 16383

__LX16:

	ld.w	%r10,%r5	; o = ( cc >> 14 );
	xsrl	%r10,14

	ld.w	%r6,%r5		; x = ( cc & 0x3fff );
	and	%r6,%r9

	add	%r10,%r7
	xld.b	%r4,[%r10]	; d1 = tbl[o];
	xld.b	%r11,[%r10+1]	; d2 = tbl[o+1];

	xld.w	%r10,[%r12+8]	; cc += mp->freq
	add	%r5,%r10

	xld.w	%r10,[%r12+12]	; >= mp->loop_end
	cmp	%r5,%r10
	jrult.d	__LX19

	sub	%r11,%r4	; d2 -= d1 (ディレイド対応!!)

	xld.w	%r10,[%r12+16]	; cc -= mp->loop_w
	sub	%r5,%r10
__LX19:	

	mlt.h	%r11,%r6	; d2 += x
	ld.w	%r11,%alr

	xsra	%r11,14		; d2 >>= 14

	add	%r4,%r11	; d1 += d2

	mlt.h	%r4,%r14	; d1 *= vv
	ld.w	%r4,%alr

	xsra	%r4,8		; *p++ = (d1>>=8)
	xld.uh	%r10,[%r13]
	add	%r10,%r4
	ld.h	[%r13]+,%r10

	sub	%r15,0x1	; while ( --cnt )
	jrne	__LX16

	xld.w	[%r12+4],%r5	; mp->freqwk = cc

	ret


; ----------------------------------------------------------
	.global	MakeWaveNL_fast

MakeWaveNL_fast:
	xld.w	%r7,[%r12]	; tbl = mp->pData
	xld.w	%r5,[%r12+4]	; cc = mp->freqwk

	xld.w	%r9,0x00003fff		; 16383

__LX216:

	ld.w	%r10,%r5	; o = ( cc >> 14 );
	xsrl	%r10,14

	ld.w	%r6,%r5		; x = ( cc & 0x3fff );
	and	%r6,%r9

	add	%r10,%r7
	xld.b	%r4,[%r10]	; d1 = tbl[o];
	xld.b	%r11,[%r10+1]	; d2 = tbl[o+1];

	xld.w	%r10,[%r12+8]	; cc += mp->freq
	add	%r5,%r10

	xld.w	%r10,[%r12+12]	; >= mp->loop_end
	cmp	%r5,%r10
	jrult.d	__LX219

	sub	%r11,%r4	; d2 -= d1 (ディレイド対応!!)

	xld.w	%r5,0
	xld.w	[%r12+8],%r5	; mp->freqwk = 0
	ret

__LX219:

	mlt.h	%r11,%r6	; d2 += x
	ld.w	%r11,%alr

	xsra	%r11,14		; d2 >>= 14

	add	%r4,%r11	; d1 += d2

	mlt.h	%r4,%r14	; d1 *= vv
	ld.w	%r4,%alr

	xsra	%r4,8		; *p++ = (d1>>=8)
	xld.uh	%r10,[%r13]
	add	%r10,%r4
	ld.h	[%r13]+,%r10

	sub	%r15,0x1	; while ( --cnt )
	jrne	__LX216

	xld.w	[%r12+4],%r5	; mp->freqwk = cc

	ret


; ----------------------------------------------------------
	.global	MakeWaveSQR_fast

MakeWaveSQR_fast:

	xld.w	%r4,[%r12+4]	; cc = mp->freqwk
	xld.uh	%r6,[%r12+8]	; ff = mp->freq

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

	xld.w	[%r12+4],%r4	; mp->freqwk = cc

	ret

; ----------------------------------------------------------
	.global	MakeWaveSAW_fast

MakeWaveSAW_fast:

	xld.w	%r4,[%r12+4]	; cc = mp->freqwk
	xld.uh	%r6,[%r12+8]	; ff = mp->freq

__LX24:

	ld.h	%r11,[%r13]	; tmp = *p
	ld.h	%r10,%r4
	mlt.h	%r10,%r14
	ld.w	%r11,%alr

	xsra	%r11,16

	ld.uh	%r10,[%r13]
	add	%r10,%r11
	ld.h	[%r13]+,%r10	; *p++ = tmp

	add	%r4,%r6		; cc += ff

	sub	%r15,1
	jrne	__LX24

	xld.w	[%r12+4],%r4	; mp->freqwk = cc

	ret

; ----------------------------------------------------------
	.global	MakeWaveTRI_fast

MakeWaveTRI_fast:
	xld.uh	%r4,[%r12+4]	; cc = mp->freqwk
	xld.uh	%r6,[%r12+8]

	sra	%r14,1		; vv >= 1

	ld.w	%r5,%r14
	xsla	%r5,14		; vv2 = (vv<<14)
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
	xsra	%r11,14

	ld.uh	%r10,[%r13]
	add	%r10,%r11
	ld.h	[%r13]+,%r10


	sub	%r15,1
	jrne	__LX25

	xld.w	[%r12+4],%r4	; mp->freqwk = cc

	ret

	.endfile
