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

	for (i = priv->first_ctrl; i < priv->first_ctrl + priv->num_ctrls; i++) {
		if (i >= CONFIG_SYS_NUM_DDR_CTLRS) {
			debug("Not supported controller number ");
			dbgprint_uint(i);
			debug("\n");
			return;
		}
		popts = &priv->opts[i];
		conf = &priv->conf[i];
		pdimm = &priv->dimms[i];
		if (popts->registered_dimm_en) {
			pbsp = rdimms[i];
			debug_int("RDIMM ", i);
		} else {
			pbsp = udimms[i];
			debug_int("UDIMM ", i);
		}
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
		if (i == CONFIG_DP_DDR_CTRL) {
			/* force DDR bus width to 32 bits */
			popts->data_bus_used = DDR_DATA_BUS_WIDTH_32;
			popts->otf_burst_chop_en = 0;
			popts->burst_length = DDR_BL8;
			popts->bstopre = 0;	/* enable auto precharge */
			/*
			 * Layout optimization results byte mapping
			 * Byte 0 -> Byte ECC
			 * Byte 1 -> Byte 3
			 * Byte 2 -> Byte 2
			 * Byte 3 -> Byte 1
			 * Byte ECC -> Byte 0
			 */
			dq_mapping_0 = pdimm->dq_mapping[0];
			dq_mapping_2 = pdimm->dq_mapping[2];
			dq_mapping_3 = pdimm->dq_mapping[3];
			pdimm->dq_mapping[0] = pdimm->dq_mapping[8];
			pdimm->dq_mapping[1] = pdimm->dq_mapping[9];
			pdimm->dq_mapping[2] = pdimm->dq_mapping[6];
			pdimm->dq_mapping[3] = pdimm->dq_mapping[7];
			pdimm->dq_mapping[6] = dq_mapping_2;
			pdimm->dq_mapping[7] = dq_mapping_3;
			pdimm->dq_mapping[8] = dq_mapping_0;
			pdimm->dq_mapping[9] = 0;
			pdimm->dq_mapping[10] = 0;
			pdimm->dq_mapping[11] = 0;
			pdimm->dq_mapping[12] = 0;
			pdimm->dq_mapping[13] = 0;
			pdimm->dq_mapping[14] = 0;
			pdimm->dq_mapping[15] = 0;
			pdimm->dq_mapping[16] = 0;
			pdimm->dq_mapping[17] = 0;
		}
		/* To work at higher than 1333MT/s */
		popts->half_strength_driver_enable = 0;
		/*
		 * Write leveling override
		 */
		popts->wrlvl_override = 1;
		popts->wrlvl_sample = 0x0;	/* 32 clocks */

		if (ddr_freq < 2350) {
			if (conf->cs_in_use == 0xf) {
				/* four chip-selects */
				popts->ddr_cdr1 = DDR_CDR1_DHC_EN |
						  DDR_CDR1_ODT(DDR_CDR_ODT_80ohm);
				popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_80ohm);
				popts->twot_en = 1;	/* enable 2T timing */
			} else {
				popts->ddr_cdr1 = DDR_CDR1_DHC_EN |
						  DDR_CDR1_ODT(DDR_CDR_ODT_60ohm);
				popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_60ohm) |
						  DDR_CDR2_VREF_RANGE_2;
			}
		} else {
			popts->ddr_cdr1 = DDR_CDR1_DHC_EN |
					  DDR_CDR1_ODT(DDR_CDR_ODT_100ohm);
			popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_100ohm) |
					  DDR_CDR2_VREF_RANGE_2;
		}
	}
}

bool run_bist(void)
{
	return true;
}

struct fsl_ifc_cspr {
	unsigned int cspr_ext;
	unsigned int cspr;
	unsigned int res;
};

struct fsl_ifc_amask {
	unsigned int amask;
	unsigned int res[0x2];
};

struct fsl_ifc_csor {
	unsigned int csor;
	unsigned int csor_ext;
	unsigned int res;
};

struct fsl_ifc_ftim {
	unsigned int ftim[4];
	unsigned int res[0x8];
};

#define IFC_CSPR_REG_LEN	148
#define IFC_AMASK_REG_LEN	144
#define IFC_CSOR_REG_LEN	144
#define IFC_FTIM_REG_LEN	576

#define IFC_CSPR_USED_LEN	sizeof(struct fsl_ifc_cspr) * CONFIG_SYS_FSL_IFC_BANK_COUNT
#define IFC_AMASK_USED_LEN	sizeof(struct fsl_ifc_amask) * CONFIG_SYS_FSL_IFC_BANK_COUNT
#define IFC_CSOR_USED_LEN	sizeof(struct fsl_ifc_csor) * CONFIG_SYS_FSL_IFC_BANK_COUNT
#define IFC_FTIM_USED_LEN	sizeof(struct fsl_ifc_ftim) * CONFIG_SYS_FSL_IFC_BANK_COUNT

