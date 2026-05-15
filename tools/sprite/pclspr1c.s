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
	.global	sp_write_1c

sp_write_1c:
	pushn	%r1
	ld.w	%r0,%r15
	ld.h	%r10,%r14
	ld.w	%r6,%r10
	xsra	%r6,8

	ld.b	%r14,%r14

	ld.w	%r11,%r0
	xand	%r11,%r11,0x00001fff
	xld.w	%r10,0x00000018		; 24
	mlt.h	%r11,%r10
	ld.w	%r5,%r13
	ld.w	%r1,%alr
	add	%r5,%r1

	ld.w	%r10,%r14
	xsra	%r10,3
	ld.w	%r11,%r6
	xsll	%r11,5
	add	%r10,%r11
	ld.w	%r4,%r12
	add	%r4,%r10

	ld.w	%r10,%r0
	xand	%r10,%r10,0x00008000
	xjreq	__L169

	xld.w	%r15,-48			; 0xffffffd0

	xadd	%r4,%r4,224

	xcmp	%r6,80
	xjrule	__L223

	cmp	%r6,0x0
	xjrlt	__L172

	xcmp	%r6,87
	xjrgt	__L168

	xld.w	%r10,0x00000058		; 88
	ld.w	%r7,%r10
	sub	%r7,%r6

	xld.w	%r10,0x00000008		; 8
	sub	%r10,%r7
	xld.w	%r11,0x00000003		; 3
	mlt.h	%r10,%r11
	ld.w	%r1,%alr
	add	%r5,%r1
	xsll	%r10,5
	sub	%r4,%r10

	xjp	__L178
__L172:

	xcmp	%r6,-7
	xjrlt	__L168

	xadd	%r7,%r6,8

	xjp	__L178
__L169:

	xld.w	%r15,0x00000010		; 16

	xcmp	%r6,80
	xjrugt	__L179

__L223:
	xld.w	%r7,0x00000008		; 8

	xjp	__L178
__L179:

	cmp	%r6,0x0
	xjrlt	__L181

	xcmp	%r6,87
	xjrgt	__L168

	xld.w	%r10,0x00000058		; 88
	ld.w	%r7,%r10
	sub	%r7,%r6

	xjp	__L178
__L181:

	xcmp	%r6,-7
	xjrlt	__L168

	xadd	%r7,%r6,8

	not	%r10,%r6
	add	%r10,1
	xld.w	%r11,0x00000003		; 3
	mlt.h	%r10,%r11
	ld.w	%r1,%alr
	add	%r5,%r1
	xsll	%r10,5
	add	%r4,%r10
__L178:


	ld.w	%r10,%r0
	xand	%r10,%r10,0x00004000
	xjreq	__L187

	xcmp	%r14,120
	xjrugt	__L188

	xsub	%r15,%r15,1

	xand	%r14,%r14,0x00000007
__L189:
	ld.ub	%r12,[%r5]+
	ld.ub	%r13,[%r5]+
	ld.ub	%r11,[%r5]+
	xoor	%r11,%r11,0xffffff00

	mirror	%r12,%r12
	mirror	%r13,%r13
	mirror	%r11,%r11

	sll	%r12,%r14
;	sll	%r13,%r14
;	rl	%r11,%r14

	xld.ub	%r10,[%r4]
	rl	%r11,%r14 ;^
	and	%r10,%r11
	or	%r10,%r12
	xld.b	[%r4],%r10
	xadd	%r4,%r4,16
	xld.ub	%r10,[%r4]
	sll	%r13,%r14 ;^
	and	%r10,%r11
	or	%r10,%r13
	xld.b	[%r4],%r10
	xsub	%r4,%r4,15

	xsrl	%r12,8
;	xsrl	%r13,8
;	xsrl	%r11,8

	xld.ub	%r10,[%r4]
	xsrl	%r11,8 ;^
	and	%r10,%r11
	or	%r10,%r12
	xld.b	[%r4],%r10
	xadd	%r4,%r4,16
	xld.ub	%r10,[%r4]
	xsrl	%r13,8 ;^
	and	%r10,%r11
	or	%r10,%r13
	xld.b	[%r4],%r10
;	add	%r4,%r15	

	sub	%r7,1
	jrne.d	__L189
	add	%r4,%r15	; delay

	xjp	__L168
