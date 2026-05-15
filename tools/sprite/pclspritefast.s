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

ready:
	.half	0
	.code
	.data
	.align	2
sprmax:
	.word	-1
	.code
	.align	1

	.global	pclSpriteDispMode

pclSpriteDispMode:
	cmp	%r12,0x0
	xjrlt	__L2

	xld.w	%r10,[disp]
	xld.w	[disp],%r12

	xjp	__L3
__L2:

	xld.w	%r10,[disp]
__L3:

	ret
	.align	1
	.global	pclSpriteBGSetCharacter

pclSpriteBGSetCharacter:
	xld.h	%r10,[ready]
	cmp	%r10,0x0
	xjreq	__L4

	xcmp	%r13,31
	xjrugt	__L4
	xcmp	%r14,31
	xjrugt	__L4
	xcmp	%r12,1
	xjrugt	__L4

	cmp	%r12,0x0
	xjrne	__L8

	xld.w	%r12,[bg0a]
	xjp	__L10
__L8:

	xld.w	%r12,[bg1a]
__L10:
	ld.w	%r11,%r13
	xsll	%r11,1
	ld.w	%r10,%r14
	xsll	%r10,6
	add	%r10,%r12
	add	%r11,%r10
	xld.h	[%r11],%r15

__L4:
	ret
	.align	1
	.global	pclSpriteBGGetCharacter

pclSpriteBGGetCharacter:
	xld.h	%r10,[ready]
	cmp	%r10,0x0
	xjreq	__L14

	xcmp	%r13,31
	xjrugt	__L14
	xcmp	%r14,31
	xjrugt	__L14
	xcmp	%r12,1
	xjrule	__L13
__L14:

	xld.w	%r10,-2147483648			; 0x80000000
	xjp	__L17
__L13:

	cmp	%r12,0x0
	xjreq	__L15

	xld.w	%r12,[bg1a]
	xjp	__L18
__L15:

	xld.w	%r12,[bg0a]
__L18:
	ld.w	%r11,%r13
	xsll	%r11,1
	ld.w	%r10,%r14
	xsll	%r10,6
	add	%r10,%r12
	add	%r11,%r10
	xld.uh	%r10,[%r11]
__L17:

	ret
	.align	1
	.global	pclSpriteBGGetAdr

pclSpriteBGGetAdr:

	xld.h	%r10,[ready]
	cmp	%r10,0x0
	xjrne	__L20

	xld.w	%r10,-2147483648			; 0x80000000
	xjp	__L23
__L20:

	cmp	%r12,0x0
	xjreq	__L21

	xld.w	%r10,[bg1a]
	xjp	__L23
__L21:

	xld.w	%r10,[bg0a]
__L23:

	ret
	.align	1
	.global	pclSpriteSetCharacter

pclSpriteSetCharacter:
	ld.w	%r4,%r13

	xld.w	%r10,[sprmax]
	cmp	%r12,%r10
	xjrge	__L24
	cmp	%r12,0x0
	xjrlt	__L24

	xld.w	%r10,[sreg]
	xsll	%r12,2
	add	%r12,%r10
	ld.w	%r11,%r15
	xsll	%r11,16
	xld.w	%r13,0x000000ff		; 255
	ld.w	%r10,%r14
	and	%r10,%r13
	xsll	%r10,8
	or	%r11,%r10
	ld.w	%r10,%r4
	and	%r10,%r13
	or	%r11,%r10
	xld.w	[%r12],%r11

__L24:
	ret
	.align	1
	.global	pclSpriteBGSetCursor

pclSpriteBGSetCursor:

	xld.ub	%r10,[cur_lx]
	cmp	%r12,%r10
	xjrlt	__L28
	xld.ub	%r10,[cur_rx]
	cmp	%r12,%r10
	xjrge	__L28

	xld.b	[cursor_x],%r12
__L28:

	xld.ub	%r10,[cur_uy]
	cmp	%r13,%r10
	xjrlt	__L29
	xld.ub	%r10,[cur_dy]
	cmp	%r13,%r10
	xjrge	__L29

	xld.b	[cursor_y],%r13
__L29:

	ret
	.align	1
	.global	pclSpriteBGPutCharacter

pclSpriteBGPutCharacter:
	pushn	%r0
	ld.w	%r0,%r12

	xld.ub	%r11,[cursor_x]
	xld.ub	%r10,[cur_rx]
	cmp	%r11,%r10
	xjrult	__L31

	xld.ub	%r10,[cur_lx]
	xld.b	[cursor_x],%r10

	xld.ub	%r10,[cursor_y]
	xadd	%r10,%r10,1
	xld.b	[cursor_y],%r10

	ld.ub	%r10,%r10
	xld.ub	%r11,[cur_dy]
	cmp	%r10,%r11
	xjrult	__L31

	ld.w	%r12,0x0
	xcall	pclSpriteBGScroll

	xld.ub	%r11,[cur_lx]
	xld.ub	%r12,[cur_dy]
	xsub	%r12,%r12,1
	xsll	%r12,5
	ld.w	%r4,%r11
	add	%r4,%r12
	ld.w	%r12,%r4
	xsll	%r12,1
	xld.w	%r10,[bg0a]
	xld.ub	%r14,[cur_rx]
	sub	%r14,%r11
	add	%r10,%r12
	ld.w	%r12,%r10
	ld.w	%r13,0x0
	xsll	%r14,1
	xcall	memset

	xld.ub	%r10,[cursor_y]
	xsub	%r10,%r10,1
	xld.b	[cursor_y],%r10
