# Makefile — P/ECE CRT and library build
#
# Produces:
#   $(SYSROOT)/lib/crt0.o      — application header + BSS init + callback wrappers
#   $(SYSROOT)/lib/crti.o      — default pceAppNotify (weak, can be overridden)
#   $(SYSROOT)/lib/libpceapi.a — kernel API stubs + utility objects
#   $(SYSROOT)/lib/libclang_rt.builtins-s1c33.a
#                              — compiler-rt builtins + fp.lib/idiv.lib assembly
#   $(SYSROOT)/lib/lib{io,lib,math,string,ctype}.a
#                              — EPSON SDK libraries (minus fp/idiv, now in compiler-rt)
#   $(SYSROOT)/include/        — newlib standard headers + P/ECE SDK headers
#
# Usage:
#   make                   — build everything into the sysroot
#   make regen-builtins-asm — regenerate recovered fp.lib/idiv.lib assembly
#                             into llvm/compiler-rt/lib/builtins/s1c33/
#   make clean             — remove build intermediates and sysroot outputs
#
# -----------------------------------------------------------------------
# NOTE: CRT/library builds use -ffreestanding.
#   This tells the compiler not to assume a hosted standard library
#   environment, and suppresses implicit builtin substitutions for
#   functions like memset/memcpy (important because this library IS
#   the one providing those functions — substitution would cause
#   infinite recursion).
#
#   -nostdlib is a linker flag and is not needed for -c (compile only).
#   Application builds (user .c files linked against this sysroot) do
#   NOT need -ffreestanding; they run in a freestanding environment but
#   see the sysroot headers normally.
# -----------------------------------------------------------------------

TOPDIR  := $(realpath ../..)
BUILDDIR := $(TOPDIR)/build/crt
SYSROOT  := $(TOPDIR)/sysroot/s1c33-none-elf

CLANG   := $(TOPDIR)/build/bin/clang
LLVM_AR := $(TOPDIR)/build/bin/llvm-ar
LLVM_OBJDUMP := $(TOPDIR)/build/bin/llvm-objdump
PYTHON  := python3
SRF2ELF := $(PYTHON) $(TOPDIR)/tools/srf2elf/srf2elf.py
OBJDUMP2ASM := $(PYTHON) $(TOPDIR)/tools/objdump2asm/objdump2asm.py
SDKLIB  := $(TOPDIR)/sdk/lib
SDKINC  := $(TOPDIR)/sdk/include

TARGET  := s1c33-none-elf
INCLUDE := -I$(SYSROOT)/include

# newlib header source
NEWLIB_INCLUDE := $(TOPDIR)/newlib/newlib/libc/include

# newlib build sources/outputs
NEWLIB_SRC      := $(TOPDIR)/newlib/newlib
NEWLIB_BUILDDIR := $(BUILDDIR)/newlib
LLVM_RANLIB     := $(TOPDIR)/build/bin/llvm-ranlib

# compiler-rt builtins source
BUILTINS_SRC  := $(TOPDIR)/llvm/compiler-rt/lib/builtins
S1C33_ASM_DIR := $(BUILTINS_SRC)/s1c33

# CRT/library compiler flags
# -ffreestanding: no hosted-env assumptions; suppresses memset/memcpy builtins
# -Wall -Wno-incompatible-library-redeclaration: SDK headers use non-standard
#   function signatures; suppress the resulting redeclaration warnings
CFLAGS_CRT := --target=$(TARGET) -c -ffreestanding \
              $(INCLUDE) \
			  -O1 \
			  -fno-inline \
              -Wall \
              -Wno-incompatible-library-redeclaration \
              -Wno-int-conversion

ASFLAGS_CRT := --target=$(TARGET) -c -ffreestanding

# -----------------------------------------------------------------------
# Top-level targets
# -----------------------------------------------------------------------

SDK_LIBS := \
	$(SYSROOT)/lib/libio.a \
	$(SYSROOT)/lib/liblib.a \
	$(SYSROOT)/lib/libmath.a \
	$(SYSROOT)/lib/libstring.a \
	$(SYSROOT)/lib/libctype.a

NEWLIB_LIBS := \
	$(SYSROOT)/lib/libc.a \
	$(SYSROOT)/lib/libm.a

.PHONY: all clean muslib-install regen-builtins-asm

all: $(SYSROOT)/include/.newlib-stamp \
     $(SYSROOT)/lib/crt0.o \
     $(SYSROOT)/lib/crti.o \
     $(SYSROOT)/lib/libpceapi.a \
     $(SYSROOT)/lib/libcxxrt.a \
     $(SYSROOT)/lib/libclang_rt.builtins-s1c33.a \
     $(NEWLIB_LIBS) \
     $(SDK_LIBS) \
     $(SYSROOT)/lib/piece.ld \
     muslib-install

