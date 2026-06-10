
C33HOME = $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
SDK = $(C33HOME)/sdk
BIN = $(C33HOME)/build/bin
SYSROOT = $(C33HOME)/sysroot/s1c33-none-piece

ifdef LTO
    LTOFLAGS = -flto=full -ffat-lto-objects
else
    LTOFLAGS =
endif

CC = $(BIN)/clang --sysroot=$(SYSROOT)
CFLAGS = $(LTOFLAGS) -fno-inline
CXX = $(BIN)/clang++ --sysroot=$(SYSROOT)
CXXFLAGS = $(CFLAGS)
AS = $(BIN)/clang --sysroot=$(SYSROOT)
ASFLAGS =
LD = $(BIN)/clang --sysroot=$(SYSROOT)
LDFLAGS = $(LTOFLAGS)
PPACK = $(C33HOME)/tools/ppack/ppack

AR = $(BIN)/llvm-ar
