
#include <stdio.h>
#include <stdlib.h>
#include "zlib.h"               /* /usr/local/include/zlib.h */

#define OUTBUFSIZ (512*1024)

typedef unsigned char u_char;

unsigned zlbencode( u_char *inptr, unsigned size, u_char *code )
{
	z_stream z;                     /* ライブラリとやりとりするための構造体 */
    int status;

    /* すべてのメモリ管理をライブラリに任せる */
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
#if 0
    /* 初期化 */
    /* 第2引数は圧縮の度合。0～9 の範囲の整数で，0 は無圧縮 */
    /* Z_DEFAULT_COMPRESSION (= 6) が標準 */
    if (deflateInit(&z, Z_DEFAULT_COMPRESSION) != Z_OK) {
#else
    if (deflateInit2_(&z, 6, Z_DEFLATED, 10, /*DEF_MEM_LEVEL=*/8,
			 Z_DEFAULT_STRATEGY, ZLIB_VERSION, sizeof(z_stream)) != Z_OK) {
#endif

        fprintf(stderr, "deflateInit: %s\n", (z.msg) ? z.msg : "???");
        exit(1);
    }

    z.avail_in = 0;		/* 入力バッファ中のデータのバイト数 */
    z.next_in = inptr;		/* 入力ポインタを入力バッファの先頭に */
    z.avail_in = size;		/* データを読み込む */
    z.next_out = code;		/* 出力ポインタ */
    z.avail_out = OUTBUFSIZ;	/* 出力バッファのサイズ */

    /* 通常は deflate() の第2引数は Z_NO_FLUSH にして呼び出す */

    while (1) {
        status = deflate(&z, Z_FINISH); /* 圧縮する */
        if (status == Z_STREAM_END) break; /* 完了 */
        if (status != Z_OK) {   /* エラー */
            fprintf(stderr, "deflate: %s\n", (z.msg) ? z.msg : "???");
            exit(1);
        }
    }

    /* 後始末 */
    if (deflateEnd(&z) != Z_OK) {
        fprintf(stderr, "deflateEnd: %s\n", (z.msg) ? z.msg : "???");
        exit(1);
    }

	return OUTBUFSIZ - z.avail_out;
}

unsigned zlbdecode( u_char *inptr, unsigned size, u_char *data )
{
	z_stream z;                     /* ライブラリとやりとりするための構造体 */
    int status;

    /* すべてのメモリ管理をライブラリに任せる */
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;

    /* 初期化 */
    z.next_in = Z_NULL;
    z.avail_in = 0;
    if (inflateInit(&z) != Z_OK) {
        fprintf(stderr, "inflateInit: %s\n", (z.msg) ? z.msg : "???");
        exit(1);
    }

    z.next_in = inptr;  /* 入力ポインタを元に戻す */
    z.avail_in = size; /* データを読む */
    z.next_out = data;        /* 出力ポインタ */
    z.avail_out = OUTBUFSIZ;    /* 出力バッファ残量 */
    status = Z_OK;

    while (status != Z_STREAM_END) {
        status = inflate(&z, Z_NO_FLUSH); /* 展開 */
        if (status == Z_STREAM_END) break; /* 完了 */
        if (status != Z_OK) {   /* エラー */
            fprintf(stderr, "inflate: %s\n", (z.msg) ? z.msg : "???");
            exit(1);
        }
    }

    /* 後始末 */
    if (inflateEnd(&z) != Z_OK) {
        fprintf(stderr, "inflateEnd: %s\n", (z.msg) ? z.msg : "???");
        exit(1);
    }

	return OUTBUFSIZ - z.avail_out;
}


