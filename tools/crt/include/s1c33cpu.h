// Core CPU control macros for the S1C33000.
//
// This file overrides sdk/include/s1c33cpu.h. The EPSON original uses the
// as33 assembler's extended-immediate mnemonics ("xand", "xoor") inside
// inline asm() statements, plus 32-bit hexadecimal immediates on plain
// "and"/"or". The LLVM S1C33 assembler accepts only standard mnemonics
// and small immediates, so each macro is rewritten to emit explicit
// "ext" prefix instructions followed by the basic 2-operand ALU form.
//
// Encoding notes (see DESIGN_SPEC / CLAUDE.md, "ext Immediate Signedness"):
//   - "and %rd, sign6" with 1 ext: combined = sign_extend_19(imm13<<6 | sign6)
//   - "or  %rd, sign6" with 1 ext: same, sign-extended
//
// Reconstructed ext values:
//   and %r9, 0xfffff0ff  (= -3841): ext 0x1FC3 ; and %r9, -1
//   and %r9, 0xffffffef  (=   -17): fits in sign6 directly
//   or  %r9, 0x100..0x700        : ext 0x04..0x1C ; or %r9, 0
//   or  %r9, 0x10                : fits in sign6 directly

#ifndef S1C33CPU_H
#define S1C33CPU_H

// Enable / disable maskable interrupts (PSR.IE bit, mask 0x10).
#define ENABLE  {asm("ld.w %r9,%psr");asm("or %r9,0x10");asm("ld.w %psr,%r9");}
#define DISABLE {asm("ld.w %r9,%psr");asm("and %r9,-17");asm("ld.w %psr,%r9");}

// Set interrupt level (PSR.IL bits 10:8). Mask out the current level
// (and with 0xfffff0ff) then OR in the desired level.
#define SET_IL0 {asm("ld.w %r9,%psr");asm("ext 0x1FC3");asm("and %r9,-1");asm("ld.w %psr,%r9");}
#define SET_IL1 {asm("ld.w %r9,%psr");asm("ext 0x1FC3");asm("and %r9,-1");asm("ext 0x04");asm("or %r9,0");asm("ld.w %psr,%r9");}
#define SET_IL2 {asm("ld.w %r9,%psr");asm("ext 0x1FC3");asm("and %r9,-1");asm("ext 0x08");asm("or %r9,0");asm("ld.w %psr,%r9");}
#define SET_IL3 {asm("ld.w %r9,%psr");asm("ext 0x1FC3");asm("and %r9,-1");asm("ext 0x0C");asm("or %r9,0");asm("ld.w %psr,%r9");}
#define SET_IL4 {asm("ld.w %r9,%psr");asm("ext 0x1FC3");asm("and %r9,-1");asm("ext 0x10");asm("or %r9,0");asm("ld.w %psr,%r9");}
#define SET_IL5 {asm("ld.w %r9,%psr");asm("ext 0x1FC3");asm("and %r9,-1");asm("ext 0x14");asm("or %r9,0");asm("ld.w %psr,%r9");}
#define SET_IL6 {asm("ld.w %r9,%psr");asm("ext 0x1FC3");asm("and %r9,-1");asm("ext 0x18");asm("or %r9,0");asm("ld.w %psr,%r9");}
#define SET_IL7 {asm("ld.w %r9,%psr");asm("ext 0x1FC3");asm("and %r9,-1");asm("ext 0x1C");asm("or %r9,0");asm("ld.w %psr,%r9");}

// Interrupt handler prologue / epilogue (used by application-level IRQs
// that do not need %alr / %ahr saved). The "2" variants also save
// %alr / %ahr for handlers that may use the MAC unit.
#define INT_BEGIN  asm("pushn %r15")
#define INT_END    asm("popn %r15\n\treti")

#define INT_BEGIN2 asm("pushn %r15\n\tld.w %r0,%alr\n\tld.w %r1,%ahr\n\tpushn %r1")
#define INT_END2   asm("popn %r1\n\tld.w %ahr,%r1\n\tld.w %alr,%r0\n\tpopn %r15\n\treti")

#endif // S1C33CPU_H
