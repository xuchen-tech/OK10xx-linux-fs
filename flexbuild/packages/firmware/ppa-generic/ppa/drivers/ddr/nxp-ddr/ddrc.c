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
#include "regs.h"
#include "immap.h"
#include "timer.h"
#include "utility.h"

#define BIST_PATTERN1	0xFFFFFFFF
#define BIST_PATTERN2	0x0
#define BIST_CR		0x80010000
#define BIST_CR_EN	0x80000000
#define BIST_CR_STAT	0x00000001
#define CTLR_INTLV_MASK	0x20000000

extern void update_test_result(const unsigned long clk,
				const unsigned char clk_adjust,
				const unsigned charwrlvl_start,
				const int ctrl_num,
				const int result);

bool run_bist(void)  __attribute__ ((weak));
bool run_bist(void)
{
	return false;
}

static unsigned int get_ddrc_version(struct ccsr_ddr *ddr)
{
	unsigned int ver;

	ver = (ddr_in32(&ddr->ip_rev1) & 0xFFFF) << 8;
	ver |= (ddr_in32(&ddr->ip_rev2) & 0xFF00) >> 8;

	return ver;
}

/*
 * Perform build-in test on memory
 * timeout value in 10ms
 */
int bist(struct ccsr_ddr *ddr, int timeout)
{
	unsigned int mtcr, err_detect, err_sbe;
	unsigned int cs0_bnds, cs1_bnds, cs2_bnds, cs3_bnds, cs0_config;
	int ret = 0;

	cs0_config = ddr_in32(&ddr->cs0_config);
	cs0_bnds = ddr_in32(&ddr->cs0_bnds);
	cs1_bnds = ddr_in32(&ddr->cs1_bnds);
	cs2_bnds = ddr_in32(&ddr->cs2_bnds);
	cs3_bnds = ddr_in32(&ddr->cs3_bnds);
	if (cs0_config & CTLR_INTLV_MASK) {
		/* set bnds to non-interleaving */
		ddr_out32(&ddr->cs0_bnds, (cs0_bnds & 0xfffefffe) >> 1);
		ddr_out32(&ddr->cs1_bnds, (cs1_bnds & 0xfffefffe) >> 1);
		ddr_out32(&ddr->cs2_bnds, (cs2_bnds & 0xfffefffe) >> 1);
		ddr_out32(&ddr->cs3_bnds, (cs3_bnds & 0xfffefffe) >> 1);
	}
	ddr_out32(&ddr->mtp1, BIST_PATTERN1);
	ddr_out32(&ddr->mtp2, BIST_PATTERN1);
	ddr_out32(&ddr->mtp3, BIST_PATTERN2);
	ddr_out32(&ddr->mtp4, BIST_PATTERN2);
	ddr_out32(&ddr->mtp5, BIST_PATTERN1);
	ddr_out32(&ddr->mtp6, BIST_PATTERN1);
	ddr_out32(&ddr->mtp7, BIST_PATTERN2);
	ddr_out32(&ddr->mtp8, BIST_PATTERN2);
	ddr_out32(&ddr->mtp9, BIST_PATTERN1);
	ddr_out32(&ddr->mtp10, BIST_PATTERN2);
	mtcr = BIST_CR;
	ddr_out32(&ddr->mtcr, mtcr);
	while (timeout > 0 && (mtcr & BIST_CR_EN)) {
		mdelay(10);	/* timeout in 10ms unit */
		timeout--;
		mtcr = ddr_in32(&ddr->mtcr);
	}
	if (timeout <= 0) {
		debug("Timeout\n");
    }
	else {
		debug("Done");
		debug("\n");
	}

	err_detect = ddr_in32(&ddr->err_detect);
	err_sbe = ddr_in32(&ddr->err_sbe);
	if (err_detect || (err_sbe & 0xffff)) {
		debug("ECC error detected\n");
		ret = -EIO;
	}

	if (cs0_config & CTLR_INTLV_MASK) {
		/* restore bnds registers */
		ddr_out32(&ddr->cs0_bnds, cs0_bnds);
		ddr_out32(&ddr->cs1_bnds, cs1_bnds);
		ddr_out32(&ddr->cs2_bnds, cs2_bnds);
		ddr_out32(&ddr->cs3_bnds, cs3_bnds);
	}
	if (mtcr & BIST_CR_STAT) {
		debug("BIST test failed\n");
		ret = -EIO;
	}

	return ret;
}

