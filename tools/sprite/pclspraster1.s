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

	.global	write_raster1_a0
	
write_raster1_a0:
	pushn	%r2
	ld.w	%r2,%r14

	xld.w	%r10,0x00000008		; 8
	sub	%r10,%r15
	xld.w	%r0,-16777216			; 0xff000000
	sll	%r0,%r10
; 	not	%r1,%r0

	xld.w	%r5,[%r13]
	xld.w	%r7,[%r13+32]
	xld.w	%r4,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L225
	not	%r1,%r0		; delay
	xsub	%r13,%r13,32
__L225:
	srl	%r5,%r15
	srl	%r7,%r15
; 	srl	%r4,%r15

	xld.w	%r11,[%r13]
	xld.w	%r14,[%r13+32]
	xld.w	%r6,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L226
	srl	%r4,%r15	; delay
	xsub	%r13,%r13,32
__L226:

	rr	%r11,%r15
	rr	%r14,%r15
	rr	%r6,%r15

	ld.w	%r10,%r11
	and	%r10,%r0
	or	%r5,%r10
	ld.w	%r10,%r14
	and	%r10,%r0
	or	%r7,%r10
	ld.w	%r10,%r6
	and	%r10,%r0
	or	%r4,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r4
	or	%r10,%r5
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r4
	or	%r10,%r7
	xld.w	[%r12+16],%r10

	xadd	%r12,%r12,4
	and	%r11,%r1
	and	%r14,%r1
; 	and	%r6,%r1

	xld.w	%r5,[%r13]
	xld.w	%r7,[%r13+32]
	xld.w	%r4,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L227
	and	%r6,%r1		; delay
	xsub	%r13,%r13,32
__L227:

	rr	%r5,%r15
	rr	%r7,%r15
	rr	%r4,%r15

	ld.w	%r10,%r5
	and	%r10,%r0
	or	%r11,%r10
	ld.w	%r10,%r7
	and	%r10,%r0
	or	%r14,%r10
	ld.w	%r10,%r4
	and	%r10,%r0
	or	%r6,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r6
	or	%r10,%r11
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r6
	or	%r10,%r14
	xld.w	[%r12+16],%r10

	xadd	%r12,%r12,4
	and	%r5,%r1
	and	%r7,%r1
; 	and	%r4,%r1

	xld.w	%r11,[%r13]
	xld.w	%r14,[%r13+32]
	xld.w	%r6,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L228
	and	%r4,%r1		; delay
	xsub	%r13,%r13,32
__L228:

	rr	%r11,%r15
	rr	%r14,%r15
	rr	%r6,%r15

	ld.w	%r10,%r11
	and	%r10,%r0
	or	%r5,%r10
	ld.w	%r10,%r14
	and	%r10,%r0
	or	%r7,%r10
	ld.w	%r10,%r6
	and	%r10,%r0
	or	%r4,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r4
	or	%r10,%r5
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r4
	or	%r10,%r7
	xld.w	[%r12+16],%r10

	xadd	%r12,%r12,4
	and	%r11,%r1
	and	%r14,%r1
	and	%r6,%r1

	xld.w	%r5,[%r13]
	xld.w	%r7,[%r13+32]
	xld.w	%r4,[%r13+64]

	rr	%r5,%r15
	rr	%r7,%r15
	rr	%r4,%r15

	ld.w	%r10,%r5
	and	%r10,%r0
	or	%r11,%r10
	ld.w	%r10,%r7
	and	%r10,%r0
	or	%r14,%r10
	ld.w	%r10,%r4
	and	%r10,%r0
	or	%r6,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r6
	or	%r10,%r11
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r6
	or	%r10,%r14
	xld.w	[%r12+16],%r10

	popn	%r2
	ret


	.global	write_raster1_a1
	
write_raster1_a1:
	pushn	%r2
	ld.w	%r0,%r12
	ld.w	%r2,%r14
	xsub	%r15,%r15,8

	xld.w	%r10,0x00000008		; 8
	sub	%r10,%r15
	xld.w	%r14,-65536			; 0xffff0000
	sll	%r14,%r10
