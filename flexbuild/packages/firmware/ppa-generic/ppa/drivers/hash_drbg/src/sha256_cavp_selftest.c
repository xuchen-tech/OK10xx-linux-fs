//-----------------------------------------------------------------------------
// 
// Copyright (C) 2015 Freescale Semiconductor
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
// Authors:
//  Tom Tkacik <tom.tkacik@nxp.com>
//  Ruchika Gupta <ruchika.gupta@nxp.com> 
//
//-----------------------------------------------------------------------------

#include "sha256.h"
#include "lib.h"

 // Perform a SHA256 Long or Short Message CAVP test
 // This is defined in Sec. 6.2.2 (pg. 5) of
 // The Secure Hash Algorithm Validation System (SHAVS)
 // This tests both sha256() and sha256_hex()
 // Parameters:
 // const sha_msg_test* sha_tests - CAVP test data
 // int   count                   - Number of tests
 // Return code:
 //  0 - All is well
 // >0 - Number of failures
int sha256_testMsg(const sha_msg_test * sha_tests, int count)
{
    int i;
    char hash_string[65];
    uint8_t hash_bytes[32];
    int datalen;
    int result;
    int fails;
    #define MAX_SIZE    512
    uint8_t data[MAX_SIZE];

    fails = 0;

     // Now, run the test again, but fur the sha256()
    for (i = 0; i < count; i += 1) {
        int j;

         // Initialize hash_string to a known non-hex string
         //  This is just to verify that the output is indeed being set
        for (j = 0; j < 64; j += 1) {
            hash_string[j] = 'z';
	}

        hash_string[64] = 0;

         // Convert the hex string to a byte array for hashing
        datalen =
            hex_to_bytes(sha_tests[i].Msg, sha_tests[i].Bitlen / 4,
                 data, MAX_SIZE);

         // Check that the conversion succeeded
        if (datalen == -1) {
            fails += 1;
            debug("Test failedl bad message\n");
#ifdef LINUX
            printf
                ("Test %u failed; bad message:\n Len = %u\n MSG = \"%s\"\n MD  = \"%s\"\n\n",
                 i, sha_tests[i].Bitlen, sha_tests[i].Msg,
                 sha_tests[i].MD);
#endif
        } else {
             // Hash the byte array data
            sha256(data, datalen, hash_bytes);

             // Convert the hash result from a byte array to a hex string,
             //  and compare it to the expected result
            result = bytes_to_hex(hash_bytes, 32, hash_string, 65);


            if (strcmp(hash_string, sha_tests[i].MD) != 0) {
                fails += 1;
                debug("SHA256 test failed; wrong hash\n");
#ifdef LINUX
                printf
                    ("SHA256 test %u failed; wrong hash:\n Len = %u\n MSG = \"%s\"\n MD  = \"%s\"\n SHA = \"%s\"\n\n",
                     i, sha_tests[i].Bitlen, sha_tests[i].Msg,
                     sha_tests[i].MD, hash_string);
#endif
            }
        }
    }

    return fails;
}

 // Run the CAVP SHA Short Msg test
int sha256_ShortMsg()
{
    return sha256_testMsg(sha_short, sha_short_count);
}
