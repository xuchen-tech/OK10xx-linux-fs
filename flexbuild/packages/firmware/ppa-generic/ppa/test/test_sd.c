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
 // Author Sahil Malhotra <sahil.malhotra@nxp.com>
 //
 //-----------------------------------------------------------------------------

#include "lib.h"
#include "io.h"
#include "uart.h"
#include "sd_mmc.h"

 //****** This data will be written on first 3 blocks of SD Card ******

 //***************** MMC_DATA will be *************
 // 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 // 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01
 // 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02
 // 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03
 // 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04 04
 // 05 05 05 05 05 05 05 05 05 05 05 05 05 05 05 05
 // 06 06 06 06 06 06 06 06 06 06 06 06 06 06 06 06
 // 07 07 07 07 07 07 07 07 07 07 07 07 07 07 07 07
 // 08 08 08 08 08 08 08 08 08 08 08 08 08 08 08 08
 // 09 09 09 09 09 09 09 09 09 09 09 09 09 09 09 09
 // 0a 0a 0a 0a 0a 0a 0a 0a 0a 0a 0a 0a 0a 0a 0a 0a
 // 0b 0b 0b 0b 0b 0b 0b 0b 0b 0b 0b 0b 0b 0b 0b 0b
 // 0c 0c 0c 0c 0c 0c 0c 0c 0c 0c 0c 0c 0c 0c 0c 0c
 // 0d 0d 0d 0d 0d 0d 0d 0d 0d 0d 0d 0d 0d 0d 0d 0d
 // 0e 0e 0e 0e 0e 0e 0e 0e 0e 0e 0e 0e 0e 0e 0e 0e
 // 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f
 // 10 10 10 10 10 10 10 10 10 10 10 10 10 10 10 10
 // 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
 // 12 12 12 12 12 12 12 12 12 12 12 12 12 12 12 12
 // 13 13 13 13 13 13 13 13 13 13 13 13 13 13 13 13
 // 14 14 14 14 14 14 14 14 14 14 14 14 14 14 14 14
 // 15 15 15 15 15 15 15 15 15 15 15 15 15 15 15 15
 // 16 16 16 16 16 16 16 16 16 16 16 16 16 16 16 16
 // 17 17 17 17 17 17 17 17 17 17 17 17 17 17 17 17
 // 18 18 18 18 18 18 18 18 18 18 18 18 18 18 18 18
 // 19 19 19 19 19 19 19 19 19 19 19 19 19 19 19 19
 // 1a 1a 1a 1a 1a 1a 1a 1a 1a 1a 1a 1a 1a 1a 1a 1a
 // 1b 1b 1b 1b 1b 1b 1b 1b 1b 1b 1b 1b 1b 1b 1b 1b
 // 1c 1c 1c 1c 1c 1c 1c 1c 1c 1c 1c 1c 1c 1c 1c 1c
 // 1d 1d 1d 1d 1d 1d 1d 1d 1d 1d 1d 1d 1d 1d 1d 1d
 // 1e 1e 1e 1e 1e 1e 1e 1e 1e 1e 1e 1e 1e 1e 1e 1e
 // 1f 1f 1f 1f 1f 1f 1f 1f 1f 1f 1f 1f 1f 1f 1f 1f
 // 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
 // 21 21 21 21 21 21 21 21 21 21 21 21 21 21 21 21
 // 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22
 // 23 23 23 23 23 23 23 23 23 23 23 23 23 23 23 23
 // 24 24 24 24 24 24 24 24 24 24 24 24 24 24 24 24
 // 25 25 25 25 25 25 25 25 25 25 25 25 25 25 25 25
 // 26 26 26 26 26 26 26 26 26 26 26 26 26 26 26 26
 // 27 27 27 27 27 27 27 27 27 27 27 27 27 27 27 27
 // 28 28 28 28 28 28 28 28 28 28 28 28 28 28 28 28
 // 29 29 29 29 29 29 29 29 29 29 29 29 29 29 29 29
 // 2a 2a 2a 2a 2a 2a 2a 2a 2a 2a 2a 2a 2a 2a 2a 2a
 // 2b 2b 2b 2b 2b 2b 2b 2b 2b 2b 2b 2b 2b 2b 2b 2b
 // 2c 2c 2c 2c 2c 2c 2c 2c 2c 2c 2c 2c 2c 2c 2c 2c
 // 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d
 // 2e 2e 2e 2e 2e 2e 2e 2e 2e 2e 2e 2e 2e 2e 2e 2e
 // 2f 2f 2f 2f 2f 2f 2f 2f 2f 2f 2f 2f 2f 2f 2f 2f
 // 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30
 // 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31
 // 32 32 32 32 32 32 32 32 32 32 32 32 32 32 32 32
 // 33 33 33 33 33 33 33 33 33 33 33 33 33 33 33 33
 // 34 34 34 34 34 34 34 34 34 34 34 34 34 34 34 34
 // 35 35 35 35 35 35 35 35 35 35 35 35 35 35 35 35
 // 36 36 36 36 36 36 36 36 36 36 36 36 36 36 36 36
 // 37 37 37 37 37 37 37 37 37 37 37 37 37 37 37 37
 // 38 38 38 38 38 38 38 38 38 38 38 38 38 38 38 38
 // 39 39 39 39 39 39 39 39 39 39 39 39 39 39 39 39
 // 3a 3a 3a 3a 3a 3a 3a 3a 3a 3a 3a 3a 3a 3a 3a 3a
 // 3b 3b 3b 3b 3b 3b 3b 3b 3b 3b 3b 3b 3b 3b 3b 3b
 // 3c 3c 3c 3c 3c 3c 3c 3c 3c 3c 3c 3c 3c 3c 3c 3c
 // 3d 3d 3d 3d 3d 3d 3d 3d 3d 3d 3d 3d 3d 3d 3d 3d
 // 3e 3e 3e 3e 3e 3e 3e 3e 3e 3e 3e 3e 3e 3e 3e 3e
 // 3f 3f 3f 3f 3f 3f 3f 3f 3f 3f 3f 3f 3f 3f 3f 3f
 // 40 40 40 40 40 40 40 40 40 40 40 40 40 40 40 40
 // 41 41 41 41 41 41 41 41 41 41 41 41 41 41 41 41
 // 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
 // 43 43 43 43 43 43 43 43 43 43 43 43 43 43 43 43
 // 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44
 // 45 45 45 45 45 45 45 45 45 45 45 45 45 45 45 45
 // 46 46 46 46 46 46 46 46 46 46 46 46 46 46 46 46
 // 47 47 47 47 47 47 47 47 47 47 47 47 47 47 47 47
 // 48 48 48 48 48 48 48 48 48 48 48 48 48 48 48 48
 // 49 49 49 49 49 49 49 49 49 49 49 49 49 49 49 49
 // 4a 4a 4a 4a 4a 4a 4a 4a 4a 4a 4a 4a 4a 4a 4a 4a
 // 4b 4b 4b 4b 4b 4b 4b 4b 4b 4b 4b 4b 4b 4b 4b 4b
 // 4c 4c 4c 4c 4c 4c 4c 4c 4c 4c 4c 4c 4c 4c 4c 4c
 // 4d 4d 4d 4d 4d 4d 4d 4d 4d 4d 4d 4d 4d 4d 4d 4d
 // 4e 4e 4e 4e 4e 4e 4e 4e 4e 4e 4e 4e 4e 4e 4e 4e
 // 4f 4f 4f 4f 4f 4f 4f 4f 4f 4f 4f 4f 4f 4f 4f 4f
 // 50 50 50 50 50 50 50 50 50 50 50 50 50 50 50 50
 // 51 51 51 51 51 51 51 51 51 51 51 51 51 51 51 51
 // 52 52 52 52 52 52 52 52 52 52 52 52 52 52 52 52
 // 53 53 53 53 53 53 53 53 53 53 53 53 53 53 53 53
 // 54 54 54 54 54 54 54 54 54 54 54 54 54 54 54 54
 // 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
 // 56 56 56 56 56 56 56 56 56 56 56 56 56 56 56 56
 // 57 57 57 57 57 57 57 57 57 57 57 57 57 57 57 57
 // 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58
 // 59 59 59 59 59 59 59 59 59 59 59 59 59 59 59 59
 // 5a 5a 5a 5a 5a 5a 5a 5a 5a 5a 5a 5a 5a 5a 5a 5a
 // 5b 5b 5b 5b 5b 5b 5b 5b 5b 5b 5b 5b 5b 5b 5b 5b
 // 5c 5c 5c 5c 5c 5c 5c 5c 5c 5c 5c 5c 5c 5c 5c 5c
 // 5d 5d 5d 5d 5d 5d 5d 5d 5d 5d 5d 5d 5d 5d 5d 5d
 // 5e 5e 5e 5e 5e 5e 5e 5e 5e 5e 5e 5e 5e 5e 5e 5e
 // 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f
 //************************************************