; 	not	%r1,%r14

	xld.w	%r5,[%r13]
	xld.w	%r6,[%r13+32]
	xld.w	%r4,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L231
	not	%r1,%r14	; delay
	xsub	%r13,%r13,32
__L231:

	srl	%r5,8
	srl	%r5,%r15
	srl	%r6,8
	srl	%r6,%r15
	srl	%r4,8
; 	srl	%r4,%r15

	xld.w	%r11,[%r13]
	xld.w	%r12,[%r13+32]
	xld.w	%r7,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L232
	srl	%r4,%r15	; delay
	xsub	%r13,%r13,32
__L232:

	rr	%r11,8
	rr	%r11,%r15
	rr	%r12,8
	rr	%r12,%r15
	rr	%r7,8
	rr	%r7,%r15

	ld.w	%r10,%r11
	and	%r10,%r14
	or	%r5,%r10
	ld.w	%r10,%r12
	and	%r10,%r14
	or	%r6,%r10
	ld.w	%r10,%r7
	and	%r10,%r14
	or	%r4,%r10
	xld.w	%r10,[%r0]
	and	%r10,%r4
	or	%r10,%r5
	xld.w	[%r0],%r10
	xld.w	%r10,[%r0+16]
	and	%r10,%r4
	or	%r10,%r6
	xld.w	[%r0+16],%r10

	xadd	%r0,%r0,4
	and	%r11,%r1
	and	%r12,%r1
; 	and	%r7,%r1

	xld.w	%r5,[%r13]
	xld.w	%r6,[%r13+32]
	xld.w	%r4,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L233
	and	%r7,%r1		; delay
	xsub	%r13,%r13,32
__L233:

	rr	%r5,8
	rr	%r5,%r15
	rr	%r6,8
	rr	%r6,%r15
	rr	%r4,8
	rr	%r4,%r15

	ld.w	%r10,%r5
	and	%r10,%r14
	or	%r11,%r10
	ld.w	%r10,%r6
	and	%r10,%r14
	or	%r12,%r10
	ld.w	%r10,%r4
	and	%r10,%r14
	or	%r7,%r10
	xld.w	%r10,[%r0]
	and	%r10,%r7
	or	%r10,%r11
	xld.w	[%r0],%r10
	xld.w	%r10,[%r0+16]
	and	%r10,%r7
	or	%r10,%r12
	xld.w	[%r0+16],%r10

	xadd	%r0,%r0,4
	and	%r5,%r1
	and	%r6,%r1
; 	and	%r4,%r1

	xld.w	%r11,[%r13]
	xld.w	%r12,[%r13+32]
	xld.w	%r7,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L234
	and	%r4,%r1		; delay
	xsub	%r13,%r13,32
__L234:

	rr	%r11,8
	rr	%r11,%r15
	rr	%r12,8
	rr	%r12,%r15
	rr	%r7,8
	rr	%r7,%r15

	ld.w	%r10,%r11
	and	%r10,%r14
	or	%r5,%r10
	ld.w	%r10,%r12
	and	%r10,%r14
	or	%r6,%r10
	ld.w	%r10,%r7
	and	%r10,%r14
	or	%r4,%r10
	xld.w	%r10,[%r0]
	and	%r10,%r4
	or	%r10,%r5
	xld.w	[%r0],%r10
	xld.w	%r10,[%r0+16]
	and	%r10,%r4
	or	%r10,%r6
	xld.w	[%r0+16],%r10

	xadd	%r0,%r0,4
	and	%r11,%r1
	and	%r12,%r1
	and	%r7,%r1

	xld.w	%r5,[%r13]
	xld.w	%r6,[%r13+32]
	xld.w	%r4,[%r13+64]

	rr	%r5,8
	rr	%r5,%r15
	rr	%r6,8
	rr	%r6,%r15
	rr	%r4,8
	rr	%r4,%r15

	ld.w	%r10,%r5
	and	%r10,%r14
	or	%r11,%r10
	ld.w	%r10,%r6
	and	%r10,%r14
	or	%r12,%r10
	ld.w	%r10,%r4
	and	%r10,%r14
	or	%r7,%r10
	xld.w	%r10,[%r0]
	and	%r10,%r7
	or	%r10,%r11
	xld.w	[%r0],%r10
	xld.w	%r10,[%r0+16]
	and	%r10,%r7
	or	%r10,%r12
	xld.w	[%r0+16],%r10

	popn	%r2
	ret

	.global	write_raster1_a2
	
