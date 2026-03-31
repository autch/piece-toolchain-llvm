# CLAUDE.md — S1C33000 LLVM Backend Project

## What This Project Is

An LLVM backend for the EPSON S1C33000 32-bit RISC CPU core, targeting the Aquaplus P/ECE (S1C33209 SoC). The goal is to rebuild the P/ECE development environment on LLVM/Clang, producing binaries that can link with existing P/ECE SDK libraries and call kernel APIs with full ABI compatibility.

## Critical Context

Read `DESIGN_SPEC.md` first — it contains all architecture details, design decisions, ABI specifications, and implementation phasing. Do not deviate from the decisions documented there without explicit discussion.

## Key Architecture Facts (Quick Reference)

- 16-bit fixed-length instructions, 32-bit registers × 16 (R0–R15)
- 28-bit address space (256MB), little-endian
- `ext` prefix instruction extends immediates (max 2 × ext before any instruction)
- **ext does NOT change the signedness of the target instruction's immediate** — signed instructions get sign-extension, unsigned instructions get zero-extension (see "ext Immediate Signedness Rules" section below)
- Hardware guarantees atomicity of ext+target instruction sequences (trap-masked)
- Delayed branch with strict slot constraints (1-cycle, no memory, no ext, no branch)
- No hardware divider; step-division sequence (div0s/div1/div2s/div3s)
- S1C33209 has optional hardware multiplier and MAC unit

## ABI (S5U1C33000C — DO NOT use S5U1C33001C ABI)

- **Args**: R12→R13→R14→R15 (overflow to stack)
- **Return**: R10 (R10+R11 for 64-bit)
- **Callee-saved**: R0–R3
- **Scratch**: R4–R7
- **Reserved**: R8 (カーネルテーブルベース = 0x0), R9 (reserved — NOT used as scratch in our design)
- **sret**: struct return pointer passed implicitly in R12
- **Frame**: SP-based, no frame pointer

## Design Decisions (Non-Negotiable)

1. **R9 is NOT used as an implicit scratch register.** Unlike the original ext33 tool, our backend uses the register allocator to pick any available register for address materialization. This is safer for interrupt handling.

2. **ext instruction generation follows pessimistic-then-relax strategy.** Code generation emits maximum-size sequences (ext+ext+op), then MC-layer relaxation shrinks them.

3. **R8 is the kernel table base pointer, always 0x0.** The P/ECE kernel sets R8 = 0x0 before calling any application callback. pceapi stubs use `ext N / ld.w %r9, [%r8]` to fetch kernel function pointers from address N. User-compiled code must never modify R8. R8 is Reserved to enforce this. This is NOT a MIPS-style user-data GP — "GP optimization" is not implemented for user apps. Future kernel compilation will need assembly startup code that explicitly sets R8 = 0.

4. **Interrupt handlers** use `__attribute__((interrupt_handler))` generating `pushn %r15` / `popn %r15` / `reti`.

5. **as33/pp33/ext33 assembly syntax is NOT supported.** We generate standard LLVM assembly. The `^H/^M/^L` operators, `@rh/@rm/@rl` modifiers, and `x`-prefixed mnemonics are handled internally, not as assembly syntax.

## Reference Documentation

All reference documents are in the `docs/` directory:

| File | What it contains | When to consult |
|---|---|---|
| `S1C33000_コアCPUマニュアル_2001-03.pdf` | Instruction set, encoding, pipeline, traps | Instruction definitions, ext behavior, delay slots |
| `S1C33_Family_Cコンパイラパッケージ.pdf` | ABI (§6.5), ext33/pp33/as33 specs, SRF format (Appendix) | Calling convention, register usage, stack frame, SRF→ELF conversion |
| `S1C33209_201_222テクニカルマニュアル_PRODUCT_FUNCTION.pdf` | Memory map, peripherals | Linker script, multiplier features |
| `S1C33_family_スタンダードコア用アプリケーションノート.pdf` | Interrupt handling patterns, boot sequence | Interrupt handler codegen, trap table layout |
| `errata.md` | CPU/compiler/library errata | Hardware bugs (jp.d %rb), alignment traps, C library issues |

These PDFs are in Japanese. Key sections are summarized in DESIGN_SPEC.md, but consult the originals for encoding details, bit-field layouts, and edge cases.

## Build Workflow

After any change to the backend source (`llvm/llvm/lib/Target/S1C33/`), always run from the **repo root**:

```bash
make        # rebuilds clang + llc + lld, then regenerates tools/crt sysroot
make tests  # runs S1C33 lit tests
```

Do **not** use `ninja llc` alone — clang-22 links against `libLLVMS1C33CodeGen.a` separately and will silently use the old codegen until `clang` is rebuilt. `tools/crt` must also be regenerated any time the calling convention, frame layout, or codegen changes.

## Implementation Order