# muslib is built from source in tools/muslib/ (not converted from SDK SRF).
muslib-install:
	$(MAKE) -C $(TOPDIR)/tools/muslib install

$(SYSROOT)/lib/piece.ld: $(TOPDIR)/tools/piece.ld
	cp $< $@

clean:
	rm -rf $(BUILDDIR)
	rm -f $(SYSROOT)/lib/crt0.o \
	      $(SYSROOT)/lib/crti.o \
	      $(SYSROOT)/lib/libpceapi.a \
	      $(SYSROOT)/lib/libcxxrt.a \
	      $(SYSROOT)/lib/libclang_rt.builtins-s1c33.a \
	      $(NEWLIB_LIBS) \
	      $(SDK_LIBS)
	rm -f $(SYSROOT)/include/.newlib-stamp

$(BUILDDIR):
	mkdir -p $@

# -----------------------------------------------------------------------
# newlib headers — install from newlib/newlib/libc/include/ into sysroot
#
# P/ECE SDK headers (piece.h, PIECE_Bmp.h, etc.) are preserved; newlib
# provides the standard C headers (stdio.h, string.h, stdint.h, etc.).
#
# stdarg.h is intentionally excluded: clang provides it via -resource-dir.
#
# _REENT_ONLY: suppresses the full reentrancy structure (_reent) in newlib.
#   P/ECE is single-threaded; the full reent machinery is unnecessary and
#   would add overhead to every stdio call.
# -----------------------------------------------------------------------

# P/ECE SDK headers: canonical source is sdk/include/.
# Listed as bare names here; install rule copies from $(SDKINC)/.
PIECE_HEADER_NAMES := \
	PIECE_Bmp.h \
	PIECE_Std.h \
	draw.h \
	pclsprite.h \
	piece.h \
	s1c33cpu.h \
	simple.h \
	smcvals.h \
	thread.h

$(SYSROOT)/include/.newlib-stamp: $(NEWLIB_INCLUDE) | $(BUILDDIR)
	@echo "Installing newlib headers into sysroot..."
	@mkdir -p $(SYSROOT)/include/machine $(SYSROOT)/include/sys
	@# Copy all newlib standard C headers
	cp -r $(NEWLIB_INCLUDE)/. $(SYSROOT)/include/
	@# Remove headers that Clang provides via resource-dir (not newlib's job):
	@#   stdarg.h  — clang's __stdarg_*.h mechanism
	@#   stddef.h  — gcc33 SDK relic: defines wchar_t as 16-bit unsigned short,
	@#               and does not handle __need_wint_t; use Clang's instead
	@#   float.h   — gcc33 SDK relic: hardcoded 1993 values; use Clang's instead
	rm -f $(SYSROOT)/include/stdarg.h \
	      $(SYSROOT)/include/stddef.h \
	      $(SYSROOT)/include/float.h
	@# Install P/ECE SDK headers from sdk/include/ (canonical source, always
	@# available from a fresh clone; these override any newlib equivalents)
	cp $(addprefix $(SDKINC)/,$(PIECE_HEADER_NAMES)) $(SYSROOT)/include/
	@# Add _REENT_ONLY wrapper so #include <stdio.h> etc. doesn't pull in full reent
	@printf '#ifndef _REENT_ONLY\n#define _REENT_ONLY\n#endif\n' > $(SYSROOT)/include/newlib_config.h
	@touch $@
	@echo "newlib headers installed."

# -----------------------------------------------------------------------
# libclang_rt.builtins-s1c33.a — compiler-rt builtins
#
# Combines:
#   1. S1C33-specific assembly (recovered from fp.lib / idiv.lib):
#      float ops, hardware integer divide/modulo
#   2. compiler-rt C sources for operations not covered by assembly:
#      64-bit integer arithmetic, bit-counting, __fixdfsi/__fixsfsi, etc.
#
# fp.lib/idiv.lib assembly overrides must precede the C objects in the
# archive so the linker picks them up first. The REMOVE_ITEM logic in
# CMakeLists.txt already excludes the C equivalents; here we just omit
# them from BUILTIN_C_SRCS directly.
# -----------------------------------------------------------------------

BUILTINS_BUILDDIR := $(BUILDDIR)/builtins
$(BUILTINS_BUILDDIR): | $(BUILDDIR)
	mkdir -p $@

