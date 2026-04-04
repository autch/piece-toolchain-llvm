# S1C33000 LLVM Backend — Copilot Instructions

This repository implements an LLVM backend for the EPSON S1C33000 32-bit RISC CPU, targeting the Aquaplus P/ECE (S1C33209 SoC). The goal is to produce binaries compatible with the existing P/ECE SDK libraries and kernel APIs.

## Critical Reading

**Before making any changes, read these in order:**

1. **DESIGN_SPEC.md** — Complete architecture specification, ABI details, design decisions, and implementation phasing
2. **CLAUDE.md** — Quick reference for key architecture facts, common pitfalls, and coding conventions
3. **docs/errata.md** — Hardware bugs, compiler bugs, and library bugs that affect implementation

**Do not deviate from design decisions in DESIGN_SPEC.md without explicit discussion.**

## Build Commands

The project uses CMake + Ninja to build LLVM with the S1C33 backend.

### Initial LLVM build

```bash
cd llvm
cmake -G Ninja ../llvm/llvm \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="S1C33" \
  -DLLVM_DEFAULT_TARGET_TRIPLE="s1c33-none-elf" \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_INSTALL_UTILS=ON \
  -DLLVM_USE_LINKER=lld \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

ninja -C ../build
```

### Incremental build after backend changes

```bash
ninja -C build
```

### Building example applications

Example projects (hello/, minimal/, fpkplay/, mini_nocrt/) use Makefiles that invoke the built toolchain:

```bash
cd hello
make              # Build hello_l.pex
make hello.s      # Generate assembly for inspection
make clean
```

## Test Commands

### Run all S1C33 backend tests

```bash
cd build
ninja check-llvm-codegen-s1c33
```

Or using lit directly:

```bash
./bin/llvm-lit -v ../llvm/llvm/test/CodeGen/S1C33/
```

### Run a single test

```bash
./bin/llvm-lit -v ../llvm/llvm/test/CodeGen/S1C33/varargs.ll
```

### Manual testing with llc

```bash
./bin/llc -march=s1c33 -filetype=asm input.ll -o output.s
./bin/llc -march=s1c33 -filetype=obj input.ll -o output.o
```

### Manual compilation with clang

```bash
./bin/clang --target=s1c33-none-elf --sysroot=../sysroot/s1c33-none-elf \
  -O2 -c test.c -o test.o
```

## Architecture Overview

### Target Triple and Naming

- **Target triple**: `s1c33-none-elf`
- **Backend directory**: `llvm/llvm/lib/Target/S1C33/`
- **Class prefix**: `S1C33` (e.g., `S1C33TargetMachine`, `S1C33InstrInfo`)
- **Tests**: `llvm/llvm/test/CodeGen/S1C33/`

### Hardware Characteristics

- **Instruction length**: 16-bit fixed (no Thumb-like variable length)
- **Registers**: 16 × 32-bit (R0–R15)
- **Address space**: 28-bit (256MB), little-endian
- **Instruction extension**: `ext` prefix instruction extends immediates (max 2× ext before target instruction)
- **Branches**: Delayed (1-cycle delay slot with strict constraints)
- **Division**: No hardware divider; uses step-division sequence or library call
- **Multiplication**: Optional hardware multiplier + MAC unit (S1C33209)

### ABI (S5U1C33000C — Critical: NOT S5U1C33001C)

Two ABIs exist for this CPU family. **We use S5U1C33000C** (gcc33 ABI), which the P/ECE SDK uses. The S5U1C33001C ABI is incompatible.

| Purpose | Registers |
|---------|-----------|
| Arguments | R12 → R13 → R14 → R15 (overflow to stack) |
| Return value | R10 (R10+R11 for 64-bit) |
| Callee-saved | R0–R3 |
| Scratch | R4–R7 |
| Reserved | R8 (GP), R9 (reserved — NOT scratch in our design) |
| Struct return | Implicit pointer in R12 (sret) |
| Frame pointer | None (SP-based frames) |

**Variadic functions**: All arguments go on stack (R12–R15 NOT used). This is because SDK's stdarg.h requires contiguous stack layout.

**Double arguments**: If only 1 register remains, both halves go to stack (no split across register/stack boundary).

### Code Generation Strategy

1. **ext instruction generation**: Pessimistic-then-relax. Codegen emits maximum-size sequences (ext+ext+op), then MC-layer relaxation shrinks them.

2. **GP register (R8)**: MIPS-style approach. R8 is Reserved in register allocation. GlobalAddress lowering generates `[R8 + offset]` when offset fits 26 bits.

3. **R9 is NOT implicit scratch**: Unlike EPSON's ext33 tool, we use the register allocator to pick any available register for address materialization. Safer for interrupt handling.

4. **Delay slot filling**: Conservative. Only safe instructions fill slots; when in doubt, emit `nop`.

5. **Interrupt handlers**: Use `__attribute__((interrupt_handler))` → generates `pushn %r15` / `popn %r15` / `reti`.

### TableGen Organization

