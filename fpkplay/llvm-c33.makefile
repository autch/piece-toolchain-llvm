
C33HOME = $(HOME)/src/llvm-c33
SDK = $(C33HOME)/sdk
BIN=$(C33HOME)/build/bin

CC = $(BIN)/clang --sysroot=$(C33HOME)/sysroot/s1c33-none-elf
CFLAGS = -g -O1 -fno-inline -Wall -DFASTCODE -I$(SDK)/include -Wno-incompatible-library-redeclaration
AS = $(BIN)/clang --sysroot=$(C33HOME)/sysroot/s1c33-none-elf
ASFLAGS = -g
LD = $(BIN)/clang --sysroot=$(C33HOME)/sysroot/s1c33-none-elf
LDFLAGS = -g
PPACK = $(C33HOME)/tools/ppack/ppack

AR = $(BIN)/llvm-ar
