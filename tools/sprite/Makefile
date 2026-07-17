include ../../llvm-c33.makefile

ASM33CONV = $(PYTHON) $(C33HOME)/tools/asm33conv/asm33conv.py

CFLAGS := -O2 $(CFLAGS)

LIBOBJS = pclsprite.o pclspritefast.o pclspbg1c.o pclspraster00.o pclspraster01.o pclspraster02.o pclspraster03.o pclspraster1.o pclspr1c.o pclspcvf.o pclspbgcheck.o pclspbgcheck_asm.o
OBJS = sprtest2.o pat.o

all: lib app install
lib: libsprite.a

PROJECT = sprtest2
CAPTION = SpriteTest2@LLVM

app: $(PROJECT).elf $(PROJECT).pex

$(PROJECT).elf: $(OBJS) libsprite.a
	$(LD) $(LDFLAGS) -o $@ $^
$(PROJECT).pex: $(PROJECT).elf
	$(PPACK) -e $< -o$@ -n$(CAPTION)

libsprite.a : $(LIBOBJS)
	$(AR) -rcu $@ $^

clean:
	rm -f *.o libsprite.a

%.o: %.s
	$(ASM33CONV) $< | $(AS) $(ASFLAGS) -c -x assembler - -o $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


install: $(SYSROOT)/lib/libsprite.a $(SYSROOT)/include/pclsprite.h

$(SYSROOT)/lib/libsprite.a: libsprite.a
	install -D -m 644 $< $@
$(SYSROOT)/include/pclsprite.h: pclsprite.h
	install -D -m 644 $< $@
