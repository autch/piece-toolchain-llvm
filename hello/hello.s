	.file	"hello.c"
	.text
	.globl	pceAppInit                      ; -- Begin function pceAppInit
	.type	pceAppInit,@function
pceAppInit:                             ; @pceAppInit
; %bb.0:                                ; %entry
	pushn	%r1
	ext	vbuff@h
	ext	vbuff@m
	ld.w	%r0, vbuff@l
	ld.w	%r1, 0
	ext	176
	ld.w	%r14, 0
	ld.w	%r12, %r0
	ld.w	%r13, %r1
	call	memset
	call	pceLCDDispStop
	ld.w	%r12, %r0
	call	pceLCDSetBuffer
	ld.w	%r12, %r1
	ld.w	%r13, %r1
	call	pceFontSetPos
	ext	.L.str@h
	ext	.L.str@m
	ld.w	%r12, .L.str@l
	call	pceFontPutStr
	ext	0
	ld.w	%r12, -14
	ld.w	%r4, 1
	ext	update@h
	ext	update@m
	ld.w	%r5, update@l
	ld.w	[%r5], %r4
	call	pceAppSetProcPeriod
	call	pceLCDDispStart
	popn	%r1
	ret.d
	nop
.Lfunc_end0:
	.size	pceAppInit, .Lfunc_end0-pceAppInit
                                        ; -- End function
	.globl	pceAppProc                      ; -- Begin function pceAppProc
	.type	pceAppProc,@function
pceAppProc:                             ; @pceAppProc
; %bb.0:                                ; %entry
	pushn	%r0
	ext	update@h
	ext	update@m
	ld.w	%r0, update@l
	ld.w	%r4, [%r0]
	cmp	%r4, 0
	jreq	.LBB1_2
; %bb.1:                                ; %if.then
	call	pceLCDTrans
	ld.w	%r4, 0
	ld.w	[%r0], %r4
.LBB1_2:                                ; %if.end
	call	pcePadGet
	ext	2
	ext	0
	and	%r4, %r10
	cmp	%r4, 0
	jreq	.LBB1_4
; %bb.3:                                ; %if.then2
	ld.w	%r12, 0
	call	pceAppReqExit
.LBB1_4:                                ; %if.end3
	popn	%r0
	ret.d
	nop
.Lfunc_end1:
	.size	pceAppProc, .Lfunc_end1-pceAppProc
                                        ; -- End function
	.globl	pceAppExit                      ; -- Begin function pceAppExit
	.type	pceAppExit,@function
pceAppExit:                             ; @pceAppExit
; %bb.0:                                ; %entry
	ret.d
	nop
.Lfunc_end2:
	.size	pceAppExit, .Lfunc_end2-pceAppExit
                                        ; -- End function
	.type	update,@object                  ; @update
	.section	.bss,"aw",@nobits
	.globl	update
	.p2align	2, 0x0
update:
	.long	0                               ; 0x0
	.size	update, 4

	.type	vbuff,@object                   ; @vbuff
	.globl	vbuff
	.p2align	2, 0x0
vbuff:
	.zero	11264
	.size	vbuff, 11264

	.type	.L.str,@object                  ; @.str
	.section	.rodata.str1.4,"aMS",@progbits,1
	.p2align	2, 0x0
.L.str:
	.asciz	"Hello, world\nfrom LLVM toolchain"
	.size	.L.str, 33

	.ident	"clang version 22.1.1 (git@github.com:autch/llvm-s1c33.git bf507edbe402d6912ad70086581f26a70174e8cf)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym vbuff
