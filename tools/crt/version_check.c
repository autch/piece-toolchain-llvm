/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              System Programs
//
//
// PIECE LIBRARY : pceapi : Ver 1.00
//
// Copyright (C)2001 AUQAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
//
// Coded by MIO.H (OeRSTED)
//
// Comments:
//
//	○ デフォルトのバージョンチェック処理
//
//  v1.09 2001.11.30 MIO.H
//

#include <piece.h>
#include <string.h>

extern unsigned char _def_vbuff[128*88];

int __version_check( int opef )
{
	const SYSTEMINFO *sip = pceSystemGetInfo();

	if ( sip->bios_ver >= APPSYSVER ) return 0;	// OK

	if ( opef == 1 ) {
		pceLCDDispStop();
		pceLCDSetBuffer( _def_vbuff );
		memset( _def_vbuff, 0, 128*88 );
		pceFontSetPos( 0, 10 );
		pceFontPrintf(
			"このプログラムは、\n"
			"BIOS ver%d.%02d 以上\n"
			"でないと動作しません。\n\n"
			"Aボタンを押すと\n"
			"終了します。",
			APPSYSVER>>8,
			APPSYSVER&255
			);
		pceLCDTrans();
		pceLCDDispStart();
	}
	else if ( opef ) {
		if ( pcePadGet() & TRG_A ) pceAppReqExit(0);
	}

	return 1;
}