regen-builtins-asm: | $(BUILDDIR)
	@set -eu; \
	tmpdir=$$(mktemp -d $(BUILDDIR)/regen-builtins-asm.XXXXXX); \
	trap 'rm -rf "$$tmpdir"' EXIT; \
	cp $(SDKLIB)/fp.lib $(SDKLIB)/idiv.lib "$$tmpdir"/; \
	$(SRF2ELF) "$$tmpdir/fp.lib" "$$tmpdir/fp.a"; \
	$(SRF2ELF) "$$tmpdir/idiv.lib" "$$tmpdir/idiv.a"; \
	mkdir -p "$$tmpdir/fp" "$$tmpdir/idiv"; \
	$(LLVM_OBJDUMP) -d -r --symbolize-operands "$$tmpdir/fp.a" | $(OBJDUMP2ASM) -o "$$tmpdir/fp"; \
	$(LLVM_OBJDUMP) -d -r --symbolize-operands "$$tmpdir/idiv.a" | $(OBJDUMP2ASM) -o "$$tmpdir/idiv"; \
	for src in "$$tmpdir"/fp/*.s "$$tmpdir"/idiv/*.s; do \
		dst="$(S1C33_ASM_DIR)/$$(basename "$${src%.s}").S"; \
		cp "$$src" "$$dst"; \
	done; \
	echo "Recovered builtins assembly updated in $(S1C33_ASM_DIR)."

# S1C33-specific assembly (recovered fp.lib / idiv.lib)
S1C33_ASM_SRCS := $(wildcard $(S1C33_ASM_DIR)/*.S)
S1C33_ASM_OBJS := $(patsubst $(S1C33_ASM_DIR)/%.S,$(BUILTINS_BUILDDIR)/s1c33_%.o,$(S1C33_ASM_SRCS))

# compiler-rt C sources needed for bare-metal 32-bit S1C33
# Excludes: anything covered by fp.lib/idiv.lib, *tf* (128-bit fp), OS-specific
BUILTIN_C_SRCS_NAMES := \
	muldi3.c udivdi3.c divdi3.c moddi3.c umoddi3.c \
	ashldi3.c ashrdi3.c lshrdi3.c \
	cmpdi2.c ucmpdi2.c \
	fixdfdi.c fixsfdi.c \
	fixdfsi.c fixsfsi.c \
	fixunsdfdi.c fixunssfdi.c \
	floatdidf.c floatdisf.c \
	floatundidf.c floatundisf.c \
	floatunsidf.c floatunsisf.c \
	divmoddi4.c udivmoddi4.c \
	mulodi4.c mulosi4.c \
	negdi2.c \
	absvdi2.c absvsi2.c \
	addvdi3.c addvsi3.c \
	subvdi3.c subvsi3.c \
	mulvdi3.c mulvsi3.c \
	negvdi2.c negvsi2.c \
	clzdi2.c clzsi2.c \
	ctzdi2.c ctzsi2.c \
	ffsdi2.c ffssi2.c \
	bswapdi2.c bswapsi2.c \
	popcountdi2.c popcountsi2.c \
	paritydi2.c paritysi2.c \
	divmodsi4.c udivmodsi4.c \
	powidf2.c powisf2.c \
	int_util.c fp_mode.c

BUILTIN_C_OBJS := $(patsubst %.c,$(BUILTINS_BUILDDIR)/%.o,$(BUILTIN_C_SRCS_NAMES))

# Compile each C source from compiler-rt
$(BUILTINS_BUILDDIR)/%.o: $(BUILTINS_SRC)/%.c | $(BUILTINS_BUILDDIR)
	$(CLANG) --target=$(TARGET) -c -ffreestanding -fno-builtin -O2 \
	         -I$(BUILTINS_SRC) $< -o $@

# Assemble each recovered S1C33 assembly source
$(BUILTINS_BUILDDIR)/s1c33_%.o: $(S1C33_ASM_DIR)/%.S | $(BUILTINS_BUILDDIR)
	$(CLANG) --target=$(TARGET) -c -ffreestanding $< -o $@

$(SYSROOT)/lib/libclang_rt.builtins-s1c33.a: $(S1C33_ASM_OBJS) $(BUILTIN_C_OBJS)
	$(LLVM_AR) rcs $@ $^


# -----------------------------------------------------------------------
# CRT startup objects (directly linked into every application)
# -----------------------------------------------------------------------

# crt0.o: application header (pceAPPHEAD at 0x100000), BSS zero-fill,
#         pceAppInit00/pceAppProc00/pceAppExit00/pceAppNotify00 wrappers.
$(SYSROOT)/lib/crt0.o: crt0.c | $(BUILDDIR) $(SYSROOT)/include/.newlib-stamp
	$(CLANG) $(CFLAGS_CRT) $< -o $@

# crti.o: default pceAppNotify (weak symbol; overridable by the application).
$(SYSROOT)/lib/crti.o: defnotify.c | $(BUILDDIR) $(SYSROOT)/include/.newlib-stamp
	$(CLANG) $(CFLAGS_CRT) $< -o $@

# -----------------------------------------------------------------------
# libpceapi.a — kernel API stubs + utility objects
# -----------------------------------------------------------------------

PCEAPI_STUBS_DIR := $(BUILDDIR)/pceapi_stubs

$(PCEAPI_STUBS_DIR): | $(BUILDDIR)
	mkdir -p $@

# Step 1: compile/assemble utility sources
$(BUILDDIR)/version_check.o: version_check.c | $(BUILDDIR) $(SYSROOT)/include/.newlib-stamp
	$(CLANG) $(CFLAGS_CRT) $< -o $@

$(BUILDDIR)/memset.o: memset.s | $(BUILDDIR)
	$(CLANG) $(ASFLAGS_CRT) $< -o $@

$(BUILDDIR)/memcpy.o: memcpy.s | $(BUILDDIR)
	$(CLANG) $(ASFLAGS_CRT) $< -o $@

$(BUILDDIR)/stacklen.o: stacklen.s | $(BUILDDIR)
	$(CLANG) $(ASFLAGS_CRT) $< -o $@

$(BUILDDIR)/iodef.o: iodef.s | $(BUILDDIR)
	$(CLANG) $(ASFLAGS_CRT) $< -o $@

UTILITY_OBJS := \
	$(BUILDDIR)/version_check.o \
	$(BUILDDIR)/memset.o \
	$(BUILDDIR)/memcpy.o \
	$(BUILDDIR)/stacklen.o \
	$(BUILDDIR)/iodef.o

# Step 2: generate one .s per stub, assemble each to its own .o, pack archive.
# Each function lands in a separate object file so the linker only pulls in
# the stubs that are actually referenced (no unused API symbols).
$(SYSROOT)/lib/libpceapi.a: gen_pceapi.py vector.h $(UTILITY_OBJS) | $(PCEAPI_STUBS_DIR)
	python3 gen_pceapi.py vector.h --split-dir $(PCEAPI_STUBS_DIR)
	for s in $(PCEAPI_STUBS_DIR)/pce*.s; do \
		$(CLANG) $(ASFLAGS_CRT) $$s -o $${s%.s}.o || exit 1; \
	done
	$(LLVM_AR) rcs $@ $(PCEAPI_STUBS_DIR)/pce*.o $(UTILITY_OBJS)

# -----------------------------------------------------------------------
# libcxxrt.a — minimal C++ runtime stubs (operator new/delete, guards, etc.)
#
# All objects are compiled WITHOUT -flto.  Runtime stubs must be native ELF
# so the linker can always resolve their symbols, regardless of whether the
# application uses LTO.  Bitcode runtime objects cause subtle issues: LLVM
# LTO treats certain well-known functions (abort, memcpy, …) as external
# references even when defined in bitcode, leaving them unresolved.
# -----------------------------------------------------------------------

$(BUILDDIR)/cxxrt.o: cxxrt.c | $(BUILDDIR)
	$(CLANG) $(CFLAGS_CRT) $< -o $@

$(BUILDDIR)/cxxnew.o: cxxnew.c | $(BUILDDIR)
	$(CLANG) $(CFLAGS_CRT) $< -o $@

$(BUILDDIR)/halt.o: halt.c | $(BUILDDIR)
	$(CLANG) $(CFLAGS_CRT) $< -o $@

$(SYSROOT)/lib/libcxxrt.a: $(BUILDDIR)/cxxrt.o $(BUILDDIR)/cxxnew.o $(BUILDDIR)/halt.o
	$(LLVM_AR) rcs $@ $^

# -----------------------------------------------------------------------
# EPSON SDK libraries — converted from lib33 SRF format to GNU ar ELF
#
# Note: fp.lib and idiv.lib are NOT converted here — their functions are
# now provided by libclang_rt.builtins-s1c33.a (recovered assembly).
# -----------------------------------------------------------------------

$(SYSROOT)/lib/libio.a:     $(SDKLIB)/io.lib     ; $(SRF2ELF) $< $@
$(SYSROOT)/lib/liblib.a:    $(SDKLIB)/lib.lib    ; $(SRF2ELF) $< $@
$(SYSROOT)/lib/libmath.a:   $(SDKLIB)/math.lib   ; $(SRF2ELF) $< $@
$(SYSROOT)/lib/libstring.a: $(SDKLIB)/string.lib ; $(SRF2ELF) $< $@
# libctype.a: SDK function implementations + _ctype_[] table (newlib macros need it).
$(BUILDDIR)/ctype_table.o: ctype_table.c | $(BUILDDIR) $(SYSROOT)/include/.newlib-stamp
	$(CLANG) $(CFLAGS_CRT) $< -o $@

$(SYSROOT)/lib/libctype.a: $(SDKLIB)/ctype.lib $(BUILDDIR)/ctype_table.o
	$(SRF2ELF) $(SDKLIB)/ctype.lib $@
	$(LLVM_AR) q $@ $(BUILDDIR)/ctype_table.o

# -----------------------------------------------------------------------
# newlib — built from the newlib/ submodule via its own autotools.
#
# We configure newlib in $(NEWLIB_BUILDDIR) targeting s1c33-none-elf and
# use the freshly-built clang as CC_FOR_TARGET.  Headers are NOT
# installed from this path — the existing recipe above already manages
# the sysroot include/ tree by direct copy, dropping headers that clang
# provides (stdarg.h / stddef.h / float.h) and overlaying P/ECE-specific
# headers.
#
# Configure flags chosen for the P/ECE memory budget:
#   nano-malloc / nano-formatted-io / reent-small / lite-exit
#   no multithread, no iconv, no wide-orient, no fseek-optimization
#
# The build only runs all-target-newlib; we then copy libc.a / libm.a
# directly out of the build tree into $(SYSROOT)/lib/.
# -----------------------------------------------------------------------

NEWLIB_CONFIGURE_FLAGS := \
    --build=x86_64-pc-linux-gnu \
    --host=s1c33-none-elf \
    --target=s1c33-none-elf \
    --prefix=$(TOPDIR)/sysroot \
    --enable-newlib-nano-malloc \
    --enable-newlib-nano-formatted-io \
    --enable-newlib-reent-small \
    --disable-newlib-multithread \
    --disable-newlib-fvwrite-in-streamio \
    --disable-newlib-fseek-optimization \
    --disable-newlib-wide-orient \
    --enable-lite-exit \
    --disable-newlib-unbuf-stream-opt \
    --disable-newlib-iconv \
    --enable-target-optspace

# Tools the inner newlib build will use.  -ffreestanding so newlib does
# not assume hosted libc, -fno-builtin so its memcpy/memset/etc are not
# folded into compiler intrinsics that recurse into themselves.
NEWLIB_CC := $(CLANG) --target=$(TARGET) -ffreestanding -fno-builtin

$(NEWLIB_BUILDDIR)/Makefile: $(NEWLIB_SRC)/configure | $(BUILDDIR)
	mkdir -p $(NEWLIB_BUILDDIR)
	cd $(NEWLIB_BUILDDIR) && \
	  $(NEWLIB_SRC)/configure \
	    CC="$(NEWLIB_CC)" \
	    AS="$(NEWLIB_CC) -c" \
	    AR="$(LLVM_AR)" \
	    RANLIB="$(LLVM_RANLIB)" \
	    CC_FOR_TARGET="$(NEWLIB_CC)" \
	    AS_FOR_TARGET="$(NEWLIB_CC) -c" \
	    AR_FOR_TARGET="$(LLVM_AR)" \
	    RANLIB_FOR_TARGET="$(LLVM_RANLIB)" \
	    $(NEWLIB_CONFIGURE_FLAGS)

# Build & install newlib.  We use install-target-newlib + install-target-libm
# but redirect into a staging dir so the existing sysroot include layout is
# not touched; only libc.a / libm.a are then copied out.
$(NEWLIB_LIBS) &: $(NEWLIB_BUILDDIR)/Makefile
	$(MAKE) -C $(NEWLIB_BUILDDIR)
	@# The build dir layout puts libc.a / libm.a directly at the build root
	@# for non-recursive newlib.  Locate and copy them.
	@for lib in libc.a libm.a; do \
	  src=$$(find $(NEWLIB_BUILDDIR) -maxdepth 3 -name $$lib -type f | head -1); \
	  if [ -z "$$src" ]; then echo "ERROR: $$lib not produced" >&2; exit 1; fi; \
	  echo "Installing $$src -> $(SYSROOT)/lib/$$lib"; \
	  cp "$$src" $(SYSROOT)/lib/$$lib; \
	done
