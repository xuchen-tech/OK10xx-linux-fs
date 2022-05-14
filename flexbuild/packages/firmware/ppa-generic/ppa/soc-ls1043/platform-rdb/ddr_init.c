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
#include "lsch2.h"
#include "ddr.h"
#include "plat.h"
#include "timer.h"

void ddr_board_options(struct ddr_info *priv)
{
	unsigned char dq_mapping_0, dq_mapping_2, dq_mapping_3;
	const struct board_specific_parameters *pbsp, *pbsp_highest;
	const unsigned long ddr_freq = priv->clk / 1000000;
	struct memctl_opt *popts;
	struct ddr_conf *conf;
	struct dimm_params *pdimm;
	int i;

	if (priv->num_ctrls > 1) {
		debug("Not supported controller number ");
		dbgprint_uint(priv->num_ctrls);
		debug("\n");
		return;
	}

	popts = &priv->opts[0];
	conf = &priv->conf[0];
	pdimm = &priv->dimms[0];
	pbsp = udimm;
	debug("UDIMM");
	debug_hex("pbsp = 0x", (unsigned long)pbsp);
	pbsp_highest = NULL;

	/* Get clk_adjust, wrlvl_start, wrlvl_ctl, according to the board ddr
	 * freqency and n_banks specified in board_specific_parameters table.
	 */
	while (pbsp->datarate_mhz_high) {
		debug_int("datarate_mhz_high = ", pbsp->datarate_mhz_high);
		if (pbsp->n_ranks == pdimm->n_ranks &&
		    (pdimm->rank_density >> 30) >= pbsp->rank_gb) {
			if (ddr_freq <= pbsp->datarate_mhz_high) {
				popts->clk_adjust = pbsp->clk_adjust;
				popts->wrlvl_start = pbsp->wrlvl_start;
				popts->wrlvl_ctl_2 = pbsp->wrlvl_ctl_2;
				popts->wrlvl_ctl_3 = pbsp->wrlvl_ctl_3;
				goto found;
			}
			pbsp_highest = pbsp;
		}
		pbsp++;
	}

	if (pbsp_highest) {
		debug("Error: board specific timing not found for data rate ");
		dbgprint_uint(ddr_freq);
		debug(". Trying to use the highest speed parameters\n");
		popts->clk_adjust = pbsp_highest->clk_adjust;
		popts->wrlvl_start = pbsp_highest->wrlvl_start;
		popts->wrlvl_ctl_2 = pbsp->wrlvl_ctl_2;
		popts->wrlvl_ctl_3 = pbsp->wrlvl_ctl_3;
	} else {
		panic("DIMM is not supported by this board\n");
	}
found:
	popts->half_strength_driver_enable = 1;
	/*
	 * Write leveling override
	 */
	popts->wrlvl_override = 1;
	popts->wrlvl_sample = 0xf;

	popts->ddr_cdr1 = DDR_CDR1_DHC_EN | DDR_CDR1_ODT(DDR_CDR_ODT_80ohm);
	popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_80ohm) |
			  DDR_CDR2_VREF_OVRD(70);	/* Vref = 70% */
}

/* DDR model number: MT40A512M8HX-093E */
struct dimm_params ddr_raw_timing = {
	.n_ranks = 1,
	.rank_density = 2147483648u,
	.capacity = 2147483648u,
	.primary_sdram_width = 32,
	.ec_sdram_width = 0,
	.registered_dimm = 0,
	.mirrored_dimm = 0,
	.n_row_addr = 15,
	.n_col_addr = 10,
	.bank_addr_bits = 0,
	.bank_group_bits = 2,
	.edc_config = 0,
	.burst_lengths_bitmask = 0x0c,

	.tckmin_x_ps = 938,
	.tckmax_ps = 1500,
	.caslat_x = 0x000DFA00,
	.taa_ps = 13500,
	.trcd_ps = 13500,
	.trp_ps = 13500,
	.tras_ps = 33000,
	.trc_ps = 46500,
	.twr_ps = 15000,
	.trfc1_ps = 260000,
	.trfc2_ps = 160000,
	.trfc4_ps = 110000,
	.tfaw_ps = 21000,
	.trrds_ps = 3700,
	.trrdl_ps = 5300,
	.tccdl_ps = 5355,
	.refresh_rate_ps = 7800000,
	.dq_mapping[0] = 0x0,
	.dq_mapping[1] = 0x0,
	.dq_mapping[2] = 0x0,
	.dq_mapping[3] = 0x0,
	.dq_mapping[4] = 0x0,
	.dq_mapping[5] = 0x0,
	.dq_mapping[6] = 0x0,
	.dq_mapping[7] = 0x0,
	.dq_mapping[8] = 0x0,
	.dq_mapping[9] = 0x0,
	.dq_mapping[10] = 0x0,
	.dq_mapping[11] = 0x0,
	.dq_mapping[12] = 0x0,
	.dq_mapping[13] = 0x0,
	.dq_mapping[14] = 0x0,
	.dq_mapping[15] = 0x0,
	.dq_mapping[16] = 0x0,
	.dq_mapping[17] = 0x0,
	.dq_mapping_ors = 0,
};

int fsl_ddr_get_dimm_params(struct dimm_params *pdimm,
			    unsigned int controller_number,
			    unsigned int dimm_number)
{
	static const char dimm_model[] = "Fixed DDR on board";

	if (controller_number != 0)
		return -EINVAL;

	if (dimm_number != 0)
		return -ENODEV;

	memcpy(pdimm, &ddr_raw_timing, sizeof(struct dimm_params));
	bzero(pdimm->mpart, sizeof(pdimm->mpart));
	memcpy(pdimm->mpart, dimm_model, sizeof(dimm_model) - 1);

	return 0;
}

bool run_bist(void)
{
	return true;
}

extern long long dram_init(struct sysinfo *sys);


void _init_ddr(void)
{
	struct sysinfo sys;
	long long dram_size;

	bzero(&sys, sizeof(sys));
	get_clocks(&sys);
	dram_size = dram_init(&sys);

	if (dram_size < 0) {
		debug("Error\n");
    }
}