__L31:

	xcmp	%r0,10
	xjrne	__L33

	xld.ub	%r10,[cur_lx]
	xld.b	[cursor_x],%r10

	xld.ub	%r10,[cursor_y]
	xadd	%r10,%r10,1
	xld.b	[cursor_y],%r10

	ld.ub	%r10,%r10
	xld.ub	%r11,[cur_dy]
	cmp	%r10,%r11
	xjrult	__L30

	ld.w	%r12,0x0
	xcall	pclSpriteBGScroll

	xld.ub	%r10,[cursor_y]
	xsub	%r10,%r10,1
	xld.b	[cursor_y],%r10

	xjp	__L30
__L33:

	xld.ub	%r13,[cursor_x]
	xld.ub	%r14,[cursor_y]
	ld.w	%r12,0x0
	ld.w	%r15,%r0
	xcall	pclSpriteBGSetCharacter

	xld.ub	%r10,[cursor_x]
	xadd	%r10,%r10,1
	xld.b	[cursor_x],%r10

__L30:
	popn	%r0
	ret
	.align	1
	.global	pclSpriteBGClear

pclSpriteBGClear:
	pushn	%r0


	xld.ub	%r0,[cur_uy]
	xjp	__L42
__L40:

	xld.ub	%r11,[cur_lx]
	ld.w	%r12,%r0
	xsll	%r12,5
	ld.w	%r15,%r11
	add	%r15,%r12
	ld.w	%r12,%r15
	xsll	%r12,1
	xld.w	%r10,[bg0a]
	xld.ub	%r14,[cur_rx]
	sub	%r14,%r11
	add	%r10,%r12
	ld.w	%r12,%r10
	ld.w	%r13,0x0
	xsll	%r14,1
	xcall	memset

	xadd	%r0,%r0,1
__L42:
	xld.ub	%r10,[cur_dy]
	cmp	%r0,%r10
	xjrlt	__L40

	popn	%r0
	ret
	.align	1
	.global	pclSpriteBGScroll

pclSpriteBGScroll:
	pushn	%r1


	xcmp	%r12,1
	xjreq	__L51
	xjrgt	__L91
	cmp	%r12,0x0
	xjreq	__L45
	xjp	__L44
__L91:
	xcmp	%r12,2
	xjreq	__L57
	xcmp	%r12,3
	xjreq	__L73
	xjp	__L44
__L45:

	xld.ub	%r0,[cur_uy]
	xjp	__L92
__L49:

	xld.ub	%r11,[cur_lx]
	ld.w	%r12,%r0
	xsll	%r12,5
	add	%r12,%r11
	xld.w	%r10,[bg0a]
	xsll	%r12,1
	add	%r12,%r10
	xld.ub	%r14,[cur_rx]
	sub	%r14,%r11
	xadd	%r13,%r12,64
	xsll	%r14,1
	xcall	memcpy

	xadd	%r0,%r0,1
__L92:
	xld.ub	%r10,[cur_dy]
	xsub	%r10,%r10,1
	cmp	%r0,%r10
	xjrlt	__L49

	xld.ub	%r11,[cur_lx]
	xld.ub	%r12,[cur_dy]
	xsub	%r12,%r12,1

	xjp	__L93
__L51:

	xld.ub	%r10,[cur_dy]
	xsub	%r0,%r10,1
	xjp	__L94
__L55:

	xld.ub	%r11,[cur_lx]
	ld.w	%r12,%r0
	xsll	%r12,5
	add	%r12,%r11
	xld.w	%r10,[bg0a]
	xsll	%r12,1
	add	%r12,%r10
	xld.ub	%r14,[cur_rx]
	sub	%r14,%r11
	xsub	%r13,%r12,64
	xsll	%r14,1
	xcall	memcpy

	xsub	%r0,%r0,1
__L94:
	xld.ub	%r10,[cur_uy]
	cmp	%r0,%r10
	xjrgt	__L55

	xld.ub	%r11,[cur_lx]
	xld.ub	%r12,[cur_uy]
__L93:
	xsll	%r12,5
	ld.w	%r1,%r11
	add	%r1,%r12
	ld.w	%r12,%r1
	xsll	%r12,1
	xld.w	%r10,[bg0a]
	xld.ub	%r14,[cur_rx]
	sub	%r14,%r11
	add	%r10,%r12
	ld.w	%r12,%r10
	ld.w	%r13,0x0
	xsll	%r14,1
	xcall	memset

	xjp	__L44
__L57:

	xld.ub	%r0,[cur_uy]
	xld.ub	%r11,[cur_dy]
	cmp	%r0,%r11
	xjrge	__L59
	xld.ub	%r7,[cur_lx]
	xld.ub	%r4,[cur_rx]
	ld.ub	%r10,%r4
	xsub	%r6,%r10,1
	xld.w	%r15,[bg0a]
	ld.w	%r5,%r11