Follow the phasing in DESIGN_SPEC.md §8:

1. **Phase 1** — TableGen + basic instruction definitions → `llc` outputs assembly text
2. **Phase 2** — Calling convention + frame lowering → functions compile correctly
3. **Phase 3** — MC layer (assembler + relaxation) → ELF object output
4. **Phase 4** — Optimizations (delay slot filler, GP, interrupts, step division, MAC)
5. **Phase 5** — SRF→ELF conversion tool + linker script → can link with P/ECE SDK libraries
6. **Phase 6** — P/ECE SDK integration tests → full application build verified

**Current status**: Phase 6 complete. mini_nocrt / minimal / hello / print / jien all verified on real P/ECE hardware (2026-03).

Within each phase, write lit tests alongside the implementation. Every instruction encoding, every calling convention edge case, every relaxation pattern should have a test.

## P/ECE SDK Integration (Critical Path)

The project's end goal is not just a compiler — it must link with existing P/ECE SDK binaries. Key points:

- **SDK libraries are SRF format** (EPSON proprietary), not ELF. A `srf2elf` conversion tool is needed (Phase 5). The SRF format spec is in the C compiler manual Appendix.
- **fp.lib and idiv.lib** contain compiler runtime functions (`__addsf3`, `__divsi3`, etc.). These can alternatively be replaced by building LLVM's compiler-rt for S1C33000.
- **Kernel APIs** (pceLCDTrans, pcePadGet, etc.) live at fixed addresses in ROM. Symbol addresses come from `pcekn.sym`. Define these in the linker script or a header.
- **Application entry** is via callbacks: the app implements `pceAppInit()` / `pceAppProc()` / `pceAppExit()`, which the kernel calls.
- **Build pipeline**: `.c → clang → .o (ELF)` + `SDK .lib → srf2elf → .a (ELF)` → `ld.lld` → `.elf` → `llvm-objcopy -O binary`

## Reference Backends

When unsure how to implement something, look at these existing backends in priority order:

- **RISC-V** (`llvm/lib/Target/RISCV/`) — MC relaxation, ELF relocation splitting
- **AVR** (`llvm/lib/Target/AVR/`) — 16-bit instruction encoding, small register set
- **MIPS** (`llvm/lib/Target/Mips/`) — Delay slot handling, GP register
- **Lanai** (`llvm/lib/Target/Lanai/`) — Clean, minimal backend structure

## Coding Conventions

- Target name in code: `S1C33` (e.g., `S1C33TargetMachine`, `S1C33InstrInfo`)
- Triple: `s1c33-none-elf`
- All files go under `llvm/lib/Target/S1C33/`
- Use TableGen for anything TableGen can express (instructions, registers, calling conventions)
- Comment non-obvious encoding decisions with reference to the CPU manual section

## ext Immediate Signedness Rules — CRITICAL

**ext does NOT change the signedness of the target instruction's immediate.** If the target instruction takes a signed immediate (sign6/sign8), ext+instruction produces a sign-extended combined value. If the target instruction takes an unsigned immediate (imm6), ext+instruction produces a zero-extended combined value. Getting this wrong causes silent data corruption that is extremely difficult to debug.

### Signed immediate instructions (sign6 / sign8)

These instructions treat their immediate field as **signed**. With ext, the combined value is **sign-extended**:

| Instruction | Immediate | With 1 ext | With 2 ext |
|---|---|---|---|
| `ld.w %rd, sign6` | sign_ext_6 → 32bit | sign_ext_19(ext13:sign6) | sign_ext_32(ext13:ext13:sign6) |
| `and %rd, sign6` | sign_ext_6 | sign_ext_19 | sign_ext_32 |
| `or %rd, sign6` | sign_ext_6 | sign_ext_19 | sign_ext_32 |
| `xor %rd, sign6` | sign_ext_6 | sign_ext_19 | sign_ext_32 |
| `not %rd, sign6` | sign_ext_6 | sign_ext_19 | sign_ext_32 |
| `cmp %rd, sign6` | sign_ext_6 | sign_ext_19 | sign_ext_32 |
| `jr** sign8` (all cond. branches, incl. `.d`) | sign_ext_8 | sign_ext_21 | sign_ext_32 |
| `call sign8` (incl. `call.d`) | sign_ext_8 | sign_ext_21 | sign_ext_32 |
| `jp sign8` (incl. `jp.d`) | sign_ext_8 | sign_ext_21 | sign_ext_32 |

### Unsigned immediate instructions (imm6 / imm10)

**Everything else** uses unsigned immediates. With ext, the combined value is **zero-extended**:

