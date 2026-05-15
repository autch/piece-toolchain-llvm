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
	.global	write_raster0_a3

write_raster0_a3:
	xld.w	%r10,0x00000020		; 32
	ld.w	%r4,%r10
	sub	%r4,%r15

	xld.w	%r5,-1			; 0xffffffff
	sll	%r5,%r4
	not	%r6,%r5

	ld.w	%r11,[%r13]+
	cmp	%r13,%r14
	xjrne	__L298
	xsub	%r13,%r13,32
__L298:

	rl	%r11,%r4

	and	%r11,%r6
	ld.w	%r15,[%r13]+
	cmp	%r13,%r14
	xjrne	__L299
	xsub	%r13,%r13,32
__L299:

	rl	%r15,%r4

	ld.w	%r10,%r15
	and	%r10,%r5
	or	%r11,%r10
	ld.w	[%r12]+,%r11

	and	%r15,%r6
	ld.w	%r11,[%r13]+
	cmp	%r13,%r14
	xjrne	__L300
	xsub	%r13,%r13,32
__L300:

	rl	%r11,%r4

	ld.w	%r10,%r11
	and	%r10,%r5
	or	%r15,%r10
	ld.w	[%r12]+,%r15

	and	%r11,%r6
	ld.w	%r15,[%r13]+
	cmp	%r13,%r14
	xjrne	__L301
	xsub	%r13,%r13,32
__L301:

	rl	%r15,%r4

	ld.w	%r10,%r15
	and	%r10,%r5
	or	%r11,%r10
	ld.w	[%r12]+,%r11

	and	%r15,%r6
	xld.w	%r11,[%r13]

	rl	%r11,%r4

	ld.w	%r10,%r11
	and	%r10,%r5
	or	%r15,%r10
	xld.w	[%r12],%r15

	ret
