#ifndef itoa_h
#define itoa_h

int itoa(int val, char* buf, int radix);
int itoa_pad(int val, char* buf, int radix, int width, char pad);

#endif // !itoa_h
