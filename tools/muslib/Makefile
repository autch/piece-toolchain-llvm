include ../../llvm-c33.makefile

LIBOBJS = mus.o musfast.o seq.o exp12.o wavetable.o instdef.o
INSTOBJS =\
	wave/i_BD909.o\
	wave/i_CYMBD.o\
	wave/i_HANDCLAP.o\
	wave/i_HC909.o\
	wave/i_HO909.o\
	wave/i_SD909.o\
	wave/i_SDGATE.o\
	wave/i_TOMH1.o\
	wave/i_TOML1.o\
	wave/i_TOMM1.o\

all: lib install
lib: libmuslib.a

libmuslib.a : $(LIBOBJS) $(INSTOBJS)
	$(AR) -rcu $@ $^

musfast.o: musfast_llvm.s
	$(AS) $(ASFLAGS) -c -o $@ $<
musfast_llvm.s: musfast.s
	python $(C33HOME)/tools/asm33conv/asm33conv.py $< -o $@

clean:
	rm -f *.o libmuslib.a wave/*.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


install: $(SYSROOT)/lib/libmuslib.a $(SYSROOT)/include/musdef.h $(SYSROOT)/include/muslib.h

$(SYSROOT)/lib/libmuslib.a: libmuslib.a
	cp $< $@
$(SYSROOT)/include/musdef.h: musdef.h
	cp $< $@
$(SYSROOT)/include/muslib.h: muslib.h
	cp $< $@
