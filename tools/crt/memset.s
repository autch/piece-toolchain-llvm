
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
//	○ memset
//
//  v1.00 2001.11.09 MIO.H
//


;*****************************************************
; memset
;   ワード転送・ハーフワード転送も活用し転送時の無駄を
;   無くしたルーチンです。
;   処理選択の判断は最大効率は捨て軽めとしています。
;   それでも転送が少ない場合はかえって無駄になります。(^^;
;
;   引数： %r12 転送先アドレス
;          %r13 転送データ
;          %r14 転送長さ(バイト)
;
;   戻り値 : %r10 転送先アドレス
;*****************************************************

.global memset
memset:
	cmp	%r14, 0
	jreq.d	memset_ret
	ld.w 	%r10, %r12	; 戻り値をセット(直前はディレイド分岐)

	cmp	%r14,3
	jrule	memset_loop

	ld.w	%r4,%r12
	and	%r4,3
	jreq	memset4
	and	%r4, 1
	jreq	memset2

memset_loop:
	ld.b	[%r12]+, %r13	; 1バイト・ストア
	sub	%r14, 1		; カウンター減
	jrne	memset_loop

memset_ret:
	ret


memset2:
	ld.ub	%r13,%r13
	ld.w	%r5,%r13
	sll	%r5,8
	or	%r13,%r5

	ld.w	%r5,%r14
	srl	%r5,1

memset2_loop:
	ld.h	[%r12]+, %r13	; 2バイト・ストア
	sub	%r5, 1		; カウンター減
	jrne	memset2_loop

	and	%r14,1		; 転送残りをチェック
	jrne	memset_loop	; バイトコピーへ

	ret

memset4:
	ld.ub	%r13,%r13
	ld.w	%r5,%r13
	sll	%r5,8
	or	%r13,%r5
	swap	%r5,%r13
	or	%r13,%r5

	ld.w	%r5,%r14
	srl	%r5,2

memset4_loop:
	ld.w	[%r12]+, %r13	; 4バイト・ストア
	sub	%r5,1		; カウンター減
	jrne	memset4_loop

	and	%r14,3		; 転送残りをチェック
	jrne	memset_loop	; バイトコピーへ

	ret

