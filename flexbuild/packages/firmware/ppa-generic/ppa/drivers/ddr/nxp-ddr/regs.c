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

/*
 * compute CAS write latency according to DDR4 spec
 * CWL =  9 for <= 1600MT/s
 *       10 for <= 1866MT/s
 *       11 for <= 2133MT/s
 *       12 for <= 2400MT/s
 *       14 for <= 2667MT/s
 *       16 for <= 2933MT/s
 *       18 for higher
 */
static inline unsigned int
compute_cas_write_latency(const unsigned long clk)
{
	unsigned int cwl;
	const unsigned int mclk_ps = get_memory_clk_period_ps(clk);
	if (mclk_ps >= 1250)
		cwl = 9;
	else if (mclk_ps >= 1070)
		cwl = 10;
	else if (mclk_ps >= 935)
		cwl = 11;
	else if (mclk_ps >= 833)
		cwl = 12;
	else if (mclk_ps >= 750)
		cwl = 14;
	else if (mclk_ps >= 681)
		cwl = 16;
	else
		cwl = 18;

	return cwl;
}

/* Chip Select Configuration (CSn_CONFIG) */
static void set_csn_config(int i, int cs_n_en,
			   struct ddr_cfg_regs *ddr,
			   const struct memctl_opt *popts,
			   const struct dimm_params *pdimm)
{
	unsigned int intlv_en = 0; /* Memory controller interleave enable */
	unsigned int intlv_ctl = 0; /* Interleaving control */
	unsigned int ap_n_en = 0; /* Chip select n auto-precharge enable */
	unsigned int odt_rd_cfg = 0; /* ODT for reads configuration */
	unsigned int odt_wr_cfg = 0; /* ODT for writes configuration */
	unsigned int ba_bits_cs_n = 0; /* Num of bank bits for SDRAM on CSn */
	unsigned int row_bits_cs_n = 0; /* Num of row bits for SDRAM on CSn */
	unsigned int col_bits_cs_n = 0; /* Num of ocl bits for SDRAM on CSn */
	unsigned int bg_bits_cs_n = 0; /* Num of bank group bits */

	/* Compute CS_CONFIG only for existing ranks of each DIMM.  */
	if (!i) {
		/* These fields only available in CS0_CONFIG */
		if (popts->memctl_interleaving) {
			switch (popts->memctl_interleaving_mode) {
			case FSL_DDR_256B_INTERLEAVING:
			case FSL_DDR_CACHE_LINE_INTERLEAVING:
			case FSL_DDR_PAGE_INTERLEAVING:
			case FSL_DDR_BANK_INTERLEAVING:
			case FSL_DDR_SUPERBANK_INTERLEAVING:
				intlv_en = popts->memctl_interleaving;
				intlv_ctl = popts->memctl_interleaving_mode;
				break;
			default:
				break;
			}
		}
	}
	if (cs_n_en) {
		ap_n_en = popts->cs_local_opts[i].auto_precharge;
		odt_rd_cfg = popts->cs_local_opts[i].odt_rd_cfg;
		odt_wr_cfg = popts->cs_local_opts[i].odt_wr_cfg;
		ba_bits_cs_n = pdimm->bank_addr_bits;
		bg_bits_cs_n = pdimm->bank_group_bits;
		row_bits_cs_n = pdimm->n_row_addr - 12;
		col_bits_cs_n = pdimm->n_col_addr - 8;
	}
	ddr->cs[i].config = (0
		| ((cs_n_en & 0x1) << 31)
		| ((intlv_en & 0x3) << 29)
		| ((intlv_ctl & 0xf) << 24)
		| ((ap_n_en & 0x1) << 23)
		| ((odt_rd_cfg & 0x7) << 20)
		| ((odt_wr_cfg & 0x7) << 16)
		| ((ba_bits_cs_n & 0x3) << 14)
		| ((row_bits_cs_n & 0x7) << 8)
		| ((bg_bits_cs_n & 0x3) << 4)
		| ((col_bits_cs_n & 0x7) << 0)
		);
	debug_int("Debug: cs", i);
	debug_hex("          _config = 0x", ddr->cs[i].config);
}

static void set_csn_config_2(int i, struct ddr_cfg_regs *ddr)
{
	unsigned int pasr_cfg = 0;	/* Partial array self refresh config */

	ddr->cs[i].config_2 = ((pasr_cfg & 7) << 24);
	debug_int("Debug: cs", i);
	debug_hex("          _config_2 = 0x", ddr->cs[i].config_2);
}

/*
 * Check DIMM configuration, return 2 if quad-rank or two dual-rank
 * Return 1 if other two slots configuration. Return 0 if single slot.
 */
static inline int avoid_odt_overlap(const struct ddr_conf *conf,
				    const struct dimm_params *pdimm)
{
	if (conf->cs_in_use == 0xf)
		return 2;

#if CONFIG_DIMM_SLOTS_PER_CTLR == 2
	if (conf->dimm_in_use[0] && conf->dimm_in_use[1])
		return 1;
#endif
	return 0;
}

/*
 * Calculate timing
 * timing_cfg_0: RWT, WRT, RRT, WWT, ACTPD, PREPD, MRD/MOD
 * timing_cfg_1: PRETOACT, ACTTOPRE, ACTTORW, CASLAT, REFREC, WRREC, ACTTOACT, WRTORD
 * timing_cfg_2: ADD_LAT, WR_LAT, RDTOPRE, WRDATADELAY, CKE_PLS, FOUR_ACT
 * timing_cfg_3: EXT bits of PRETOACT, ACTTOPRE, ACTTORW, REFREC, CASLAT, ADD_LAT, WRREC, CNTL_ADJ
 * timing_cfg_4: Same cs RWT, WRT, RRT, WWT, REFINT, DLL_LOCK
 * timing_cfg_5: RODT_ON, RODT_OFF, WODT_ON, WODT_OFF
 * timing_cfg_6: HS_CASLAT, HS_WRLAT, HS_WRREC
 * timing_cfg_7: CKE_RST, CKERE, CKSRX, PAR_LAT, CS_TO_CMD
 * timing_cfg_8: Same cs same bank group, RWT, WRT, RRT, WWT, ACTTOACT, WRTORD, PRE_ALL_REC
 */
static void set_timing_cfg_0(const unsigned long clk,
				struct ddr_cfg_regs *ddr,
				const struct memctl_opt *popts,
				const struct ddr_conf *conf,
				const struct dimm_params *pdimm)
{
	unsigned char trwt_mclk = 0;   /* Read-to-write turnaround */
	unsigned char twrt_mclk = 0;   /* Write-to-read turnaround */
	unsigned char trrt_mclk = 0;   /* Read-to-read turnaround */
	unsigned char twwt_mclk = 0;   /* Write-to-write turnaround */
	unsigned char act_pd_exit_mclk;
	unsigned char pre_pd_exit_mclk;
	unsigned char taxpd_mclk = 0;
	unsigned char tmrd_mclk;
	const unsigned int mclk_ps = get_memory_clk_period_ps(clk);
	/* tXP=max(4nCK, 6ns) */
	int txp = max((int)mclk_ps * 4, 6000); /* unit=ps */

	/* for faster clock, need more time for data setup */
	trwt_mclk = (clk/1000000 > 1900) ? 3 : 2;

	/*
	 * for single quad-rank DIMM and two-slot DIMMs
	 * to avoid ODT overlap
	 */
	switch (avoid_odt_overlap(conf, pdimm)) {
	case 2:
		twrt_mclk = 2;
		twwt_mclk = 2;
		trrt_mclk = 2;
		break;
	default:
		twrt_mclk = 1;
		twwt_mclk = 1;
		trrt_mclk = 0;
		break;
	}

	act_pd_exit_mclk = picos_to_mclk(clk, txp);
	pre_pd_exit_mclk = act_pd_exit_mclk;
	/*
	 * MRS_CYC = max(tMRD, tMOD)
	 * tMRD = 8nCK, tMOD = max(24nCK, 15ns)
	 */
	tmrd_mclk = max(24U, picos_to_mclk(clk, 15000));

	if (popts->trwt_override)
		trwt_mclk = popts->trwt;

	ddr->timing_cfg_0 = (0
		| ((trwt_mclk & 0x3) << 30)	/* RWT */
		| ((twrt_mclk & 0x3) << 28)	/* WRT */
		| ((trrt_mclk & 0x3) << 26)	/* RRT */
		| ((twwt_mclk & 0x3) << 24)	/* WWT */
		| ((act_pd_exit_mclk & 0xf) << 20)  /* ACT_PD_EXIT */
		| ((pre_pd_exit_mclk & 0xF) << 16)  /* PRE_PD_EXIT */
		| ((taxpd_mclk & 0xf) << 8)	/* ODT_PD_EXIT */
		| ((tmrd_mclk & 0x1f) << 0)	/* MRS_CYC */
		);
	debug_hex("Debug: timing_cfg_0 = 0x%08x\n", ddr->timing_cfg_0);
}

