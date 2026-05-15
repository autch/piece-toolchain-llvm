/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              Library Programs
//
//
// PIECE sprite library : Ver 0.70
//
// Copyright (C)2001 AUQAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
//
// Coded by Katsumasa Tsuneyoshi
//
// Comments:
//
//  スプライト/ＢＧライブラリ
//  v0.70 2001.11.26 Katsumasa Tsuneyoshi
//                   とりあえず実装、未高速化、ラインスクロール未実装
//                   基本関数の仕様を確定
//                   
	.global	write_raster0_a0
	
write_raster0_a0:
	xld.w	%r5,-1			; 0xffffffff
	srl	%r5,%r15
; 	not	%r6,%r5

	ld.w	%r11,[%r13]+
	cmp	%r13,%r14
	jrne.d	__L280
	not	%r6,%r5		; delay
	xsub	%r13,%r13,32
__L280:
; 	srl	%r11,%r15

	ld.w	%r4,[%r13]+
	cmp	%r13,%r14
	jrne.d	__L281
	srl	%r11,%r15	; delay
	xsub	%r13,%r13,32
__L281:

	rr	%r4,%r15

	ld.w	%r10,%r4
	and	%r10,%r6
	or	%r11,%r10
	ld.w	[%r12]+,%r11

; 	and	%r4,%r5		
	ld.w	%r11,[%r13]+
	cmp	%r13,%r14
	jrne.d	__L282
	and	%r4,%r5		; delay
	xsub	%r13,%r13,32
__L282:

	rr	%r11,%r15

	ld.w	%r10,%r11
	and	%r10,%r6
	or	%r4,%r10
	ld.w	[%r12]+,%r4

; 	and	%r11,%r5
	ld.w	%r4,[%r13]+
	cmp	%r13,%r14
	jrne.d	__L283
	and	%r11,%r5	; delay
	xsub	%r13,%r13,32
__L283:

	rr	%r4,%r15

	ld.w	%r10,%r4
	and	%r10,%r6
	or	%r11,%r10
	ld.w	[%r12]+,%r11

	and	%r4,%r5
	xld.w	%r11,[%r13]

	rr	%r11,%r15

	ld.w	%r10,%r11
	and	%r10,%r6
	or	%r4,%r10
	xld.w	[%r12],%r4

	ret

