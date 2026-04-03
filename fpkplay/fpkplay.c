#include <piece.h>
#include "music/muslib.h"
#include <string.h>

#include "pcestdint.h"
#include "fpk.h"
#include "fpack.h"
#include "lzss.h"

// ���z���
uint8_t g_vScreen[128 * 88];
// �_�[�e�B�t���O
int g_fDraw;

// �ȃf�[�^
uint8_t seq_buffer[65536];
// �W�J�̃o�b�t�@
uint8_t fpk_buffer[65536];

fpack_header_t g_fpkHeader;
// �Đ����̃t�@�C���G���g���ƁA�I�𒆂̃t�@�C���G���g��
fpack_file_entry_t g_fpkPlaying, g_fpkSelection;
// �I�� par �t�@�C���C���f�b�N�X�ƁA�z�[���h�t���O
int g_nMusicIndex, g_fHold;
int g_fDebug, g_dbgPage;

// �������Ă���Ȗ���\��
void ShowNowPlaying();
// ���S�������ɋȂ��~�߂�
void musStopImmediately();
// ���I�𒆂̋Ȃ�\��
void PrintSelection();
void ShowDebug();
// ���g�p�F���͂� pmd ����^�C�g��������Ă���
void ReadTitle(unsigned char* pSeq, char* szTitle, int nTitleSize,
                                    char* szTitle2, int nTitle2Size);
// par �p PlayMusic()
void fpkPlayMusic(int nIndex);

// �Ӗ��� VB �Ƃ� BASIC �Ɠ��� :)
void Cls()
{
	memset(g_vScreen, 0, 128 * 88);
	pceFontSetPos(0, 0);
	g_fDraw = 1;
}

// �Ӗ��� VB �Ɠ��� :)
void Refresh()
{
	if(g_fDraw && !g_fHold)
	{
		pceLCDTrans();
		g_fDraw = 0;
	}
}


void pceAppInit()
{
	pceLCDDispStop();
	pceLCDSetBuffer(g_vScreen);
	pceAppSetProcPeriod(50);

	InitMusic();
	Cls();

	title = title2 = "";
	g_nMusicIndex = -1;
	g_fHold = 0;

	if(fpkOpenArchive("fpkplay.fpk", &g_fpkHeader) == 0 && g_fpkHeader.files_count > 0)
	{
		g_nMusicIndex = 0;
		// par �̍ŏ��̃t�@�C�����Ƃ��Ă���
		fpkGetFileInfoN(g_nMusicIndex, &g_fpkSelection);
		// g_fpkPlaying = g_fpkSelection;
		
		// fpkPlayMusic(g_nMusicIndex);
		// ShowNowPlaying();
		PrintSelection();
	}
	pceLCDDispStart();
}

int error_shown = 0;

void pceAppProc(int cnt)
{
	// fpk ���J�����Ȃ��Ă�����
	if(g_nMusicIndex < 0 && !error_shown)
	{
		pceFontSetPos(0, 0); 
		pceFontPrintf(
			"fpkplay.fpk������\n"
			"����Ȃ����ALZSS���k��\n"
			"�Ȃ��Ă��܂���B\n"
			"\n"
			"�I������fpack -e�ō����"
			"fpkplay.fpk��z�u���Ă���"
			"��蒼���Ă�������"
		);

		Refresh();

		error_shown = 1;
		return;
	}

	// START �{�^��
	if(pcePadGet() & TRG_C)
	{
		if(!g_fHold)
			pceLCDDispStop();
		else
		{
			pceLCDDispStart();
			g_fDraw = 1;
		}
		g_fHold ^= 1;
	}
	// �z�[���h��ԂłȂ����
	if(!g_fHold)
	{
		if(pcePadGet() & TRG_A)
		{
			musStopImmediately();
			fpkPlayMusic(g_nMusicIndex);
		}
		if(pcePadGet() & TRG_B)
		{
			musStopImmediately();
			g_fDebug = 0;
			Cls();
			PrintSelection();
		}
		if(pcePadGet() & TRG_UP)
		{
			/* toggle debug view; UP = show debug data */
			g_fDebug ^= 1;
			g_dbgPage = 0;
			if(g_fDebug) ShowDebug(); else ShowNowPlaying();
		}
		if(pcePadGet() & TRG_DN)
		{
			if(g_fDebug) {
				/* page through debug data */
				g_dbgPage++;
				ShowDebug();
			}
		}
		if(pcePadGet() & TRG_LF)
		{
			if(g_nMusicIndex > 0)	g_nMusicIndex--; else g_nMusicIndex = g_fpkHeader.files_count - 1;
			fpkGetFileInfoN(g_nMusicIndex, &g_fpkSelection);
			PrintSelection();
		}
		if(pcePadGet() & TRG_RI)
		{
			if(g_nMusicIndex < g_fpkHeader.files_count - 1)	g_nMusicIndex++; else g_nMusicIndex = 0;
			fpkGetFileInfoN(g_nMusicIndex, &g_fpkSelection);
			PrintSelection();
		}
	}

	pceAppActiveResponse(MusicCheck() ? AAR_NOACTIVE : AAR_ACTIVE);

	Refresh();
}