static void set_timing_cfg_1(const unsigned long clk,
			     struct ddr_cfg_regs *ddr,
			     const struct memctl_opt *popts,
			     const struct dimm_params *pdimm,
			     unsigned int cas_latency)
{
	/* Precharge-to-activate interval (tRP) */
	unsigned char pretoact_mclk;
	/* Activate to precharge interval (tRAS) */
	unsigned char acttopre_mclk;
	/*  Activate to read/write interval (tRCD) */
	unsigned char acttorw_mclk;
	/* CASLAT */
	unsigned char caslat_ctrl;
	/*  Refresh recovery time (tRFC) ; trfc_low */
	unsigned char refrec_ctrl;
	/* Last data to precharge minimum interval (tWR) */
	unsigned char wrrec_mclk;
	/* Activate-to-activate interval (tRRD) */
	unsigned char acttoact_mclk;
	/* Last write data pair to read command issue interval (tWTR) */
	unsigned char wrtord_mclk;
	/* DDR4 supports 10, 12, 14, 16, 18, 20, 24 */
	static const unsigned char wrrec_table[] = {
		10, 10, 10, 10, 10,
		10, 10, 10, 10, 10,
		12, 12, 14, 14, 16,
		16, 18, 18, 20, 20,
		24, 24, 24, 24};

	pretoact_mclk = picos_to_mclk(clk, pdimm->trp_ps);
	acttopre_mclk = picos_to_mclk(clk, pdimm->tras_ps);
	acttorw_mclk = picos_to_mclk(clk, pdimm->trcd_ps);

	/*
	 * Translate CAS Latency to a DDR controller field value:
	 *
	 *      CAS Lat DDR I   DDR II  Ctrl
	 *      Clocks  SPD Bit SPD Bit Value
	 *      ------- ------- ------- -----
	 *      1.0     0               0001
	 *      1.5     1               0010
	 *      2.0     2       2       0011
	 *      2.5     3               0100
	 *      3.0     4       3       0101
	 *      3.5     5               0110
	 *      4.0             4       0111
	 *      4.5                     1000
	 *      5.0             5       1001
	 */
	/*
	 * if the CAS latency more than 8 cycle,
	 * we need set extend bit for it at
	 * TIMING_CFG_3[EXT_CASLAT]
	 */
	caslat_ctrl = (cas_latency - 1) << 1;

	refrec_ctrl = picos_to_mclk(clk, pdimm->trfc1_ps) - 8;
	wrrec_mclk = picos_to_mclk(clk, pdimm->twr_ps);
	acttoact_mclk = max(picos_to_mclk(clk, pdimm->trrds_ps), 4U);
	wrtord_mclk = max(2U, picos_to_mclk(clk, 2500));
	if ((wrrec_mclk < 1) || (wrrec_mclk > 24)) {
		debug("Error: WRREC doesn't support clock ");
		dbgprint_uint(wrrec_mclk);
		debug("\n");
	} else
		wrrec_mclk = wrrec_table[wrrec_mclk - 1];
	if (popts->otf_burst_chop_en)
		wrrec_mclk += 2;

	if (popts->otf_burst_chop_en)
		wrtord_mclk += 2;

	ddr->timing_cfg_1 = (0
		| ((pretoact_mclk & 0x0F) << 28)
		| ((acttopre_mclk & 0x0F) << 24)
		| ((acttorw_mclk & 0xF) << 20)
		| ((caslat_ctrl & 0xF) << 16)
		| ((refrec_ctrl & 0xF) << 12)
		| ((wrrec_mclk & 0x0F) << 8)
		| ((acttoact_mclk & 0x0F) << 4)
		| ((wrtord_mclk & 0x0F) << 0)
		);
	debug_hex("Debug: timing_cfg_1 = 0x", ddr->timing_cfg_1);
}

static void set_timing_cfg_2(const unsigned long clk,
			     struct ddr_cfg_regs *ddr,
			     const struct memctl_opt *popts,
			     unsigned int additive_latency)
{
	/* Additive latency */
	unsigned char add_lat_mclk;
	/* CAS-to-preamble override */
	unsigned short cpo;
	/* Write latency */
	unsigned char wr_lat;
	/*  Read to precharge (tRTP) */
	unsigned char rd_to_pre;
	/* Write command to write data strobe timing adjustment */
	unsigned char wr_data_delay;
	/* Minimum CKE pulse width (tCKE) */
	unsigned char cke_pls;
	/* Window for four activates (tFAW) */
	unsigned short four_act;

	add_lat_mclk = additive_latency;
	cpo = popts->cpo_override;

	wr_lat = compute_cas_write_latency(clk);

	rd_to_pre = picos_to_mclk(clk, 7500);
	/*
	 * JEDEC has some min requirements for tRTP
	 */
	if (rd_to_pre < 4)
		rd_to_pre = 4;
	if (popts->otf_burst_chop_en)
		rd_to_pre += 2; /* according to UM */

	wr_data_delay = popts->write_data_delay;
	cpo = 0;
	cke_pls = max(3U, picos_to_mclk(clk, 5000));
	four_act = picos_to_mclk(clk,
				 popts->tfaw_window_four_activates_ps);

	ddr->timing_cfg_2 = (0
		| ((add_lat_mclk & 0xf) << 28)
		| ((cpo & 0x1f) << 23)
		| ((wr_lat & 0xf) << 19)
		| ((wr_lat & 0x10) << 18)
		| ((rd_to_pre & RD_TO_PRE_MASK) << RD_TO_PRE_SHIFT)
		| ((wr_data_delay & WR_DATA_DELAY_MASK) << WR_DATA_DELAY_SHIFT)
		| ((cke_pls & 0x7) << 6)
		| ((four_act & 0x3f) << 0)
		);
	debug_hex("Debug: timing_cfg_2 = 0x", ddr->timing_cfg_2);
}

