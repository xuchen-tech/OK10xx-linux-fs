//-----------------------------------------------------------------------------
// 
// Copyright (c) 2016, NXP Semiconductors
// Copyright 2017 NXP Semiconductors
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Author York Sun <york.sun@nxp.com>
// 
//-----------------------------------------------------------------------------

#ifndef __LIB_H__
#define __LIB_H__

#include "types.h"
#include "errno.h"
#include "uart.h"

#define offsetof(type, member) ((unsigned long)&((type *)0)->member)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define min(x, y) ({				\
        typeof(x) _min1 = (x);			\
        typeof(y) _min2 = (y);			\
        (void) (&_min1 == &_min2);		\
        _min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({				\
        typeof(x) _max1 = (x);			\
        typeof(y) _max2 = (y);			\
        (void) (&_max1 == &_max2);		\
        _max1 > _max2 ? _max1 : _max2; })

static inline void bzero(register void *b, register unsigned long len)
{
	while(len--)
		*(unsigned char *)b++ = 0;
}


static inline int fls1(register unsigned long mask)
{
	register int i;

	if (mask == 0)
		return 0;

	for (i = 1; mask != 1; i++) {
		mask >>= 1;
	}

	return i;
}

static inline unsigned long ilog2(register unsigned long x)
{
	return fls1(x) - 1;
}

unsigned int crc32(unsigned int crc, const void *buf, unsigned int size);

#if ((DEBUG_BUILD) && (CNFG_UART))
#define debug(s) puts(s)
#define debug_int(s, a) { puts(s); print_uint(a); puts("\n"); }
#define debug_hex(s, a) { puts(s); print_hex(a); puts("\n"); }

#define dbgprint_uint(a) print_uint(a)
#define dbgprint_hex(a)  print_hex(a)

#else
#define debug(s) {}
#define debug_int(s, a) {}
#define debug_hex(s, a) {}

#define dbgprint_uint(a) {}
#define dbgprint_hex(a)  {}
#endif

static inline void hang(void)
{
	while(1)
		;
}

static inline void panic(char *s)
{
	debug(s);
	hang();
}

#define assert(e) ((e) ? (void)0 : hang())

void memcpy(void *dest, const void *src, register unsigned long count);
void *memset(void *dest, uint8_t ch, size_t n);
size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
char *strncpy(char *dest, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
size_t strnlen(const char *s, size_t maxlen);
int isxdigit(int c);
int isdigit(int c);
int tolower(int c);

#endif /* __LIB_H__ */
