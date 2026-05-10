
/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              System Programs
//
//
// PIECE SAMPLE :  : Ver 1.00
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
//


#include <string.h>
#include <piece.h>
#include "launch.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

PIECE_BMP	pbmp;

extern unsigned char pos;
extern unsigned char draw;

void draw_frame(void);
void renewpos(int dir);

enum OS_SEQUENCE{
	OS_INIT	= 0,
	OS_INIT_MES,
	OS_MAIN,
	OS_EXIT,
};

extern unsigned char sys_bg[];
static int	Sequence = OS_INIT;

extern PIECE_BMP	pbmp;

extern unsigned char vbuff[128*88];

typedef struct {
	int px;		// Ｘ位置
	int py;		// Ｙ位置
	int frame;	// 枠起点Ｙ座標
}OS_WORK;

OS_WORK os;


void sprite2(void);

int OS_Main(void)
{
	unsigned char *vram;

	int	ret = 1;
	int pad = pcePadGet();
//	static int cnt = 0;
	vram = pceLCDSetBuffer(INVALIDPTR);
	switch(Sequence){
		default:
		case OS_INIT:

			memset(vram, 0, 128*88);
			// カーソル位置初期化
			os.px =0;
			os.py =0;
			os.frame = 0;
			renewpos(0);
			getdir();
//			Sequence = OS_INIT_MES;
			Sequence = OS_MAIN;
			pos=0;
			draw_frame();
			break;
/*
		case OS_INIT_MES:
			if(pad || cnt >=25){
				Sequence = OS_MAIN;
				draw_frame();
				cnt = 0;
			}else{
				memset(vram, 0, 128*22);
				memset(vram+128*22, 1, 128*22);
				memset(vram+128*44, 2, 128*22);
				memset(vram+128*66, 3, 128*22);
				pceFontSetPos( 10, 10 );
				pceFontPrintf("PieceOS Ver?");
				cnt++;
			}
			break;
*/
		case OS_MAIN:
			if(pad || draw){
				renewpos(pad);
				draw_frame();
				draw = 0;
			}
			break;

		case OS_EXIT:
			break;
	}
	return ret;
}

char default_img[]= {
	 68,  68,  68,  68,  68,  68,  68,  68,  17,  17,  17,  17,  17,  17,  17,  17,
	 68,  68,  68,  68,  68,  68,  68,  68,  17,  17,  17,  17,  17,  17,  17,  17,
	 68,  68,  68,  68,  68,  68,  68,  68,  17,   0,   0,   0,  17,  17,  17,  17,
	 70,  21,  85,  85,   4,  68,  68,  68,  18,  21,  85,  85,  64,   0,   1, 145,
	 70,  21,  85,  85,  85,  85,  86, 196,  18,  21,  85,  85,  85,  85,  86, 209,
	 70,  21,  85,  85,  85,  85,  86, 196,  18,  21,  85,  85,  85,  85,  86, 209,
	 70,  21,  85,  85,  85,  85,  86, 196,  18,  21,  85,  85,  85,  85,  86, 209,
	 70,  21,  85,  85,  85,  85,  86, 196,  18,  21,  85,  85,  85,  85,  86, 209,
	 70,  21,  85,  85,  85,  85,  86, 196,  18,  21,  85,  85,  85,  85,  86, 209,
	 70,  21,  85,  85,  85,  85,  86, 196,  18,  21,  85,  85,  85,  85,  86, 209,
	 70,  21,  85,  85,  85,  85,  86, 196,  18,  21,  85,  85,  85,  85,  86, 209,
	 70,  21,  85,  85,  85,  85,  86, 196,  18,  21,  85,  85,  85,  85,  86, 209,
	 70, 170, 170, 170, 170, 170, 170, 196,  17,  17,  17,  17,  17,  17,  17,  17,
	 68,  68,  68,  68,  68,  68,  68,  68,  17,  17,  17,  17,  17,  17,  17,  17,
	 68,  68,  68,  68,  68,  68,  68,  68,  17,  17,  17,  17,  17,  17,  17,  17,
	 68,  68,  68,  68,  68,  68,  68,  68,  17,  17,  17,  17,  17,  17,  17,  17
};