static void set_timing_cfg_3(const unsigned long clk,
			     struct ddr_cfg_regs *ddr,
			     const struct memctl_opt *popts,
			     const struct dimm_params *pdimm,
			     unsigned int cas_latency,
			     unsigned int additive_latency)
{
	/* Extended precharge to activate interval (tRP) */
	unsigned int ext_pretoact = 0;
	/* Extended Activate to precharge interval (tRAS) */
	unsigned int ext_acttopre = 0;
	/* Extended activate to read/write interval (tRCD) */
	unsigned int ext_acttorw = 0;
	/* Extended refresh recovery time (tRFC) */
	unsigned int ext_refrec;
	/* Extended MCAS latency from READ cmd */
	unsigned int ext_caslat = 0;
	/* Extended additive latency */
	unsigned int ext_add_lat = 0;
	/* Extended last data to precharge interval (tWR) */
	unsigned int ext_wrrec = 0;
	/* Control Adjust */
	unsigned int cntl_adj = 0;

	ext_pretoact = picos_to_mclk(clk, pdimm->trp_ps) >> 4;
	ext_acttopre = picos_to_mclk(clk, pdimm->tras_ps) >> 4;
	ext_acttorw = picos_to_mclk(clk, pdimm->trcd_ps) >> 4;
	ext_caslat = (2 * cas_latency - 1) >> 4;
	ext_add_lat = additive_latency >> 4;
	ext_refrec = (picos_to_mclk(clk, pdimm->trfc1_ps) - 8) >> 4;
	ext_wrrec = (picos_to_mclk(clk, pdimm->twr_ps) +
		     (popts->otf_burst_chop_en ? 2 : 0)) >> 4;

	ddr->timing_cfg_3 = (0
		| ((ext_pretoact & 0x1) << 28)
		| ((ext_acttopre & 0x3) << 24)
		| ((ext_acttorw & 0x1) << 22)
		| ((ext_refrec & 0x1F) << 16)
		| ((ext_caslat & 0x3) << 12)
		| ((ext_add_lat & 0x1) << 10)
		| ((ext_wrrec & 0x1) << 8)
		| ((cntl_adj & 0x7) << 0)
		);
	debug_hex("Debug: timing_cfg_3 = 0x", ddr->timing_cfg_3);
}

static void set_timing_cfg_4(struct ddr_cfg_regs *ddr,
			     const struct memctl_opt *popts)
{
	unsigned int rwt = 0; /* Read-to-write turnaround for same CS */
	unsigned int wrt = 0; /* Write-to-read turnaround for same CS */
	unsigned int rrt = 0; /* Read-to-read turnaround for same CS */
	unsigned int wwt = 0; /* Write-to-write turnaround for same CS */
	unsigned int trwt_mclk = 0;	/* ext_rwt */
	unsigned int dll_lock = 0; /* DDR SDRAM DLL Lock Time */

	if (popts->burst_length == DDR_BL8) {
		/* We set BL/2 for fixed BL8 */
		rrt = 0;	/* BL/2 clocks */
		wwt = 0;	/* BL/2 clocks */
	} else {
		/* We need to set BL/2 + 2 to BC4 and OTF */
		rrt = 2;	/* BL/2 + 2 clocks */
		wwt = 2;	/* BL/2 + 2 clocks */
	}
	dll_lock = 2;	/* tDLLK = 1024 clocks */

	if (popts->trwt_override)
		trwt_mclk = popts->trwt;

	ddr->timing_cfg_4 = (0
			     | ((rwt & 0xf) << 28)
			     | ((wrt & 0xf) << 24)
			     | ((rrt & 0xf) << 20)
			     | ((wwt & 0xf) << 16)
			     | ((trwt_mclk & 0xc) << 12)
			     | (dll_lock & 0x3)
			     );
	debug_hex("Debug: timing_cfg_4 = 0x", ddr->timing_cfg_4);
}

static void set_timing_cfg_5(struct ddr_cfg_regs *ddr,
			     unsigned int cas_latency)
{
	unsigned int rodt_on = 0;	/* Read to ODT on */
	unsigned int rodt_off = 0;	/* Read to ODT off */
	unsigned int wodt_on = 0;	/* Write to ODT on */
	unsigned int wodt_off = 0;	/* Write to ODT off */

	unsigned int wr_lat;

	/* WR_LAT cannot be zero if set */
	if (!ddr->timing_cfg_2) {
		debug("Error: timing_cfg_2 not set\n");
		return;
	}

	wr_lat = ((ddr->timing_cfg_2 & 0x00780000) >> 19) +
			      ((ddr->timing_cfg_2 & 0x00040000) >> 14);
	/* rodt_on = timing_cfg_1[caslat] - timing_cfg_2[wrlat] + 1 */
	if (cas_latency >= wr_lat)
		rodt_on = cas_latency - wr_lat + 1;
	rodt_off = 4;	/*  4 clocks */
	wodt_on = 1;	/*  1 clocks */
	wodt_off = 4;	/*  4 clocks */

	ddr->timing_cfg_5 = (0
			     | ((rodt_on & 0x1f) << 24)
			     | ((rodt_off & 0x7) << 20)
			     | ((wodt_on & 0x1f) << 12)
			     | ((wodt_off & 0x7) << 8)
			     );
	debug_hex("Debug: timing_cfg_5 = 0x", ddr->timing_cfg_5);
}

static void set_timing_cfg_6(struct ddr_cfg_regs *ddr)
{
	unsigned int hs_caslat = 0;
	unsigned int hs_wrlat = 0;
	unsigned int hs_wrrec = 0;
	unsigned int hs_clkadj = 0;
	unsigned int hs_wrlvl_start = 0;

	ddr->timing_cfg_6 = (0
			     | ((hs_caslat & 0x1f) << 24)
			     | ((hs_wrlat & 0x1f) << 19)
			     | ((hs_wrrec & 0x1f) << 12)
			     | ((hs_clkadj & 0x1f) << 6)
			     | ((hs_wrlvl_start & 0x1f) << 0)
			    );
	debug_hex("Debug: timing_cfg_6 = 0x", ddr->timing_cfg_6);
}

static void set_timing_cfg_7(const unsigned long clk,
			     struct ddr_cfg_regs *ddr,
			     const struct dimm_params *pdimm)
{
	unsigned int txpr, tcksre, tcksrx;
	unsigned int cke_rst, cksre, cksrx, par_lat = 0, cs_to_cmd;
	const unsigned int mclk_ps = get_memory_clk_period_ps(clk);

	/* NUM_PR cannot be 0 if set */
	if (!ddr->ddr_sdram_cfg_2) {
		debug("Error: sdram_cfg_2 not set\n");
		return;
	}

	txpr = max(5U, picos_to_mclk(clk, pdimm->trfc1_ps + 10000));
	tcksre = max(5U, picos_to_mclk(clk, 10000));
	tcksrx = max(5U, picos_to_mclk(clk, 10000));

	if (ddr->ddr_sdram_cfg_2 & SDRAM_CFG2_AP_EN) {
		if (mclk_ps >= 935) {
			/* parity latency 4 clocks in case of 1600/1866/2133 */
			par_lat = 4;
		} else if (mclk_ps >= 833) {
			/* parity latency 5 clocks for DDR4-2400 */
			par_lat = 5;
		} else {
			debug("Error: mclk_ps not supported for parity ");
			dbgprint_uint(mclk_ps);
			debug("\n");
		}
	}

	cs_to_cmd = 0;

	if (txpr <= 200)
		cke_rst = 0;
	else if (txpr <= 256)
		cke_rst = 1;
	else if (txpr <= 512)
		cke_rst = 2;
	else
		cke_rst = 3;

	if (tcksre <= 19)
		cksre = tcksre - 5;
	else
		cksre = 15;

	if (tcksrx <= 19)
		cksrx = tcksrx - 5;
	else
		cksrx = 15;

	ddr->timing_cfg_7 = (0
			     | ((cke_rst & 0x3) << 28)
			     | ((cksre & 0xf) << 24)
			     | ((cksrx & 0xf) << 20)
			     | ((par_lat & 0xf) << 16)
			     | ((cs_to_cmd & 0xf) << 4)
			    );
	debug_hex("Debug: timing_cfg_7 = 0x", ddr->timing_cfg_7);
}

