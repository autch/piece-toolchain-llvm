# Top-level Makefile for S1C33 LLVM backend development.
#
# After any change to the backend source, run `make` from the repo root.
# This rebuilds the backend + clang (so clang-22 picks up the new codegen),
# then regenerates the sysroot (tools/crt) so CRT/pceapi are always fresh.
#
# Targets:
#   all      -- default: build LLVM/clang, then regenerate sysroot
#   llvm     -- ninja only (skip sysroot regeneration)
#   sysroot  -- regenerate tools/crt sysroot only
#   tests    -- run S1C33 lit tests
#   clean    -- clean build artefacts and sysroot

BUILD_DIR := build
# Directory holding the built LLVM tools. Override for build layouts that
# place binaries elsewhere, e.g. MSVC puts them under build/Release/bin.
BIN       ?= $(BUILD_DIR)/bin
NINJA     := ninja -C $(BUILD_DIR)

.PHONY: all llvm sysroot tests clean

all: llvm sysroot

llvm:
	$(NINJA) clang llc lld llvm-ar llvm-ranlib llvm-nm llvm-objdump llvm-objcopy

sysroot:
	$(MAKE) -C tools/crt all

tests:
	$(NINJA) check-llvm-codegen-s1c33 check-llvm-mc-s1c33 check-clang-codegen-s1c33
	$(BIN)/llvm-lit -sv \
		llvm/clang/test/Driver/s1c33-toolchain.c \
		llvm/clang/test/Driver/s1c33-cxx-defaults.cpp \
		llvm/clang/test/Sema/s1c33-interrupt-handler-attr.c \
		llvm/clang/test/Preprocessor/s1c33-target-defines.c

clean:
	$(MAKE) -C tools/crt clean