write_raster1_a2:
	pushn	%r2
	ld.w	%r2,%r14

	xld.w	%r14,0x00000018		; 24
	sub	%r14,%r15

	xld.w	%r1,-256			; 0xffffff00
	sll	%r1,%r14
; 	not	%r0,%r1

	xld.w	%r15,[%r13]
	xld.w	%r4,[%r13+32]
	xld.w	%r11,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L237
	not	%r0,%r1		; delay
	xsub	%r13,%r13,32
__L237:

	rl	%r15,8
	rl	%r15,%r14
	rl	%r4,8
	rl	%r4,%r14
	rl	%r11,8
	rl	%r11,%r14

	and	%r15,%r0
	and	%r4,%r0
; 	and	%r11,%r0

	xld.w	%r6,[%r13]
	xld.w	%r7,[%r13+32]
	xld.w	%r5,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L238
	and	%r11,%r0	; delay
	xsub	%r13,%r13,32
__L238:

	rl	%r6,8
	rl	%r6,%r14
	rl	%r7,8
	rl	%r7,%r14
	rl	%r5,8
	rl	%r5,%r14

	ld.w	%r10,%r6
	and	%r10,%r1
	or	%r15,%r10
	ld.w	%r10,%r7
	and	%r10,%r1
	or	%r4,%r10
	ld.w	%r10,%r5
	and	%r10,%r1
	or	%r11,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r11
	or	%r10,%r15
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r11
	or	%r10,%r4
	xld.w	[%r12+16],%r10

	xadd	%r12,%r12,4
	and	%r6,%r0
	and	%r7,%r0
; 	and	%r5,%r0

	xld.w	%r15,[%r13]
	xld.w	%r4,[%r13+32]
	xld.w	%r11,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L239
	and	%r5,%r0		; delay
	xsub	%r13,%r13,32
__L239:

	rl	%r15,8
	rl	%r15,%r14
	rl	%r4,8
	rl	%r4,%r14
	rl	%r11,8
	rl	%r11,%r14

	ld.w	%r10,%r15
	and	%r10,%r1
	or	%r6,%r10
	ld.w	%r10,%r4
	and	%r10,%r1
	or	%r7,%r10
	ld.w	%r10,%r11
	and	%r10,%r1
	or	%r5,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r5
	or	%r10,%r6
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r5
	or	%r10,%r7
	xld.w	[%r12+16],%r10

	xadd	%r12,%r12,4
	and	%r15,%r0
	and	%r4,%r0
; 	and	%r11,%r0

	xld.w	%r6,[%r13]
	xld.w	%r7,[%r13+32]
	xld.w	%r5,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L240
	and	%r11,%r0	; delay
	xsub	%r13,%r13,32
__L240:

	rl	%r6,8
	rl	%r6,%r14
	rl	%r7,8
	rl	%r7,%r14
	rl	%r5,8
	rl	%r5,%r14

	ld.w	%r10,%r6
	and	%r10,%r1
	or	%r15,%r10
	ld.w	%r10,%r7
	and	%r10,%r1
	or	%r4,%r10
	ld.w	%r10,%r5
	and	%r10,%r1
	or	%r11,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r11
	or	%r10,%r15
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r11
	or	%r10,%r4
	xld.w	[%r12+16],%r10

	xadd	%r12,%r12,4
	and	%r6,%r0
	and	%r7,%r0
	and	%r5,%r0

	xld.w	%r15,[%r13]
	xld.w	%r4,[%r13+32]
	xld.w	%r11,[%r13+64]

	rl	%r15,8
	rl	%r15,%r14
	rl	%r4,8
	rl	%r4,%r14
	rl	%r11,8
	rl	%r11,%r14

	ld.w	%r10,%r15
	and	%r10,%r1
	or	%r6,%r10
	ld.w	%r10,%r4
	and	%r10,%r1
	or	%r7,%r10
	ld.w	%r10,%r11
	and	%r10,%r1
	or	%r5,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r5
	or	%r10,%r6
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r5
	or	%r10,%r7
	xld.w	[%r12+16],%r10

	popn	%r2
	ret

	.global	write_raster1_a3
	
