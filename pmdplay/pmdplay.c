#include <piece.h>
#include <string.h>
#include "music/muslib.h"
#include "pcestdint.h"
#include "pmd/pmd_data.h"

const unsigned char* g_musicSeqs[] = {
	pmd2s_01_bin,
	pmd2s_eew_bin,
	pmd2s_neruneru_bin,
	pmd2s_neruneru2_bin,
	pmd2s_yzs007_16_bin,
	pmd2s_yzs010_opna_bin,
	pmd2s_eew0_bin,
	pmd2s_eew1_bin,
	pmd2s_eew2_bin,
	pmd2s_eew3_bin,
	pmd2s_eew4_bin,
	pmd2s_eew5_bin,
	pmd2s_eewA_bin,
	NULL
};


uint8_t g_vScreen[128 * 88];
int g_fDraw;

int g_maxMusicIndex;
int g_selectedIndex, g_playingIndex;
int g_fDebug, g_dbgPage;

void ShowNowPlaying();
void musStopImmediately();
void PrintSelection();
void ReadTitle(unsigned char* pSeq, char* szTitle, int nTitleSize,
                                    char* szTitle2, int nTitle2Size);
void fpkPlayMusic(int nIndex);

void ShowDebug(); // pmdplay_debug.c


void Cls()
{
	memset(g_vScreen, 0, 128 * 88);
	pceFontSetPos(0, 0);
	g_fDraw = 1;
}

void Refresh()
{
	if(g_fDraw)
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

	g_maxMusicIndex = 0;
	unsigned char** ppSeq = g_musicSeqs;
	while(*ppSeq) {
		g_maxMusicIndex++;
		ppSeq++;
	}

	g_fDebug = g_dbgPage = 0;

	title = title2 = "";

	g_selectedIndex = 0;
	PrintSelection();
	pceLCDDispStart();
}

void pceAppProc(int cnt)
{
	if(pcePadGet() & TRG_A)
	{
		musStopImmediately();
		fpkPlayMusic(g_selectedIndex);
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
		if(g_selectedIndex > 0)	g_selectedIndex--; else g_selectedIndex = g_maxMusicIndex - 1;
		PrintSelection();
	}
	if(pcePadGet() & TRG_RI)
	{
		if(g_selectedIndex < g_maxMusicIndex - 1)	g_selectedIndex++; else g_selectedIndex = 0;
		PrintSelection();
	}

	Refresh();
}

void pceAppExit()
{
	musStopImmediately();
}

void PrintSelection()
{
	pceFontSetPos(0, 0);	pceFontPrintf("seek: %d / %d", g_selectedIndex + 1, g_maxMusicIndex);
	g_fDraw = 1;
}

void musStopImmediately()
{
	StopMusic();
	while(pceWaveCheckBuffs(music_wch));
	pceWaveAbort(music_wch);
}

void ShowNowPlaying()
{
	Cls();

	pceLCDLine(3, 0, 10, 127, 10);
	pceFontSetPos(0, 12); pceFontPrintf("playing: %d / %d", g_playingIndex + 1, g_maxMusicIndex);
	pceLCDLine(3, 0, 22, 127, 22);
	pceFontSetPos(0, 24); pceFontPrintf(title);
	pceLCDLine(3, 0, 56, 127, 56);
	pceFontSetPos(0, 58); pceFontPrintf(title2);

	PrintSelection();

	g_fDraw = 1;
}


void fpkPlayMusic(int nIndex)
{
	musStopImmediately();
	g_playingIndex = nIndex;
	PlayMusic((unsigned char*)g_musicSeqs[nIndex]);
	ShowNowPlaying();
}

#define AS_WORD(p) ((WORD)*((WORD*)p))

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
