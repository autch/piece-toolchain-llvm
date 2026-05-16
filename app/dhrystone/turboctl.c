/*
 * Derived from clippce.[ch]:
 *	CLiP - Common Library for P/ECE
 *	Copyright (C) 2001-2015 Naoyuki Sawa
 */

#include "turboctl.h"
#include <piece.h>
#include <string.h>

#include <s1c33cpu.h>
#include "c33209.h"

/* /usr/PIECE/sysdev/pcekn/pcekn.hより宣言のみコピー。
 * 実体はpceapi.libに含まれているので、ユーザーアプリケーションからも利用できます。
 * [使い方]
 *	ENTER_CS; // “__cs__ = PSR; PSR(IE) = 0;”に相当
 *	// 割り込み禁止が必要な処理を行う
 *	LEAVE_CS; // “PSR = __cs__;”に相当
 */
typedef unsigned long CRITICAL_SECTION;
void pceSyncEnterCriticalSection(CRITICAL_SECTION* pcs); /* 割り込み許可状態を保存→割り込み禁止 */
void pceSyncLeaveCriticalSection(CRITICAL_SECTION* pcs); /* 割り込み許可状態を復元 */
#define ENTER_CS do { CRITICAL_SECTION __cs__; pceSyncEnterCriticalSection(&__cs__)
#define LEAVE_CS pceSyncLeaveCriticalSection(&__cs__); } while(0)

void pdwait(int n)
{
    asm("ext 32");
    asm("ext 0");
    asm("ld.w %r13, 0");    // xld.w %r13, 0x0100'0000
    asm("ld.w %r14, %r13");
    asm("mac %r12");
}

void die_at(const char* file, int line)
{
    memset(pceLCDSetBuffer(INVALIDPTR), 0, 128 * 88);
    pceFontSetPos(0, 0);
    pceFontPrintf("DIE at %s:%d\n", file, line);
    pceLCDTrans();
    while(1) {
        pdwait(1000);
    }
}

#define DIE() die_at(__FILE__, __LINE__)


/****************************************************************************
 *	24MHz/48Mhzモード切り替え
 ****************************************************************************/

