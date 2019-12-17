#ifndef _UTIL_H_
#define _UTIL_H_

#include <stddef.h>

#define CEIL(a, b) (((a) + (b) - 1) / (b))

int memcpy_t(const void *src, void *dest, unsigned int len);
void memset(void *mem, char c, unsigned int size);

#endif