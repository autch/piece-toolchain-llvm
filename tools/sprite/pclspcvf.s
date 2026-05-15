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

	.global	conv_fram

conv_fram:
	xld.w	%r14,16
	xld.w	%r15,-2815
loopx:
	xld.w	%r11,22
loopy:
	xld.ub	%r10,[%r13]
	add	%r13,16
	ld.b	[%r12]+,%r10
	xld.ub	%r10,[%r13]
	add	%r13,16
	ld.b	[%r12]+,%r10
	xld.ub	%r10,[%r13]
	add	%r13,16
	ld.b	[%r12]+,%r10
	xld.ub	%r10,[%r13]
	add	%r13,16
	ld.b	[%r12]+,%r10
	xld.ub	%r10,[%r13]
	add	%r13,16
	ld.b	[%r12]+,%r10
	xld.ub	%r10,[%r13]
	add	%r13,16
	ld.b	[%r12]+,%r10
	xld.ub	%r10,[%r13]
	add	%r13,16
	ld.b	[%r12]+,%r10
	xld.ub	%r10,[%r13]
	sub	%r11,1
	ld.b	[%r12]+,%r10
	jrne.d	loopy
	add	%r13,16		;delay

	xsub	%r14,%r14,1
	jrne.d	loopx
	add	%r13,%r15	;delay

	ret