| Instruction | Immediate | With 1 ext | With 2 ext |
|---|---|---|---|
| `add %rd, imm6` | zero_ext_6 → 32bit | zero_ext_19(ext13:imm6) | zero_ext_32(ext13:ext13:imm6) |
| `sub %rd, imm6` | zero_ext_6 | zero_ext_19 | zero_ext_32 |
| `ld.* %rd, [%sp+imm6]` | zero_ext_6 (scaled) | zero_ext_19 | zero_ext_32 |
| `ld.* [%sp+imm6], %rs` | zero_ext_6 (scaled) | zero_ext_19 | zero_ext_32 |
| `add %sp, imm10` | zero_ext_10 | — | — |
| `sub %sp, imm10` | zero_ext_10 | — | — |

### ext with register-indirect memory (Class 1): UNSIGNED displacement

`ld.* %rd, [%rb]` and `ld.* [%rb], %rs` have **no** immediate field in the base encoding. When ext is prepended, the ext13 value provides a byte displacement that is **unsigned** (zero-extended):

- `ext imm13 / ld.w %rd, [%rb]` → EA = rb + zero_ext(imm13)
- Range: **[0, 8191]** bytes. **Negative offsets are NOT possible.**

This means the backend must **never** fold a negative constant offset into an ext+load/store pattern. If the optimizer computes `base - N`, it must use a separate `sub` instruction, not a negative ext displacement. Violating this causes the CPU to access `base + (8192 - N)` instead of `base - N`, producing silent wrong data reads.

**gcc33 never uses negative ext offsets for memory access.** It always recomputes addresses from scratch using positive offsets. This is correct behavior, not a missed optimization.

### ext with register-register ALU (Class 1): 3-operand, UNSIGNED

`ext imm13 / op %rd, %rs` converts the 2-operand register-register form into a **3-operand** operation:
- `%rd = %rs <op> zero_ext(imm13)` (the ext value replaces the ALU operand, NOT %rs)
- The immediate from ext is **unsigned**

This is distinct from the 2-operand immediate form `op %rd, sign6/imm6` where `%rd = %rd <op> imm6`.

### Backend implementation

- **InstrInfo.td**: Memory offset pseudos (`*_ri_off`) use `immZExt13` (not `immSExt13`), range [0, 8191]
- **ExpandExtPseudos.cpp**: `emitExtForImm()` takes `SignedImm6` parameter — true for sign6 instructions (and/or/xor/not/cmp/ld.w-imm), false for imm6 instructions (add/sub)
- **eliminateFrameIndex**: SP-relative offsets are always non-negative (assert enforced)

## Common Pitfalls