static void set_timing_cfg_8(const unsigned long clk,
			     struct ddr_cfg_regs *ddr,
			     const struct memctl_opt *popts,
			     const struct dimm_params *pdimm,
			     unsigned int cas_latency)
{
	unsigned int rwt_bg, wrt_bg, rrt_bg, wwt_bg;
	unsigned int acttoact_bg, wrtord_bg, pre_all_rec;
	unsigned int tccdl = picos_to_mclk(clk, pdimm->tccdl_ps);
	unsigned int wr_lat;

	/* WR_LAT cannot be 0 if set */
	if (!ddr->timing_cfg_2) {
		debug("Error: timing_cfg_2 not set\n");
		return;
	}
	wr_lat = ((ddr->timing_cfg_2 & 0x00780000) >> 19) +
			      ((ddr->timing_cfg_2 & 0x00040000) >> 14);

	rwt_bg = cas_latency + 2 + 4 - wr_lat;
	if (rwt_bg < tccdl)
		rwt_bg = tccdl - rwt_bg;
	else
		rwt_bg = 0;

	wrt_bg = wr_lat + 4 + 1 - cas_latency;
	if (wrt_bg < tccdl)
		wrt_bg = tccdl - wrt_bg;
	else
		wrt_bg = 0;

	if (popts->burst_length == DDR_BL8) {
		rrt_bg = tccdl - 4;
		wwt_bg = tccdl - 4;
	} else {
		rrt_bg = tccdl - 2;
		wwt_bg = tccdl - 2;
	}

	acttoact_bg = picos_to_mclk(clk, pdimm->trrdl_ps);
	wrtord_bg = max(4U, picos_to_mclk(clk, 7500));
	if (popts->otf_burst_chop_en)
		wrtord_bg += 2;

	pre_all_rec = 0;

	ddr->timing_cfg_8 = (0
			     | ((rwt_bg & 0xf) << 28)
			     | ((wrt_bg & 0xf) << 24)
			     | ((rrt_bg & 0xf) << 20)
			     | ((wwt_bg & 0xf) << 16)
			     | ((acttoact_bg & 0xf) << 12)
			     | ((wrtord_bg & 0xf) << 8)
			     | ((pre_all_rec & 0x1f) << 0)
			    );

	debug_hex("Debug: timing_cfg_8 = 0x", ddr->timing_cfg_8);
}


static void set_ddr_sdram_rcw(struct ddr_cfg_regs *ddr,
			      const struct memctl_opt *popts,
			      const struct dimm_params *pdimm)
{
	if (pdimm->registered_dimm) {
		if (popts->rcw_override) {
			ddr->ddr_sdram_rcw_1 = popts->rcw_1;
			ddr->ddr_sdram_rcw_2 = popts->rcw_2;
		} else {
			ddr->ddr_sdram_rcw_1 =
				pdimm->rcw[0] << 28 | \
				pdimm->rcw[1] << 24 | \
				pdimm->rcw[2] << 20 | \
				pdimm->rcw[3] << 16 | \
				pdimm->rcw[4] << 12 | \
				pdimm->rcw[5] << 8 | \
				pdimm->rcw[6] << 4 | \
				pdimm->rcw[7];
			ddr->ddr_sdram_rcw_2 =
				pdimm->rcw[8] << 28 | \
				pdimm->rcw[9] << 24 | \
				pdimm->rcw[10] << 20 | \
				pdimm->rcw[11] << 16 | \
				pdimm->rcw[12] << 12 | \
				pdimm->rcw[13] << 8 | \
				pdimm->rcw[14] << 4 | \
				pdimm->rcw[15];
		}
		debug_hex("Debug: ddr_sdram_rcw_1 = 0x", ddr->ddr_sdram_rcw_1);
		debug_hex("Debug: ddr_sdram_rcw_2 = 0x", ddr->ddr_sdram_rcw_2);
	}
}

static void set_ddr_sdram_cfg(struct ddr_cfg_regs *ddr,
			      const struct memctl_opt *popts,
			      const struct dimm_params *pdimm)
{
	const unsigned int mem_en = 1;	/* DDR SDRAM interface logic enable */
	unsigned int sren;		/* Self refresh enable (during sleep) */
	unsigned int ecc_en;		/* ECC enable. */
	unsigned int rd_en;		/* Registered DIMM enable */
	unsigned int dyn_pwr;		/* Dynamic power management mode */
	unsigned int dbw;		/* DRAM dta bus width */
	unsigned int eight_be = 0;	/* 8-beat burst enable, DDR2 is zero */
	unsigned int ncap = 0;		/* Non-concurrent auto-precharge */
	unsigned int threet_en;		/* Enable 3T timing */
	unsigned int twot_en;		/* Enable 2T timing */
	unsigned int ba_intlv_ctl;	/* Bank (CS) interleaving control */
	unsigned int x32_en = 0;	/* x32 enable */
	unsigned int pchb8 = 0;		/* precharge bit 8 enable */
	unsigned int hse;		/* Global half strength override */
	unsigned int acc_ecc_en = 0;	/* Accumulated ECC enable */
	unsigned int mem_halt = 0;	/* memory controller halt */
	unsigned int bi = 0;		/* Bypass initialization */
	const unsigned int sdram_type = SDRAM_TYPE_DDR4;	/* Type of SDRAM */

	sren = popts->self_refresh_in_sleep;
	if (pdimm->edc_config == 0x02) {
		ecc_en = popts->ecc_mode;
	} else {
		ecc_en = 0;
	}

	if (pdimm->registered_dimm) {
		rd_en = 1;
		twot_en = 0;
	} else {
		rd_en = 0;
		twot_en = popts->twot_en;
	}

	dyn_pwr = popts->dynamic_power;
	dbw = popts->data_bus_used;
	/* 8-beat burst enable DDR-III case
	 * we must clear it when use the on-the-fly mode,
	 * must set it when use the 32-bits bus mode.
	 */
	if (popts->burst_length == DDR_BL8)
		eight_be = 1;
	if (popts->burst_length == DDR_OTF)
		eight_be = 0;
	if (dbw == 0x1)
		eight_be = 1;

	threet_en = popts->threet_en;
	ba_intlv_ctl = popts->ba_intlv_ctl;
	hse = popts->half_strength_driver_enable;

	/* set when ddr bus width < 64 */
	acc_ecc_en = (dbw != 0 && ecc_en == 1) ? 1 : 0;

	ddr->ddr_sdram_cfg = (0
			| ((mem_en & 0x1) << 31)
			| ((sren & 0x1) << 30)
			| ((ecc_en & 0x1) << 29)
			| ((rd_en & 0x1) << 28)
			| ((sdram_type & 0x7) << 24)
			| ((dyn_pwr & 0x1) << 21)
			| ((dbw & 0x3) << 19)
			| ((eight_be & 0x1) << 18)
			| ((ncap & 0x1) << 17)
			| ((threet_en & 0x1) << 16)
			| ((twot_en & 0x1) << 15)
			| ((ba_intlv_ctl & 0x7F) << 8)
			| ((x32_en & 0x1) << 5)
			| ((pchb8 & 0x1) << 4)
			| ((hse & 0x1) << 3)
			| ((acc_ecc_en & 0x1) << 2)
			| ((mem_halt & 0x1) << 1)
			| ((bi & 0x1) << 0)
			);
	debug_hex("Debug: ddr_sdram_cfg = 0x", ddr->ddr_sdram_cfg);
}

