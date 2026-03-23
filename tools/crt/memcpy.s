
/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              System Programs
//
//
// PIECE LIBRARY : pceapi : Ver 1.00
//
// Copyright (C)2001 AUQAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
//
// Coded by MIO.H (OeRSTED)
//
// Comments:
//
//	○ memcpy
//
//  v1.00 2001.11.09 MIO.H
//


;*****************************************************
; memcpy
;   ワード転送・ハーフワード転送も活用し転送時の無駄を
;   無くしたルーチンです。
;   処理選択の判断は最大効率は捨て軽めとしています。
;   それでも転送が少ない場合はかえって無駄になります。(^^;
;
;   引数： %r12 転送先アドレス
;          %r13 転送元アドレス
;          %r14 転送長さ(バイト)
;
;   戻り値 : %r10 転送先アドレス
;*****************************************************

.global memcpy
memcpy:
	cmp	%r14, 0
	jreq.d	memcpy_ret
	ld.w 	%r10, %r12	; 戻り値をセット(直前はディレイド分岐)

	cmp	%r14,3
	jrule	memcpy_loop

	ld.w	%r4,%r12
	or	%r4,%r13
	and	%r4,3
	jreq	memcpy4
	and	%r4, 1
	jreq	memcpy2

memcpy_loop:
	ld.ub	%r4, [%r13]+	; 1バイトコピー
	ld.b	[%r12]+, %r4
	sub	%r14, 1		; カウンター減
	jrne	memcpy_loop

memcpy_ret:
	ret


memcpy2:
	ld.w	%r5,%r14
	srl	%r5,1

memcpy2_loop:
	ld.uh	%r4, [%r13]+	; 2バイトコピー
	ld.h	[%r12]+, %r4
	sub	%r5, 1		; カウンター減
	jrne	memcpy2_loop

	and	%r14,1		; 転送残りをチェック
	jrne	memcpy_loop	; バイトコピーへ

	ret

memcpy4:
	ld.w	%r5,%r14
	srl	%r5,2

memcpy4_loop:
	ld.w	%r4,[%r13]+	; 4バイトコピー
	ld.w	[%r12]+,%r4
	sub	%r5,1		; カウンター減
	jrne	memcpy4_loop

	and	%r14,3		; 転送残りをチェック
	jrne	memcpy_loop	; バイトコピーへ

	ret