/* * 「P/ECE hacking」さん(http://junkbox.info/piece/)の
 *   『P/ECEを48MHzで動かす(1～3/3)』(http://junkbox.info/diary/dy200202.htm)を参考にさせて頂きました。
 * * SRAM/FLASHのオーバークロックは行っていません。
 *   今回の目標はあくまで各部品の仕様内で、CPUのみ48MHz動作させることだからです。
 * * 48MHz動作中に利用できないAPIは、次のとおりです。
 *	      pceCPUSetSpeed()			OSC3の倍速設定が想定されていないため
 *	      pcePowerEnterStandby()		 8bitタイマ1を設定しているため
 *	      pceTimerGetPrecisionCount()	後述
 *	      pceTimerAdjustPrecisionCount()	後述
 *	重要! pceFileCreate/Delete/WriteSec()	後述
 *	      pceIR系API全て			16bitタイマ5を設定しているため
 *   これらのAPIを利用する場合は、いったん24MHzモードに戻してから利用してください。
 * * pcePowerEnterStandby()は、アプリケーションから明示的に呼ばれる場合に加えて、
 *   電池駆動・五分間無操作時にシステムから自動的に呼ばれる場合もあります。
 *   本当は、 48MHzへの切り替え時に、pcePowerEnterStandby()だけはフックしておきたかったのですが、
 *   システム内からのpcePowerEnterStandby()呼び出しはフックを経由していないので、できませんでした。
 *   仕方がないので、48MHzモードを使うアプリケーションに次のようなコードを追加して対処してください。
 *	int pceAppNotify(int type, int param) {
 *		int m;
 *		switch(type) {
 *		case APPNF_SMSTART:	return APPNR_ACCEPT;
 *		case APPNF_SMREQVBUF:	pceLCDSetBuffer(_def_vbuff); return APPNR_ACCEPT;
 *		case APPNF_STANDBY:	m = turbo(0); pcePowerEnterStandby(0); turbo(m); return APPNR_REJECT;
 *		}
 *		return APPNR_IGNORE;
 *	}
 *   もっとも、pcePowerEnterStandby()内での8bitタイマ1利用目的は、
 *   スタンバイ復帰時のOSC3発振安定待ちのためであり、さほど重要ではありません。
 *   いや、重要といえば重要なのですが、待ち時間には充分なマージンが取られているはずなので、
 *   待ち時間が半分になった程度では即問題は発生しないと思うのです。
 *   もともと、初期のカーネルでは待ち時間の設定が正しく行われていなかった、にも関らず問題なく動いていたので、
 *   あまり気にしなくてもいいのかもしれません。
 *   というわけで、結論としては、アプリケーション側で上記コード(pceAppNotify)による対策を取ったほうが安全だが、
 *   対策しなくてもまあいいかな？、という結論になります。
 *   ちなみに、なぜCLiPライブラリ内に上記コード(pceAppNotify)を含めてしまわなかったかというと、
 *   pceAppNotify()はもっと多目的に利用されるので、アプリケーション側で定義できなくなってしまうのは問題だと思うからです。
 *   ↓2004/11/05追記
 *   pceAppNotify()をライブラリに含めました。
 *   アプリケーションがNotify処理を必要とする場合は、atnotify()を呼び出して関数を登録してください。
 * * 48MHzモードでは、P/ECEカーネル内のウェイト関数pdwait(\usr\PIECE\sysdev\pcekn\critical.c)のタイミングも
 *   変わってしまうのが心配ですが、いまのところとりあえず大丈夫みたいです。
 *   pdwait()のタイミングの大半は、CPU自身の命令実行時間ではなく、エリア11(0x1000000～)へのアクセス時間によって決定されます。
 *   エリア11のアクセスタイミングは、最低速に設定されています。(\usr\PIECE\sysdev\pcekn\hard.c 143行)
 *   外部バスへのアクセスなので、24MHz/48MHzの違いには影響されず、エリア11のアクセスはどちらでも同じです。
 *   エリア11のアクセス時間に較べれば、CPU自身の命令実行時間は小さいので、影響は小さいはずです。
 *   確かにpdwait()のタイミングは速くなってしまいますが、ぎりぎりの調整がなされていない限り問題ないと思います。
 *   唯一心配なのは、フラッシュメモリの消去待ちループ内での利用です。(\usr\PIECE\sysdev\pcekn\fmacc1.c 69行)
 *   pceFileCreate/Delete/WriteSec()利用時は、24MHzモードに戻しておいた方が安全でしょう。
 *   (※未検証です。もしかしたら48MHzでも問題なく使えるかもしれません)
 *   →ちょっと検証してみます。P/ECEのフラッシュメモリSST39VF400Aでは、1セクタ消去に要する最大時間は32[ミリ秒]です。
 *   \usr\PIECE\sysdev\pcekn\fmacc1.c 63～76行は、pdwait(5)×一万回の呼び出し以内に消去が完了しないと、タイムアウトします。
 *   簡単のために、mac命令五万回分の待ち時間と仮定します。(実際はもっとかかります)
 *   mac命令一回毎にエリア11から二回の読み出しを行うので、(1(基本リード)＋7(ウェイト))×2＝16クロックかかります。
 *   外部バスアクセスですので、ここでのクロック単位は、48MHzではなく24MHzです。
 *   1秒/24MHz×16クロック×五万回＝33ミリ秒となり、1セクタ消去に要する時間を満たしています。
 *   実際にはpdwait()の呼び出し時間やmac以外の命令実行時間も含まれるので、もっと多くのウェイトが取られているはずです。
 *   以上の検証より、48MHzモード中にpceFileCreate/Delete/WriteSec()を使ってもたぶん大丈夫だろうと思います。
 *   なお、フラッシュメモリへのワード書き込み(\usr\PIECE\sysdev\pcekn\fmacc2.c)は、
 *   タイムアウトなしで書き込み完了するまで待ち続けますので、pdwait()のタイミングには関係ありません。
 *   →と思ったのですが、実験してみると、48MHzモードではかなりの高確率でpceFileWriteSct()に失敗するようです。←★★★重要★★★
 *   一時的にturbo(0)で24MHzに戻し、直後にpceFileWriteCreate/Open/WriteSct/Close、そしてturbo(1)で48MHzを復元、
 *   とすれば、いまのところ失敗はありません。
 *   やっぱり、48MHzモードでのフラッシュメモリ書き込みはやめたほうがいいようです。
 *   →2002/12/10追記: 48MHzモードでpceFileCreate/WriteSecを使うとハングアップしていた理由がわかりました。
 *   フラッシュメモリへの書き込みが問題なのではなく、上記APIが内部でpceCPUSetSpeed()を呼び出しているのが原因でした。
 *   \usr\PIECE\sysdev\pcekn\file.c内のフラッシュメモリ消去/書き込み用補助関数の中でpceCPUSetSpeed(3)を呼び出し、
 *   いったん3MHzモードにしてフラッシュメモリ消去/書き込みを行っています。
 *   そして、pceCPUSetSpeed()によって、USBCエリアのアクセススピードが既定値に戻ってしまうのが、ハングアップの原因です。
 *   48MHzモードでpceFileCreate/WriteSecを使っても、USB通信が発生するまでは限りハングアップしないようですので、
 *   上記の推測はたぶん間違いないと思います。
 *   結論として、48MHzモードでpceFileCreate/WriteSecが使えないことには変わりありません。
 * * 16bitタイマ0(1msタイマ)の再設定について。
 *   他の16bitタイマ再設定と違って、これだけプリスケーラ分周比ではなくリロード値を再設定しています。
 *   このようにした理由は、P/ECEカーネル(BIOS 1.18)による分周比設定(=4)の倍(=8)の設定がないからです。
 *   止むを得ず、リロード値を倍(5999→11999)にしています。
 *   16bitタイマ0のカウント値は、実時間ベースで24MHz時の倍速で変化することになり、
 *   また、カウント値の最大値も24MHz時の倍の値までカウントアップされます。
 *   このため、pceTimerGetPrecisionCount()/pceTimerAdjustPrecisionCount()は正しい値を返しません。
 *   幸い、P/ECEカーネル内部では、16bitタイマ0のカウント値や上記APIは利用されていません。
 *   アプリケーション側でも、48MHz動作中は16bitタイマ0のカウント値や上記APIを利用しないようご注意ください。
 *   (pceTimerGetPrecisionCount()をフックして戻り値を半分にし、擬似的に24MHz互換の値を返すことは可能ですが、
 *    今回はそこまでやらないことにしました。)
 *   →2004/12/02追記: pceTimerGetPrecisionCount()はそのままで、pceTimerAdjustPrecisionCount()をTurboモード対応に置き換えました。
 *                     このモジュールの下の方にある、pceTimerAdjustPrecisionCount()の実装を参照してください。
 * * USBCのアクセスタイミング設定について。
 *   本来、48MHzモードにしても外部バスへのアクセスタイミングは変わらないので、
 *   USBCのアクセスタイミングを遅くする必要はないはずです。
 *   しかし実際には、現在のP/ECEカーネル(BIOS 1.18)では、USBCへのアクセスタイミングは、
 *   USBCの規定値よりも速く設定してあり、CPU側での処理時間も含めて要求タイミングを満たしています。
 *   (例えば通常の24MHzモードでも、連続してUSBCのポートを読み書きすると正しくアクセスできません。)
 *   48MHzモードではCPU側の処理時間が短くなるので、そのままでは要求タイミングを満たせず、
 *   USB通信ができなくなってしまいます。
 *   この問題を防ぐため、USBCのアクセスタイミングを遅くすることにしました。
 *   設定値は、S1C33209のBCUで可能な一番遅い設定にしてあります。(※設定値の検証はまだ行っていません)
 * * 8bitタイマ5(LCD転送)の再設定は行っていません。
 *   現在のP/ECEカーネル(BIOS 1.18)は、LCDCへのタイミングをかなり遅めに設定しているからです。
 *   P/ECEのLCDC、S6B0741のデータシート(66ページ)によると、SPI接続・2.7V以上で使った場合、
 *   S6B0741は約17MHzまでのSCLKについてくるようです。
 *   P/ECEカーネルは、LCDC転送速度を6MHzに設定しています。(\usr\PIECE\sysdev\pcekn\lcd.c 701～705行)
 *   倍速になっても12MHzですので、充分許容範囲です。
 *   それに、転送速度を遅くすると、pceLCDTrans()のライン追い越し問題発生する恐れもあります。(※実験まだです)
 *   以上の理由により、8bitタイマ5(LCD転送)の再設定は行わないことにしました。
 * * さて、24MHz→48MHzモードに切り替えることにより、どれぐらい速くなるか、です。
 *   コードを高速RAMに、データとスタックをSRAMに置いた場合を想定します。
 *   具体的には、高速RAM上に置いたnew_LCDDrawObject()の連続呼び出しで、24MHzと48MHz時の実行時間を比較しました。
 *   実験の結果、24MHz時に較べ、約20～30%程度の高速化となるようです。
 *   アプリケーション全体の実行時間の20～30%というと、MOD再生時間がまるまる儲かるぐらいの時間です。
 *   即ち、24MHzでBGMなしの場合と、48MHzでMODによるBGM再生を付けた場合の処理の重さがだいたい同じ、ということです。
 *   それなりに使いみちはありそうです。。
 */

