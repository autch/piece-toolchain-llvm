
#include <stdio.h>
#include <limits.h>

#define CRCPOLY  0xedb88320U

static unsigned long crctable[256];
static unsigned long crc_val;
static int crc_table_f = 0;


unsigned long calcrc( unsigned char *c, unsigned n )
{
	unsigned long r;

	r = crc_val;
	while ( n-- )
		r = (r >> CHAR_BIT) ^ crctable[(unsigned char)r ^ *c++];
	return (unsigned long)(( crc_val = r ) ^ 0xffffffff);
}


void calcrc_init( void )
{
	int i, j;
	unsigned long r;

	crc_val = 0xffffffff;

	if ( crc_table_f ) return;

	for (i = 0; i <= 255; i++) {
		r = i;
		for (j = 0; j < 8; j++)
			if (r & 1) r = (r >> 1) ^ CRCPOLY;
			else       r >>= 1;
		crctable[i] = r;
	}
	crc_table_f = 1;
}


