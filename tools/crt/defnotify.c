#include <piece.h>

extern unsigned char _def_vbuff[128*88];

/* __attribute__((weak)): allows applications to override pceAppNotify.
 * Without this, directly linking crti.o alongside an app that defines its
 * own pceAppNotify would produce a duplicate-symbol link error. */
__attribute__((weak)) int pceAppNotify( int type, int param )
{
	switch ( type ) {
		case APPNF_SMSTART:
			return APPNR_ACCEPT;
		case APPNF_SMREQVBUF:
			pceLCDSetBuffer( _def_vbuff );
			return APPNR_ACCEPT;
	}
	return APPNR_IGNORE;
}