//１バイト４ピクセル簡易描画！
void bitmapdraw(int x, int y, int w, int h, char *adr)
{
	int i,j;
	unsigned char *vram,*ptr;
	vram = pceLCDSetBuffer(INVALIDPTR);
	for(j=y;j<(y+h<88?y+h:88);j++){			//範囲外クリップ
		for(i=0;i<w/4;i++,adr++){
			ptr = (vram+(x+i*4)+128*j);
			*(ptr+0) = (*adr>>6)&0x3;
			*(ptr+1) = (*adr>>4)&0x3;
			*(ptr+2) = (*adr>>2)&0x3;
			*(ptr+3) = (*adr>>0)&0x3;
		}
	}
}

int pt1[3][4]={
	{22,24,26,28,},
	{18,16,14,12,},
	{17,12, 7, 2,},
};

int pt2[3][4]={
	{23,28,33,38,},
	{19,20,21,22,},
	{18,16,14,12,},
};
int oft[3]={
	0,45,85,
};

void draw_frame(void)
{
	int i,j,off;
	int x,y;
	long* vraml;
	unsigned char *vram;
	PBMP_FILEHEADER	pbhead;
	DRAW_OBJECT obj;
	char *icon_data;
	char buff[256+1];

	vram = pceLCDSetBuffer(INVALIDPTR);

	//画面ベース
	vraml = (long*)vram;
	for(j=0;j<88;j+=2){
		for(i=0;i<32;i++){
			*(vraml+i+j*32)=0x00010001;
			*(vraml+i+32+j*32)=0x01000100;
		}
	}
	memcpy((char *)&pbhead, sys_bg, sizeof(PBMP_FILEHEADER));
	pbmp.header = pbhead;
	pbmp.buf = sys_bg+sizeof(PBMP_FILEHEADER);
	pbmp.mask= sys_bg+sizeof(PBMP_FILEHEADER)+(pbmp.header.h*pbmp.header.w)/4;

	if(os.frame==0){
		memset(vram+128* 0, 2, 128*8);
		pceLCDSetObject(&obj, &pbmp,   0,  0, 64, 0, 20,  8, DRW_NOMAL);
		pceLCDDrawObject(obj);
		pceLCDSetObject(&obj, &pbmp, 102,  0, 84, 0, 24,  8, DRW_NOMAL);
		pceLCDDrawObject(obj);
	}else{
		//上カーソル
//		pceLCDSetObject(&obj, &pbmp,  56,  0,  0, 0, 16, 8, DRW_NOMAL);
		pceLCDSetObject(&obj, &pbmp,   8,  0,  0, 0, 16, 8, DRW_NOMAL);
		pceLCDDrawObject(obj);
		pceLCDSetObject(&obj, &pbmp, 104,  0,  0, 0, 16, 8, DRW_NOMAL);
		pceLCDDrawObject(obj);
	}
	if(filec <= os.frame*3 + 6 || ((os.py+1)*3 >= filec)){
		memset(vram+128*80, 2, 128*8);
		pceLCDSetObject(&obj, &pbmp,   2, 80, 84, 0, 24,  8, DRW_NOMAL);
		pceLCDDrawObject(obj);
		pceLCDSetObject(&obj, &pbmp, 106, 80,108, 0, 20,  8, DRW_NOMAL);
		pceLCDDrawObject(obj);
	}else{
		//下カーソル
//		pceLCDSetObject(&obj, &pbmp,  56, 80, 16, 0, 16, 8, DRW_NOMAL);
		pceLCDSetObject(&obj, &pbmp,   8, 80, 16, 0, 16, 8, DRW_NOMAL);
		pceLCDDrawObject(obj);
		pceLCDSetObject(&obj, &pbmp, 104, 80, 16, 0, 16, 8, DRW_NOMAL);
		pceLCDDrawObject(obj);
	}
	//アイコン描画
	for(i=os.frame*3;i<filec;i++){
		if(files[i].iconf){		//iconfがなければ(オフセット０)、デフォルトイメージを表示
			geticondata(files[i].name, buff);
			icon_data = buff;
		}else{
			icon_data = default_img;
		}
		bitmapdraw(4+44*((i-os.frame*3)%3), 8+36*((i-os.frame*3)/3), 32, 32, (char *)icon_data);
	}

	//選択カーソル
	off=(os.py-os.frame);

	memcpy(&pbhead, sys_bg, sizeof(PBMP_FILEHEADER));
	pbmp.header = pbhead;
	pbmp.buf = sys_bg+sizeof(PBMP_FILEHEADER);
	pbmp.mask= sys_bg+sizeof(PBMP_FILEHEADER)+(pbmp.header.h*pbmp.header.w)/4;

	pceLCDSetObject(&obj, &pbmp,  4 + 44*os.px,  8 + 36*off, 32,  0,  8,  8, DRW_NOMAL);
	pceLCDDrawObject(obj);
	pceLCDSetObject(&obj, &pbmp, 28 + 44*os.px,  8 + 36*off, 40,  0,  8,  8, DRW_NOMAL);
	pceLCDDrawObject(obj);
	pceLCDSetObject(&obj, &pbmp,  4 + 44*os.px, 32 + 36*off, 48,  0,  8,  8, DRW_NOMAL);
	pceLCDDrawObject(obj);
	pceLCDSetObject(&obj, &pbmp, 28 + 44*os.px, 32 + 36*off, 56,  0,  8,  8, DRW_NOMAL);
	pceLCDDrawObject(obj);

	// 吹き出し
	if(off){
		y= 44;
		for(x=0;x<4;x++)
			pceLCDLine(2, oft[os.px]+pt1[os.px][x], y-1-x, oft[os.px]+pt2[os.px][x], y-1-x);
		pceLCDPaint(2, 21, 20, 86, 20);
	}else{
		y= 39;
		for(x=0;x<4;x++)
			pceLCDLine(2, oft[os.px]+pt1[os.px][x], y+1+x, oft[os.px]+pt2[os.px][x], y+1+x);
		pceLCDPaint(2, 21, 44, 86, 20);
	}

	//選択ファイルのテキスト
	pceFontSetBkColor(2);
	pceFontSetTxColor( 0 );

	if(off)
		pceFontSetPos( 64-(strlen(files[pos].caption))*5/2, 20);
	else
		pceFontSetPos( 64-(strlen(files[pos].caption))*5/2, 44);
	pceFontPrintf("%s\n",files[pos].caption);	// キャプション

	if(off)
		pceFontSetPos( 64-(strlen(files[pos].name))*5/2, 30);
	else
		pceFontSetPos( 64-(strlen(files[pos].name))*5/2, 54);
	pceFontPrintf("%s\n",files[pos].name);	// キャプション

	pceFontSetBkColor(0);
	pceFontSetTxColor(3);

#if 0
	//選択ファイルのテキスト
	pceFontSetPos( 32, 80 );
	pceFontSetBkColor(2);
	pceFontSetTxColor( 0 );
	pceFontPrintf("%s\n",files[pos].name);	// ファイル名
	if(off)
		pceFontSetPos( 64-(strlen(files[pos].caption))*5/2, 32);
	else
		pceFontSetPos( 64-(strlen(files[pos].caption))*5/2, 42);

	pceFontPrintf("%s\n",files[pos].caption);	// キャプション
	pceFontSetBkColor(0);
	pceFontSetTxColor(3);
#endif

}