static int
turbo_CLOCK(int mode)
{
	int old_mode = !bP0_P0D_P07D;		/* 直前のモードを保存(0<->1逆であることに注意!) */

	mode = mode != 0;			/* 新しいモードの指定を確実に0か1にする */
	if(old_mode == mode) return old_mode;	/* 変化なしなら何もしない */

	/* * 48MHzモードに切り替える前に、現在の設定がP/ECEカーネル(BIOS 1.18)の規定値かどうか検査しています。
	 *   P/ECEカーネルの規定値と異なっていた場合は、アサートします。
	 *   もしも今後、カーネルのバージョンアップによって規定値が変化した場合は、修正が必要です。
	 * * DI～EI内でDIE()した場合、エラー画面は表示されますが、SELECTによる終了はできません。
	 *   終了するには、背面のリセットボタンを押してください。
	 *   DIE()前にEIを発行して対処することは可能ですが、わざとやらないことにしました。
	 *   中途半端な設定状態メニューに戻ってしまうと非常に危険なので、
	 *   それよりもリセットボタンでの完全リセットを強制した方が安全だからです。
	 *   ↓2004/12/18 Naoyuki Sawa 変更
	 *   割り込み禁止/許可の制御は、呼び出し元のturbo()関数内で行うように変更しました。
	 */
	if(mode) {
		/* 16bitタイマ0(1msタイマ) */
		if(!bT16_CTL0_PRUN0) DIE();		bT16_CTL0_PRUN0 = 0;
		if(pT16_CR0B != 5999) DIE();		pT16_CR0B = 11999;
							bT16_CTL0_PRUN0 = 1; /* PRESETなしでも大丈夫かな？ */

		/* 16bitタイマ1(サウンド再生) */
		if(!bT16_CTL1_PRUN1) DIE();		bT16_CTL1_PRUN1 = 0;
		if(bCLKCTL_T16_1_P16TS1 != 0) DIE();	bCLKCTL_T16_1_P16TS1 = 1;
							bT16_CTL1_PRUN1 = 1; /* PRESETなしでも大丈夫かな？ */

		/* 16bitタイマ4(USB 6MHz) */
		if(!bT16_CTL4_PRUN4) DIE();		bT16_CTL4_PRUN4 = 0;
		if(bCLKCTL_T16_4_P16TS4 != 0) DIE();	bCLKCTL_T16_4_P16TS4 = 1;
							bT16_CTL4_PRUN4 = 1; /* PRESETなしでも大丈夫かな？ */

		/* USBコントローラのアクセスタイミング調整 */
		if(bA8_7_A8WT != 4) DIE();		bA8_7_A8WT = 7; /* 6でも一応動くようです */
		if(bA8_7_A8DF != 2) DIE();		bA8_7_A8DF = 3; /* 2でも一応動くようです */

		/* 入出力兼用ポート7(OSC3クロック制御) */
		if(bP0_P0D_P07D != 1) DIE();		bP0_P0D_P07D = 0;

	} else {
		/* 16bitタイマ0(1msタイマ) */
		if(!bT16_CTL0_PRUN0) DIE();		bT16_CTL0_PRUN0 = 0;
		if(pT16_CR0B != 11999) DIE();		pT16_CR0B = 5999;
							bT16_CTL0_PRUN0 = 1; /* PRESETなしでも大丈夫かな？ */

		/* 16bitタイマ1(サウンド再生) */
		if(!bT16_CTL1_PRUN1) DIE();		bT16_CTL1_PRUN1 = 0;
		if(bCLKCTL_T16_1_P16TS1 != 1) DIE();	bCLKCTL_T16_1_P16TS1 = 0;
							bT16_CTL1_PRUN1 = 1; /* PRESETなしでも大丈夫かな？ */

		/* 16bitタイマ4(USB 6MHz) */
		if(!bT16_CTL4_PRUN4) DIE();		bT16_CTL4_PRUN4 = 0;
		if(bCLKCTL_T16_4_P16TS4 != 1) DIE();	bCLKCTL_T16_4_P16TS4 = 0;
							bT16_CTL4_PRUN4 = 1; /* PRESETなしでも大丈夫かな？ */

		/* USBコントローラのアクセスタイミング調整 */
		if(bA8_7_A8WT != 7) DIE();		bA8_7_A8WT = 4;
		if(bA8_7_A8DF != 3) DIE();		bA8_7_A8DF = 2;

		/* 入出力兼用ポート7(OSC3クロック制御) */
		if(bP0_P0D_P07D != 0) DIE();		bP0_P0D_P07D = 1;
	}

	return old_mode;
}

