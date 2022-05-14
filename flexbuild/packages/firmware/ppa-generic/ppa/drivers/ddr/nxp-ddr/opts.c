//---------------------------------------------------------------------------
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

#include "io.h"
#include "plat.h"
#include "lib.h"
#include "ddr.h"
#include "debug.h"

struct dynamic_odt {
	unsigned int odt_rd_cfg;
	unsigned int odt_wr_cfg;
	unsigned int odt_rtt_norm;
	unsigned int odt_rtt_wr;
};

#if CONFIG_CHIP_SELECTS_PER_CTRL > 4
#error Unsupported CONFIG_CHIP_SELECTS_PER_CTRL
#endif

/* Quad rank is not verified yet due availability. */
static const struct dynamic_odt single_Q[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS_AND_OTHER_DIMM,
		DDR4_RTT_34_OHM,	/* unverified */
		DDR4_RTT_120_OHM
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR4_RTT_OFF,
		DDR4_RTT_120_OHM
	},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS_AND_OTHER_DIMM,
		DDR4_RTT_34_OHM,
		DDR4_RTT_120_OHM
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,	/* tied high */
		DDR4_RTT_OFF,
		DDR4_RTT_120_OHM
	}
};

static const struct dynamic_odt single_D[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_ALL,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR4_RTT_OFF,
		DDR4_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

static const struct dynamic_odt single_S[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_ALL,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
};

static const struct dynamic_odt dual_DD[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR4_RTT_34_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR4_RTT_34_OHM,
		DDR4_RTT_OFF
	}
};

static const struct dynamic_odt dual_SS[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_ALL,
		DDR4_RTT_34_OHM,
		DDR4_RTT_120_OHM
	},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_ALL,
		DDR4_RTT_34_OHM,
		DDR4_RTT_120_OHM
	},
	{0, 0, 0, 0}
};

static const struct dynamic_odt dual_D0[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR4_RTT_OFF,
		DDR4_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

static const struct dynamic_odt dual_0D[4] = {
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR4_RTT_OFF,
		DDR4_RTT_OFF
	}
};

static const struct dynamic_odt dual_S0[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}

};

static const struct dynamic_odt dual_0S[4] = {
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{0, 0, 0, 0}

};

static const struct dynamic_odt odt_unknown[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	}
};

/*
 * Automatically seleect bank interleaving mode based on DIMMs
 * in this order: cs0_cs1_cs2_cs3, cs0_cs1, null.
 * This function only deal with one or two slots per controller.
 */
static inline unsigned int auto_bank_intlv(const int cs_in_use,
				const struct dimm_params *pdimm)
{
	switch (cs_in_use) {
	case 0xf:
		return FSL_DDR_CS0_CS1_CS2_CS3;
	case 0x3:
		return FSL_DDR_CS0_CS1;
	default:
		break;
	}

	return 0;
}

static int pop_ctrl_opts(const unsigned clk,
			 struct memctl_opt *popts,
			 struct ddr_conf *conf,
			 struct dimm_params *pdimm,
			 const int ctrl_num,
			 const int dimm_slot_per_ctrl)

