/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              Library Programs
//
//
// PIECE sprite library : Ver 0.90
//
// Copyright (C)2001 AUQAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
//
// Coded by Katsumasa Tsuneyoshi
//
// bg_check : extracted from pclspbgcheck.c's inline asm() so the LLVM
// toolchain (which does not accept gcc's multi-line literal newlines in
// asm() strings) can build it. Pass through asm33conv to expand
// x-prefixed mnemonics into ext + base instruction sequences.
//
	.global	bg_check

bg_check:
	pushn	%r3
	xsub	%sp,%sp,4
	xld.w	[%sp],%r12
	ld.w	%r2,%r13
	ld.w	%r0,%r2
	ld.w	%r1,%r14
	xadd	%r3,%r0,2048
__L2:
	ld.w	%r11,[%r0]+
	ld.w	%r10,[%r1]+
	cmp	%r11,%r10
	xjrne	__L5A
__L5:
	ld.w	%r11,[%r0]+
	ld.w	%r10,[%r1]+
	cmp	%r11,%r10
	xjrne	__L6A
__L6:
	ld.w	%r11,[%r0]+
	ld.w	%r10,[%r1]+
	cmp	%r11,%r10
	xjrne	__L7A
__L7:
	ld.w	%r11,[%r0]+
	ld.w	%r10,[%r1]+
	cmp	%r11,%r10
	xjrne	__L4A
__L4:
	cmp	%r0,%r3
	xjrult	__L2
	xadd	%sp,%sp,4
	popn	%r3
	ret
__L5A:
	xld.w	%r12,[%sp]
	ld.w	%r13,%r0
	ld.w	%r14,%r1
	ld.w	%r15,%r2
	xcall	bg_check_in
	jp	__L5

__L6A:
	xld.w	%r12,[%sp]
	ld.w	%r13,%r0
	ld.w	%r14,%r1
	ld.w	%r15,%r2
	xcall	bg_check_in
	jp	__L6

__L7A:
	xld.w	%r12,[%sp]
	ld.w	%r13,%r0
	ld.w	%r14,%r1
	ld.w	%r15,%r2
	xcall	bg_check_in
	jp	__L7

__L4A:
	xld.w	%r12,[%sp]
	ld.w	%r13,%r0
	ld.w	%r14,%r1
	ld.w	%r15,%r2
	xcall	bg_check_in
	jp	__L4