__L61:

	ld.ub	%r12,%r7
	cmp	%r12,%r6
	xjrge	__L60
	ld.w	%r14,%r0
	xsll	%r14,5
	ld.ub	%r10,%r4
	xsub	%r13,%r10,1
__L65:

	ld.w	%r10,%r14
	add	%r10,%r12
	xsll	%r10,1
	add	%r10,%r15
	xld.uh	%r11,[%r10+2]
	xld.h	[%r10],%r11

	xadd	%r12,%r12,1
	cmp	%r12,%r13
	xjrlt	__L65

__L60:
	xadd	%r0,%r0,1
	cmp	%r0,%r5
	xjrlt	__L61
__L59:

	xld.ub	%r0,[cur_uy]
	xld.ub	%r10,[cur_dy]
	cmp	%r0,%r10
	xjrge	__L44
	xld.ub	%r11,[cur_rx]
	xld.w	%r13,[bg0a]
	ld.w	%r12,%r10
	ld.w	%r10,%r0
	xsll	%r10,5
	add	%r10,%r11
	ld.w	%r11,%r10
__L71:

	ld.w	%r10,%r11
	xsll	%r10,1
	add	%r10,%r13
	xsub	%r10,%r10,2
	ld.w	%r1,0x0
	xld.h	[%r10],%r1

	xadd	%r11,%r11,32
	xadd	%r0,%r0,1
	cmp	%r0,%r12
	xjrlt	__L71

	xjp	__L44
__L73:

	xld.ub	%r0,[cur_uy]
	xld.ub	%r10,[cur_dy]
	cmp	%r0,%r10
	xjrge	__L75
	xld.ub	%r7,[cur_rx]
	xld.ub	%r4,[cur_lx]
	ld.ub	%r6,%r4
	xld.w	%r15,[bg0a]
	ld.w	%r5,%r10
__L77:

	xsub	%r12,%r7,1
	cmp	%r12,%r6
	xjrle	__L76
	ld.w	%r14,%r0
	xsll	%r14,5
	ld.ub	%r13,%r4
__L81:

	ld.w	%r10,%r14
	add	%r10,%r12
	xsll	%r10,1
	add	%r10,%r15
	xsub	%r11,%r10,2
	xld.uh	%r11,[%r11]
	xld.h	[%r10],%r11

	xsub	%r12,%r12,1
	cmp	%r12,%r13
	xjrgt	__L81

__L76:
	xadd	%r0,%r0,1
	cmp	%r0,%r5
	xjrlt	__L77
__L75:

	xld.ub	%r0,[cur_uy]
	xld.ub	%r10,[cur_dy]
	cmp	%r0,%r10
	xjrge	__L44
	xld.ub	%r11,[cur_lx]
	xld.w	%r13,[bg0a]
	ld.w	%r12,%r10
	ld.w	%r10,%r0
	xsll	%r10,5
	add	%r10,%r11
	ld.w	%r11,%r10
__L87:

	ld.w	%r10,%r11
	xsll	%r10,1
	add	%r10,%r13
	ld.w	%r1,0x0
	xld.h	[%r10],%r1

	xadd	%r11,%r11,32
	xadd	%r0,%r0,1
	cmp	%r0,%r12
	xjrlt	__L87
__L44:

	popn	%r1
	ret
	.align	1
	.global	pclSpriteGetAdr

pclSpriteGetAdr:
	xld.w	%r10,[sreg]

	ret
	.align	1
	.global	pclSpriteGetX

pclSpriteGetX:

	xld.w	%r10,[sprmax]
	cmp	%r12,%r10
	xjrge	__L98
	cmp	%r12,0x0
	xjrge	__L97
__L98:

	xld.w	%r10,-2147483648			; 0x80000000
	xjp	__L99
__L97:

	xld.w	%r10,[sreg]
	ld.w	%r11,%r12
	xsll	%r11,2
	add	%r11,%r10
	xld.ub	%r10,[%r11]
__L99:

	ret
	.align	1
	.global	pclSpriteGetY

pclSpriteGetY:

	xld.w	%r10,[sprmax]
	cmp	%r12,%r10
	xjrge	__L102
	cmp	%r12,0x0
	xjrge	__L101
__L102:

	xld.w	%r10,-2147483648			; 0x80000000
	xjp	__L103
__L101:

	xld.w	%r10,[sreg]
	ld.w	%r11,%r12
	xsll	%r11,2
	add	%r11,%r10
	xld.ub	%r10,[%r11+1]
__L103:

	ret
	.align	1
	.global	pclSpriteGetCharacter

pclSpriteGetCharacter:

	xld.w	%r10,[sprmax]
	cmp	%r12,%r10
	xjrge	__L106
	cmp	%r12,0x0
	xjrge	__L105
__L106:

	xld.w	%r10,-2147483648			; 0x80000000
	xjp	__L107