static int
turbo_FLASH_WAIT(int mode)
{
	/* 現在のモードを取得します。 */
	int old_mode = bA10_9_A10WT == 1;	/* 1WAITならモード1、2WAITならモード0。ありえないが、1or2WAIT以外もモード0と見なす */

	/* 新しいモード指定を確実に0か1とします。 */
	mode = mode != 0;

	/* モード変化なしなら、何もしません。 */
	if(old_mode == mode) return old_mode;

	/* 新しいモードに応じたウェイトを設定します。 */
	bA10_9_A10WT = mode ? /*モード1*/1 : /*モード0*/2;

	return old_mode;
}

static int
turbo_SRAM_WAIT(int mode)
{
	/* 現在のモードを取得します。 */
	int old_mode = bA6_4_A5WT == 1;		/* 1WAITならモード1、2WAITならモード0。ありえないが、1or2WAIT以外もモード0と見なす */

	/* 新しいモード指定を確実に0か1とします。 */
	mode = mode != 0;

	/* モード変化なしなら、何もしません。 */
	if(old_mode == mode) return old_mode;

	/* 新しいモードに応じたウェイトを設定します。 */
	bA6_4_A5WT = mode ? /*モード1*/1 : /*モード0*/2;

	return old_mode;
}

static int
turbo_FLASH_OUTPUT_DISABLE(int mode)
{
	/* 現在のモードを取得します。 */
	int old_mode = bA10_9_A10DF == 0;	/* 0.5Cycleならモード1、1.5Cycleならモード0。ありえないが、0.5or1.5Cycle以外もモード0と見なす */

	/* 新しいモード指定を確実に0か1とします。 */
	mode = mode != 0;

	/* モード変化なしなら、何もしません。 */
	if(old_mode == mode) return old_mode;

	/* 新しいモードに応じた出力ディセーブル遅延時間を設定します。 */
	bA10_9_A10DF = mode ? /*モード1*/0 : /*モード0*/1;

	return old_mode;
}