struct fsl_ifc_fcm {
	unsigned int ifc_rev;
	unsigned int res1[0x2];
	struct fsl_ifc_cspr cspr_cs[CONFIG_SYS_FSL_IFC_BANK_COUNT];
	unsigned char res2[IFC_CSPR_REG_LEN - IFC_CSPR_USED_LEN];
	struct fsl_ifc_amask amask_cs[CONFIG_SYS_FSL_IFC_BANK_COUNT];
	unsigned char res3[IFC_AMASK_REG_LEN - IFC_AMASK_USED_LEN];
	struct fsl_ifc_csor csor_cs[CONFIG_SYS_FSL_IFC_BANK_COUNT];
	unsigned char res4[IFC_CSOR_REG_LEN - IFC_CSOR_USED_LEN];
	struct fsl_ifc_ftim ftim_cs[CONFIG_SYS_FSL_IFC_BANK_COUNT];
	unsigned char res5[IFC_FTIM_REG_LEN - IFC_FTIM_USED_LEN];
	unsigned int rb_stat;
	unsigned int rb_map;
	unsigned int wp_map;
	unsigned int ifc_gcr;
	unsigned int res7[0x2];
	unsigned int cm_evter_stat;
	unsigned int res8[0x2];
	unsigned int cm_evter_en;
	unsigned int res9[0x2];
	unsigned int cm_evter_intr_en;
	unsigned int res10[0x2];
	unsigned int cm_erattr0;
	unsigned int cm_erattr1;
	unsigned int res11[0x2];
	unsigned int ifc_ccr;
	unsigned int ifc_csr;
	unsigned int ddr_ccr_low;
};

#define IFC_FCM_BASE_ADDR ((struct fsl_ifc_fcm *)CONFIG_SYS_IFC_ADDR)
#define set_ifc_ftim(i, j, v)	out_le32(&(IFC_FCM_BASE_ADDR)->ftim_cs[i].ftim[j], v)
#define set_ifc_cspr_ext(i, v)	out_le32(&(IFC_FCM_BASE_ADDR)->cspr_cs[i].cspr_ext, v)
#define set_ifc_cspr(i, v)	out_le32(&(IFC_FCM_BASE_ADDR)->cspr_cs[i].cspr, v)
#define set_ifc_amask(i, v)	out_le32(&(IFC_FCM_BASE_ADDR)->amask_cs[i].amask, v)
#define set_ifc_csor(i, v)	out_le32(&(IFC_FCM_BASE_ADDR)->csor_cs[i].csor, v)

void reset(int ctrl_num)
{
	void *p = (void *)0x520000000ULL;

	set_ifc_ftim(IFC_CS0, IFC_FTIM0, CONFIG_SYS_CS0_FTIM0);
	set_ifc_ftim(IFC_CS0, IFC_FTIM1, CONFIG_SYS_CS0_FTIM1);
	set_ifc_ftim(IFC_CS0, IFC_FTIM2, CONFIG_SYS_CS0_FTIM2);
	set_ifc_ftim(IFC_CS0, IFC_FTIM3, CONFIG_SYS_CS0_FTIM3);
	set_ifc_cspr_ext(IFC_CS0, CONFIG_SYS_CSPR0_EXT);
	set_ifc_cspr(IFC_CS0, CONFIG_SYS_CSPR0);
	set_ifc_amask(IFC_CS0, CONFIG_SYS_AMASK0);
	set_ifc_csor(IFC_CS0, CONFIG_SYS_CSOR0);

	set_ifc_ftim(IFC_CS3, IFC_FTIM0, CONFIG_SYS_CS3_FTIM0);
	set_ifc_ftim(IFC_CS3, IFC_FTIM1, CONFIG_SYS_CS3_FTIM1);
	set_ifc_ftim(IFC_CS3, IFC_FTIM2, CONFIG_SYS_CS3_FTIM2);
	set_ifc_ftim(IFC_CS3, IFC_FTIM3, CONFIG_SYS_CS3_FTIM3);
	set_ifc_cspr_ext(IFC_CS3, CONFIG_SYS_CSPR3_EXT);
	set_ifc_cspr(IFC_CS3, CONFIG_SYS_CSPR3);
	set_ifc_amask(IFC_CS3, CONFIG_SYS_AMASK3);
	set_ifc_csor(IFC_CS3, CONFIG_SYS_CSOR3);

#if 0
	out8(p + 0x40, 0x31);	/* reset through rst_ctrl */
#else
	switch (ctrl_num) {
	case 0:
		out8(p + 0x44, 0xc0);
		break;
	case 1:
		out8(p + 0x44, 0x30);
		break;
	case 2:
		out8(p + 0x44, 0x08);
		break;
	default:
		break;
	}
	mdelay(100);
	out8(p + 0x44, 0);	/* deassert reset */
#endif
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
