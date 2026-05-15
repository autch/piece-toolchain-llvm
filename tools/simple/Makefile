include ../../llvm-c33.makefile

CFLAGS := -O2 $(CFLAGS)

LIBOBJS = simple.o pat0.o pat1.o pspdisp.o pspload.o home.o homex.o homey.o pset.o printnum.o printstr.o \
	locate.o printnl.o scroll.o scan.o cls.o siprintf.o wait.o pad.o rnd.o inchr.o sppos.o spdisp.o \
	spmove.o spstop.o spcont.o spposx.o spposy.o spscan.o s1.o s2.o s3.o s4.o sound.o spchr.o spstat.o \
	thread.o in_pset.o dmy_val.o pmdload.o pmdplay.o definst.o line.o fill.o symbol.o dispmode.o \
	point.o circle.o ppdload.o ppdplay.o msec.o bgload.o bgset.o
DEFINST = definst.o

all: lib install
lib: libsimple.a libdefinst.a

libsimple.a : $(LIBOBJS)
	$(AR) -rcu $@ $^
libdefinst.a : $(DEFINST)
	$(AR) -rcu $@ $^

clean:
	rm -f *.o libsimple.a libdefinst.a

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


install: $(SYSROOT)/lib/libsimple.a $(SYSROOT)/lib/libdefinst.a $(SYSROOT)/include/simple.h

$(SYSROOT)/lib/libsimple.a: libsimple.a
	cp $< $@
$(SYSROOT)/lib/libdefinst.a: libdefinst.a
	cp $< $@
$(SYSROOT)/include/simple.h: simple.h
	cp $< $@