static int
turbo_SRAM_OUTPUT_DISABLE(int mode)
{
	/* 現在のモードを取得します。 */
	int old_mode = bA6_4_A5DF == 0;		/* 0.5Cycleならモード1、1.5Cycleならモード0。ありえないが、0.5or1.5Cycle以外もモード0と見なす */

	/* 新しいモード指定を確実に0か1とします。 */
	mode = mode != 0;

	/* モード変化なしなら、何もしません。 */
	if(old_mode == mode) return old_mode;

	/* 新しいモードに応じた出力ディセーブル遅延時間を設定します。 */
	bA6_4_A5DF = mode ? /*モード1*/0 : /*モード0*/1;

	return old_mode;
}

int
turbo(int mode)
{
	int old_mode = 0;

ENTER_CS;
									/* ---cffss */
	old_mode |= turbo_CLOCK               ((mode >> 4) & 1) << 4;	/* ---c---- */
	old_mode |= turbo_FLASH_OUTPUT_DISABLE((mode >> 3) & 1) << 3;	/* ----f--- */
	old_mode |= turbo_FLASH_WAIT          ((mode >> 2) & 1) << 2;	/* -----f-- */
	old_mode |= turbo_SRAM_OUTPUT_DISABLE ((mode >> 1) & 1) << 1;	/* ------s- */
	old_mode |= turbo_SRAM_WAIT           ((mode >> 0) & 1) << 0;	/* -------s */

LEAVE_CS;

	return old_mode;
}