- **Two ABIs exist** — S5U1C33000C vs S5U1C33001C. We use S5U1C33000C. If you see R6–R9 as args or R4/R5 as return, you're looking at the wrong ABI.
- **`jp.d %rb` is FORBIDDEN** — Hardware bug: delay slot not executed when DMA is active (unavoidable in practice). Use `jp %rb` (non-delayed) instead. `call.d %rb` and `ret.d` are safe.
- **ext atomicity** — ext+target sequences are trap-masked by hardware. But multi-instruction sequences (ext+ext+ld.w %rN, symbol + ld.w %rM, [%rN]) are NOT fully atomic — the gap after the ext-protected sequence is interruptible. This is why we don't use R9 as implicit scratch.
- **Address space is 28 bits** — Pointers are 32-bit in LLVM but the upper 4 bits are ignored by hardware. Be careful with address calculations.
- **Delayed branch slot constraints** — Not every instruction can fill the slot. When in doubt, emit `nop`.
- **pushn/popn operate on ranges** — `pushn %rN` pushes R0 through RN. `popn %rN` pops RN through R0. They are not single-register operations.
- **Unaligned access traps** — S1C33000 requires alignment for word/halfword access. memcpy/memset lowering and packed struct access must use byte loads/stores when alignment is not guaranteed.
- **Nearly all ALU instructions clobber PSR flags** — add, sub, and, or, xor, not, shifts, rotates, and even `ld` with immediates all update N/Z/V/C. Every such instruction needs `Defs = [PSR]` in TableGen. This is critical for correct instruction scheduling around conditional branches.
- **64-bit integer runtime is incomplete in EPSON's SDK** — `__fixsfdi`, `__fixunssfdi`, `__floatdisf`, `__cmpdi2` were never implemented. compiler-rt must provide these. 64-bit args use register pairs: R12(lo)+R13(hi) for first arg, R14(lo)+R15(hi) for second.
- **Division is expensive** — 35 instructions for a single div. Default to library call (`__divsi3`), not inline expansion, to avoid code size explosion.
- **EPSON's C library (lib.lib) has known bugs** — sin(), strtok(), pow(), strtod(), ispunct() are broken. Replace only the buggy functions individually; do not remove lib.lib entirely as other SDK components may depend on it.
- **Shift/rotate instructions do NOT support ext** — Manual states "シフト・ローテート命令を除き、ext命令による即値拡張が行えます". Max shift amount is 8 bits (imm4 mapping: 0000=0, ..., 0111=7, 1xxx=8). Shift > 8 must be split into multiple instructions in ISelDAGToDAG using MachineNodes (NOT PerformDAGCombine, which gets re-combined).
- **Class 1 memory ext displacement is UNSIGNED — negative offsets silently break** — `ext imm13 / ld.* [%rb]` zero-extends imm13. If the optimizer folds a negative offset (e.g., -2 → ext 8190), the CPU reads address `rb + 8190` instead of `rb - 2`. This caused pmdplay O1 bugs (parts not playing, loops stopping). The fix: use `immZExt13` (not `immSExt13`) in DAG patterns so negative offsets are never folded. See "ext Immediate Signedness Rules" section above.
- **SP-relative offsets are in scaled units, not bytes** — `ld.w [%sp+imm6]`: imm6 is in word units (×4). `ld.h`: halfword units (×2). `ld.b`: byte units (×1). `add/sub %sp, imm10`: word units (×4). The byte offset from eliminateFrameIndex must be divided by the scale factor before encoding. gcc33 encodes `ld.w [%sp+0xd]` for byte offset 52 (13 words × 4).
- **PC-relative relocations use branch instruction address as base** — For ext+ext+call patterns, REL_H/REL_M/REL_L all use the call instruction's address (not the ext instruction's address) as PC reference. Same for REL21.
- **Conditional branch PC = instruction's own address, NOT next instruction** — S1C33 manual: `target = instr_addr + 2 × sign8`. `applyFixup` for `fixup_s1c33_pc_rel_8` uses `Offset = Value` (not `Value - 2`). The `Value - 2` form is only correct for `fixup_s1c33_pc_rel_21` where the fixup sits on the ext instruction 2 bytes before the branch. Getting this wrong causes back-edges to land 2 bytes off, potentially inside ext+ld pairs, causing loop-invariant hoisted loads to read 0.
- **crt0 must be compiled with -O1** — At `-O0`, the BSS-clearing loop counter lives at `[SP+0]`. If the kernel places SP at `bss_end`, the loop overwrites its own stack variable while clearing BSS, causing silent corruption. `tools/crt/Makefile` already sets `-O1` in `CFLAGS_CRT` — do not override it.
- **Backend uses SHT_RELA (not SHT_REL)** — `HasRelAddend=true` in `S1C33ELFObjectWriter.cpp`. Addends are stored in the `r_addend` field of RELA entries. This is essential for correctness with ELF mergeable sections (`.rodata.str1.1` etc.) where split relocations (ABS_H/M/L) with SHT_REL would each carry only partial addend bits, causing wrong merge lookups. All sysroot libraries (via srf2elf) also use SHT_RELA.
- **eliminateFrameIndex must remap opcodes for byte/halfword** — When a FrameIndex appears in LDUB_ri/LDB_ri/LDH_ri/LDUH_ri/STB_ri/STH_ri, the opcode must be changed to the SP-relative variant (LDUB_sp etc). LDW/STW were handled but byte/halfword were missing.
- **double args: no split across register/stack** — If only 1 register remains when a double arg arrives, both halves go to stack. No splitting one half to register and one to stack. Implemented via CCCustom handler.
- **Variadic function ABI is all-stack** — When calling a variadic function (printf, scanf, etc.), ALL arguments go on the stack. R12–R15 are NOT used. This is because the SDK's stdarg.h uses `&(lastparm)` to get the stack address, which requires all args to be in contiguous stack memory. Do NOT implement a "spill register args in callee prologue" pattern like ARM/MIPS — the caller handles everything.
- **ret.d delay slot must not contain popn** — `ret.d` reads the return address from `[SP]` AFTER the delay slot executes. If `popn %rN` is in the delay slot (which restores SP), the return address is read from the wrong stack position. The delay slot filler excludes `popn` from `ret.d` slots.
- **Struct-by-value args go entirely on stack (§6.5.4)** — For `callee(struct _foo foo, int d)`, only `d` uses R12; `foo` is passed entirely via stack. Structs do NOT consume register slots. This is implemented in `LowerCall` by excluding byval args from CC analysis and storing struct words directly to the outgoing stack area after CC-assigned stack args.
- **MinGlobalAlign=32 matches gcc33** — All global variables are aligned to at least 4 bytes. Without this, `const unsigned char[]` arrays (bitmap data) can land at odd addresses, causing unaligned access traps when SDK library code does halfword/word access on them. Set via `MinGlobalAlign=32` in the Clang target and `-a:0:32` in the data layout string.

## SDK Library Link Order

The correct default link order (P/ECE SDK convention):
```
crt0.o crti.o [user .o files]
-lpceapi -lio -llib -lmath -lstring -lctype -lfp -lidiv
```
Libraries not in this list (muslib etc) are not included by default.
