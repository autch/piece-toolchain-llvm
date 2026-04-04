
C33HOME = $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
SDK = $(C33HOME)/sdk
BIN = $(C33HOME)/build/bin
SYSROOT = $(C33HOME)/sysroot/s1c33-none-elf

CC = $(BIN)/clang --sysroot=$(SYSROOT)
CFLAGS = -O1 -flto=full -fno-inline -Wall -Wno-incompatible-library-redeclaration
CXX = $(BIN)/clang++ --sysroot=$(SYSROOT)
CXXFLAGS = $(CFLAGS)
AS = $(BIN)/clang --sysroot=$(SYSROOT)
ASFLAGS =
LD = $(BIN)/clang --sysroot=$(SYSROOT)
LDFLAGS = -flto=full
PPACK = $(C33HOME)/tools/ppack/ppack

AR = $(BIN)/llvm-ar