static void set_ddr_sdram_cfg_2(const unsigned long clk,
				struct ddr_cfg_regs *ddr,
				const struct memctl_opt *popts)
{
	unsigned int frc_sr = 0;	/* Force self refresh */
	unsigned int sr_ie = 0;		/* Self-refresh interrupt enable */
	unsigned int odt_cfg = 0;	/* ODT configuration */
	unsigned int num_pr;		/* Number of posted refreshes */
	unsigned int slow = 0;		/* DDR will be run less than 1250 */
	unsigned int x4_en = 0;		/* x4 DRAM enable */
	unsigned int obc_cfg;		/* On-The-Fly Burst Chop Cfg */
	unsigned int ap_en;		/* Address Parity Enable */
	unsigned int d_init;		/* DRAM data initialization */
	unsigned int rcw_en = 0;	/* Register Control Word Enable */
	unsigned int md_en = 0;		/* Mirrored DIMM Enable */
	unsigned int qd_en = 0;		/* quad-rank DIMM Enable */
	const unsigned int unq_mrs_en = 1;
	int i;
	for (i = 0; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
		if (popts->cs_local_opts[i].odt_rd_cfg
			|| popts->cs_local_opts[i].odt_wr_cfg) {
			odt_cfg = SDRAM_CFG2_ODT_ONLY_READ;
			break;
		}
	}
	sr_ie = popts->self_refresh_interrupt_en;
	num_pr = 1;	/* Make this configurable */

	obc_cfg = popts->otf_burst_chop_en;

	slow = clk < 1249000000;

	if (popts->registered_dimm_en)
		rcw_en = 1;

	/* DDR4 can have address parity for UDIMM and discrete */
	ap_en = popts->ap_en;

	x4_en = popts->x4_en ? 1 : 0;

	/* Use the DDR controller to auto initialize memory. */
	d_init = popts->ecc_init_using_memctl;

	md_en = popts->mirrored_dimm;
	qd_en = popts->quad_rank_present ? 1 : 0;
	ddr->ddr_sdram_cfg_2 = (0
		| ((frc_sr & 0x1) << 31)
		| ((sr_ie & 0x1) << 30)
		| ((odt_cfg & 0x3) << 21)
		| ((num_pr & 0xf) << 12)
		| ((slow & 1) << 11)
		| (x4_en << 10)
		| (qd_en << 9)
		| (unq_mrs_en << 8)
		| ((obc_cfg & 0x1) << 6)
		| ((ap_en & 0x1) << 5)
		| ((d_init & 0x1) << 4)
		| ((rcw_en & 0x1) << 2)
		| ((md_en & 0x1) << 0)
		);
	debug_hex("Debug: ddr_sdram_cfg_2 = 0x", ddr->ddr_sdram_cfg_2);
}

static void set_ddr_sdram_interval(const unsigned long clk,
				   struct ddr_cfg_regs *ddr,
				   const struct memctl_opt *popts,
				   const struct dimm_params *pdimm)
{
	unsigned int refint;	/* Refresh interval */
	unsigned int bstopre;	/* Precharge interval */

	refint = picos_to_mclk(clk, pdimm->refresh_rate_ps);

	bstopre = popts->bstopre;

	/* refint field used 0x3FFF in earlier controllers */
	ddr->ddr_sdram_interval = (0
				   | ((refint & 0xFFFF) << 16)
				   | ((bstopre & 0x3FFF) << 0)
				   );
	debug_hex("Debug: ddr_sdram_interval = 0x", ddr->ddr_sdram_interval);
}

static void set_ddr_sdram_mode_1_3_5_7(const unsigned long clk,
				       struct ddr_cfg_regs *ddr,
				       const struct memctl_opt *popts,
				       const struct dimm_params *pdimm,
				       unsigned int cas_latency,
				       unsigned int additive_latency)
{
	int i;
	unsigned short esdmode;		/* Extended SDRAM mode */
	unsigned short sdmode;		/* SDRAM mode */

	/* Mode Register - MR1 */
	unsigned int qoff = 0;		/* Output buffer enable 0=yes, 1=no */
	unsigned int tdqs_en = 0;	/* TDQS Enable: 0=no, 1=yes */
	unsigned int rtt;
	unsigned int wrlvl_en = 0;	/* Write level enable: 0=no, 1=yes */
	unsigned int al = 0;		/* Posted CAS# additive latency (AL) */
	unsigned int dic = 0;		/* Output driver impedance, 40ohm */
	unsigned int dll_en = 1;	/* DLL Enable  1=Enable (Normal),
						       0=Disable (Test/Debug) */

	/* Mode Register - MR0 */
	unsigned int wr = 0;	/* Write Recovery */
	unsigned int dll_rst;	/* DLL Reset */
	unsigned int mode;	/* Normal=0 or Test=1 */
	unsigned int caslat = 4;/* CAS# latency, default set as 6 cycles */
	/* BT: Burst Type (0=Nibble Sequential, 1=Interleaved) */
	unsigned int bt;
	unsigned int bl;	/* BL: Burst Length */

	unsigned int wr_mclk;
	/* DDR4 support WR 10, 12, 14, 16, 18, 20, 24 */
	static const unsigned char wr_table[] = {
		0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6};
	/* DDR4 support CAS 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22, 24 */
	static const unsigned char cas_latency_table[] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 8,
		9, 9, 10, 10, 11, 11};

	if (popts->rtt_override)
		rtt = popts->rtt_override_value;
	else
		rtt = popts->cs_local_opts[0].odt_rtt_norm;

	if (additive_latency == (cas_latency - 1))
		al = 1;
	if (additive_latency == (cas_latency - 2))
		al = 2;

	if (popts->quad_rank_present)
		dic = 1;	/* output driver impedance 240/7 ohm */

	/*
	 * The esdmode value will also be used for writing
	 * MR1 during write leveling for DDR4, although the
	 * bits specifically related to the write leveling
	 * scheme will be handled automatically by the DDR
	 * controller. so we set the wrlvl_en = 0 here.
	 */
	esdmode = (0
		| ((qoff & 0x1) << 12)
		| ((tdqs_en & 0x1) << 11)
		| ((rtt & 0x7) << 8)
		| ((wrlvl_en & 0x1) << 7)
		| ((al & 0x3) << 3)
		| ((dic & 0x3) << 1)   /* DIC field is split */
		| ((dll_en & 0x1) << 0)
		);

	/*
	 * DLL control for precharge PD
	 * 0=slow exit DLL off (tXPDLL)
	 * 1=fast exit DLL on (tXP)
	 */

	wr_mclk = picos_to_mclk(clk, pdimm->twr_ps);
	if (wr_mclk <= 24) {
		wr = wr_table[wr_mclk - 10];
	} else {
		debug("Error: unsupported write recovery for mode register wr_mclk = ");
		dbgprint_uint(wr_mclk);
		debug("\n");
	}

	dll_rst = 0;	/* dll no reset */
	mode = 0;	/* normal mode */

	/* look up table to get the cas latency bits */
	if (cas_latency >= 9 && cas_latency <= 24)
		caslat = cas_latency_table[cas_latency - 9];
	else
		debug("Error: unsupported cas latency for mode register\n");

	bt = 0;	/* Nibble sequential */

	switch (popts->burst_length) {
	case DDR_BL8:
		bl = 0;
		break;
	case DDR_OTF:
		bl = 1;
		break;
	case DDR_BC4:
		bl = 2;
		break;
	default:
		debug("Error: invalid burst length ");
		dbgprint_uint(popts->burst_length);
		debug("\nDefaulting to on-the-fly BC4 or BL8 beats.\n");
		bl = 1;
		break;
	}

	sdmode = (0
		  | ((wr & 0x7) << 9)
		  | ((dll_rst & 0x1) << 8)
		  | ((mode & 0x1) << 7)
		  | (((caslat >> 1) & 0x7) << 4)
		  | ((bt & 0x1) << 3)
		  | ((caslat & 1) << 2)
		  | ((bl & 0x3) << 0)
		  );

	ddr->ddr_sdram_mode = (0
			       | ((esdmode & 0xFFFF) << 16)
			       | ((sdmode & 0xFFFF) << 0)
			       );
	debug_hex("Debug: ddr_sdram_mode = 0x", ddr->ddr_sdram_mode);

	for (i = 1; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
		if (popts->rtt_override)
			rtt = popts->rtt_override_value;
		else
			rtt = popts->cs_local_opts[i].odt_rtt_norm;

		esdmode &= 0xF8FF;	/* clear bit 10,9,8 for rtt */
		esdmode |= (rtt & 0x7) << 8;
		switch (i) {
		case 1:
			ddr->ddr_sdram_mode_3 = (0
			       | ((esdmode & 0xFFFF) << 16)
			       | ((sdmode & 0xFFFF) << 0)
			       );
			debug_hex("Debug: ddr_sdram_mode_3 = 0x", ddr->ddr_sdram_mode_3);
			break;
		case 2:
			ddr->ddr_sdram_mode_5 = (0
			       | ((esdmode & 0xFFFF) << 16)
			       | ((sdmode & 0xFFFF) << 0)
			       );
			debug_hex("Debug: ddr_sdram_mode_5 = 0x", ddr->ddr_sdram_mode_5);
			break;
		case 3:
			ddr->ddr_sdram_mode_7 = (0
			       | ((esdmode & 0xFFFF) << 16)
			       | ((sdmode & 0xFFFF) << 0)
			       );
			debug_hex("Debug: ddr_sdram_mode_5 = 0x", ddr->ddr_sdram_mode_5);
			break;
		}
	}

}