void renewpos(int dir){
	int num;

	num = filec;
	//ポジション整合性チェック(範囲外の時の処理)
	if(3*os.py+os.px > num){
		os.px = num%3;
		os.py = num/3;
		if(os.py <= 0)
			os.frame = 0;
		else
			os.frame = os.py-1;

	}
	if(dir&TRG_LF){
		if((num+2)/3-1 > os.py || num%3==0){	//３つある
			if(os.px==0)
				os.px = 2;
			else
				os.px--;
		}else{
			if(num%3==1){
				os.px = 0;
			}else{
				if(os.px==0)
					os.px = 1;
				else
					os.px = 0;
			}
		}
		pos = os.px+os.py*3;
	}
	if(dir&TRG_RI){
		if((num+2)/3-1 > os.py || num%3==0){	//３つある
			if(os.px==2)
				os.px = 0;
			else
				os.px++;
		}else{
			if(num%3==1)
				os.px = 0;
			else{
				if(os.px==0)
					os.px = 1;
				else
					os.px = 0;
			}
		}
		pos = os.px+os.py*3;
	}
	if(dir&TRG_UP){
		if(os.py > 0){
			os.py--;
			if(os.py < 1)
				os.frame = 0;
			else
				os.frame = os.py;
		}
		pos = os.px+os.py*3;
	}
	if(dir&TRG_DN){
		if((os.py+1)*3+(os.px) < num){
			os.py++;
			if(os.py > 1)
				os.frame = os.py-1;
		}else if((os.py+1)*3 < num){
			os.py++;
			if(os.py > 1)
				os.frame = os.py-1;
			os.px=(num+2)%3;
		}
		pos = os.px+os.py*3;
	}
	if (dir&TRG_A)
		run( files + pos );
}

