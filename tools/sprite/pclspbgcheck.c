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
// Comments:
//
//  �X�v���C�g/�a�f���C�u����
//  v0.70 2001.11.26 �Ƃ肠���������A���������A���C���X�N���[��������
//                   ��{�֐��̎d�l���m��
//  v0.75 2001.12.06 pclSpriteBGGetFrameAdr�ǉ�
//  v0.90 2002.01.16 ���C���X�N���[��
//
#include "pclsprite.h"
void bg_check_in(int *frame,int *pn,int *po,int *base);

// bg_check is implemented in pclspbgcheck_asm.s. The original PIECE SDK source
// kept it as a multi-line inline asm() inside this .c file, using literal
// newlines in the asm string -- a gcc extension that clang does not accept.
// The asm body has been moved verbatim to pclspbgcheck.s and is built
// through asm33conv like the other sprite library .s files.
void bg_check(int *frame,int *new,int *old);

#if 0
// Reference C implementation kept for documentation; the actual code path
// is the assembly version in pclspbgcheck.s.
void bg_check_c(int *frame,int *new,int *old)
{
  int *pn=new,*po=old,*endcheck=new+(BG_CBUFF_SIZE/4);
  do{
/*    int w0,w1,w2,w3;
    w0=*pn++;w1=*po++;
    w2=*pn++;w3=*po++; */
    if(*pn++!=*po++) bg_check_in(frame,pn,po,new);
    if(*pn++!=*po++) bg_check_in(frame,pn,po,new);
    if(*pn++!=*po++) bg_check_in(frame,pn,po,new);
    if(*pn++!=*po++) bg_check_in(frame,pn,po,new);
  }while(pn<endcheck);
}
#endif

#if 0
asm("
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
");
#endif