static void set_ddr_sdram_mode_2_4_6_8(const unsigned long clk,
				       struct ddr_cfg_regs *ddr,
				       const struct memctl_opt *popts,
				       const struct dimm_params *pdimm)
{
	int i;
	unsigned short esdmode2 = 0;	/* Extended SDRAM mode 2 */
	unsigned short esdmode3 = 0;	/* Extended SDRAM mode 3 */
	unsigned int wr_crc = 0;	/* Disable */
	unsigned int rtt_wr = 0;	/* Rtt_WR - dynamic ODT off */
	const unsigned int srt = 0;	/* self-refresh temerature, normal range */
	unsigned int cwl = compute_cas_write_latency(clk) - 9;
	unsigned int mpr = 0;	/* serial */
	unsigned int wc_lat;
	const unsigned int mclk_ps = get_memory_clk_period_ps(clk);

	if (popts->rtt_override)
		rtt_wr = popts->rtt_wr_override_value;
	else
		rtt_wr = popts->cs_local_opts[0].odt_rtt_wr;

	esdmode2 = (0
		| ((wr_crc & 0x1) << 12)
		| ((rtt_wr & 0x3) << 9)
		| ((srt & 0x3) << 6)
		| ((cwl & 0x7) << 3));

	if (mclk_ps >= 1250)
		wc_lat = 0;
	else if (mclk_ps >= 833)
		wc_lat = 1;
	else
		wc_lat = 2;

	esdmode3 = (0
		| ((mpr & 0x3) << 11)
		| ((wc_lat & 0x3) << 9));

	ddr->ddr_sdram_mode_2 = (0
				 | ((esdmode2 & 0xFFFF) << 16)
				 | ((esdmode3 & 0xFFFF) << 0)
				 );
	debug_hex("Debug: ddr_sdram_mode_2 = 0x", ddr->ddr_sdram_mode_2);

	for (i = 1; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
		if (popts->rtt_override)
			rtt_wr = popts->rtt_wr_override_value;
		else
			rtt_wr = popts->cs_local_opts[i].odt_rtt_wr;

		esdmode2 &= 0xF9FF;	/* clear bit 10, 9 */
		esdmode2 |= (rtt_wr & 0x3) << 9;
		switch (i) {
		case 1:
			ddr->ddr_sdram_mode_4 = (0
				| ((esdmode2 & 0xFFFF) << 16)
				| ((esdmode3 & 0xFFFF) << 0)
				);
			debug_hex("Debug: ddr_sdram_mode_4 = 0x", ddr->ddr_sdram_mode_4);
			break;
		case 2:
			ddr->ddr_sdram_mode_6 = (0
				| ((esdmode2 & 0xFFFF) << 16)
				| ((esdmode3 & 0xFFFF) << 0)
				);
			debug_hex("Debug: ddr_sdram_mode_6 = 0x", ddr->ddr_sdram_mode_6);
			break;
		case 3:
			ddr->ddr_sdram_mode_8 = (0
				| ((esdmode2 & 0xFFFF) << 16)
				| ((esdmode3 & 0xFFFF) << 0)
				);
			debug_hex("Debug: ddr_sdram_mode_8 = 0x", ddr->ddr_sdram_mode_8);
			break;
		}
	}
}

static void set_ddr_sdram_mode_9_11_13_15(const unsigned long clk,
					  struct ddr_cfg_regs *ddr,
					  const struct ddr_conf *conf)
{
	int i;
	unsigned short esdmode4 = 0;	/* Extended SDRAM mode 4 */
	unsigned short esdmode5;	/* Extended SDRAM mode 5 */
	int rtt_park = 0;
	bool four_cs = false;
	const unsigned int mclk_ps = get_memory_clk_period_ps(clk);

	if (conf->cs_in_use == 0xf)
		four_cs = true;
	if (ddr->cs[0].config & SDRAM_CS_CONFIG_EN) {
		esdmode5 = 0x00000500;	/* Data mask enable, RTT_PARK CS0 */
		rtt_park = four_cs ? 0 : 1;
	} else {
		esdmode5 = 0x00000400;	/* Data mask enabled */
	}

	/* set command/address parity latency */
	if (ddr->ddr_sdram_cfg_2 & SDRAM_CFG2_AP_EN) {
		if (mclk_ps >= 935) {
			/* for DDR4-1600/1866/2133 */
			esdmode5 |= DDR_MR5_CA_PARITY_LAT_4_CLK;
		} else if (mclk_ps >= 833) {
			/* for DDR4-2400 */
			esdmode5 |= DDR_MR5_CA_PARITY_LAT_5_CLK;
		} else {
			esdmode5 |= DDR_MR5_CA_PARITY_LAT_5_CLK;
			debug("Warning: mclk_ps not supported ");
			dbgprint_uint(mclk_ps);
			debug("\n");
			
		}
	}

	ddr->ddr_sdram_mode_9 = (0
				 | ((esdmode4 & 0xffff) << 16)
				 | ((esdmode5 & 0xffff) << 0)
				);

	/* Normally only the first enabled CS use 0x500, others use 0x400
	 * But when four chip-selects are all enabled, all mode registers
	 * need 0x500 to park.
	 */

	debug_hex("Debug: ddr_sdram_mode_9 = 0x", ddr->ddr_sdram_mode_9);

	for (i = 1; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
		if (!rtt_park &&
		    (ddr->cs[i].config & SDRAM_CS_CONFIG_EN)) {
			esdmode5 |= 0x00000500;	/* RTT_PARK */
			rtt_park = four_cs ? 0 : 1;
		} else {
			esdmode5 = 0x00000400;
		}

		if (ddr->ddr_sdram_cfg_2 & SDRAM_CFG2_AP_EN) {
			if (mclk_ps >= 935) {
				/* for DDR4-1600/1866/2133 */
				esdmode5 |= DDR_MR5_CA_PARITY_LAT_4_CLK;
			} else if (mclk_ps >= 833) {
				/* for DDR4-2400 */
				esdmode5 |= DDR_MR5_CA_PARITY_LAT_5_CLK;
			} else {
				esdmode5 |= DDR_MR5_CA_PARITY_LAT_5_CLK;
				debug("Warning: mclk_ps not supported ");
				dbgprint_uint(mclk_ps);
				debug("\n");
			}
		}

		switch (i) {
		case 1:
			ddr->ddr_sdram_mode_11 = (0
				| ((esdmode4 & 0xFFFF) << 16)
				| ((esdmode5 & 0xFFFF) << 0)
				);
			debug_hex("Debug: ddr_sdram_mode_11 = 0x", ddr->ddr_sdram_mode_11);
			break;
		case 2:
			ddr->ddr_sdram_mode_13 = (0
				| ((esdmode4 & 0xFFFF) << 16)
				| ((esdmode5 & 0xFFFF) << 0)
				);
			debug_hex("Debug: ddr_sdram_mode_13 = 0x", ddr->ddr_sdram_mode_13);
			break;
		case 3:
			ddr->ddr_sdram_mode_15 = (0
				| ((esdmode4 & 0xFFFF) << 16)
				| ((esdmode5 & 0xFFFF) << 0)
				);
			debug_hex("Debug: ddr_sdram_mode_15 = 0x", ddr->ddr_sdram_mode_15);
			break;
		}
	}
}

