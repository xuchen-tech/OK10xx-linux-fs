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

#include "lib.h"
#include "io.h"
#include "plat.h"
#include "ddr.h"
#include "immap.h"

#define UL_5POW12	244140625UL
#define ULL_2E12	2000000000000ULL
#define UL_2POW13	(1UL << 13)
#define ULL_8FS		0xFFFFFFFFULL

#if 0
void memcpy(void *dest, const void *src,
			  register unsigned long count)
{
	register unsigned long *dest_long = dest;
	register unsigned long *src_long = (void *)src;
	register unsigned char *dest_u8;
	register unsigned char *src_u8;

	if (dest == src)
		return;

	/* First copy aligned unsigned long data if aligned */
	if (((sizeof(unsigned long) - 1) &
	    ((unsigned long)dest | (unsigned long)src)) == 0) {
		while (count >= sizeof(unsigned long)) {
			*dest_long++ = *src_long++;
			count -= sizeof(unsigned long);
		}
	}
	/* Then deal with the remaining unaligned data */
	dest_u8 = (unsigned char *)dest_long;
	src_u8 = (unsigned char *)src_long;
	while (count--)
		*dest_u8++ = *src_u8++;
}
#endif

#define do_div(n, base) ({			\
	unsigned int __base = (base);		\
	unsigned int __rem;				\
	__rem = ((unsigned long long)(n)) % __base;	\
	(n) = ((unsigned long long)(n)) / __base;	\
	__rem;					\
})

void get_clocks(struct sysinfo *sys)
{
	unsigned int *rcwsr0 = (void *)FSL_CHASSIS_RCWSR0;
	const unsigned long sysclk = CONFIG_SYSCLK_FREQ;
	const unsigned long ddrclk = CONFIG_DDRCLK_FREQ;

	sys->freq_platform = sysclk;
	sys->freq_ddr_pll0 = ddrclk;
	sys->freq_ddr_pll1 = ddrclk;

	sys->freq_platform *= (gur_in32(rcwsr0) >>
				FSL_CHASSIS_RCWSR0_SYS_PLL_RAT_SHIFT) &
				FSL_CHASSIS_RCWSR0_SYS_PLL_RAT_MASK;
	sys->freq_platform /= 2;
	sys->freq_ddr_pll0 *= (gur_in32(rcwsr0) >>
				FSL_CHASSIS_RCWSR0_MEM_PLL_RAT_SHIFT) &
				FSL_CHASSIS_RCWSR0_MEM_PLL_RAT_MASK;
	sys->freq_ddr_pll1 *= (gur_in32(rcwsr0) >>
				FSL_CHASSIS_RCWSR0_MEM2_PLL_RAT_SHIFT) &
				FSL_CHASSIS_RCWSR0_MEM2_PLL_RAT_MASK;

	debug("platform clock "); dbgprint_uint(sys->freq_platform);
	debug("\nDDR PLL1 "); dbgprint_uint(sys->freq_ddr_pll0);
	debug("\nDDR PLL2 "); dbgprint_uint(sys->freq_ddr_pll1);
	debug("\n");
}

unsigned long get_ddr_freq(struct sysinfo *sys, int ctrl_num)
{
	if (sys->freq_ddr_pll0 == 0)
		get_clocks(sys);

	switch (ctrl_num) {
		case 0:
			return sys->freq_ddr_pll0;
		case 1:
			return sys->freq_ddr_pll0;
		case 2:
			return sys->freq_ddr_pll1;
	}

	return 0;
}

unsigned int get_memory_clk_period_ps(const unsigned long data_rate)
{
        unsigned int result;
        /* Round to nearest 10ps, being careful about 64-bit multiply/divide */
        unsigned long long rem, mclk_ps = ULL_2E12;

        /* Now perform the big divide, the result fits in 32-bits */
        rem = do_div(mclk_ps, data_rate);
        result = (rem >= (data_rate >> 1)) ? mclk_ps + 1 : mclk_ps;

        return result;
}

unsigned int picos_to_mclk(unsigned long data_rate, unsigned int picos)
{
        unsigned long long clks, clks_rem;

        /* Short circuit for zero picos */
        if (!picos || !data_rate)
                return 0;

        /* First multiply the time by the data rate (32x32 => 64) */
        clks = picos * (unsigned long long)data_rate;
        /*
         * Now divide by 5^12 and track the 32-bit remainder, then divide
         * by 2*(2^12) using shifts (and updating the remainder).
         */
        clks_rem = do_div(clks, UL_5POW12);
        clks_rem += (clks & (UL_2POW13-1)) * UL_5POW12;
        clks >>= 13;

        /* If we had a remainder greater than the 1ps error, then round up */
        if (clks_rem > data_rate)
                clks++;

        /* Clamp to the maximum representable value */
        if (clks > ULL_8FS)
                clks = ULL_8FS;
        return (unsigned int) clks;
}