#if defined(CONFIG_SYS_FSL_ERRATUM_A008511) || defined(CONFIG_SYS_FSL_ERRATUM_A009803)
static void set_wait_for_bits_clear(void *ptr, unsigned int value, unsigned int bits)
{
	int timeout = 1000;

	ddr_out32(ptr, value);

	while (ddr_in32(ptr) & bits) {
		udelay(100);
		timeout--;
	}
	if (timeout <= 0)
		debug("Error: wait for clear timeout.\n");
}
#endif

#if (CONFIG_CHIP_SELECTS_PER_CTRL > 4)
#error Invalid setting for CONFIG_CHIP_SELECTS_PER_CTRL
#endif

/*
 * regs has the to-be-set values for DDR controller registers
 * ctrl_num is the DDR controller number
 * step: 0 goes through the initialization in one pass
 *       1 sets registers and returns before enabling controller
 *       2 resumes from step 1 and continues to initialize
 * Dividing the initialization to two steps to deassert DDR reset signal
 * to comply with JEDEC specs for RDIMMs.
 */
int ddrc_set_regs(const unsigned long clk,
		  const struct ddr_cfg_regs *regs,
		  int ctrl_num, int step)
{
	unsigned int i, bus_width;
	struct ccsr_ddr *ddr;
	unsigned int temp_sdram_cfg;
	unsigned int total_mem_per_ctrl, total_mem_per_ctrl_adj;
	int timeout;
	int ret = 0;
#ifdef CONFIG_SYS_FSL_ERRATUM_A008511
	unsigned int temp32, mr6;
	unsigned int vref_seq1[3] = {0x80, 0x96, 0x16};	/* for range 1 */
	unsigned int vref_seq2[3] = {0xc0, 0xf0, 0x70};	/* for range 2 */
	unsigned int *vref_seq = vref_seq1;
#endif
#if defined(CONFIG_SYS_FSL_ERRATUM_A009942) | defined(CONFIG_SYS_FSL_ERRATUM_A010165)
	unsigned long ddr_freq;
	unsigned int tmp;
#endif

	switch (ctrl_num) {
	case 0:
		ddr = (void *)CONFIG_SYS_FSL_DDR_ADDR;
		break;
#if defined(CONFIG_SYS_FSL_DDR2_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 1)
	case 1:
		ddr = (void *)CONFIG_SYS_FSL_DDR2_ADDR;
		break;
#endif
#if defined(CONFIG_SYS_FSL_DDR3_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 2)
	case 2:
		ddr = (void *)CONFIG_SYS_FSL_DDR3_ADDR;
		break;
#endif
#if defined(CONFIG_SYS_FSL_DDR4_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 3)
	case 3:
		ddr = (void *)CONFIG_SYS_FSL_DDR4_ADDR;
		break;
#endif
	default:
		debug("%s unexpected ctrl_num = ");
		dbgprint_uint(ctrl_num);
		debug("\n");
		return -EINVAL;
	}

	if (step == 2)
		goto step2;

	if (regs->ddr_eor)
		ddr_out32(&ddr->eor, regs->ddr_eor);

	ddr_out32(&ddr->sdram_clk_cntl, regs->ddr_sdram_clk_cntl);

	for (i = 0; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
		if (i == 0) {
			ddr_out32(&ddr->cs0_bnds, regs->cs[i].bnds);
			ddr_out32(&ddr->cs0_config, regs->cs[i].config);
			ddr_out32(&ddr->cs0_config_2, regs->cs[i].config_2);

		} else if (i == 1) {
			ddr_out32(&ddr->cs1_bnds, regs->cs[i].bnds);
			ddr_out32(&ddr->cs1_config, regs->cs[i].config);
			ddr_out32(&ddr->cs1_config_2, regs->cs[i].config_2);

		} else if (i == 2) {
			ddr_out32(&ddr->cs2_bnds, regs->cs[i].bnds);
			ddr_out32(&ddr->cs2_config, regs->cs[i].config);
			ddr_out32(&ddr->cs2_config_2, regs->cs[i].config_2);

		} else if (i == 3) {
			ddr_out32(&ddr->cs3_bnds, regs->cs[i].bnds);
			ddr_out32(&ddr->cs3_config, regs->cs[i].config);
			ddr_out32(&ddr->cs3_config_2, regs->cs[i].config_2);
		}
	}

	ddr_out32(&ddr->timing_cfg_3, regs->timing_cfg_3);
	ddr_out32(&ddr->timing_cfg_0, regs->timing_cfg_0);
	ddr_out32(&ddr->timing_cfg_1, regs->timing_cfg_1);
	ddr_out32(&ddr->timing_cfg_2, regs->timing_cfg_2);
	ddr_out32(&ddr->timing_cfg_4, regs->timing_cfg_4);
	ddr_out32(&ddr->timing_cfg_5, regs->timing_cfg_5);
	ddr_out32(&ddr->timing_cfg_6, regs->timing_cfg_6);
	ddr_out32(&ddr->timing_cfg_7, regs->timing_cfg_7);
	ddr_out32(&ddr->timing_cfg_8, regs->timing_cfg_8);
	ddr_out32(&ddr->timing_cfg_9, regs->timing_cfg_9);
	ddr_out32(&ddr->ddr_zq_cntl, regs->ddr_zq_cntl);
	ddr_out32(&ddr->dq_map_0, regs->dq_map_0);
	ddr_out32(&ddr->dq_map_1, regs->dq_map_1);
	ddr_out32(&ddr->dq_map_2, regs->dq_map_2);
	ddr_out32(&ddr->dq_map_3, regs->dq_map_3);
	ddr_out32(&ddr->sdram_cfg_3, regs->ddr_sdram_cfg_3);
	ddr_out32(&ddr->sdram_mode, regs->ddr_sdram_mode);
	ddr_out32(&ddr->sdram_mode_2, regs->ddr_sdram_mode_2);
	ddr_out32(&ddr->sdram_mode_3, regs->ddr_sdram_mode_3);
	ddr_out32(&ddr->sdram_mode_4, regs->ddr_sdram_mode_4);
	ddr_out32(&ddr->sdram_mode_5, regs->ddr_sdram_mode_5);
	ddr_out32(&ddr->sdram_mode_6, regs->ddr_sdram_mode_6);
	ddr_out32(&ddr->sdram_mode_7, regs->ddr_sdram_mode_7);
	ddr_out32(&ddr->sdram_mode_8, regs->ddr_sdram_mode_8);
	ddr_out32(&ddr->sdram_mode_9, regs->ddr_sdram_mode_9);
	ddr_out32(&ddr->sdram_mode_10, regs->ddr_sdram_mode_10);
	ddr_out32(&ddr->sdram_mode_11, regs->ddr_sdram_mode_11);
	ddr_out32(&ddr->sdram_mode_12, regs->ddr_sdram_mode_12);
	ddr_out32(&ddr->sdram_mode_13, regs->ddr_sdram_mode_13);
	ddr_out32(&ddr->sdram_mode_14, regs->ddr_sdram_mode_14);
	ddr_out32(&ddr->sdram_mode_15, regs->ddr_sdram_mode_15);
	ddr_out32(&ddr->sdram_mode_16, regs->ddr_sdram_mode_16);
	ddr_out32(&ddr->sdram_md_cntl, regs->ddr_sdram_md_cntl);
#ifdef CONFIG_SYS_FSL_ERRATUM_A009663
	ddr_out32(&ddr->sdram_interval,
		  regs->ddr_sdram_interval & ~SDRAM_INTERVAL_BSTOPRE);
#else
	ddr_out32(&ddr->sdram_interval, regs->ddr_sdram_interval);
#endif
	ddr_out32(&ddr->sdram_data_init, regs->ddr_data_init);
	ddr_out32(&ddr->ddr_wrlvl_cntl, regs->ddr_wrlvl_cntl);
#ifndef CONFIG_SYS_FSL_DDR_EMU
	/*
	 * Skip these two registers if running on emulator
	 * because emulator doesn't have skew between bytes.
	 */

	if (regs->ddr_wrlvl_cntl_2)
		ddr_out32(&ddr->ddr_wrlvl_cntl_2, regs->ddr_wrlvl_cntl_2);
	if (regs->ddr_wrlvl_cntl_3)
		ddr_out32(&ddr->ddr_wrlvl_cntl_3, regs->ddr_wrlvl_cntl_3);
#endif

	ddr_out32(&ddr->ddr_sr_cntr, regs->ddr_sr_cntr);
	ddr_out32(&ddr->ddr_sdram_rcw_1, regs->ddr_sdram_rcw_1);
	ddr_out32(&ddr->ddr_sdram_rcw_2, regs->ddr_sdram_rcw_2);
	ddr_out32(&ddr->ddr_sdram_rcw_3, regs->ddr_sdram_rcw_3);
	ddr_out32(&ddr->ddr_sdram_rcw_4, regs->ddr_sdram_rcw_4);
	ddr_out32(&ddr->ddr_sdram_rcw_5, regs->ddr_sdram_rcw_5);
	ddr_out32(&ddr->ddr_sdram_rcw_6, regs->ddr_sdram_rcw_6);
	ddr_out32(&ddr->ddr_cdr1, regs->ddr_cdr1);
#ifdef CONFIG_DEEP_SLEEP
	if (is_warm_boot()) {
		ddr_out32(&ddr->sdram_cfg_2,
			  regs->ddr_sdram_cfg_2 & ~SDRAM_CFG2_D_INIT);
		ddr_out32(&ddr->init_addr, CONFIG_SYS_SDRAM_BASE);
		ddr_out32(&ddr->init_ext_addr, DDR_INIT_ADDR_EXT_UIA);

		/* DRAM VRef will not be trained */
		ddr_out32(&ddr->ddr_cdr2,
			  regs->ddr_cdr2 & ~DDR_CDR2_VREF_TRAIN_EN);
	} else
#endif
	{
		ddr_out32(&ddr->sdram_cfg_2, regs->ddr_sdram_cfg_2);
		ddr_out32(&ddr->init_addr, regs->ddr_init_addr);
		ddr_out32(&ddr->init_ext_addr, regs->ddr_init_ext_addr);
		ddr_out32(&ddr->ddr_cdr2, regs->ddr_cdr2);
	}

#ifdef CONFIG_SYS_FSL_ERRATUM_A009803
	/* part 1 of 2 */
	if (regs->ddr_sdram_cfg_2 & SDRAM_CFG2_AP_EN) {
		if (regs->ddr_sdram_cfg & SDRAM_CFG_RD_EN) { /* for RDIMM */
			ddr_out32(&ddr->ddr_sdram_rcw_2,
				  regs->ddr_sdram_rcw_2 & ~0x0f000000);
		}

		ddr_out32(&ddr->err_disable, regs->err_disable | DDR_ERR_DISABLE_APED);
	}
#else
	ddr_out32(&ddr->err_disable, regs->err_disable);
#endif
	ddr_out32(&ddr->err_int_en, regs->err_int_en);
	for (i = 0; i < 64; i++) {
		if (regs->debug[i])
			ddr_out32(&ddr->debug[i], regs->debug[i]);
	}
#ifdef CONFIG_SYS_FSL_ERRATUM_A008378
	/* Erratum applies when accumulated ECC is used, or DBI is enabled */
#define IS_ACC_ECC_EN(v) ((v) & 0x4)
#define IS_DBI(v) ((((v) >> 12) & 0x3) == 0x2)
	if (has_erratum_a008378()) {
		if (IS_ACC_ECC_EN(regs->ddr_sdram_cfg) ||
		    IS_DBI(regs->ddr_sdram_cfg_3))
			ddr_setbits32(&ddr->debug[28], 0x9 << 20);
	}
#endif

#ifdef CONFIG_SYS_FSL_ERRATUM_A008511
	/* Part 1 of 2 */
	/* This erraum only applies to verion 5.2.0 */
	if (get_ddrc_version(ddr) == 0x50200) {
		/* Disable DRAM VRef training */
		ddr_out32(&ddr->ddr_cdr2,
			  regs->ddr_cdr2 & ~DDR_CDR2_VREF_TRAIN_EN);
		/* disable transmit bit deskew */
		temp32 = ddr_in32(&ddr->debug[28]);
		temp32 |= DDR_TX_BD_DIS;
		ddr_out32(&ddr->debug[28], temp32);
		ddr_out32(&ddr->debug[25], 0x9000);
	} else if (get_ddrc_version(ddr) == 0x50201) {
		/* Output enable forced off */
		ddr_out32(&ddr->debug[37], 1 << 31);
		/* Enable Vref training */
		ddr_out32(&ddr->ddr_cdr2,
			  regs->ddr_cdr2 | DDR_CDR2_VREF_TRAIN_EN);
	} else {
		debug("Erratum A008511 doesn't apply.\n");
	}
#endif

#if defined(CONFIG_SYS_FSL_ERRATUM_A009803) || \
	defined(CONFIG_SYS_FSL_ERRATUM_A008511)
	/* Disable D_INIT */
	ddr_out32(&ddr->sdram_cfg_2,
		  regs->ddr_sdram_cfg_2 & ~SDRAM_CFG2_D_INIT);
#endif

#ifdef CONFIG_SYS_FSL_ERRATUM_A009942
	ddr_freq = clk / 1000000;
	tmp = ddr_in32(&ddr->debug[28]);
	if (ddr_freq <= 1333)
		ddr_out32(&ddr->debug[28], tmp | 0x0080006a);
	else if (ddr_freq <= 1600)
		ddr_out32(&ddr->debug[28], tmp | 0x0070006f);
	else if (ddr_freq <= 1867)
		ddr_out32(&ddr->debug[28], tmp | 0x00700076);
	else if (ddr_freq <= 2133)
		ddr_out32(&ddr->debug[28], tmp | 0x0060007b);
#endif

#ifdef CONFIG_SYS_FSL_ERRATUM_A010165
	ddr_freq = clk / 1000000;
	if ((ddr_freq > 1900) && (ddr_freq < 2300)) {
		tmp = ddr_in32(&ddr->debug[28]);
		ddr_out32(&ddr->debug[28], tmp | 0x000a0000);
	}
#endif
	/*
	 * For RDIMMs, JEDEC spec requires clocks to be stable before reset is
	 * deasserted. Clocks start when any chip select is enabled and clock
	 * control register is set. Because all DDR components are connected to
	 * one reset signal, this needs to be done in two steps. Step 1 is to
	 * get the clocks started. Step 2 resumes after reset signal is
	 * deasserted.
	 */
	if (step == 1) {
		udelay(200);
		return 0;
	}

step2:
	/* Set, but do not enable the memory */
	temp_sdram_cfg = regs->ddr_sdram_cfg;
	temp_sdram_cfg &= ~(SDRAM_CFG_MEM_EN);
	ddr_out32(&ddr->sdram_cfg, temp_sdram_cfg);

	/*
	 * 500 painful micro-seconds must elapse between
	 * the DDR clock setup and the DDR config enable.
	 * DDR2 need 200 us, and DDR3 need 500 us from spec,
	 * we choose the max, that is 500 us for all of case.
	 */
	udelay(500);
	mb();
	isb();

#ifdef CONFIG_DEEP_SLEEP
	if (is_warm_boot()) {
		/* enter self-refresh */
		temp_sdram_cfg = ddr_in32(&ddr->sdram_cfg_2);
		temp_sdram_cfg |= SDRAM_CFG2_FRC_SR;
		ddr_out32(&ddr->sdram_cfg_2, temp_sdram_cfg);
		/* do board specific memory setup */
		board_mem_sleep_setup();

		temp_sdram_cfg = (ddr_in32(&ddr->sdram_cfg) | SDRAM_CFG_BI);
	} else
#endif
		temp_sdram_cfg = ddr_in32(&ddr->sdram_cfg) & ~SDRAM_CFG_BI;
	/* Let the controller go */
	ddr_out32(&ddr->sdram_cfg, temp_sdram_cfg | SDRAM_CFG_MEM_EN);
	mb();
	isb();

#if defined(CONFIG_SYS_FSL_ERRATUM_A008511) || defined(CONFIG_SYS_FSL_ERRATUM_A009803)
	/* Part 2 of 2 */
	timeout = 40;
	/* Wait for idle. D_INIT needs to be cleared earlier, or timeout */
	while (!(ddr_in32(&ddr->debug[1]) & 0x2) &&
	       (timeout > 0)) {
		udelay(1000);
		timeout--;
	}
	if (timeout <= 0) {
		debug("Controler ");
		dbgprint_uint(ctrl_num);
		debug(" timeout waiting for idle\n");
		ret = -EIO;
		goto out;
	}

#ifdef CONFIG_SYS_FSL_ERRATUM_A008511
	/* This erraum only applies to verion 5.2.0 */
	if (get_ddrc_version(ddr) == 0x50200) {
		/* The vref setting sequence is different for range 2 */
		if (regs->ddr_cdr2 & DDR_CDR2_VREF_RANGE_2)
			vref_seq = vref_seq2;

		/* Set VREF */
		for (i = 0; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
			if (!(regs->cs[i].config & SDRAM_CS_CONFIG_EN))
				continue;

			mr6 = (regs->ddr_sdram_mode_10 >> 16)		|
				 MD_CNTL_MD_EN				|
				 MD_CNTL_CS_SEL(i)			|
				 MD_CNTL_MD_SEL(6)			|
				 0x00200000;
			temp32 = mr6 | vref_seq[0];
			set_wait_for_bits_clear(&ddr->sdram_md_cntl,
						temp32, MD_CNTL_MD_EN);
			udelay(1);
			debug_hex("MR6 = 0x", temp32);
			temp32 = mr6 | vref_seq[1];
			set_wait_for_bits_clear(&ddr->sdram_md_cntl,
						temp32, MD_CNTL_MD_EN);
			udelay(1);
			debug_hex("MR6 = 0x", temp32);
			temp32 = mr6 | vref_seq[2];
			set_wait_for_bits_clear(&ddr->sdram_md_cntl,
						temp32, MD_CNTL_MD_EN);
			udelay(1);
			debug_hex("MR6 = 0x", temp32);
		}
		ddr_out32(&ddr->sdram_md_cntl, 0);
		temp32 = ddr_in32(&ddr->debug[28]);
		temp32 &= ~DDR_TX_BD_DIS; /* Enable deskew */
		ddr_out32(&ddr->debug[28], temp32);
		ddr_out32(&ddr->debug[1], 0x400);	/* restart deskew */
		/* wait for idle */
		timeout = 40;
		while (!(ddr_in32(&ddr->debug[1]) & 0x2) &&
		       (timeout > 0)) {
			udelay(1000);
			timeout--;
		}
		if (timeout <= 0) {
			debug("Controler ");
			dbgprint_uint(ctrl_num);
			debug(" timeout for idle after restarting deskew\n");
			ret = -EIO;
			goto out;
		}
	}
#endif /* CONFIG_SYS_FSL_ERRATUM_A008511 */

#ifdef CONFIG_SYS_FSL_ERRATUM_A009803
	if (regs->ddr_sdram_cfg_2 & SDRAM_CFG2_AP_EN) {
		/* if it's RDIMM */
		if (regs->ddr_sdram_cfg & SDRAM_CFG_RD_EN) {
			for (i = 0; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
				if (!(regs->cs[i].config & SDRAM_CS_CONFIG_EN))
					continue;
				set_wait_for_bits_clear(&ddr->sdram_md_cntl,
						MD_CNTL_MD_EN |
						MD_CNTL_CS_SEL(i) |
						0x070000ed,
						MD_CNTL_MD_EN);
				udelay(1);
			}
		}

		ddr_out32(&ddr->err_disable,
			  regs->err_disable & ~DDR_ERR_DISABLE_APED);
	}
#endif
	/* Restore D_INIT */
	ddr_out32(&ddr->sdram_cfg_2, regs->ddr_sdram_cfg_2);
#endif

	total_mem_per_ctrl = 0;
	for (i = 0; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
		if (!(regs->cs[i].config & 0x80000000))
			continue;
		total_mem_per_ctrl += 1 << (
			((regs->cs[i].config >> 14) & 0x3) + 2 +
			((regs->cs[i].config >> 8) & 0x7) + 12 +
			((regs->cs[i].config >> 4) & 0x3) + 0 +
			((regs->cs[i].config >> 0) & 0x7) + 8 +
			3 - ((regs->ddr_sdram_cfg >> 19) & 0x3) -
			26);			/* minus 26 (count of 64M) */
	}
	total_mem_per_ctrl_adj = total_mem_per_ctrl;
	/* 3-way interleaving is not considered because only available on T4240 */
	if (regs->cs[0].config & 0x20000000) /* 2-way interleaving */
		total_mem_per_ctrl_adj <<= 1;
	/*
	 * total memory / bus width = transactions needed
	 * transactions needed / data rate = seconds
	 * to add plenty of buffer, double the time
	 * For example, 2GB on 666MT/s 64-bit bus takes about 402ms
	 * Let's wait for 800ms
	 */
	bus_width = 3 - ((ddr_in32(&ddr->sdram_cfg) & SDRAM_CFG_DBW_MASK)
			>> SDRAM_CFG_DBW_SHIFT);
	timeout = ((total_mem_per_ctrl_adj << (6 - bus_width)) * 100 /
		   (clk >> 20)) << 2;
	total_mem_per_ctrl_adj >>= 4;	/* shift down to gb size */
	debug_int("total size in GB ", total_mem_per_ctrl_adj);
	debug_int("Need to wait up to (x 10ms) ", timeout);

	/* Poll DDR_SDRAM_CFG_2[D_INIT] bit until auto-data init is done.  */
	while ((ddr_in32(&ddr->sdram_cfg_2) & SDRAM_CFG2_D_INIT) &&
		(timeout >= 0)) {
		udelay(10000);		/* throttle polling rate */
		timeout--;
	}

	if (timeout <= 0) {
		debug("Error: Waiting for D_INIT timeout.\n");
		ret = -EIO;
		goto out;
	}

#ifdef CONFIG_SYS_FSL_ERRATUM_A009663
	ddr_out32(&ddr->sdram_interval, regs->ddr_sdram_interval);
#endif

#ifdef CONFIG_DEEP_SLEEP
	if (is_warm_boot()) {
		/* exit self-refresh */
		temp_sdram_cfg = ddr_in32(&ddr->sdram_cfg_2);
		temp_sdram_cfg &= ~SDRAM_CFG2_FRC_SR;
		ddr_out32(&ddr->sdram_cfg_2, temp_sdram_cfg);
	}
#endif
#ifndef CONFIG_SHMOO_DDR
	if (run_bist())
#endif
	{
		if (ddr_in32(&ddr->debug[1]) & 0x3d00) {
			debug("Found training error(s): 0x");
			dbgprint_hex(ddr_in32(&ddr->debug[1]));
			debug("\n");
			ret = -EIO;
			goto out;
		}
		debug("BIST test ");
		dbgprint_uint(ctrl_num);
		debug("...");
		/* give it 10x time to cover whole memory */
		timeout = ((total_mem_per_ctrl << (6 - bus_width)) *
			   100 / (clk >> 20)) * 10;
		debug_int("\nWait up to (x 10ms) ", timeout);
		ret = bist(ddr, timeout);
	}

out:
#ifdef CONFIG_SHMOO_DDR
	update_test_result(clk,
			   (ddr_in32(&ddr->sdram_clk_cntl) >> 22) & 0x1f,
			   (ddr_in32(&ddr->ddr_wrlvl_cntl) & 0x1f),
			   ctrl_num, ret);
#endif

	return ret;
}

