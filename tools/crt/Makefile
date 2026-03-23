# Makefile — P/ECE CRT and library build
#
# Produces:
#   $(SYSROOT)/lib/crt0.o      — application header + BSS init + callback wrappers
#   $(SYSROOT)/lib/crti.o      — default pceAppNotify (weak, can be overridden)
#   $(SYSROOT)/lib/libpceapi.a — kernel API stubs + utility objects
#   $(SYSROOT)/lib/lib{io,lib,math,string,ctype,fp,idiv}.a
#                              — EPSON SDK libraries converted from SRF lib33 format
#
# Usage:
#   make              — build everything into the sysroot
#   make clean        — remove build intermediates and sysroot outputs
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
PYTHON  := python3
SRF2ELF := $(PYTHON) $(TOPDIR)/tools/srf2elf/srf2elf.py
SDKLIB  := $(TOPDIR)/sdk/lib

TARGET  := s1c33-none-elf
INCLUDE := -I$(SYSROOT)/include

# CRT/library compiler flags
# -ffreestanding: no hosted-env assumptions; suppresses memset/memcpy builtins
# -Wall -Wno-incompatible-library-redeclaration: SDK headers use non-standard
#   function signatures; suppress the resulting redeclaration warnings
CFLAGS_CRT := --target=$(TARGET) -c -ffreestanding \
              $(INCLUDE) \
			  -O1 \
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
	$(SYSROOT)/lib/libctype.a \
	$(SYSROOT)/lib/libfp.a \
	$(SYSROOT)/lib/libidiv.a

.PHONY: all clean

all: $(SYSROOT)/lib/crt0.o \
     $(SYSROOT)/lib/crti.o \
     $(SYSROOT)/lib/libpceapi.a \
     $(SDK_LIBS)

clean:
	rm -rf $(BUILDDIR)
	rm -f $(SYSROOT)/lib/crt0.o \
	      $(SYSROOT)/lib/crti.o \
	      $(SYSROOT)/lib/libpceapi.a \
	      $(SDK_LIBS)

$(BUILDDIR):
	mkdir -p $@

# -----------------------------------------------------------------------
# CRT startup objects (directly linked into every application)
# -----------------------------------------------------------------------

# crt0.o: application header (pceAPPHEAD at 0x100000), BSS zero-fill,
#         pceAppInit00/pceAppProc00/pceAppExit00/pceAppNotify00 wrappers.
$(SYSROOT)/lib/crt0.o: crt0.c | $(BUILDDIR)
	$(CLANG) $(CFLAGS_CRT) $< -o $@

# crti.o: default pceAppNotify (weak symbol; overridable by the application).
$(SYSROOT)/lib/crti.o: defnotify.c | $(BUILDDIR)
	$(CLANG) $(CFLAGS_CRT) $< -o $@

# -----------------------------------------------------------------------
# libpceapi.a — kernel API stubs + utility objects
# -----------------------------------------------------------------------

PCEAPI_STUBS_DIR := $(BUILDDIR)/pceapi_stubs

$(PCEAPI_STUBS_DIR): | $(BUILDDIR)
	mkdir -p $@

# Step 1: compile/assemble utility sources
$(BUILDDIR)/version_check.o: version_check.c | $(BUILDDIR)
	$(CLANG) $(CFLAGS_CRT) $< -o $@

$(BUILDDIR)/def_vbuff.o: def_vbuff.c | $(BUILDDIR)
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
	$(BUILDDIR)/def_vbuff.o \
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
# EPSON SDK libraries — converted from lib33 SRF format to GNU ar ELF
# -----------------------------------------------------------------------

$(SYSROOT)/lib/libio.a:     $(SDKLIB)/io.lib     ; $(SRF2ELF) $< $@
$(SYSROOT)/lib/liblib.a:    $(SDKLIB)/lib.lib    ; $(SRF2ELF) $< $@
$(SYSROOT)/lib/libmath.a:   $(SDKLIB)/math.lib   ; $(SRF2ELF) $< $@
$(SYSROOT)/lib/libstring.a: $(SDKLIB)/string.lib ; $(SRF2ELF) $< $@
$(SYSROOT)/lib/libctype.a:  $(SDKLIB)/ctype.lib  ; $(SRF2ELF) $< $@
$(SYSROOT)/lib/libfp.a:     $(SDKLIB)/fp.lib     ; $(SRF2ELF) $< $@
$(SYSROOT)/lib/libidiv.a:   $(SDKLIB)/idiv.lib   ; $(SRF2ELF) $< $@