__L105:

	xld.w	%r10,[sreg]
	ld.w	%r11,%r12
	xsll	%r11,2
	add	%r11,%r10
	xld.h	%r10,[%r11+2]
	ld.uh	%r10,%r10
__L107:

	ret
	.align	1
	.global	pclSpriteBGFlush

pclSpriteBGFlush:
	pushn	%r1

	xld.h	%r10,[ready]
	cmp	%r10,0x0
	xjreq	__L108

	xld.w	%r0,0x000000ff		; 255
	xld.w	%r1,0x00000800		; 2048
	xld.w	%r12,[bg0b]
	ld.w	%r13,%r0
	ld.w	%r14,%r1
	xcall	memset

	xld.w	%r12,[bg1b]
	ld.w	%r13,%r0
	ld.w	%r14,%r1
	xcall	memset

__L108:
	popn	%r1
	ret
	.align	1
	.global	pclSpriteInit

pclSpriteInit:
	pushn	%r2
	ld.w	%r2,%r14


	xld.w	[pclsprite_pat],%r12

	xsub	%r11,%r2,60160
	xjrge	__L111
	xsub	%r11,%r2,60157
__L111:
	xsra	%r11,2
	xld.w	[sprmax],%r11

	xld.w	[bg0a],%r13

	xld.w	%r0,0x00000800		; 2048
	ld.w	%r10,%r13
	add	%r10,%r0
	xld.w	[bg1a],%r10

	add	%r10,%r0
	xld.w	[sreg],%r10

	xsll	%r11,2
	add	%r10,%r11
	xld.w	[bg0b],%r10

	add	%r10,%r0
	xld.w	[bg1b],%r10

	add	%r10,%r0
	xld.w	[bg0f],%r10

	xld.w	%r11,0x00006000		; 24576
	add	%r10,%r11
	xld.w	[bg1f],%r10

	add	%r10,%r11
	xld.w	[fram],%r10

	ld.w	%r12,%r13
	ld.w	%r13,0x0
	ld.w	%r14,%r0
	xcall	memset

	xld.w	%r1,0x000000ff		; 255
	xld.w	%r12,[bg0b]
	ld.w	%r13,%r1
	ld.w	%r14,%r0
	xcall	memset

	xld.w	%r12,[bg1a]
	ld.w	%r13,0x0
	ld.w	%r14,%r0
	xcall	memset

	xld.w	%r12,[bg1b]
	ld.w	%r13,%r1
	ld.w	%r14,%r0
	xcall	memset

	ld.w	%r11,0x0
	xld.w	%r12,[sprmax]
	cmp	%r11,%r12
	xjrge	__L113
	xld.w	%r13,0x0000f8f8		; 63736
	xld.w	%r10,[sreg]
__L115:

	xld.w	[%r10],%r13

	xadd	%r10,%r10,4
	xadd	%r11,%r11,1
	cmp	%r11,%r12
	xjrlt	__L115
__L113:

	xld.w	%r10,0x00000007		; 7
	xld.w	[disp],%r10

	ld.w	%r15,0x0
	xld.b	[cursor_x],%r15
	xld.b	[cursor_y],%r15

	xld.b	[cur_lx],%r15
	xld.w	%r10,0x00000010		; 16
	xld.b	[cur_rx],%r10
	xld.b	[cur_uy],%r15
	xld.w	%r10,0x0000000b		; 11
	xld.b	[cur_dy],%r10

	xcmp	%r2,60160
	xjrle	__L117

	xld.w	%r10,-1			; 0xffffffff
	xld.h	[ready],%r10

	ld.w	%r10,0x0
	xjp	__L118
__L117:

	xld.w	%r10,-1			; 0xffffffff
__L118:

	popn	%r2
	ret
	.align	1
	.global	pclSpriteBGSetPosition

pclSpriteBGSetPosition:

	cmp	%r12,0x0
	xjrne	__L120

	xld.b	[bg0x],%r13

	xld.b	[bg0y],%r14

	xjp	__L121
__L120:

	xld.b	[bg1x],%r13

	xld.b	[bg1y],%r14
__L121:

	ret
	.align	1
	.global	pclSpriteBGGetX

pclSpriteBGGetX:

	cmp	%r12,0x0
	xjreq	__L123

	xld.b	%r10,[bg1x]
	xjp	__L125
__L123:

	xld.b	%r10,[bg0x]
__L125:

	ret
	.align	1
	.global	pclSpriteBGGetY

pclSpriteBGGetY:

	cmp	%r12,0x0
	xjreq	__L127

	xld.b	%r10,[bg1y]
	xjp	__L129
__L127:

	xld.b	%r10,[bg0y]
__L129:

	ret
	.align	1
	.global	pclSpriteMakeFrame

pclSpriteMakeFrame:
	pushn	%r3
	ld.w	%r3,%r12

	xld.h	%r10,[ready]
	cmp	%r10,0x0
	xjreq	__L130

	xld.w	%r12,[bg0f]
	xld.w	%r13,[bg0a]
	xld.w	%r14,[bg0b]
	xcall	bg_check

	xld.w	%r12,[bg1f]
	xld.w	%r13,[bg1a]
	xld.w	%r14,[bg1b]
	xcall	bg_check

	xld.w	%r13,[disp]
	ld.w	%r10,%r13
	xand	%r10,%r10,0x00000001
	xjreq	__L132

	xld.b	%r14,[bg0x]
	xld.b	%r15,[bg0y]
	xld.w	%r12,[fram]
	xld.w	%r13,[bg0f]
	xcall	write_bg0

	xjp	__L133
