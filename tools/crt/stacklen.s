// stacklen.s — Default stack size symbol for P/ECE applications.
//
// _stacklen = 0 means "use kernel default".  Applications that need a
// specific stack size can override this by defining _stacklen themselves
// (e.g. in the linker script: _stacklen = 0x2000;).
//
// Converted from sdk/lib/src/stacklen.s (EPSON .abs/.set syntax)
// to LLVM assembler syntax.

	.globl	_stacklen
	.set	_stacklen, 0
