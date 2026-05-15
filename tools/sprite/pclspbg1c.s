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
	.global	bg_write_1c
	
bg_write_1c:
	ld.w	%r11,%r13
	xand	%r11,%r11,0x00003fff
	xld.w	%r10,0x00000018		; 24
	mlt.w	%r11,%r10
	ld.w	%r10,%r15
	ld.w	%r15,%r10
	ld.w	%r5,%alr
	add	%r15,%r5

	xld.w	%r4,0x00000006		; 6

	ld.w	%r11,%r14
	xand	%r11,%r11,0xffffffc0
	xld.w	%r10,0x0000000c		; 12
	mlt.w	%r11,%r10
	xand	%r14,%r14,0x0000003f
	xsra	%r14,1
	ld.w	%r5,%alr
	add	%r5,%r14
	ld.w	%r14,%r5
	add	%r12,%r14

	ld.w	%r10,%r13
	xand	%r10,%r10,0x00008000
	xjreq	__L339

	xadd	%r12,%r12,832

	xld.w	%r4,0x00000002		; 2

	ld.w	%r10,%r13
	xand	%r10,%r10,0x00004000
	xjreq	__L340

	xld.w	%r13,-160			; 0xffffff60
	xld.w	%r11,0x00000020		; 32
__L341:

	ld.w	%r10,[%r15]+

	mirror	%r10,%r10

	add	%r12,%r13
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8

	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r13
	xld.b	[%r12],%r10

	ld.w	%r10,[%r15]+

	mirror	%r10,%r10

	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8

	add	%r12,%r13
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r11
	xld.b	[%r12],%r10

	ld.w	%r10,[%r15]+

	mirror	%r10,%r10

	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r13
	xld.b	[%r12],%r10
	xsra	%r10,8

	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r11
	xld.b	[%r12],%r10

	xsub	%r4,%r4,1
	xjrne	__L341

	xjp	__L350
__L340:

	xld.w	%r13,-160			; 0xffffff60
	xld.w	%r11,0x00000020		; 32
__L346:

	ld.w	%r10,[%r15]+

	add	%r12,%r13
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8

	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r13
	xld.b	[%r12],%r10

	ld.w	%r10,[%r15]+

	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8

	add	%r12,%r13
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r11
	xld.b	[%r12],%r10

	ld.w	%r10,[%r15]+

	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r13
	xld.b	[%r12],%r10
	xsra	%r10,8

	add	%r12,%r11
	xld.b	[%r12],%r10
	xsra	%r10,8
	add	%r12,%r11
	xld.b	[%r12],%r10

	xsub	%r4,%r4,1
	xjrne	__L346

	xjp	__L350
__L339:

	ld.w	%r10,%r13
	xand	%r10,%r10,0x00004000
	xjreq	__L351

	xld.w	%r11,0x00000020		; 32
__L352:

	ld.w	%r10,[%r15]+

	mirror	%r10,%r10

	xld.b	[%r12],%r10
	add	%r12,%r11
	xsra	%r10,8
	xld.b	[%r12],%r10
	add	%r12,%r11
	xsra	%r10,8

	xld.b	[%r12],%r10
	add	%r12,%r11
	xsra	%r10,8
	xld.b	[%r12],%r10
	add	%r12,%r11

	xsub	%r4,%r4,1
	xjrne	__L352

	xjp	__L350
__L351:

	xld.w	%r11,0x00000020		; 32
__L357:

	ld.w	%r10,[%r15]+

	xld.b	[%r12],%r10
	add	%r12,%r11
	xsra	%r10,8
	xld.b	[%r12],%r10
	add	%r12,%r11
	xsra	%r10,8

	xld.b	[%r12],%r10
	add	%r12,%r11
	xsra	%r10,8
	xld.b	[%r12],%r10
	add	%r12,%r11

	xsub	%r4,%r4,1
	xjrne	__L357
__L350:
	ret