{
	unsigned int i;
	const struct dynamic_odt *pdodt = odt_unknown;

	/* Chip select options. */
	switch (dimm_slot_per_ctrl) {
	case 1:
		switch (pdimm->n_ranks) {
		case 1:
			pdodt = single_S;
			break;
		case 2:
			pdodt = single_D;
			break;
		case 4:
			pdodt = single_Q;
			break;
		case 0:
			return -ENODEV;
		}
		break;
	case 2:
		switch (pdimm->n_ranks) {
		case 4:
			pdodt = single_Q;
			break;
		case 2:
			switch (conf->cs_in_use) {
			case 0xf:
				pdodt = dual_DD;
				break;
			case 0x3:
				pdodt = dual_D0;
				break;
			case 0xc:
				pdodt = dual_0D;
				break;
			default:
				debug("Error: invalid cs_in_use\n");
				return -EINVAL;
			}
			break;
		case 1:
			switch (conf->cs_in_use) {
			case 0x5:
				pdodt = dual_SS;
				break;
			case 0x1:
				pdodt = dual_S0;
				break;
			case 0x4:
				pdodt = dual_0S;
				break;
			default:
				debug("Error: invalid cs_in_use\n");
				return -EINVAL;
			}
			break;
		case 0:
			debug("Error: n_ranks = 0?\n");
			return -EINVAL;
		}
		break;
	default:
		debug("Unsupported number of DIMMs\n");
		return -EINVAL;
	}

	/* Pick chip-select local options. */
	for (i = 0; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
		popts->cs_local_opts[i].odt_rd_cfg = pdodt[i].odt_rd_cfg;
		popts->cs_local_opts[i].odt_wr_cfg = pdodt[i].odt_wr_cfg;
		popts->cs_local_opts[i].odt_rtt_norm = pdodt[i].odt_rtt_norm;
		popts->cs_local_opts[i].odt_rtt_wr = pdodt[i].odt_rtt_wr;
		popts->cs_local_opts[i].auto_precharge = 0;
	}

	popts->registered_dimm_en = pdimm->registered_dimm;
	popts->mirrored_dimm = pdimm->mirrored_dimm;
	popts->ecc_mode = 1;		/* enable ECC if DIMM supports it */
	popts->ecc_init_using_memctl = 1; /* 1 = use memctl */
	popts->self_refresh_in_sleep = 1;
	popts->dynamic_power = 0;

	/*
	 * check sdram width, allow platform override
	 * 0 = 64-bit, 1 = 32-bit, 2 = 16-bit
	 */
	if (pdimm->primary_sdram_width == 64) {
		popts->data_bus_width = DDR_DATA_BUS_WIDTH_64;
		popts->data_bus_used = DDR_DATA_BUS_WIDTH_64;
	} else if (pdimm->primary_sdram_width == 32) {
		popts->data_bus_width = DDR_DATA_BUS_WIDTH_32;
		popts->data_bus_used = DDR_DATA_BUS_WIDTH_32;
	} else if (pdimm->primary_sdram_width == 16) {
		popts->data_bus_width = DDR_DATA_BUS_WIDTH_16;
		popts->data_bus_used = DDR_DATA_BUS_WIDTH_16;
	} else {
		debug("Error: primary sdram width invalid!\n");
		return -EINVAL;
	}

	popts->x4_en = (pdimm->device_width == 4) ? 1 : 0;

	/* for RDIMM and DDR4 UDIMM/discrete memory, address parity enable */
	if (popts->registered_dimm_en)
		popts->ap_en = 1; /* 0 = disable,  1 = enable */
	else
		popts->ap_en = 0; /* disabled for DDR4 UDIMM/discrete default */

	/*
	 * BSTTOPRE precharge interval
	 *
	 * Set this to 0 for global auto precharge
	 * The value of 0x100 has been used for DDR1, DDR2, DDR3.
	 * It is not wrong. Any value should be OK. The performance depends on
	 * applications. There is no one good value for all. One way to set
	 * is to use 1/4 of refint value.
	 */
	popts->bstopre = picos_to_mclk(clk, pdimm->refresh_rate_ps) >> 2;

	popts->tfaw_window_four_activates_ps = pdimm->tfaw_ps;

	/* default interleaving mode, solo ctrl will be disabled later */
	/* controller interleaving requires cs0 */
	if (!(conf->cs_in_use & 0x1))
		popts->memctl_interleaving = 0;
	else
		popts->memctl_interleaving = 1;
#ifdef CONFIG_SYS_FSL_DDR_INTLV_256B
	popts->memctl_interleaving_mode = FSL_DDR_256B_INTERLEAVING;
#else
	popts->memctl_interleaving_mode = FSL_DDR_CACHE_LINE_INTERLEAVING;
#endif /* CONFIG_SYS_FSL_DDR_INTLV_256B */

	popts->ba_intlv_ctl = auto_bank_intlv(conf->cs_in_use, pdimm);

	if (pdimm->n_ranks == 4)
		popts->quad_rank_present = 1;

	if (popts->registered_dimm_en) {
		popts->rcw_override = 1;
		popts->rcw_1 = 0x000a5a00;
		if (clk <= 800000000)
			popts->rcw_2 = 0x00000000;
		else if (clk <= 1066666666)
			popts->rcw_2 = 0x00100000;
		else if (clk <= 1333333333)
			popts->rcw_2 = 0x00200000;
		else
			popts->rcw_2 = 0x00300000;
	}

	/* Choose burst length. */
#if defined(CONFIG_E500MC)
	popts->otf_burst_chop_en = 0;	/* on-the-fly burst chop disable */
	popts->burst_length = DDR_BL8;	/* Fixed 8-beat burst len */
#else
	if ((popts->data_bus_used == DDR_DATA_BUS_WIDTH_32) ||
	    (popts->data_bus_used == DDR_DATA_BUS_WIDTH_16)) {
		/* 32-bit or 16-bit bus */
		popts->otf_burst_chop_en = 0;
		popts->burst_length = DDR_BL8;
	} else {
		popts->otf_burst_chop_en = 1;	/* on-the-fly burst chop */
		popts->burst_length = DDR_OTF;	/* on-the-fly BC4 and BL8 */
	}
#endif

	return 0;
}

