C33HOME = $(PWD)/../
SDK=$(C33HOME)/sdk
BIN=$(C33HOME)/build/bin
SYSROOT=$(C33HOME)/sysroot/s1c33-none-elf

CC=$(BIN)/clang
CFLAGS=-O1 -Wall -I$(SYSROOT)/include
# crt0.c uses -O1 to prevent the BSS-clearing loop variable from being
# spilled to the stack.  At -O0 the loop var lives at [SP+0] which would
# be within the BSS range if the kernel sets SP at bss_end.
CFLAGS_CRT0=-ffreestanding -O1 -Wall -I$(SYSROOT)/include
LD=$(BIN)/ld.lld -T $(C33HOME)/tools/piece.ld
LDFLAGS=-Map minimal.map
PPACK=$(C33HOME)/tools/ppack/ppack
PCEAPI_OBJS=$(SYSROOT)/lib/libpceapi.a

all: mini1.pex mini2.pex

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

mini1.pex: mini1.elf
	$(PPACK) -e -n"mini@NC" $< -o$@
mini2.pex: mini2.elf
	$(PPACK) -e -n"mini@NC_NF" $< -o$@
mini1.elf: crt0.o mini1.o
	$(LD) $(LDFLAGS) -o $@ $^ $(PCEAPI_OBJS)
mini2.elf: crt0.o mini2.o
	$(LD) $(LDFLAGS) -o $@ $^ $(PCEAPI_OBJS)

mini1.o: mini.c
	$(CC) $(CFLAGS) -c $< -o $@
mini2.o: mini.c
	$(CC) $(CFLAGS) -DNO_FILLOUT_VBUF -c $< -o $@

mini1.s: mini.c
	$(CC) $(CFLAGS) -S $< -o $@
mini2.s: mini.c
	$(CC) $(CFLAGS) -DNO_FILLOUT_VBUF -S $< -o $@


crt0.o: crt0.c
	$(CC) $(CFLAGS_CRT0) -c $< -o $@
crt0.s: crt0.c
	$(CC) $(CFLAGS_CRT0) -S $< -o $@
clean:
	rm -f *.o mini*.elf mini*.pex mini*.map *.i *.s *.bc