#define TEST_CASES 3 //Number of test cases to be executed.
#define MMC_DATA_LEN 1536 //3 * Block Len

 // This function will generate dummy data to be compared
 // with data on SD/MMC card.
void generate_dummy_data(uint8_t *ptr)
{
    int i = 1, a = 0x0;
    memset(ptr, 0, MMC_DATA_LEN);
    for (i = 1; i <= MMC_DATA_LEN; i++) {
        ptr[i-1] = a;
        if ((i % 16) == 0) {
            a++;
        }
    }
}

 // This function will read data from SD/MMC card and compare
 // it with the generated data.
void test_sd(void)
{
    uint8_t mmc_data[MMC_DATA_LEN];
    uint8_t *data = 0x83000000;

    int i = 0, j = 0, k = 0;
   int offset[TEST_CASES] = { 0, 512, 1024 };
    int len[TEST_CASES] = { 512 * 3 , 1024, 512};

    generate_dummy_data(mmc_data);
    memset(data, 0, 512);

    for (i = 0; i < TEST_CASES; i++) {
        esdhc_read(512 * 512 + offset[i], data, len[i]);
        if (memcmp((void *)data, &mmc_data[offset[i]], len[i])) {
            debug_hex("SD card data and assumed data DOESNOT MATCH for case.", i);

#if 0
        debug("\nMMC read data \n");
        debug_hex("offset = ", offset[i]);
        debug_hex("len = ", len[i]);
        for (j = 0; j < len[i]; j++) {
            debug_hex(" ", data[j]);
                if ((j % 16) == 0) {
                    debug("\n");
                }
	}

        debug("\nDummy data generated\n");
        debug_hex("offset = ", offset[i]);
        debug_hex("len = ", len[i]);

        for (k = offset[i]; k < offset[i] + len[i]; k++) {
            debug_hex(" ", mmc_data[k]);
            if ((k % 16) == 0) {
                debug("\n");
            }
	}
#endif
            continue;
	}
        debug_hex("\nSD card data and assumed data MATCHES for case.", i);
    }
}