/* * Sat Dec 2 06:00:00 JST 2004 Naoyuki Sawa
 * - TurboON/OFF両モード対応のpceTimerAdjustPrecisionCount()を追加しました。
 *   pceTimerGetPrecisionCount()は、TurboONモードでも、P/ECEカーネル内のルーチンをそのまま利用して大丈夫です。
 * - 実行時にベクタをフックするのではなく、リンク時にpceapi.lib内のpceTimerAdjustPrecisionCount()を置き換えます。
 *   カーネル自身はpceTimerAdjustPrecisionCount()を呼び出していないので、ベクタをフックする必要はないからです。
 */
unsigned long
pceTimerAdjustPrecisionCount(unsigned long st, unsigned long ed)
{
	int tc = pT16_CR0B + 1;				/* 1[ms]未満の変化量の単位 = [(1/tc)ms] */
	int al = (ed & 0xffff) - (st & 0xffff);		/* 1[ms]未満の変化量 */
	int ah = ((ed >> 16) - (st >> 16)) & 0xffff;	/* 1[ms]単位の変化量 */

	/* (1[ms]未満の変化量)から(1[ms]単位の変化量)への繰り下がりを計算します。 */
	if(al < 0) {
		al += tc;
		ah--;
	}

	/* (1[ms]未満の変化量)の単位を[(1/tc)ms]→[us]に変換します。 */
	al = al * 1000 / tc;

	/* (1[ms]単位の変化量)の単位を[ms]→[us]に変換します。 */
	ah = ah * 1000;

	/* 1[us]単位の変化量を合計して返します。 */
	return ah + al;
}