static void set_ddr_sdram_mode_10_12_14_16(const unsigned long clk,
					   struct ddr_cfg_regs *ddr,
					   const struct memctl_opt *popts,
					   const struct dimm_params *pdimm)
{
	int i;
	unsigned short esdmode6 = 0;	/* Extended SDRAM mode 6 */
	unsigned short esdmode7 = 0;	/* Extended SDRAM mode 7 */
	unsigned int tccdl_min = picos_to_mclk(clk, pdimm->tccdl_ps);

	esdmode6 = ((tccdl_min - 4) & 0x7) << 10;

	if (popts->ddr_cdr2 & DDR_CDR2_VREF_RANGE_2)
		esdmode6 |= 1 << 6;	/* Range 2 */

	ddr->ddr_sdram_mode_10 = (0
				 | ((esdmode6 & 0xffff) << 16)
				 | ((esdmode7 & 0xffff) << 0)
				);
	debug_hex("Debug: ddr_sdram_mode_10) = 0x%", ddr->ddr_sdram_mode_10);

	for (i = 1; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
		switch (i) {
		case 1:
			ddr->ddr_sdram_mode_12 = (0
				| ((esdmode6 & 0xFFFF) << 16)
				| ((esdmode7 & 0xFFFF) << 0)
				);
			debug_hex("Debug: ddr_sdram_mode_12 = 0x", ddr->ddr_sdram_mode_12);
			break;
		case 2:
			ddr->ddr_sdram_mode_14 = (0
				| ((esdmode6 & 0xFFFF) << 16)
				| ((esdmode7 & 0xFFFF) << 0)
				);
			debug_hex("Debug: ddr_sdram_mode_14 = 0x", ddr->ddr_sdram_mode_14);
			break;
		case 3:
			ddr->ddr_sdram_mode_16 = (0
				| ((esdmode6 & 0xFFFF) << 16)
				| ((esdmode7 & 0xFFFF) << 0)
				);
			debug_hex("Debug: ddr_sdram_mode_16 = 0x", ddr->ddr_sdram_mode_16);
			break;
		}
	}
}

#ifndef CONFIG_MEM_INIT_VALUE
#define CONFIG_MEM_INIT_VALUE 0xDEADBEEF
#endif
static void set_ddr_data_init(struct ddr_cfg_regs *ddr)
{
	ddr->ddr_data_init = CONFIG_MEM_INIT_VALUE;
}

static void set_ddr_sdram_clk_cntl(struct ddr_cfg_regs *ddr,
					 const struct memctl_opt *popts)
{
	unsigned int clk_adjust;	/* Clock adjust */
	unsigned int ss_en = 0;		/* Source synchronous enable */

	clk_adjust = popts->clk_adjust;
	ddr->ddr_sdram_clk_cntl = (0
				   | ((ss_en & 0x1) << 31)
				   | ((clk_adjust & 0x1F) << 22)
				   );
	debug_hex("Debug: clk_cntl = 0x", ddr->ddr_sdram_clk_cntl);
}

static void set_ddr_init_addr(struct ddr_cfg_regs *ddr)
{
	const unsigned int init_addr = 0;	/* Initialization address */

	ddr->ddr_init_addr = init_addr;
}

static void set_ddr_init_ext_addr(struct ddr_cfg_regs *ddr)
{
	const unsigned int uia = 0;	/* Use initialization address */
	const unsigned int init_ext_addr = 0;	/* Initialization address */

	ddr->ddr_init_ext_addr = (0
				  | ((uia & 0x1) << 31)
				  | (init_ext_addr & 0xF)
				  );
}

static void set_timing_cfg_9(struct ddr_cfg_regs *ddr)
{
	ddr->timing_cfg_9 = 0;
	debug_hex("Debug: timing_cfg_9 = 0x", ddr->timing_cfg_9);
}

static void set_ddr_dq_mapping(struct ddr_cfg_regs *ddr,
			       const struct dimm_params *pdimm)
{
	unsigned int acc_ecc_en;

	/* SDRAM_TYPE cannot be 0 if set */
	if (!ddr->ddr_sdram_cfg) {
		debug("Error: sdram_cfg not set\n");
		return;
	}

	acc_ecc_en = (ddr->ddr_sdram_cfg >> 2) & 0x1;

	ddr->dq_map_0 = ((pdimm->dq_mapping[0] & 0x3F) << 26) |
			((pdimm->dq_mapping[1] & 0x3F) << 20) |
			((pdimm->dq_mapping[2] & 0x3F) << 14) |
			((pdimm->dq_mapping[3] & 0x3F) << 8) |
			((pdimm->dq_mapping[4] & 0x3F) << 2);

	ddr->dq_map_1 = ((pdimm->dq_mapping[5] & 0x3F) << 26) |
			((pdimm->dq_mapping[6] & 0x3F) << 20) |
			((pdimm->dq_mapping[7] & 0x3F) << 14) |
			((pdimm->dq_mapping[10] & 0x3F) << 8) |
			((pdimm->dq_mapping[11] & 0x3F) << 2);

	ddr->dq_map_2 = ((pdimm->dq_mapping[12] & 0x3F) << 26) |
			((pdimm->dq_mapping[13] & 0x3F) << 20) |
			((pdimm->dq_mapping[14] & 0x3F) << 14) |
			((pdimm->dq_mapping[15] & 0x3F) << 8) |
			((pdimm->dq_mapping[16] & 0x3F) << 2);

	/* dq_map for ECC[4:7] is set to 0 if accumulated ECC is enabled */
	ddr->dq_map_3 = ((pdimm->dq_mapping[17] & 0x3F) << 26) |
			((pdimm->dq_mapping[8] & 0x3F) << 20) |
			(acc_ecc_en ? 0 :
			 (pdimm->dq_mapping[9] & 0x3F) << 14) |
			pdimm->dq_mapping_ors;

	debug_hex("Debug: dq_map_0 = 0x", ddr->dq_map_0);
	debug_hex("Debug: dq_map_1 = 0x", ddr->dq_map_1);
	debug_hex("Debug: dq_map_2 = 0x", ddr->dq_map_2);
	debug_hex("Debug: dq_map_3 = 0x", ddr->dq_map_3);
}
static void set_ddr_sdram_cfg_3(struct ddr_cfg_regs *ddr,
			       const struct memctl_opt *popts)
{
	int rd_pre;

	rd_pre = popts->quad_rank_present ? 1 : 0;

	ddr->ddr_sdram_cfg_3 = (rd_pre & 0x1) << 16;

	debug_hex("Debug: ddr_sdram_cfg_3 = 0x", ddr->ddr_sdram_cfg_3);
}

static void set_ddr_zq_cntl(struct ddr_cfg_regs *ddr)
{
	const unsigned int zqinit = 10;	/* 1024 clocks */
	const unsigned int zqoper = 9;	/* 512 clocks */
	const unsigned int zqcs = 7;	/* 128 clocks */
	const unsigned int zqcs_init = 5;	/* 1024 refresh seqences */
	const unsigned int zq_en = 1;	/* enabled */

	ddr->ddr_zq_cntl = (0
			    | ((zq_en & 0x1) << 31)
			    | ((zqinit & 0xF) << 24)
			    | ((zqoper & 0xF) << 16)
			    | ((zqcs & 0xF) << 8)
			    | ((zqcs_init & 0xF) << 0)
			    );
	debug_hex("Debug: zq_cntl = 0x", ddr->ddr_zq_cntl);
}

