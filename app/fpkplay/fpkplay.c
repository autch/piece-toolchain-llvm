// -*- encoding: shift_jis; -*-
#include <piece.h>
#include <muslib.h>
#include <string.h>

#include "pcestdint.h"
#include "fpk.h"
#include "fpack.h"
#include "lzss.h"

// 仮想画面
uint8_t g_vScreen[128 * 88];
// ダーティフラグ
int g_fDraw;

// 曲データ
uint8_t seq_buffer[65536];
// 展開のバッファ
uint8_t fpk_buffer[65536];

fpack_header_t g_fpkHeader;
// 再生中のファイルエントリと、選択中のファイルエントリ
fpack_file_entry_t g_fpkPlaying, g_fpkSelection;
// 選択中 par ファイルインデックスと、ホールドフラグ
int g_nMusicIndex, g_fHold;

// 今聞いている曲名を表示
void ShowNowPlaying();
// 安全かつ直ちに曲を止める
void musStopImmediately();
// 今選択中の曲を表示
void PrintSelection();
// 未使用：自力で pmd からタイトルを取ってくる
void ReadTitle(unsigned char* pSeq, char* szTitle, int nTitleSize,
                                    char* szTitle2, int nTitle2Size);
// par 用 PlayMusic()
void fpkPlayMusic(int nIndex);

// 意味は VB とか BASIC と同じ :)
void Cls()
{
	memset(g_vScreen, 0, 128 * 88);
	pceFontSetPos(0, 0);
	g_fDraw = 1;
}

// 意味は VB と同じ :)
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
		// par の最初のファイルをとってくる
		fpkGetFileInfoN(g_nMusicIndex, &g_fpkSelection);
		g_fpkPlaying = g_fpkSelection;
		
		fpkPlayMusic(g_nMusicIndex);
		ShowNowPlaying();
		PrintSelection();
	}
	pceLCDDispStart();
}

int error_shown = 0;

void pceAppProc(int cnt)
{
	// fpk を開き損なっていたら
	if(g_nMusicIndex < 0 && !error_shown)
	{
		pceFontSetPos(0, 0); 
		pceFontPrintf(
			"fpkplay.fpkが見つ\n"
			"からないか、LZSS圧縮に\n"
			"なっていません。\n"
			"\n"
			"終了してfpack -eで作った"
			"fpkplay.fpkを配置してから"
			"やり直してください"
		);

		Refresh();

		error_shown = 1;
		return;
	}

	// START ボタン
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
	// ホールド状態でなければ
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
			Cls();
			PrintSelection();
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
	// ホールドのまま終了すると液晶がおかしくなる
	pceLCDDispStart();
}

void PrintSelection()
{
	pceFontSetPos(0, 0);	pceFontPrintf("seek: %-16s", g_fpkSelection.filename);
	g_fDraw = 1;
}

void musStopImmediately()
{
	// まずドライバを止める
	StopMusic();
	// 再生待ちバッファがなくなるのを待つ
	while(pceWaveCheckBuffs(music_wch));
	// カーネルに停止要求を出す
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

// par 用 PlayMusic()
void fpkPlayMusic(int nIndex)
{
	musStopImmediately();
	fpkGetFileInfoN(nIndex, &g_fpkPlaying);
	if(strcmp(strrchr(g_fpkPlaying.filename, '.'), ".pmd") == 0)
	{
		fpkExtractToBuffer(&g_fpkPlaying, fpk_buffer);
		decodeLZSS(fpk_buffer, g_fpkPlaying.size, seq_buffer, sizeof(seq_buffer));
		PlayMusic(seq_buffer);
		ShowNowPlaying();
	}
}

#define AS_WORD(p) ((WORD)*((WORD*)p))

// 未使用：自力で pmd からタイトルを取ってくる
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
