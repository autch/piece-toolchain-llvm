include ../../llvm-c33.makefile

# picolibc retargeting layer: stdio console (stdout discard / stdin EOF)
# and sbrk for the libc heap above the kernel pceHeap zone.
#
# These sources include picolibc's <stdio.h> (FILE / FDEV_SETUP_STREAM),
# which during coexistence is NOT yet the sysroot's stdio.h (that may still
# be newlib's).  Compile against the staged picolibc headers directly, with
# the bare target (no --sysroot), so picortt always matches the picolibc
# libc.a it is paired with.
PICOLIBC_INC ?= $(C33HOME)/build/crt/picolibc-stage/include

PICORTT_CC := $(BIN)/clang --target=s1c33-none-piece
CFLAGS_PICORTT := -ffreestanding -fno-builtin -Os -Wall -I$(PICOLIBC_INC)

LIBOBJS = console.o picesbrk.o

all: lib install
lib: libpicortt.a

libpicortt.a: $(LIBOBJS)
	$(AR) -rcu $@ $^

clean:
	rm -f *.o libpicortt.a

%.o: %.c
	$(PICORTT_CC) $(CFLAGS_PICORTT) -c $< -o $@

install: $(SYSROOT)/lib/libpicortt.a

$(SYSROOT)/lib/libpicortt.a: libpicortt.a
	install -D -m 644 $< $@