void remove_unused_controllers(struct ddr_info *info)
{
#ifdef CONFIG_SYS_FSL_HAS_CCN504
	int i;
	unsigned long long nodeid;
	void *hnf_sam_ctrl = (void *)(CCI_HN_F_0_BASE + CCN_HN_F_SAM_CTL);
	bool ddr0_used = false;
	bool ddr1_used = false;

	for (i = 0; i < 8; i++) {
		nodeid = in_le64(hnf_sam_ctrl) & CCN_HN_F_SAM_NODEID_MASK;
		if (nodeid == CCN_HN_F_SAM_NODEID_DDR0) {
			ddr0_used = true;
		} else if (nodeid == CCN_HN_F_SAM_NODEID_DDR1) {
			ddr1_used = true;
		} else {
			debug("Unknown nodeid in HN-F SAM control: 0x");
			dbgprint_hex(nodeid);
			debug("\n");
		}
		hnf_sam_ctrl += (CCI_HN_F_1_BASE - CCI_HN_F_0_BASE);
	}
	if (!ddr0_used && !ddr1_used) {
		debug("Invalid configuration in HN-F SAM control\n");
		return;
	}

	if (!ddr0_used && info->first_ctrl == 0) {
		info->first_ctrl = 1;
		info->num_ctrls = 1;

		debug("First DDR controller disabled\n");
		return;
	}

	if (!ddr1_used && info->first_ctrl + info->num_ctrls > 1) {
		info->num_ctrls = 1;
		debug("Second DDR controller disabled\n");
	}
#endif /* CONFIG_SYS_FSL_HAS_CCN504 */
}

void print_ddr_info(int start_ctrl)
{
	struct ccsr_ddr *ddr = (void *)CONFIG_SYS_FSL_DDR_ADDR;

#if (CONFIG_SYS_NUM_DDR_CTLRS > 1)
	unsigned int cs0_config = ddr_in32(&ddr->cs0_config);
#endif
	unsigned int sdram_cfg = ddr_in32(&ddr->sdram_cfg);
	int cas_lat;

#if CONFIG_SYS_NUM_DDR_CTLRS > 1
	if ((!(sdram_cfg & SDRAM_CFG_MEM_EN)) || (start_ctrl == 1)) {
		ddr = (void *)CONFIG_SYS_FSL_DDR2_ADDR;
		sdram_cfg = ddr_in32(&ddr->sdram_cfg);
	}
#endif
#if CONFIG_SYS_NUM_DDR_CTLRS > 2
	if ((!(sdram_cfg & SDRAM_CFG_MEM_EN)) || (start_ctrl == 2)) {
		ddr = (void *)CONFIG_SYS_FSL_DDR3_ADDR;
		sdram_cfg = ddr_in32(&ddr->sdram_cfg);
	}
#endif

	if (!(sdram_cfg & SDRAM_CFG_MEM_EN)) {
		debug(" (DDR not enabled)\n");
		return;
	}

	debug("DDR");
	switch ((sdram_cfg & SDRAM_CFG_SDRAM_TYPE_MASK) >>
		SDRAM_CFG_SDRAM_TYPE_SHIFT) {
	case SDRAM_TYPE_DDR4:
		debug("4");
		break;
	default:
		debug("?");
		break;
	}

	switch (sdram_cfg & SDRAM_CFG_DBW_MASK) {
	case SDRAM_CFG_32_BW:
		debug(", 32-bit");
		break;
	case SDRAM_CFG_16_BW:
		debug(", 16-bit");
		break;
	case SDRAM_CFG_8_BW:
		debug(", 8-bit");
		break;
	default:
		debug(", 64-bit");
		break;
	}

	/* Calculate CAS latency based on timing cfg values */
	cas_lat = ((ddr_in32(&ddr->timing_cfg_1) >> 16) & 0xf);
	cas_lat += 2;	/* for DDRC newer than 4.4 */
	cas_lat += ((ddr_in32(&ddr->timing_cfg_3) >> 12) & 3) << 4;
	debug(", CL=");
	dbgprint_uint(cas_lat >> 1);
	if (cas_lat & 0x1)
		debug(".5");

#if ((DEBUG_BUILD) && (CNFG_UART))
	if (sdram_cfg & SDRAM_CFG_ECC_EN)
		debug(", ECC on");
	else
		debug(", ECC off");
#endif

#if (CONFIG_SYS_NUM_DDR_CTLRS >= 2)
	if ((cs0_config & 0x20000000) && (start_ctrl == 0)) {
		debug(", ");
		switch ((cs0_config >> 24) & 0xf) {
		case FSL_DDR_256B_INTERLEAVING:
			debug("256B");
			break;
		case FSL_DDR_CACHE_LINE_INTERLEAVING:
			debug("cache line");
			break;
		case FSL_DDR_PAGE_INTERLEAVING:
			debug("page");
			break;
		case FSL_DDR_BANK_INTERLEAVING:
			debug("bank");
			break;
		case FSL_DDR_SUPERBANK_INTERLEAVING:
			debug("super-bank");
			break;
		default:
			debug("invalid");
			break;
		}
	}
#endif

	if ((sdram_cfg >> 8) & 0x7f) {
		debug(", ");
		switch(sdram_cfg >> 8 & 0x7f) {
		case FSL_DDR_CS0_CS1_CS2_CS3:
			debug("CS0+CS1+CS2+CS3");
			break;
		case FSL_DDR_CS0_CS1:
			debug("CS0+CS1");
			break;
		case FSL_DDR_CS2_CS3:
			debug("CS2+CS3");
			break;
		case FSL_DDR_CS0_CS1_AND_CS2_CS3:
			debug("CS0+CS1 and CS2+CS3");
			break;
		default:
			debug("invalid");
			break;
		}
	}
	debug("\n");
}