void ddrc_disable(int ctrl_num)
{
	struct ccsr_ddr *ddr;
	int timeout = 100;

	switch (ctrl_num) {
	case 0:
		ddr = (void *)CONFIG_SYS_FSL_DDR_ADDR;
		break;
#if defined(CONFIG_SYS_FSL_DDR2_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 1)
	case 1:
		ddr = (void *)CONFIG_SYS_FSL_DDR2_ADDR;
		break;
#endif
#if defined(CONFIG_SYS_FSL_DDR3_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 2)
	case 2:
		ddr = (void *)CONFIG_SYS_FSL_DDR3_ADDR;
		break;
#endif
#if defined(CONFIG_SYS_FSL_DDR4_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 3)
	case 3:
		ddr = (void *)CONFIG_SYS_FSL_DDR4_ADDR;
		break;
#endif
	default:
		debug("%s unexpected ctrl_num = ");
		dbgprint_uint(ctrl_num);
		debug("\n");
		return;
	}

	ddr_out32(&ddr->sdram_md_cntl, MD_CNTL_CKE(1));	/* force CKE low */
	ddr_out32(&ddr->sdram_cfg, ddr_in32(&ddr->sdram_cfg) & ~SDRAM_CFG_MEM_EN);
	ddr_out32(&ddr->sdram_md_cntl, MD_CNTL_CKE(0));
	ddr_out32(&ddr->cs0_config, 0);
	ddr_out32(&ddr->cs1_config, 0);
	ddr_out32(&ddr->cs2_config, 0);
	ddr_out32(&ddr->cs3_config, 0);
	ddr_out32(&ddr->sdram_md_cntl_2, 0);
	ddr_out32(&ddr->sdram_cfg_3, SDRAM_CFG3_DDRC_RST);
	while(timeout && ddr_in32(&ddr->sdram_cfg_3) & SDRAM_CFG3_DDRC_RST) {
		mdelay(10);
		timeout--;
	}
	if (!timeout)
		debug("Error: reset DDRC timeout\n");

	ddr_out32(&ddr->err_detect, ddr_in32(&ddr->err_detect));
	ddr_out32(&ddr->err_sbe, 0);
	ddr_out32(&ddr->capture_attributes, 0);
	ddr_out32(&ddr->capture_data_hi, 0);
	ddr_out32(&ddr->capture_data_lo, 0);
	ddr_out32(&ddr->capture_ecc, 0);
	ddr_out32(&ddr->debug[1], ddr_in32(&ddr->debug[1]));
	ddr_out32(&ddr->debug[1], 0);
	ddr_out32(&ddr->debug[25], 0);
	ddr_out32(&ddr->debug[28], 0);
	ddr_clrbits32(&ddr->sdram_cfg_2, SDRAM_CFG2_D_INIT);
}