__L132:

	xld.w	%r14,0x00000058		; 88
	xld.w	%r11,[fram]

	ld.w	%r10,%r13
	xand	%r10,%r10,0x00000200
	not	%r10,%r10
	add	%r10,1
	ld.w	%r12,%r10
	xsra	%r12,31

	ld.w	%r10,%r13
	xand	%r10,%r10,0x00000100
	not	%r10,%r10
	add	%r10,1
	xsra	%r10,31
__L138:

	ld.w	[%r11]+,%r12
	ld.w	[%r11]+,%r12
	ld.w	[%r11]+,%r12
	ld.w	[%r11]+,%r12

	ld.w	[%r11]+,%r10
	ld.w	[%r11]+,%r10
	ld.w	[%r11]+,%r10
	ld.w	[%r11]+,%r10

	xsub	%r14,%r14,1
	xjrne	__L138

__L133:

	xld.w	%r10,[disp]
	xand	%r10,%r10,0x00000004
	xjreq	__L142

	xld.w	%r11,[sreg]
	xadd	%r2,%r11,2

	ld.w	%r0,0x0
	xld.w	%r10,[sprmax]
	cmp	%r0,%r10
	xjrge	__L142
	ld.w	%r1,%r11
__L146:

	xld.uh	%r15,[%r2]
	ld.w	%r10,%r15
	xand	%r10,%r10,0x00002000
	xjrne	__L147

	xld.uh	%r14,[%r1]
	ld.uh	%r15,%r15
	xld.w	%r12,[fram]
	xld.w	%r13,[pclsprite_pat]
	xcall	sp_write_1c
__L147:

	xadd	%r1,%r1,4
	xadd	%r2,%r2,4

	xadd	%r0,%r0,1
	xld.w	%r10,[sprmax]
	cmp	%r0,%r10
	xjrlt	__L146

__L142:

	xld.w	%r10,[disp]
	xand	%r10,%r10,0x00000002
	xjreq	__L149

	xld.b	%r14,[bg1x]
	xld.b	%r15,[bg1y]
	xld.w	%r12,[fram]
	xld.w	%r13,[bg1f]
	xcall	write_bg1
__L149:

	xld.w	%r10,[disp]
	xand	%r10,%r10,0x00000004
	xjreq	__L150

	xld.w	%r11,[sreg]
	xadd	%r2,%r11,2

	ld.w	%r0,0x0
	xld.w	%r10,[sprmax]
	cmp	%r0,%r10
	xjrge	__L150
	ld.w	%r1,%r11
__L154:

	xld.uh	%r15,[%r2]
	ld.w	%r10,%r15
	xand	%r10,%r10,0x00002000
	xjreq	__L155

	xld.uh	%r14,[%r1]
	ld.uh	%r15,%r15
	xld.w	%r12,[fram]
	xld.w	%r13,[pclsprite_pat]
	xcall	sp_write_1c
__L155:

	xadd	%r1,%r1,4
	xadd	%r2,%r2,4

	xadd	%r0,%r0,1
	xld.w	%r10,[sprmax]
	cmp	%r0,%r10
	xjrlt	__L154

__L150:

	ld.w	%r12,%r3
	xld.w	%r13,[fram]
	xcall	conv_fram

__L130:
	popn	%r3
	ret
	.align	1

write_bg1:
	pushn	%r3
	xsub	%sp,%sp,8
	ld.w	%r3,%r12
	ld.w	%r0,%r13

	ld.w	%r11,%r14
	xand	%r11,%r11,0x0000001f
	xld.w	[%sp],%r11
	xadd	%r11,%r0,24576
	xld.w	[%sp+4],%r11
	xld.w	%r1,0x00000058		; 88

	ld.ub	%r15,%r15
	xld.w	%r10,0x00000060		; 96
	mlt.w	%r15,%r10
	ld.w	%r10,%alr
	xadd	%r10,%r10,32
	add	%r0,%r10

	xsra	%r14,3
	xand	%r14,%r14,0x0000001c
	xsub	%r2,%r14,32

	xld.w	%r11,[%sp]
	xcmp	%r11,15
	xjrgt	__L249

	xcmp	%r11,7
	xjrgt	__L258
__L251:

	ld.w	%r13,%r2
	cmp	%r2,0x0
	xjrge	__L254
	xadd	%r13,%r2,3
__L254:
	xand	%r13,%r13,0xfffffffc
	ld.w	%r12,%r3
	ld.w	%r11,%r0
	add	%r11,%r13
	ld.w	%r13,%r11
	ld.w	%r14,%r0
	xld.w	%r15,[%sp]
	xcall	write_raster1_a0
	xadd	%r3,%r3,32
	xadd	%r0,%r0,96

	xld.w	%r11,[%sp+4]
	cmp	%r0,%r11
	xjrule	__L253
	xsub	%r0,%r0,24576