unsigned char sys_bg[]={
	 80,  77,  66,  80, 148,   1,   0,   0,   2,   1, 128,   0,   8,   0,   0,   0,
	128,   1,   0,   0,   0,  15, 240,   0, 255, 255, 255, 255,  15, 252,  63, 240,
	 63,   0,   0, 252, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170,
	170, 170, 170, 170,   0,  58, 172,   0, 234, 170, 170, 171,  53,  87, 213,  92,
	213, 192,   3,  71, 170, 170, 170, 170, 170, 160, 168, 168,  42,   2, 160, 170,
	170, 170, 170, 170,   0, 213,  87,   0,  53,  85,  85,  92, 208,   7, 192,   7,
	209, 192,   3,  71, 168,  10, 168,  42, 162, 136, 168, 168, 170,  42, 162, 138,
	170,  10, 170,   2,   3,  64,   1, 192,  13,   0,   0, 112, 208,  23, 212,   7,
	208, 124,  61,   7, 160,   2, 160,  10, 128, 136, 162, 162, 168, 170, 138,   2,
	168,   2, 168,   0,  13,   0,   0, 112,   3,  64,   1, 192, 208, 124,  61,   7,
	208,  23, 212,   7, 160,   2, 160,  10, 128, 130, 162, 160, 168, 170, 130,   2,
	168,   2, 168,   0,  53,  85,  85,  92,   0, 213,  87,   0, 209, 192,   3,  71,
	208,   3, 208,   7, 168,  10, 168,  42, 162,  42, 138, 138, 162, 170,  42, 138,
	170,  10, 170,   2, 234, 170, 170, 171,   0,  58, 172,   0, 209, 192,   3,  87,
	 53,  87, 213,  92, 170, 170, 170, 170, 170,  42, 138, 130, 160,  42,  10, 170,
	170, 170, 170, 170, 255, 255, 255, 255,   0,  15, 240,   0,  63,   0,   0, 252,
	 15, 252,  63, 240, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170,
	170, 170, 170, 170,   3, 192, 255, 255,  62, 124, 112,  14, 255, 255, 255, 255,
	255, 255, 255, 255,   7, 224, 255, 255, 127, 254, 248,  31, 255, 255, 255, 255,
	255, 255, 255, 255,  15, 240, 127, 254, 255, 255, 248,  31, 255, 255, 255, 255,
	255, 255, 255, 255,  31, 248,  63, 252, 255, 255, 254, 127, 255, 255, 255, 255,
	255, 255, 255, 255,  63, 252,  31, 248, 254, 127, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 127, 254,  15, 240, 248,  31, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,   7, 224, 248,  31, 127, 254, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,   3, 192, 112,  14,  62, 124, 255, 255, 255, 255,
	255, 255, 255, 255,
};

