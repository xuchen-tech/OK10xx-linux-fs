//-----------------------------------------------------------------------------
// 
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
// Authors:
//  Ruchika Gupta <ruchika.gupta@nxp.com> 
//
//-----------------------------------------------------------------------------

#include "types.h"

 // this function copy data from src to dest memory.
void *memcpy(void *dest, const void *src, size_t n)
{
    const uint8_t *cs = src;
    uint8_t *cd = dest;

    while (n--)
        *cd++ = *cs++;

    return dest;
}

 //  This function fills the first n bytes of the memory area pointed 
 // to by dest with the constant byte ch
void *memset(void *dest, uint8_t ch, size_t n)
{
        uint8_t *cd = dest;

        while (n--)
                *cd++ = ch;

        return dest;
};

 // this funcation compare memory regions.
int memcmp(const void *s1, const void *s2, size_t n)
{
    if (n != 0) {
        register const unsigned char *p1 = s1, *p2 = s2;

        do {
            if (*p1++ != *p2++)
                return (*--p1 - *--p2);
        } while (--n != 0);
    }
    return (0);
}

 // this function returns length of input string
size_t strlen(const char *str)
{
    const char *s = str;

    while (*s)
        ++s;
    return s - str;
};

 // this function returns length of input string
size_t strnlen(const char *s, size_t maxlen)
{
    size_t len;

    for (len = 0; len < maxlen; len++, s++) {
        if (*s == '\0')
            break;
    }
    return (len);
}

 // this function compares strings.
 // return 0 if string matches
 // else return the difference in string
int strcmp(const char *s1, const char *s2)
{
    while (*s1 == *s2++)
        if (*s1++ == '\0')
            return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1);
}

 // this function compares two strings
int strncmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return (0);
    do {
        if (*s1 != *s2++)
            return (*(const unsigned char *)s1 -
                *(const unsigned char *)(s2 - 1));
        if (*s1++ == '\0')
            break;
    } while (--n != 0);
    return (0);
}

 // this function copies src strign to destination string 
char *strncpy(char *dest, const char *src, size_t n)
{
    char *ret = dest;
    do {
        if (!n--)
            return ret;
    } while (*dest++ = *src++);
    while (n--)
        *dest++ = 0;
    return ret;
}

 // this function converts character to lower case
int tolower(int ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return ('a' + ch - 'A');
    else
        return ch;
}

 // this function checks if given number is digit
int isdigit(int c)
{
    if (c >= '0' && c <= '9')
        return c;
    else
        return 0;
}

 // this function checks if given number is hex digit
int isxdigit(int c)
{
    if ((c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
        return c;
    return 0;
}