__L188:

	cmp	%r14,0x0
	xjrlt	__L194

	xand	%r14,%r14,0x00000007

	xld.w	%r6,-256			; 0xffffff00
__L195:
	ld.ub	%r12,[%r5]+
	ld.ub	%r13,[%r5]+
	ld.ub	%r11,[%r5]+
	or	%r11,%r6

	mirror	%r12,%r12
	mirror	%r13,%r13
	mirror	%r11,%r11

	sll	%r12,%r14
	sll	%r13,%r14
	rl	%r11,%r14

	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r12
	xld.b	[%r4],%r10
	xadd	%r4,%r4,16
	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r13
	xld.b	[%r4],%r10
;	add	%r4,%r15	

	sub	%r7,1
	jrne.d	__L195
	add	%r4,%r15	; delay

	xjp	__L168
__L194:
	xcmp	%r14,-7
	xjrlt	__L168

	not	%r14,%r14
	add	%r14,1
	xadd	%r4,%r4,1

	xld.w	%r6,-256			; 0xffffff00
__L201:

	ld.ub	%r12,[%r5]+
	ld.ub	%r13,[%r5]+
	ld.ub	%r11,[%r5]+
	or	%r11,%r6

	mirror	%r12,%r12
	mirror	%r13,%r13
	mirror	%r11,%r11

	srl	%r12,%r14
	srl	%r13,%r14
	srl	%r11,%r14

	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r12
	xld.b	[%r4],%r10
	xadd	%r4,%r4,16
	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r13
	xld.b	[%r4],%r10
;	add	%r4,%r15	

	sub	%r7,1
	jrne.d	__L201
	add	%r4,%r15	; delay

	xjp	__L168
__L187:

	xcmp	%r14,120
	xjrugt	__L206

	xsub	%r15,%r15,1

	xand	%r14,%r14,0x00000007
__L207:

	ld.ub	%r12,[%r5]+
	ld.ub	%r13,[%r5]+
	ld.ub	%r11,[%r5]+
	xoor	%r11,%r11,0xffffff00

	sll	%r12,%r14
	sll	%r13,%r14
	rl	%r11,%r14

	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r12
	xld.b	[%r4],%r10
	xadd	%r4,%r4,16
	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r13
	xld.b	[%r4],%r10
	xsub	%r4,%r4,15

	xsrl	%r12,8
	xsrl	%r13,8
	xsrl	%r11,8

	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r12
	xld.b	[%r4],%r10
	xadd	%r4,%r4,16
	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r13
	xld.b	[%r4],%r10
;	add	%r4,%r15	

	sub	%r7,1
	jrne.d	__L207
	add	%r4,%r15	; delay

	xjp	__L168
__L206:

	cmp	%r14,0x0
	xjrlt	__L212

	xand	%r14,%r14,0x00000007

	xld.w	%r6,-256			; 0xffffff00
__L213:

	ld.ub	%r12,[%r5]+
	ld.ub	%r13,[%r5]+
	ld.ub	%r11,[%r5]+
	or	%r11,%r6

	sll	%r12,%r14
	sll	%r13,%r14
	rl	%r11,%r14

	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r12
	xld.b	[%r4],%r10
	xadd	%r4,%r4,16
	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r13
	xld.b	[%r4],%r10
;	add	%r4,%r15	

	sub	%r7,1
	jrne.d	__L213
	add	%r4,%r15	; delay

	xjp	__L168
__L212:
	xcmp	%r14,-7
	xjrlt	__L168

	not	%r14,%r14
	add	%r14,1
	xadd	%r4,%r4,1

	xld.w	%r6,-256			; 0xffffff00
__L219:

	ld.ub	%r12,[%r5]+
	ld.ub	%r13,[%r5]+
	ld.ub	%r11,[%r5]+
	or	%r11,%r6

	srl	%r12,%r14
	srl	%r13,%r14
	srl	%r11,%r14

	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r12
	xld.b	[%r4],%r10
	xadd	%r4,%r4,16
	xld.ub	%r10,[%r4]
	and	%r10,%r11
	or	%r10,%r13
	xld.b	[%r4],%r10
;	add	%r4,%r15	

	sub	%r7,1
	jrne.d	__L219
	add	%r4,%r15	; delay

__L168:
	popn	%r1
	ret
