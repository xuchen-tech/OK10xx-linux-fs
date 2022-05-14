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
#include "get_rand.h"
#include "lib.h"

 // this function returns a random number using HASH DRBG algorithm
 // In case of failure, random number returned is 0
 // prngWidth = 0 - 32 bit random number
 // prngWidth > 0 means 64 bit random number
unsigned long long _get_PRNG(int prngWidth)
{
    unsigned long long result;
    uint8_t rand_byte[8];
    uint8_t rand_byte_swp[8];
    int bytes = 0;
    int i = 0;
    int ret = 0;

#ifdef TEST
    rand_byte[0] = 0x12;
    rand_byte[1] = 0x34;
    rand_byte[2] = 0x56;
    rand_byte[3] = 0x78;
    rand_byte[4] = 0x9a;
    rand_byte[5] = 0xbc;
    rand_byte[6] = 0xde;
    rand_byte[7] = 0xf1;
#endif

    if (prngWidth == 0)
        bytes = 4;
    else
        bytes = 8;

    ret = get_rand_bytes(rand_byte, bytes);

    for (i = 0; i < bytes; i++) {
	if (ret) {
	     // Return 0 in case of failure
            rand_byte_swp[i] = 0;
	} else {
            rand_byte_swp[i] = rand_byte[bytes - i - 1];
        }
    } 

    result = *(unsigned long long *)&rand_byte_swp[0];

    return result;

}  // _get_PRNG()