- **S1C33.td** — Target definition, includes all other .td files
- **S1C33RegisterInfo.td** — R0–R15, PSR, SP, PC definitions
- **S1C33InstrFormats.td** — Instruction format classes (16-bit encoding patterns)
- **S1C33InstrInfo.td** — Instruction definitions, pseudo-instructions
- **S1C33CallingConv.td** — Argument/return value conventions (CCIfType, CCAssignToReg, etc.)

### MC Layer (Assembly & Object Files)

- **S1C33MCCodeEmitter.cpp** — Encodes instructions to 16-bit binary
- **S1C33AsmBackend.cpp** — MC relaxation: ext+ext+op → ext+op or op (when immediate fits)
- **S1C33ELFObjectWriter.cpp** — ELF relocation generation
- **S1C33FixupKinds.h** — Relocation types (REL21, REL_H/M/L, ABS21, etc.)

### Key Passes

| Pass | Purpose | File |
|------|---------|------|
| ISelDAGToDAG | Instruction selection, shift>8 splitting | S1C33ISelDAGToDAG.cpp |
| FrameLowering | Stack frame setup, eliminateFrameIndex | S1C33FrameLowering.cpp |
| DelaySlotFiller | Fill delay slots or insert nop | S1C33DelaySlotFiller.cpp |
| AsmPrinter | Assembly text output (not for ELF) | S1C33AsmPrinter.cpp |

## Key Coding Conventions

### Instruction Definitions Must Specify PSR Clobbers

**Nearly all ALU instructions clobber PSR flags** (N/Z/V/C). This includes: add, sub, and, or, xor, not, shifts, rotates, and even `ld` with immediates. Every such instruction needs:

```tablegen
let Defs = [PSR] in {
  def ADD_rr : InstRR<...>;
  def SUB_rr : InstRR<...>;
}
```

Forgetting this breaks instruction scheduling around conditional branches.

### Shift/Rotate Cannot Use ext

Shift and rotate instructions **do not support ext extension**. Maximum shift amount is 8 bits (encoded as imm4: 0000=0, ..., 0111=7, 1xxx=8). Shifts > 8 must be split in ISelDAGToDAG using MachineNodes (NOT PerformDAGCombine, which can be re-combined).

### PC-Relative Relocations Use Target Instruction Address

For ext+ext+call patterns, REL_H/REL_M/REL_L all use the **call instruction's address** as PC reference (not the ext instruction's address). Same for REL21.

### SHT_REL Requires Addend in Data

applyFixup must write the fixup value into section data bytes **before** calling recordRelocation. SHT_REL has no explicit addend field. Failing this breaks function pointer tables, vtables, etc.

### eliminateFrameIndex Must Remap Opcodes

When FrameIndex appears in byte/halfword load/store (LDUB_ri, LDB_ri, LDH_ri, LDUH_ri, STB_ri, STH_ri), the opcode must be changed to the SP-relative variant (LDUB_sp, etc.). Don't just fix the operand; change the instruction.

### pushn/popn Operate on Ranges

- `pushn %rN` pushes R0 through RN (inclusive)
- `popn %rN` pops RN through R0 (reverse order)

They are NOT single-register operations. Encoding uses 4-bit operand (0000=R0, 1111=R15).

## Common Pitfalls

### Hardware Bug: jp.d %rb is FORBIDDEN

**Critical hardware bug**: `jp.d %rb` delay slot is not executed when DMA is active (unavoidable in real use — sound playback always runs DMA). Use `jp %rb` (non-delayed) instead. `call.d %rb` and `ret.d` are safe.

### ext Atomicity is Limited

ext+target instruction pairs are trap-masked by hardware (atomic). But multi-instruction sequences like:
```
ext+ext+ld.w %rN, symbol
ld.w %rM, [%rN]
```
...are NOT fully atomic (gap after ext-protected sequence is interruptible). This is why we don't use R9 as implicit scratch.

### Unaligned Access Traps

S1C33000 requires alignment for word/halfword access. memcpy/memset lowering and packed struct access must use byte loads/stores when alignment is not guaranteed.

### Address Space is 28 Bits

Pointers are 32-bit in LLVM but upper 4 bits are ignored by hardware. Be careful with address calculations.

### Delay Slot Constraints

Not every instruction can fill a delay slot:
- **Allowed**: ALU operations, non-memory instructions
- **Forbidden**: Memory access, ext, branches, jumps, calls

When in doubt, emit `nop`.

### 64-bit Integer Runtime

EPSON's SDK never implemented `__fixsfdi`, `__fixunssfdi`, `__floatdisf`, `__cmpdi2`.
**These are now provided by `libclang_rt.builtins-s1c33.a`** (compiler-rt, Phase 1 complete).
64-bit args use register pairs: R12(lo)+R13(hi) for first, R14(lo)+R15(hi) for second.

### Division is Expensive

Software division is 35 instructions. Default to library call (`__divsi3`), not inline expansion, to avoid code size explosion.

## Reference Backends

When unsure how to implement something, consult these LLVM backends in priority order:

1. **RISC-V** (`llvm/lib/Target/RISCV/`) — MC relaxation, ELF relocation splitting
2. **AVR** (`llvm/lib/Target/AVR/`) — 16-bit instruction encoding, small register set
3. **MIPS** (`llvm/lib/Target/Mips/`) — Delay slot handling, GP register
4. **Lanai** (`llvm/lib/Target/Lanai/`) — Clean, minimal backend structure

## P/ECE SDK Integration

The end goal is not just a compiler — binaries must link with existing P/ECE SDK libraries.

### SDK Library Format

- **Format**: SRF (EPSON proprietary), not standard ELF
- **Conversion tool**: `tools/srf2elf/` converts .o and .lib files to ELF
- **Key libraries**: pceapi.lib (kernel stubs), lib.lib (libc — has bugs, see errata.md)
- **fp.lib / idiv.lib**: **Replaced** by `libclang_rt.builtins-s1c33.a` (compiler-rt Phase 1). SRF originals kept in `sdk/` for reference only.
- **Link order**: `crt0.o crti.o [user .o] -lclang_rt.builtins-s1c33 -lcxxrt -lpceapi -lio -llib -lmath -lstring -lctype`
- **Standard C headers**: From **newlib** (submodule `newlib/`). P/ECE-specific headers from `sdk/include/`.

### Build Pipeline

```
.c → clang → .o (ELF)
SDK .lib → srf2elf → .a (ELF)          (pceapi, io, lib, math, string, ctype)
compiler-rt → libclang_rt.builtins-s1c33.a  (fp + idiv + i64 runtime)
  ↓
ld.lld → .elf → llvm-objcopy -O binary → ppack → .pex
```

### Kernel API

- Kernel lives in ROM at fixed addresses
- Symbol addresses from `pcekn.sym`
- Application implements callbacks: `pceAppInit()`, `pceAppProc()`, `pceAppExit()`
- Define symbols in linker script: `tools/piece.ld`

## Documentation Files

All reference documents are in `docs/` (mostly Japanese PDFs):

| File | Contents |
|------|----------|
| `S1C33000_コアCPUマニュアル_2001-03.pdf` | Instruction set, encoding, pipeline, traps |
| `S1C33_Family_Cコンパイラパッケージ.pdf` | ABI (§6.5), ext33/pp33/as33 specs, SRF format |
| `S1C33209_201_222テクニカルマニュアル_PRODUCT_FUNCTION.pdf` | Memory map, peripherals |
| `S1C33_family_スタンダードコア用アプリケーションノート.pdf` | Interrupt handling, boot sequence |
| `errata.md` | CPU/compiler/library bugs |
| `lib33_format.md` | lib33 archive format (.lib files) |

## File Organization

```
llvm/llvm/lib/Target/S1C33/     # Backend implementation
llvm/llvm/test/CodeGen/S1C33/   # Lit tests
llvm/compiler-rt/lib/builtins/s1c33/  # S1C33 compiler-rt builtins (FP, div, i64)
newlib/                         # newlib submodule (standard C headers for sysroot)
tools/
  ├── srf2elf/                  # SRF → ELF converter
  ├── ppack/                    # ELF → .pex packager
  ├── crt/                      # Startup code (crt0, crti, libcxxrt.a)
  ├── piece.ld                  # Linker script for P/ECE
  └── asm33conv/                # EPSON as33 → LLVM asm converter
sysroot/s1c33-none-elf/         # Headers, libraries for --sysroot
  ├── include/                  # newlib headers + P/ECE-specific headers
  └── lib/                      # crt0.o, libclang_rt.builtins-s1c33.a, libcxxrt.a, etc.
hello/, minimal/, fpkplay/      # Example applications
docs/                           # Reference PDFs and notes
```

## Language Rules for Generated Files

- **`llvm/`, `newlib/` and `tools/`**: All code, comments, docstrings, error messages, git commit messages, and any text written into files under these directories must be in **English only**. No Japanese.
- **`docs/`**: Japanese is allowed (reference documents are in Japanese).
- **`sdk/`**: This directory contains reference material only. **Do not modify any files under `sdk/`.**

## Implementation Phases

Follow the phasing in DESIGN_SPEC.md §8. Write lit tests alongside every implementation.

1. **Phase 1** — TableGen + instruction definitions (basic codegen)
2. **Phase 2** — Calling convention + frame lowering (functions compile)
3. **Phase 3** — MC layer (assembler + relaxation → ELF objects)
4. **Phase 4** — Optimizations (delay slots, GP, interrupts, division, MAC)
5. **Phase 5** — SRF→ELF tool + linker script (link with SDK)
6. **Phase 6** — SDK integration tests (full application build verified)

**Current status**: Phase 6 complete. All sample apps verified on real P/ECE hardware (2026-03).
Post-Phase-6 work in progress:
- **compiler-rt Phase 1** ✅ — fp.lib/idiv.lib replaced by `libclang_rt.builtins-s1c33.a`; full i64 runtime
- **newlib Phase 1** ✅ — Standard C headers from newlib; sysroot bootstrap self-contained
- **newlib Phase 2** 🔲 — Replace lib.lib libc with newlib C sources (fixes sin/strtok/pow/strtod/ispunct bugs)
