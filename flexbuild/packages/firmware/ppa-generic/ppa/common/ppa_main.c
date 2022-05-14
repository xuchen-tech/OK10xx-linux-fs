//-----------------------------------------------------------------------------
//
// Copyright (C) 2015, 2016 Freescale Semiconductor, Inc. 
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
//-----------------------------------------------------------------------------

#include "lib.h"
#include "fsl_sec.h"
#include "plat.h"
#include "policy.h"

#if (CNFG_FUSE)
#include "sfp.h"
#endif

#if (CNFG_SPD)
#include "spd.h"
#endif

#if (CNFG_SD)
#include "sd_mmc.h"
#endif

char __bss_start[0] __attribute__((section(".__bss_start")));
char __bss_end[0] __attribute__((section(".__bss_end")));
char __rel_dyn_start[0] __attribute__((section(".__rel_dyn_start")));
char __rel_dyn_end[0] __attribute__((section(".__rel_dyn_end")));

struct allocator heap;

 //64 KB area reserved by u-boot for MMU page tables
#define PGTABALE_RES_SIZE	0x10000

 //1 MB area in DDR allocated for heap starting from 512K offset
#define HEAP_OFFSET	0x80000
#define HEAP_SIZE	0x100000

//512 KB area in DDR allocated for fuse script from 1.5 MB offset
#define FUSE_SCR_OFFSET	0x180000

#if (CNFG_DDR)
void _init_membank_data(void) __attribute__ ((weak));
void _init_membank_data(void)
{
}

void _init_ddr(void) __attribute__ ((weak));
void _init_ddr(void)
{
}

void timer_init(void) __attribute__ ((weak));
void timer_init(void)
{
}

void soc_errata(void) __attribute__ ((weak));
void soc_errata(void)
{
}
unsigned long long _get_PRNG(int prngWidth) __attribute__ ((weak));
unsigned long long _get_PRNG(int prngWidth)
{
	return 0;
}
unsigned long long _get_RNG(int prngWidth) __attribute__ ((weak));
unsigned long long _get_RNG(int prngWidth)
{
	return 0;
}

#if (PSCI_TEST)
void _populate_membank_data(void) __attribute__ ((weak));
void _populate_membank_data(void)
{
}
#endif
#endif

#if (CNFG_DDR) || (CNFG_I2C)
void i2c_init(void) __attribute__ ((weak));
void i2c_init(void)
{
}
#endif

#if (CNFG_UART)
void uart_init(void) __attribute__ ((weak));
void uart_init(void)
{
}
#endif

//-----------------------------------------------------------------------------

int _ppa_main(unsigned long long addr, unsigned long long loadable)
{

    unsigned long long heap_addr;

     // Substract reserved size used for PGTABLE in u-boot to get
     // exact start address of ppa.
    addr = addr - PGTABALE_RES_SIZE;

#if (CNFG_UART)
    uart_init();
#endif

#if (CNFG_DDR)
    _init_membank_data();
#if (PSCI_TEST)
    _populate_membank_data();
#else
    soc_errata();
    timer_init();
    i2c_init();
    _init_ddr();
#endif
#elif (CNFG_I2C)
    timer_init();
    i2c_init();
#endif

 // For changed bootflow i.e with CNFG_DDR on, heap has to be on DDR.
 // Currently hardcoding it to 0x80000000 to test SEC. Needs to be changed - TBD
#if (CNFG_DDR)
    heap_addr = 0x80000000;
#else
    heap_addr = addr + HEAP_OFFSET;
#endif

    alloc_init(&heap, heap_addr, HEAP_SIZE);

#if (!SUPPRESS_SEC)
    sec_init();
#endif

 // Do fuse provisioning only if CNFG_FUSE is enabled and POLICY_FUSE_PROVISION
 // policy is defined in corresponding policy file of target platform.
#if defined(CNFG_FUSE) && defined(POLICY_FUSE_PROVISION)
    unsigned long long fuse_scr_addr;
     // Fuse script loaded on DDR at 1.5 MB offset from start addr
    fuse_scr_addr = addr + FUSE_SCR_OFFSET;

    provision_fuses(fuse_scr_addr);
#endif

#if (CNFG_SPD)
    if (loadable) {
        spd_init(loadable);
    }
#endif

#if (CNFG_SD)
    debug("SD flag enabled\n");
    sd_mmc_init();
    debug("sd_mmc_init done\n");

#if (CNFG_SD_TEST)
    debug("testing sd card read write\n");
    test_sd();
#endif
#endif

	return (0);

}  // _ppa_main()

//-----------------------------------------------------------------------------