void dump_ddrc(int ctrl_num)
{
	unsigned int *ddr;
	int i;

	switch (ctrl_num) {
	case 0:
		ddr = (void *)CONFIG_SYS_FSL_DDR_ADDR;
		break;
#if defined(CONFIG_SYS_FSL_DDR2_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 1)
	case 1:
		ddr = (void *)CONFIG_SYS_FSL_DDR2_ADDR;
		break;
#endif
#if defined(CONFIG_SYS_FSL_DDR3_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 2)
	case 2:
		ddr = (void *)CONFIG_SYS_FSL_DDR3_ADDR;
		break;
#endif
#if defined(CONFIG_SYS_FSL_DDR4_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 3)
	case 3:
		ddr = (void *)CONFIG_SYS_FSL_DDR4_ADDR;
		break;
#endif
	default:
		debug("%s unexpected ctrl_num = ");
		dbgprint_uint(ctrl_num);
		debug("\n");
		return;
	}

	for (i = 0; i < 0x400; i++, ddr++) {
		debug("*0x");
		dbgprint_hex((unsigned long)ddr);
		debug(" = 0x");
		dbgprint_hex(*ddr);
		debug("\n");
	}
}

void dump_err(int ctrl_num)
{
	unsigned int *ddr;
	int i;

	switch (ctrl_num) {
	case 0:
		ddr = (void *)CONFIG_SYS_FSL_DDR_ADDR;
		break;
#if defined(CONFIG_SYS_FSL_DDR2_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 1)
	case 1:
		ddr = (void *)CONFIG_SYS_FSL_DDR2_ADDR;
		break;
#endif
#if defined(CONFIG_SYS_FSL_DDR3_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 2)
	case 2:
		ddr = (void *)CONFIG_SYS_FSL_DDR3_ADDR;
		break;
#endif
#if defined(CONFIG_SYS_FSL_DDR4_ADDR) && (CONFIG_SYS_NUM_DDR_CTLRS > 3)
	case 3:
		ddr = (void *)CONFIG_SYS_FSL_DDR4_ADDR;
		break;
#endif
	default:
		debug("%s unexpected ctrl_num = ");
		dbgprint_uint(ctrl_num);
		debug("\n");
		return;
	}

	debug("md_cntl = 0x");
	dbgprint_hex(ddr_in32(ddr + 0x48));
	debug("\nmd_cntl_2 = 0x");
	dbgprint_hex(ddr_in32(ddr + 0x9c));
	debug("\n");
	for (i = 0x388, ddr += 0x388; i < 0x400; i++, ddr++) {
		debug("*0x");
		dbgprint_hex((unsigned long)ddr);
		debug(" = 0x");
		dbgprint_hex(*ddr);
		debug("\n");
	}
}
