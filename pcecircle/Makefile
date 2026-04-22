include ../llvm-c33.makefile

all: pcecircl.pex

pcecircle.elf: pcecircle.o pceLCDCircle.o pceLCDEllipse.o
	$(LD) $(CFLAGS) -Wl,-Map,pcecircle.map -o $@ $^

pcericle.o: pcericle.c pcecircle.h

pex: pcecircl.pex
pcecircl.pex: pcecircle.elf
	$(PPACK) -e -v -n"pcecircle@llvm" $< -o$@

copy: pcecircl.pex
	isd =w"$<"
#	isd =w"pmdplay.fpk"


%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o pcecircle.elf
