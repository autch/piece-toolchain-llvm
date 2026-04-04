include ../llvm-c33.makefile

all: jien.pex

jien.pex: jien.elf
	$(PPACK) -e -n"jien@LLVM" $< -o$@
jien.elf: jien.o jien_bmp.o
	$(LD) $(LDFLAGS) -o $@ $^
jien.o: jien.c
	$(CC) $(CFLAGS) -c $< -o $@

jien.s: jien.c
	$(CC) $(CFLAGS) -S $< -o $@

clean:
	rm -f *.o jien.elf jien.pex jien.map jien.i jien.s jien.bc jien_bmp.i jien_bmp.s jien_bmp.bc jien_bmp.bc