write_raster1_a3:
	pushn	%r2
	ld.w	%r2,%r14

	xld.w	%r14,0x00000020		; 32
	sub	%r14,%r15

	xld.w	%r1,-1			; 0xffffffff
	sll	%r1,%r14
; 	not	%r0,%r1

	xld.w	%r15,[%r13]
	xld.w	%r4,[%r13+32]
	xld.w	%r11,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L243
	not	%r0,%r1		; delay
	xsub	%r13,%r13,32
__L243:

	rl	%r15,%r14
	rl	%r4,%r14
	rl	%r11,%r14

	and	%r15,%r0
	and	%r4,%r0
; 	and	%r11,%r0

	xld.w	%r6,[%r13]
	xld.w	%r7,[%r13+32]
	xld.w	%r5,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L244
	and	%r11,%r0	; delay
	xsub	%r13,%r13,32
__L244:

	rl	%r6,%r14
	rl	%r7,%r14
	rl	%r5,%r14

	ld.w	%r10,%r6
	and	%r10,%r1
	or	%r15,%r10
	ld.w	%r10,%r7
	and	%r10,%r1
	or	%r4,%r10
	ld.w	%r10,%r5
	and	%r10,%r1
	or	%r11,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r11
	or	%r10,%r15
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r11
	or	%r10,%r4
	xld.w	[%r12+16],%r10

	xadd	%r12,%r12,4
	and	%r6,%r0
	and	%r7,%r0
; 	and	%r5,%r0

	xld.w	%r15,[%r13]
	xld.w	%r4,[%r13+32]
	xld.w	%r11,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L245
	and	%r5,%r0		; delay
	xsub	%r13,%r13,32
__L245:

	rl	%r15,%r14
	rl	%r4,%r14
	rl	%r11,%r14

	ld.w	%r10,%r15
	and	%r10,%r1
	or	%r6,%r10
	ld.w	%r10,%r4
	and	%r10,%r1
	or	%r7,%r10
	ld.w	%r10,%r11
	and	%r10,%r1
	or	%r5,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r5
	or	%r10,%r6
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r5
	or	%r10,%r7
	xld.w	[%r12+16],%r10

	xadd	%r12,%r12,4
	and	%r15,%r0
	and	%r4,%r0
; 	and	%r11,%r0

	xld.w	%r6,[%r13]
	xld.w	%r7,[%r13+32]
	xld.w	%r5,[%r13+64]
	xadd	%r13,%r13,4
	cmp	%r13,%r2
	jrne.d	__L246
	and	%r11,%r0	; delay
	xsub	%r13,%r13,32
__L246:

	rl	%r6,%r14
	rl	%r7,%r14
	rl	%r5,%r14

	ld.w	%r10,%r6
	and	%r10,%r1
	or	%r15,%r10
	ld.w	%r10,%r7
	and	%r10,%r1
	or	%r4,%r10
	ld.w	%r10,%r5
	and	%r10,%r1
	or	%r11,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r11
	or	%r10,%r15
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r11
	or	%r10,%r4
	xld.w	[%r12+16],%r10

	xadd	%r12,%r12,4
	and	%r6,%r0
	and	%r7,%r0
	and	%r5,%r0

	xld.w	%r15,[%r13]
	xld.w	%r4,[%r13+32]
	xld.w	%r11,[%r13+64]

	rl	%r15,%r14
	rl	%r4,%r14
	rl	%r11,%r14

	ld.w	%r10,%r15
	and	%r10,%r1
	or	%r6,%r10
	ld.w	%r10,%r4
	and	%r10,%r1
	or	%r7,%r10
	ld.w	%r10,%r11
	and	%r10,%r1
	or	%r5,%r10
	xld.w	%r10,[%r12]
	and	%r10,%r5
	or	%r10,%r6
	xld.w	[%r12],%r10
	xld.w	%r10,[%r12+16]
	and	%r10,%r5
	or	%r10,%r7
	xld.w	[%r12+16],%r10

	popn	%r2
	ret