__L253:
	xsub	%r1,%r1,1
	xjrne	__L251

	xjp	__L264
__L258:

	ld.w	%r13,%r2
	cmp	%r2,0x0
	xjrge	__L261
	xadd	%r13,%r2,3
__L261:
	xand	%r13,%r13,0xfffffffc
	ld.w	%r12,%r3
	ld.w	%r11,%r0
	add	%r11,%r13
	ld.w	%r13,%r11
	ld.w	%r14,%r0
	xld.w	%r15,[%sp]
	xcall	write_raster1_a1
	xadd	%r3,%r3,32
	xadd	%r0,%r0,96

	xld.w	%r11,[%sp+4]
	cmp	%r0,%r11
	xjrule	__L260
	xsub	%r0,%r0,24576

__L260:
	xsub	%r1,%r1,1
	xjrne	__L258

	xjp	__L264
__L249:

	xld.w	%r11,[%sp]
	xcmp	%r11,23
	xjrgt	__L273
__L266:

	ld.w	%r13,%r2
	cmp	%r2,0x0
	xjrge	__L269
	xadd	%r13,%r2,3
__L269:
	xand	%r13,%r13,0xfffffffc
	ld.w	%r12,%r3
	ld.w	%r11,%r0
	add	%r11,%r13
	ld.w	%r13,%r11
	ld.w	%r14,%r0
	xld.w	%r15,[%sp]
	xcall	write_raster1_a2
	xadd	%r3,%r3,32
	xadd	%r0,%r0,96

	xld.w	%r11,[%sp+4]
	cmp	%r0,%r11
	xjrule	__L268
	xsub	%r0,%r0,24576

__L268:
	xsub	%r1,%r1,1
	xjrne	__L266

	xjp	__L264
__L273:

	ld.w	%r13,%r2
	cmp	%r13,0x0
	xjrge	__L276
	xadd	%r13,%r13,3
__L276:
	xand	%r13,%r13,0xfffffffc
	ld.w	%r12,%r3
	ld.w	%r11,%r0
	add	%r11,%r13
	ld.w	%r13,%r11
	ld.w	%r14,%r0
	xld.w	%r15,[%sp]
	xcall	write_raster1_a3
	xadd	%r3,%r3,32
	xadd	%r0,%r0,96

	xld.w	%r11,[%sp+4]
	cmp	%r0,%r11
	xjrule	__L275
	xsub	%r0,%r0,24576

__L275:
	xsub	%r1,%r1,1
	xjrne	__L273
__L264:

	xadd	%sp,%sp,8
	popn	%r3
	ret
	.align	1

write_raster0_a0:

	xld.w	%r5,-1			; 0xffffffff
	xsrl	%r5,%r15
	not	%r6,%r5

	ld.w	%r11,[%r13]+
	cmp	%r13,%r14
	xjrne	__L280
	xsub	%r13,%r13,32
__L280:
	xsrl	%r11,%r15

	ld.w	%r4,[%r13]+
	cmp	%r13,%r14
	xjrne	__L281
	xsub	%r13,%r13,32
__L281:

	rr	%r4,%r15

	ld.w	%r10,%r4
	and	%r10,%r6
	or	%r11,%r10
	ld.w	[%r12]+,%r11

	and	%r4,%r5
	ld.w	%r11,[%r13]+
	cmp	%r13,%r14
	xjrne	__L282
	xsub	%r13,%r13,32
__L282:

	rr	%r11,%r15

	ld.w	%r10,%r11
	and	%r10,%r6
	or	%r4,%r10
	ld.w	[%r12]+,%r4

	and	%r11,%r5
	ld.w	%r4,[%r13]+
	cmp	%r13,%r14
	xjrne	__L283
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
	.align	1

write_raster0_a1:

	xsub	%r15,%r15,8

	xld.w	%r5,0x00ffffff		; 16777215
	xsrl	%r5,%r15
	not	%r6,%r5

	ld.w	%r11,[%r13]+
	cmp	%r13,%r14
	xjrne	__L286
	xsub	%r13,%r13,32
__L286:
	xsrl	%r11,8
	xsrl	%r11,%r15

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
	.align	1

write_raster0_a2:

	xld.w	%r10,0x00000018		; 24
	ld.w	%r4,%r10
	sub	%r4,%r15

	xld.w	%r5,-256			; 0xffffff00
	xsll	%r5,%r4
	not	%r6,%r5

	ld.w	%r11,[%r13]+
	cmp	%r13,%r14
	xjrne	__L292
	xsub	%r13,%r13,32
__L292:

	rl	%r11,8
	rl	%r11,%r4

	and	%r11,%r6
	ld.w	%r15,[%r13]+
	cmp	%r13,%r14
	xjrne	__L293
	xsub	%r13,%r13,32
__L293:

	rl	%r15,8
	rl	%r15,%r4

	ld.w	%r10,%r15
	and	%r10,%r5
	or	%r11,%r10
	ld.w	[%r12]+,%r11

	and	%r15,%r6
	ld.w	%r11,[%r13]+
	cmp	%r13,%r14
	xjrne	__L294
	xsub	%r13,%r13,32