static void check_interleaving_options(struct ddr_info *priv)
{
	int i, j, k, check_n_ranks, intlv_invalid = 0;
	unsigned int check_intlv, check_n_row_addr, check_n_col_addr;
	unsigned int check_ba_intlv;
	unsigned long long check_rank_density;
	struct dimm_params *dimm;
	int first_ctrl = priv->first_ctrl;
	int last_ctrl = first_ctrl + priv->num_ctrls - 1;

	/*
	 * Check if all controllers are configured for memory
	 * controller interleaving. Identical dimms are recommended. At least
	 * the size, row and col address should be checked.
	 */
	j = 0;
	check_n_ranks = priv->dimms[first_ctrl].n_ranks;
	check_rank_density = priv->dimms[first_ctrl].rank_density;
	check_n_row_addr =  priv->dimms[first_ctrl].n_row_addr;
	check_n_col_addr = priv->dimms[first_ctrl].n_col_addr;
	check_intlv = priv->opts[first_ctrl].memctl_interleaving_mode;
	check_ba_intlv = priv->opts[first_ctrl].ba_intlv_ctl;
	for (i = first_ctrl; i <= last_ctrl; i++) {
		dimm = &priv->dimms[i];
		if (!priv->opts[i].memctl_interleaving) {
			continue;
		} else if (!priv->conf[i].dimm_in_use[0]) {
			intlv_invalid = 1;
		} else if ((check_rank_density != dimm->rank_density) ||
			   (check_n_ranks != dimm->n_ranks) ||
			   (check_n_row_addr != dimm->n_row_addr) ||
			   (check_n_col_addr != dimm->n_col_addr) ||
			   (check_ba_intlv != priv->opts[i].ba_intlv_ctl) ||
			   (check_intlv != priv->opts[i].memctl_interleaving_mode)) {
			intlv_invalid = 1;
			break;
		} else {
			j++;
		}

	}
	if (intlv_invalid) {
		for (i = first_ctrl; i <= last_ctrl; i++)
			priv->opts[i].memctl_interleaving = 0;
		debug("Warning: Invalid DIMM configuration. "
		     "Memory controller interleaving disabled.\n");
	} else {
		switch (check_intlv) {
		case FSL_DDR_256B_INTERLEAVING:
		case FSL_DDR_CACHE_LINE_INTERLEAVING:
		case FSL_DDR_PAGE_INTERLEAVING:
		case FSL_DDR_BANK_INTERLEAVING:
		case FSL_DDR_SUPERBANK_INTERLEAVING:
			k = 2;
			break;
		default:
			debug("Warning: Unknown interleaving mode. Disabling...\n");
			k = 0;
			break;
		}
		debug_int("DEBUG: Controllers in interleaving: ", j);
		if (j && (j != k)) {
			for (i = first_ctrl; i <= last_ctrl; i++)
				priv->opts[i].memctl_interleaving = 0;
			if ((last_ctrl - first_ctrl) > 1) {
				debug("Warning: incompatible interleaving mode. Disabled.\n");
            }
		}
	}
	debug("DEBUG: checking interleaving options completed\n");
}

void populate_memctl_options(struct ddr_info *priv)
{
	int i;
	int ret;

	for (i = priv->first_ctrl; i < priv->first_ctrl + priv->num_ctrls; i++) {
		if (!priv->conf[i].in_use)
			continue;
		ret = pop_ctrl_opts(priv->clk,
				    &priv->opts[i],
				    &priv->conf[i],
				    &priv->dimms[i],
				    i, priv->dimm_slots_per_ctrl);
		if (ret) {
			priv->conf[i].in_use = 0;
			debug_int("Debug: Failed DIMM on controller ", i);
			continue;
		}
	}
	ddr_board_options(priv);
	for (i = priv->first_ctrl; i < priv->first_ctrl + priv->num_ctrls; i++)
		dump_memctl_options(&priv->opts[i]);

	check_interleaving_options(priv);
}
