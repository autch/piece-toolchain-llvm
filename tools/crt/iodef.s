// iodef.s — I/O register base address for S1C33209.
//
// iobaseptr = 0x40000 is the S1C33209 peripheral register base.
// Referenced by SDK internal routines for direct I/O access.
//
// Converted from sdk/lib/src/iodef.s (EPSON .abs/.set syntax)
// to LLVM assembler syntax.

	.globl	iobaseptr
	.set	iobaseptr, 0x40000