__L294:

	rl	%r11,8
	rl	%r11,%r4

	ld.w	%r10,%r11
	and	%r10,%r5
	or	%r15,%r10
	ld.w	[%r12]+,%r15

	and	%r11,%r6
	ld.w	%r15,[%r13]+
	cmp	%r13,%r14
	xjrne	__L295
	xsub	%r13,%r13,32
__L295:

	rl	%r15,8
	rl	%r15,%r4

	ld.w	%r10,%r15
	and	%r10,%r5
	or	%r11,%r10
	ld.w	[%r12]+,%r11

	and	%r15,%r6
	xld.w	%r11,[%r13]

	rl	%r11,8
	rl	%r11,%r4

	ld.w	%r10,%r11
	and	%r10,%r5
	or	%r15,%r10
	xld.w	[%r12],%r15

	ret
	.align	1

write_raster0_a3:

	xld.w	%r10,0x00000020		; 32
	ld.w	%r4,%r10
	sub	%r4,%r15

	xld.w	%r5,-1			; 0xffffffff
	xsll	%r5,%r4
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
	.align	1

write_bg0:
	pushn	%r3
	xsub	%sp,%sp,12
	ld.w	%r2,%r12
	ld.w	%r1,%r13

	ld.w	%r11,%r14
	xand	%r11,%r11,0x0000001f
	xld.w	[%sp],%r11
	xadd	%r11,%r1,24576
	xld.w	[%sp+4],%r11
	xld.w	%r11,0x00000058		; 88
	xld.w	[%sp+8],%r11

	ld.ub	%r15,%r15
	xld.w	%r10,0x00000060		; 96
	mlt.w	%r15,%r10
	ld.w	%r10,%alr
	xadd	%r10,%r10,32
	add	%r1,%r10

	xsra	%r14,3
	xand	%r14,%r14,0x0000001c
	xsub	%r3,%r14,32

	xld.w	%r11,[%sp]
	xcmp	%r11,15
	xjrgt	__L304

	xcmp	%r11,7
	xjrgt	__L314
__L306:

	ld.w	%r0,%r3
	cmp	%r3,0x0
	xjrge	__L309
	xadd	%r0,%r3,3
__L309:
	xand	%r0,%r0,0xfffffffc
	ld.w	%r12,%r2
	ld.w	%r13,%r1
	add	%r13,%r0
	ld.w	%r14,%r1
	xld.w	%r15,[%sp]
	xcall	write_raster0_a0
	xadd	%r2,%r2,16
	xadd	%r1,%r1,32

	ld.w	%r12,%r2
	ld.w	%r13,%r1
	add	%r13,%r0
	ld.w	%r14,%r1
	xld.w	%r15,[%sp]
	xcall	write_raster0_a0
	xadd	%r2,%r2,16
	xadd	%r1,%r1,64

	xld.w	%r11,[%sp+4]
	cmp	%r1,%r11
	xjrule	__L308
	xsub	%r1,%r1,24576

__L308:
	xld.w	%r11,[%sp+8]
	xsub	%r11,%r11,1
	xld.w	[%sp+8],%r11
	xjrne	__L306

	xjp	__L321
__L314:

	ld.w	%r0,%r3
	cmp	%r3,0x0
	xjrge	__L317
	xadd	%r0,%r3,3
__L317:
	xand	%r0,%r0,0xfffffffc
	ld.w	%r12,%r2
	ld.w	%r13,%r1
	add	%r13,%r0
	ld.w	%r14,%r1
	xld.w	%r15,[%sp]
	xcall	write_raster0_a1
	xadd	%r2,%r2,16
	xadd	%r1,%r1,32

	ld.w	%r12,%r2
	ld.w	%r13,%r1
	add	%r13,%r0
	ld.w	%r14,%r1
	xld.w	%r15,[%sp]
	xcall	write_raster0_a1
	xadd	%r2,%r2,16
	xadd	%r1,%r1,64

	xld.w	%r11,[%sp+4]
	cmp	%r1,%r11
	xjrule	__L316
	xsub	%r1,%r1,24576

__L316:
	xld.w	%r11,[%sp+8]
	xsub	%r11,%r11,1
	xld.w	[%sp+8],%r11
	xjrne	__L314

	xjp	__L321
__L304:

	xld.w	%r11,[%sp]
	xcmp	%r11,23
	xjrgt	__L331
__L323:

	ld.w	%r0,%r3
	cmp	%r3,0x0
	xjrge	__L326
	xadd	%r0,%r3,3
__L326:
	xand	%r0,%r0,0xfffffffc
	ld.w	%r12,%r2
	ld.w	%r13,%r1
	add	%r13,%r0
	ld.w	%r14,%r1
	xld.w	%r15,[%sp]
	xcall	write_raster0_a2
	xadd	%r2,%r2,16
	xadd	%r1,%r1,32

	ld.w	%r12,%r2
	ld.w	%r13,%r1
	add	%r13,%r0
	ld.w	%r14,%r1
	xld.w	%r15,[%sp]
	xcall	write_raster0_a2
	xadd	%r2,%r2,16
	xadd	%r1,%r1,64

	xld.w	%r11,[%sp+4]
	cmp	%r1,%r11
	xjrule	__L325
	xsub	%r1,%r1,24576