void pceAppExit()
{
	musStopImmediately();
	fpkCloseArchive();
	// �z�[���h�̂܂܏I������Ɖt�������������Ȃ�
	pceLCDDispStart();
}

void PrintSelection()
{
	pceFontSetPos(0, 0);	pceFontPrintf("seek: %-16s", g_fpkSelection.filename);
	g_fDraw = 1;
}

void musStopImmediately()
{
	// �܂��h���C�o���~�߂�
	StopMusic();
	// �Đ��҂��o�b�t�@���Ȃ��Ȃ�̂�҂�
	while(pceWaveCheckBuffs(music_wch));
	// �J�[�l���ɒ�~�v�����o��
	pceWaveAbort(music_wch);
}

void ShowNowPlaying()
{
	Cls();

	pceLCDLine(3, 0, 10, 127, 10);
	pceFontSetPos(0, 12); pceFontPrintf("playing: %-16s", g_fpkPlaying.filename);
	pceLCDLine(3, 0, 22, 127, 22);
	pceFontSetPos(0, 24); pceFontPrintf(title);
	pceLCDLine(3, 0, 56, 127, 56);
	pceFontSetPos(0, 58); pceFontPrintf(title2);

	PrintSelection();

	g_fDraw = 1;
}

static void ShowDebugPage0(void)
{
	pceFontSetPos(0, 0);
	pceFontPrintf("(debug removed)");
}

static void ShowDebugPage1(void) {}
static void ShowDebugPage2(void) {}
static void ShowDebugPage3(void) {}

void ShowDebug()
{
	Cls();
	if (g_dbgPage == 0)
		ShowDebugPage0();
	else if (g_dbgPage == 1)
		ShowDebugPage1();
	else if (g_dbgPage == 2)
		ShowDebugPage2();
	else
		ShowDebugPage3();
	g_fDraw = 1;
}

// par �p PlayMusic()
void fpkPlayMusic(int nIndex)
{
	musStopImmediately();
	fpkGetFileInfoN(nIndex, &g_fpkPlaying);
	if(strcmp(strrchr(g_fpkPlaying.filename, '.'), ".pmd") == 0)
	{
		fpkExtractToBuffer(&g_fpkPlaying, fpk_buffer);
		uint32_t decoded_size = decodeLZSS(fpk_buffer, g_fpkPlaying.size, seq_buffer, sizeof(seq_buffer));
		if(decoded_size == 0 || decoded_size != (fpk_buffer[0] | (fpk_buffer[1] << 8) | (fpk_buffer[2] << 16) | (fpk_buffer[3] << 24))) {
			pceFontSetPos(0, 58); pceFontPrintf("LZSS�̓W�J�Ɏ��s���܂���");
			g_fDraw = 1;
			return;
		}

		PlayMusic(seq_buffer);
		ShowNowPlaying();
	}
}

#define AS_WORD(p) ((WORD)*((WORD*)p))

// ���g�p�F���͂� pmd ����^�C�g��������Ă���
void ReadTitle(unsigned char* pSeq, char* szTitle, int nTitleSize,
                                    char* szTitle2, int nTitle2Size)
{
	BYTE* p = pSeq;

	// db 0
	if(!*p) p++;

  // partn X
	BYTE partn = *p++;
	p += partn << 1;

	p += 2;

	if(AS_WORD(p))
		strncpy(szTitle, pSeq + AS_WORD(p), nTitleSize);
	p += 2;
	if(AS_WORD(p))
		strncpy(szTitle2, pSeq + AS_WORD(p), nTitle2Size);
}