static void set_ddr_wrlvl_cntl(struct ddr_cfg_regs *ddr,
				const struct memctl_opt *popts)
{
	const unsigned int wrlvl_en = 1;	/* enabled */
	const unsigned int wrlvl_mrd = 0x6;	/* > 40nCK */
	const unsigned int wrlvl_odten = 0x7;	/* 128 */
	const unsigned int wrlvl_dqsen = 0x5;	/* > 25nCK */
	const unsigned int wrlvl_wlr = 0x6;	/* > tWLO + 6 */
	unsigned int wrlvl_smpl = 0xf;	/* > tWLO + 6 */
	unsigned int wrlvl_start = 0x8;

	if (popts->wrlvl_override) {
		wrlvl_smpl = popts->wrlvl_sample;
		wrlvl_start = popts->wrlvl_start;
	}

	ddr->ddr_wrlvl_cntl = (0
			       | ((wrlvl_en & 0x1) << 31)
			       | ((wrlvl_mrd & 0x7) << 24)
			       | ((wrlvl_odten & 0x7) << 20)
			       | ((wrlvl_dqsen & 0x7) << 16)
			       | ((wrlvl_smpl & 0xf) << 12)
			       | ((wrlvl_wlr & 0x7) << 8)
			       | ((wrlvl_start & 0x1F) << 0)
			       );
	ddr->ddr_wrlvl_cntl_2 = popts->wrlvl_ctl_2;
	ddr->ddr_wrlvl_cntl_3 = popts->wrlvl_ctl_3;
	debug_hex("Debug: wrlvl_cntl = 0x", ddr->ddr_wrlvl_cntl);
	debug_hex("Debug: wrlvl_cntl_2 = 0x", ddr->ddr_wrlvl_cntl_2);
	debug_hex("Debug: wrlvl_cntl_3 = 0x", ddr->ddr_wrlvl_cntl_3);

}

static void set_ddr_sr_cntr(struct ddr_cfg_regs *ddr, unsigned int sr_it)
{
	/* Self Refresh Idle Threshold */
	ddr->ddr_sr_cntr = (sr_it & 0xF) << 16;
	debug_hex("Debug: ddr_sr_cntr = 0x", ddr->ddr_sr_cntr);
}

static void set_ddr_eor(struct ddr_cfg_regs *ddr, const struct memctl_opt *popts)
{
	if (popts->addr_hash) {
		ddr->ddr_eor = 0x40000000;	/* address hash enable */
		debug("Address hashing enabled.\n");
	}
}

static void set_ddr_cdr1(struct ddr_cfg_regs *ddr, const struct memctl_opt *popts)
{
	ddr->ddr_cdr1 = popts->ddr_cdr1;
	debug_hex("Debug: ddr_cdr1 = 0x", ddr->ddr_cdr1);
}

static void set_ddr_cdr2(struct ddr_cfg_regs *ddr, const struct memctl_opt *popts)
{
	ddr->ddr_cdr2 = popts->ddr_cdr2;
	debug_hex("Debug: ddr_cdr2 = 0x", ddr->ddr_cdr2);
}

int check_ddrc_regs(const struct ddr_cfg_regs *ddr)
{
	/*
	 * Check that DDR_SDRAM_CFG[RD_EN] and DDR_SDRAM_CFG[2T_EN] are
	 * not set at the same time.
	 */
	if (ddr->ddr_sdram_cfg & 0x10000000
	    && ddr->ddr_sdram_cfg & 0x00008000) {
		debug("Error: DDR_SDRAM_CFG[RD_EN] and DDR_SDRAM_CFG[2T_EN] both set.\n");
		return -EINVAL;
	}

	return 0;
}

int compute_ddrc_regs(const unsigned long clk,
				const struct memctl_opt *popts,
				const struct ddr_conf *conf,
				struct ddr_cfg_regs *ddr,
				const struct dimm_params *pdimm)
{
	int i;
	unsigned int cas_latency;
	unsigned int additive_latency;
	unsigned int sr_it;
	int cs_en = 0;
	const unsigned int mclk_ps = get_memory_clk_period_ps(clk);
	unsigned long long ea, sa;

	bzero(ddr, sizeof(struct ddr_cfg_regs));

	/* calculate cas latency, override first */
	cas_latency = (popts->cas_latency_override)
		? popts->cas_latency_override_value
		: (pdimm->taa_ps + mclk_ps - 1) / mclk_ps;

	/* Check if DIMM supports the cas latency */
	i = 24;
	while(!(pdimm->caslat_x & (1 << cas_latency)) && (i-- > 0))
		cas_latency++;

	if (i <= 0) {
		debug("Error: Failed to find a proper cas latency\n");
		return -EINVAL;
	}
	/* Verify cas latency does not exceed 18ns for DDR4 */
	if (cas_latency * mclk_ps > 18000) {
		debug("Error: cas latency is too large ");
		dbgprint_uint(cas_latency);
		debug("\n");
		return -EINVAL;
	}

	additive_latency = (popts->additive_latency_override)
		? popts->additive_latency_override_value
		: 0;

	sr_it = (popts->auto_self_refresh_en)
		? popts->sr_it
		: 0;

	/* Chip Select Memory Bounds (CSn_BNDS) */
	for (i = 0;
		i < CONFIG_CHIP_SELECTS_PER_CTRL && conf->cs_size[i];
		i++) {
		debug_hex("cs_in_use = 0x", conf->cs_in_use);
		if (conf->cs_in_use & (1 << i)) {
			cs_en = 1;
			sa = conf->cs_base_addr[i];
			ea = sa + conf->cs_size[i] - 1;
			sa >>= 24;
			ea >>= 24;
			ddr->cs[i].bnds = (0
				| ((sa & 0xffff) << 16) /* starting address */
				| ((ea & 0xffff) << 0)	/* ending address */
				);
		} else {
			/* setting bnds to 0xffffffff for inactive CS */
			ddr->cs[i].bnds = 0xffffffff;
		}

		debug_int("Debug: cs", i); 
		debug_hex("          bnds = 0x", ddr->cs[i].bnds);
		set_csn_config(i, cs_en, ddr, popts, pdimm);
		set_csn_config_2(i, ddr);
	}

	set_ddr_eor(ddr, popts);
	set_timing_cfg_0(clk, ddr, popts, conf, pdimm);
	set_timing_cfg_1(clk, ddr, popts, pdimm, cas_latency);
	set_timing_cfg_2(clk, ddr, popts, additive_latency);
	set_timing_cfg_3(clk, ddr, popts, pdimm, cas_latency, additive_latency);
	set_timing_cfg_4(ddr, popts);
	set_timing_cfg_5(ddr, cas_latency);
	set_timing_cfg_6(ddr);

	set_ddr_cdr1(ddr, popts);
	set_ddr_cdr2(ddr, popts);
	set_ddr_sdram_cfg(ddr, popts, pdimm);

	if (popts->cswl_override)
		ddr->debug[18] = popts->cswl_override;

	set_ddr_sdram_cfg_2(clk, ddr, popts);
	set_ddr_sdram_cfg_3(ddr, popts);

	set_timing_cfg_7(clk, ddr, pdimm);
	set_timing_cfg_8(clk, ddr, popts, pdimm, cas_latency);
	set_timing_cfg_9(ddr);

	set_ddr_init_addr(ddr);
	set_ddr_init_ext_addr(ddr);
	set_ddr_data_init(ddr);
	set_ddr_sdram_clk_cntl(ddr, popts);
	set_ddr_dq_mapping(ddr, pdimm);
	set_ddr_sdram_interval(clk, ddr, popts, pdimm);
	set_ddr_zq_cntl(ddr);
	set_ddr_wrlvl_cntl(ddr, popts);
	set_ddr_sr_cntr(ddr, sr_it);
	set_ddr_sdram_rcw(ddr, popts, pdimm);

	set_ddr_sdram_mode_1_3_5_7(clk, ddr, popts, pdimm, cas_latency, additive_latency);
	set_ddr_sdram_mode_2_4_6_8(clk, ddr, popts, pdimm);
	set_ddr_sdram_mode_9_11_13_15(clk, ddr, conf);
	set_ddr_sdram_mode_10_12_14_16(clk, ddr, popts, pdimm);

#ifdef CONFIG_SYS_FSL_DDR_EMU
	/* disble DDR training for emulator */
	ddr->debug[2] = 0x00000400;
	ddr->debug[4] = 0xff800800;
	ddr->debug[5] = 0x08000800;
	ddr->debug[6] = 0x08000800;
	ddr->debug[7] = 0x08000800;
	ddr->debug[8] = 0x08000800;
#endif

	return check_ddrc_regs(ddr);
}