__L325:
	xld.w	%r11,[%sp+8]
	xsub	%r11,%r11,1
	xld.w	[%sp+8],%r11
	xjrne	__L323

	xjp	__L321
__L331:

	ld.w	%r10,%r3
	cmp	%r3,0x0
	xjrge	__L334
	xadd	%r10,%r3,3
__L334:
	ld.w	%r0,%r10
	xand	%r0,%r0,0xfffffffc
	ld.w	%r12,%r2
	ld.w	%r13,%r1
	add	%r13,%r0
	ld.w	%r14,%r1
	xld.w	%r15,[%sp]
	xcall	write_raster0_a3
	xadd	%r2,%r2,16
	xadd	%r1,%r1,32

	ld.w	%r12,%r2
	ld.w	%r13,%r1
	add	%r13,%r0
	ld.w	%r14,%r1
	xld.w	%r15,[%sp]
	xcall	write_raster0_a3
	xadd	%r2,%r2,16
	xadd	%r1,%r1,64

	xld.w	%r11,[%sp+4]
	cmp	%r1,%r11
	xjrule	__L333
	xsub	%r1,%r1,24576

__L333:
	xld.w	%r11,[%sp+8]
	xsub	%r11,%r11,1
	xld.w	[%sp+8],%r11
	xjrne	__L331
__L321:

	xadd	%sp,%sp,12
	popn	%r3
	ret

bg_check_in:
	pushn	%r3
	ld.w	%r3,%r12
	ld.w	%r2,%r15


	xsub	%r1,%r13,4
	xsub	%r0,%r14,4

	xld.uh	%r11,[%r1]

	ld.uh	%r13,%r11
	xld.uh	%r10,[%r0]
	cmp	%r13,%r10
	xjreq	__L362

	xld.h	[%r0],%r11

	ld.w	%r14,%r1
	sub	%r14,%r2
	xcall	bg_write_1c
__L362:

	xld.uh	%r11,[%r1+2]

	ld.uh	%r13,%r11
	xld.uh	%r10,[%r0+2]
	cmp	%r13,%r10
	xjreq	__L363

	xld.h	[%r0+2],%r11

	ld.w	%r14,%r1
	sub	%r14,%r2
	ld.w	%r12,%r3
	xadd	%r14,%r14,2
	xcall	bg_write_1c
__L363:

	popn	%r3
	ret
	.align	1

bg_check:
	pushn	%r3
	xsub	%sp,%sp,4
	ld.w	%r3,%r12
	ld.w	%r2,%r13

	ld.w	%r0,%r2
	ld.w	%r1,%r14
	xadd	%r4,%r0,2048
	xld.w	[%sp],%r4
__L365:

	ld.w	%r11,[%r0]+
	ld.w	%r10,[%r1]+
	cmp	%r11,%r10
	xjreq	__L368
	ld.w	%r12,%r3
	ld.w	%r13,%r0
	ld.w	%r14,%r1
	ld.w	%r15,%r2
	xcall	bg_check_in
__L368:

	ld.w	%r11,[%r0]+
	ld.w	%r10,[%r1]+
	cmp	%r11,%r10
	xjreq	__L369
	ld.w	%r12,%r3
	ld.w	%r13,%r0
	ld.w	%r14,%r1
	ld.w	%r15,%r2
	xcall	bg_check_in
__L369:

	ld.w	%r11,[%r0]+
	ld.w	%r10,[%r1]+
	cmp	%r11,%r10
	xjreq	__L370
	ld.w	%r12,%r3
	ld.w	%r13,%r0
	ld.w	%r14,%r1
	ld.w	%r15,%r2
	xcall	bg_check_in
__L370:

	ld.w	%r11,[%r0]+
	ld.w	%r10,[%r1]+
	cmp	%r11,%r10
	xjreq	__L367
	ld.w	%r12,%r3
	ld.w	%r13,%r0
	ld.w	%r14,%r1
	ld.w	%r15,%r2
	xcall	bg_check_in

__L367:
	xld.w	%r4,[%sp]
	cmp	%r0,%r4
	xjrult	__L365

	xadd	%sp,%sp,4
	popn	%r3
	ret

	.comm	pclsprite_pat 4

	.lcomm	disp 4

	.lcomm	fram 4

	.lcomm	sreg 4

	.lcomm	bg0f 4

	.lcomm	bg0a 4

	.lcomm	bg0b 4

	.lcomm	bg1f 4

	.lcomm	bg1a 4

	.lcomm	bg1b 4

	.lcomm	bg0x 1

	.lcomm	bg0y 1

	.lcomm	bg1x 1

	.lcomm	bg1y 1

	.lcomm	cursor_x 1

	.lcomm	cursor_y 1

	.lcomm	cur_lx 1

	.lcomm	cur_rx 1

	.lcomm	cur_uy 1

	.lcomm	cur_dy 1

	.endfile
