include ../llvm-c33.makefile

# Piece 用 makefile

# 拡張子の定義

.SUFFIXES: .c .s .h .o .srf
#.SUFFIXES: .c .s .h .o .srf

# 生成コマンド・オプションのマクロ


# 生成規則

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

.s.o:
	$(AS) -c $(ASFLAGS) -o $@ $<

# 構成ファイル・生成ファイルのマクロ

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

lib: $(LIBNAME).a

$(LIBNAME).a : $(LIBOBJS) $(INSTOBJS)
	$(AR) -q $(LIBNAME).a $(LIBOBJS) $(INSTOBJS)

# 依存関係
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


# フラッシュ書き込みイメージ生成
pex : $(PRGNAME).srf
	ppack -e $(PRGNAME).srf -o$(FILENAME).pex -n$(CAPTION) -i$(ICON)


# クリーンアップ
clean:
	rm -f $(PRGNAME).srf $(PRGNAME).pex $(PRGNAME).sym $(PRGNAME).map *.o $(LIBNAME).a wave/*.o

# テスト実行
run: $(PRGNAME).srf
	cmd /c run

# インストール
install:
	copy $(LIBNAME).lib ..\..\lib
	copy muslib.h ..\..\include
	copy musdef.h ..\..\include
