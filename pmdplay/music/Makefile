#NO_LTO=1
include ../../llvm-c33.makefile

CFLAGS := -O2 $(CFLAGS) -DFASTCODE

# Piece �p makefile

# �g���q�̒�`

.SUFFIXES: .c .s .h .o .srf
#.SUFFIXES: .c .s .h .o .srf

# �����R�}���h�E�I�v�V�����̃}�N��


# �����K��

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

.s.o:
	$(AS) -c $(ASFLAGS) -o $@ $<

# �\���t�@�C���E�����t�@�C���̃}�N��

PRGNAME = sample
LIBNAME = muslib
OBJS =	app.o testseq.o instdef.o
LIBOBJS = mus.o musfast.o seq.o exp12.o wavetable.o instdef.o
FILENAME = music
CAPTION = MusicSample
ICON = music.pid

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

#$(PRGNAME).srf : $(OBJS) $(LIBOBJS)
#	$(LD) $(LDFLAGS) -e$(PRGNAME).srf $(OBJS) $(LIBOBJS) $(INSTOBJS)

all: lib

lib: $(LIBNAME).a

$(LIBNAME).a : $(LIBOBJS) $(INSTOBJS)
	rm -f $(LIBNAME).a
	$(AR) -rc $(LIBNAME).a $(LIBOBJS) $(INSTOBJS)

# �ˑ��֌W
#
testseq.o : testseq.dat
app.o : muslib.h
mus.o : muslib.h musdef.h
seq.o : musdef.h
instdef.o : musdef.h
wavetable.o : musdef.h

musfast.o: musfast_llvm.s
	$(AS) -c $(ASFLAGS) -o $@ $<
musfast_llvm.s: musfast.s
	python $(C33HOME)/tools/asm33conv/asm33conv.py $< -o $@


# �t���b�V���������݃C���[�W����
pex : $(PRGNAME).srf
	ppack -e $(PRGNAME).srf -o$(FILENAME).pex -n$(CAPTION) -i$(ICON)


# �N���[���A�b�v
clean:
	rm -f $(PRGNAME).srf $(PRGNAME).pex $(PRGNAME).sym $(PRGNAME).map *.o $(LIBNAME).a wave/*.o

# �e�X�g���s
run: $(PRGNAME).srf
	cmd /c run

# �C���X�g�[��
install:
	copy $(LIBNAME).lib ..\..\lib
	copy muslib.h ..\..\include
	copy musdef.h ..\..\include
