include llvm-c33.makefile

all: pmdplay.pex

pmdplay.elf: pmdplay.o pmdplay_debug.o music/muslib.a pmd/pmd_data.a
	$(LD) $(CFLAGS) -Wl,-Map,pmdplay.map -o $@ $^

music/muslib.a:
	make -C music muslib.a
pmd/pmd_data.a:
	make -C pmd pmd_data.a

pmdplay.o: pmdplay.c pmd/pmd_data.h
pmdplay_debug.o: pmdplay_debug.c

pex: pmdplay.pex
pmdplay.pex: pmdplay.elf
	$(PPACK) -e -v -n"PMD player@llvm" $< -o$@

pmdplay.s: pmdplay.c
	$(CC) $(CFLAGS) -S -o $@ $<

copy: pmdplay.pex
	isd =w"$<"
#	isd =w"pmdplay.fpk"


%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o pmdplay.elf
	make -C music clean
	make -C pmd clean
