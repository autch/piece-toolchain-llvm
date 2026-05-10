
/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              System Programs
//
//
// PIECE SAMPLE : menu2 : Ver 1.00
//
// Copyright (C)2001 AUQAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
//
// Coded by MIO.H (OeRSTED)
//
// Comments:
//
// 
//
//  v1.00 2001.11.09 MIO.H
//  v1.01 2001.11.29 MIO.H  SMRESUME時に再描画する
//



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <piece.h>
#include "launch.h"


unsigned char vbuff[128*88];

unsigned char pos;
unsigned char draw;

extern int OS_Main(void);


void disp_stat( void )
{
	PCETIME time;
	static unsigned char last_sec;

	pceTimeGet( &time );
	if ( last_sec != time.ss ) {
		last_sec = time.ss;
		pceFontSetPos( 128-5*20, 88-10 );
		pceFontPrintf( "%04d-%02d-%02d %02d:%02d:%02d",
			time.yy, time.mm, time.dd, time.hh, time.mi, time.ss );
		draw = 1;
	}
}


void pceAppInit( void )
{
	pceLCDDispStop();

	//pcePowerForceBatt(1);	// for test

	pceLCDSetBuffer( vbuff );
	pceAppSetProcPeriod( 40 );

	pceLCDTrans();

	pceLCDDispStart();

	pceCPUSetSpeed(CPU_SPEED_HALF);
}



void pceAppProc( int cnt )
{
	int a = pcePadGet();

	if ( a )
		getdir();
	if ( (a & TRG_B) && (a & PAD_SELECT) ) {
		go_standby();
		draw = 4;
	}
	
	OS_Main();
	pceLCDTrans();
}


void pceAppExit( void )
{
	pceCPUSetSpeed(CPU_SPEED_NORMAL);
}


