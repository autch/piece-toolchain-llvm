/*

ランチャー（メニュー）に必要な処理がまとめてあります。

と言っても、v1.14以降は簡単になっています。
（以前はアドレスの関係でかなり工夫や制約が必要でした。）

使い方が分かったら、無理にこのソースのまま使う必要はありません。

launch.h, APIマニュアルの pceAppExecFile() も参照してください。


ここには入ってませんが、ランチャーでは

pceAppInit で pceCPUSetSpeed(CPU_SPEED_HALF) を

pceAppExit で pceCPUSetSpeed(CPU_SPEED_NORMAL) を

呼んで、省電力を図る方が望ましいです。（可能であれば）

重い処理をしたいとかあれば、マメに切り替えるのも手ですね。

また、画面の更新などは必要な場合だけにして、
pceAppProc 内に居る時間は極力短くしてください。

いろいろなランチャーが出てくるといいですね。
個人的には、P/ECEなつみが使ってくれることに期待…(^^;;;

MIO.H

*/

#include <stdio.h>
#include <string.h>
#include <piece.h>
#include "launch.h"

FILES files[FMAX];
unsigned char filec;


static pffsFileHEADER *getfh( char *fname )
{
	FILEACC facc;
	int s;

	if ( pceFileOpen( &facc, fname, FOMD_RD ) ) return NULL;

	s = pceFileReadSct( &facc, NULL, 0, 4096 );

	pceFileClose( &facc );

	if ( s < 24 ) return NULL;

	return (pffsFileHEADER *)facc.aptr;
}


// 実行可能ファイル(拡張子が ".pex" )かどうかを判断します。
// ランチャー自身は対象から外します。
static int nameck( char *name )
{
	if ( !strcmp( name+strlen(name)-4, ".pex" ) ) {
		return strcmp( name, "startup.pex" );
	}
	return 0;
}



void getdir( void )
{
	FILEINFO fi;
	int i = 0;
	pceFontSetPos( 0, 10 );
	pceFileFindOpen( &fi );
	while ( pceFileFindNext( &fi ) ) {
		if ( nameck( fi.filename ) ) {
			pffsFileHEADER *pfh = getfh( fi.filename );
			if ( pfh ) {
				FILES *pf = files + i;
				strncpy( pf->name, fi.filename, FNMAX );
				strncpy( pf->caption, (char *)pfh + pfh->ofs_name, CPMAX );
				pf->iconf = !!pfh->ofs_icon;
				pf->length = fi.length;
				pf->adrs = pfh->top_adrs;
				if ( ++i >= FMAX ) break;
			}
		}
	}
	pceFileFindClose( &fi );
	filec = i;
}


int geticondata(char *filename, char *buff)
{
	pffsFileHEADER *pfh = getfh( filename );
	if ( pfh && pfh->ofs_icon) {
		memcpy( buff, (char *)pfh + pfh->ofs_icon, (32/4)*32 );
		return 0;
	}
	return -1;
}


//
// これが実行のメインです。
// すっかり単純になってしまいました。
//
void run( FILES *pf )
{
	pceAppExecFile( pf->name, 0 );
}


//
// 標準メニューでは
// 簡単スンタンバイ（SELECT+B）はここに流れてきます。
//
void go_standby( void )
{
	pcePowerEnterStandby(0);

	pceUSBReconnect();
}



