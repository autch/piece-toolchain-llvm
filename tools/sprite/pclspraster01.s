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
	.global	write_raster0_a1

write_raster0_a1:
	xsub	%r15,%r15,8

	xld.w	%r5,0x00ffffff		; 16777215
	srl	%r5,%r15
	not	%r6,%r5

	ld.w	%r11,[%r13]+
	cmp	%r13,%r14
	xjrne	__L286
	xsub	%r13,%r13,32
__L286:
	srl	%r11,8
	srl	%r11,%r15

	ld.w	%r4,[%r13]+
	cmp	%r13,%r14
	xjrne	__L287
	xsub	%r13,%r13,32
__L287:

	rr	%r4,8
	rr	%r4,%r15

	ld.w	%r10,%r4
	and	%r10,%r6
	or	%r11,%r10
	ld.w	[%r12]+,%r11

	and	%r4,%r5
	ld.w	%r11,[%r13]+
	cmp	%r13,%r14
	xjrne	__L288
	xsub	%r13,%r13,32
__L288:

	rr	%r11,8
	rr	%r11,%r15

	ld.w	%r10,%r11
	and	%r10,%r6
	or	%r4,%r10
	ld.w	[%r12]+,%r4

	and	%r11,%r5
	ld.w	%r4,[%r13]+
	cmp	%r13,%r14
	xjrne	__L289
	xsub	%r13,%r13,32
__L289:

	rr	%r4,8
	rr	%r4,%r15

	ld.w	%r10,%r4
	and	%r10,%r6
	or	%r11,%r10
	ld.w	[%r12]+,%r11

	and	%r4,%r5
	xld.w	%r11,[%r13]

	rr	%r11,8
	rr	%r11,%r15

	ld.w	%r10,%r11
	and	%r10,%r6
	or	%r4,%r10
	xld.w	[%r12],%r4

	ret

