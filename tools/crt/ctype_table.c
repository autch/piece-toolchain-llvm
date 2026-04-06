/*
 * _ctype_ — newlib-compatible character classification table.
 *
 * newlib's <ctype.h> macros (isalpha, isdigit, …) expand to direct array
 * lookups into _ctype_[].  The SDK's libctype.a was built from gcc33 source
 * and provides only the function-call forms (isalpha(), …); it has no
 * _ctype_[] data object.  This file bridges that gap.
 *
 * Flag values match <ctype.h> (newlib):
 *   _U=001  _L=002  _N=004  _S=010  _P=020  _C=040  _X=0100  _B=0200
 *
 * Layout: _ctype_[0] is unused (guards against -1 index); entries 1..256
 * correspond to characters 0..255.
 */

#define _U  0x01u   /* uppercase */
#define _L  0x02u   /* lowercase */
#define _N  0x04u   /* digit */
#define _S  0x08u   /* whitespace (space, \t, \n, \r, \f, \v) */
#define _P  0x10u   /* punctuation */
#define _C  0x20u   /* control */
#define _X  0x40u   /* hex digit (A-F, a-f) */
#define _B  0x80u   /* blank (space, \t) */

const char _ctype_[1 + 256] = {
    0,           /* index 0: guard entry (for EOF = -1 lookups) */

    /* 0x00–0x07: NUL SOH STX ETX EOT ENQ ACK BEL */
    _C, _C, _C, _C, _C, _C, _C, _C,
    /* 0x08–0x0f: BS  HT  LF  VT  FF  CR  SO  SI */
    _C, _C|_S, _C|_S, _C|_S, _C|_S, _C|_S, _C, _C,
    /* 0x10–0x17: DLE DC1 DC2 DC3 DC4 NAK SYN ETB */
    _C, _C, _C, _C, _C, _C, _C, _C,
    /* 0x18–0x1f: CAN EM  SUB ESC FS  GS  RS  US */
    _C, _C, _C, _C, _C, _C, _C, _C,
    /* 0x20–0x27: SPC !   "   #   $   %   &   '  */
    _S|_B, _P, _P, _P, _P, _P, _P, _P,
    /* 0x28–0x2f: (   )   *   +   ,   -   .   /  */
    _P, _P, _P, _P, _P, _P, _P, _P,
    /* 0x30–0x37: 0   1   2   3   4   5   6   7  */
    _N, _N, _N, _N, _N, _N, _N, _N,
    /* 0x38–0x3f: 8   9   :   ;   <   =   >   ?  */
    _N, _N, _P, _P, _P, _P, _P, _P,
    /* 0x40–0x47: @   A   B   C   D   E   F   G  */
    _P, _U|_X, _U|_X, _U|_X, _U|_X, _U|_X, _U|_X, _U,
    /* 0x48–0x4f: H   I   J   K   L   M   N   O  */
    _U, _U, _U, _U, _U, _U, _U, _U,
    /* 0x50–0x57: P   Q   R   S   T   U   V   W  */
    _U, _U, _U, _U, _U, _U, _U, _U,
    /* 0x58–0x5f: X   Y   Z   [   \   ]   ^   _  */
    _U, _U, _U, _P, _P, _P, _P, _P,
    /* 0x60–0x67: `   a   b   c   d   e   f   g  */
    _P, _L|_X, _L|_X, _L|_X, _L|_X, _L|_X, _L|_X, _L,
    /* 0x68–0x6f: h   i   j   k   l   m   n   o  */
    _L, _L, _L, _L, _L, _L, _L, _L,
    /* 0x70–0x77: p   q   r   s   t   u   v   w  */
    _L, _L, _L, _L, _L, _L, _L, _L,
    /* 0x78–0x7f: x   y   z   {   |   }   ~   DEL */
    _L, _L, _L, _P, _P, _P, _P, _C,

    /* 0x80–0xff: high bytes — no classification in default C locale */
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
};
