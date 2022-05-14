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
//-----------------------------------------------------------------------------

#include "lib.h"
#include "io.h"
#include "dimm.h"
#include "opts.h"
#include "regs.h"

struct options_string {
	const char *option_name;
	unsigned long offset;
	unsigned int size;
	const char printhex;
};

#if defined(DEBUG_DIMM) || defined(DEBUG_OPTS) || defined(DEBUG_REGS)
static void print_option_table(const struct options_string *table,
			 int table_size,
			 const void *base)
{
	unsigned int i;
	unsigned int *ptr;
	unsigned long long *ptr_l;

	for (i = 0; i < table_size; i++) {
		switch (table[i].size) {
		case 4:
			ptr = (unsigned int *) (base + table[i].offset);
			if (table[i].printhex) {
				puts(table[i].option_name);
				puts(" = 0x");
				print_hex(*ptr);
				puts("\n");
			} else {
				puts(table[i].option_name);
				puts(" = ");
				print_uint(*ptr);
				puts("\n");
			}
			break;
		case 8:
			ptr_l = (unsigned long long *) (base + table[i].offset);
			puts(table[i].option_name);
			puts(" = ");
			print_uint(*ptr_l);
			break;
		default:
			puts("Unrecognized size!\n");
			break;
		}
	}
}
#endif

#define DIMM_PARM(x) {#x, offsetof(struct dimm_params, x), \
	sizeof((struct dimm_params *)0)->x, 0}
#define DIMM_PARM_HEX(x) {#x, offsetof(struct dimm_params, x), \
	sizeof((struct dimm_params *)0)->x, 1}
void dump_dimm_parameters(const struct dimm_params *pdimm)
{
#ifdef DEBUG_DIMM
	static const struct options_string options[] = {
		DIMM_PARM(n_ranks),
		DIMM_PARM(data_width),
		DIMM_PARM(primary_sdram_width),
		DIMM_PARM(ec_sdram_width),
		DIMM_PARM(registered_dimm),
		DIMM_PARM(mirrored_dimm),
		DIMM_PARM(device_width),

		DIMM_PARM(n_row_addr),
		DIMM_PARM(n_col_addr),
		DIMM_PARM(edc_config),
		DIMM_PARM(bank_addr_bits),
		DIMM_PARM(bank_group_bits),

		DIMM_PARM(tckmin_x_ps),
		DIMM_PARM(tckmax_ps),

		DIMM_PARM(caslat_x),
		DIMM_PARM_HEX(caslat_x),
		DIMM_PARM(taa_ps),

		DIMM_PARM(trcd_ps),
		DIMM_PARM(trp_ps),
		DIMM_PARM(tras_ps),
		DIMM_PARM(tfaw_ps),
		DIMM_PARM(trfc1_ps),
		DIMM_PARM(trfc2_ps),
		DIMM_PARM(trfc4_ps),
		DIMM_PARM(trrds_ps),
		DIMM_PARM(trrdl_ps),
		DIMM_PARM(tccdl_ps),
		DIMM_PARM(trc_ps),
		DIMM_PARM(refresh_rate_ps),

		DIMM_PARM_HEX(dq_mapping[0]),
		DIMM_PARM_HEX(dq_mapping[1]),
		DIMM_PARM_HEX(dq_mapping[2]),
		DIMM_PARM_HEX(dq_mapping[3]),
		DIMM_PARM_HEX(dq_mapping[4]),
		DIMM_PARM_HEX(dq_mapping[5]),
		DIMM_PARM_HEX(dq_mapping[6]),
		DIMM_PARM_HEX(dq_mapping[7]),
		DIMM_PARM_HEX(dq_mapping[8]),
		DIMM_PARM_HEX(dq_mapping[9]),
		DIMM_PARM_HEX(dq_mapping[10]),
		DIMM_PARM_HEX(dq_mapping[11]),
		DIMM_PARM_HEX(dq_mapping[12]),
		DIMM_PARM_HEX(dq_mapping[13]),
		DIMM_PARM_HEX(dq_mapping[14]),
		DIMM_PARM_HEX(dq_mapping[15]),
		DIMM_PARM_HEX(dq_mapping[16]),
		DIMM_PARM_HEX(dq_mapping[17]),
		DIMM_PARM(dq_mapping_ors),
	};
	static const unsigned int n_opts = ARRAY_SIZE(options);

	if (pdimm->n_ranks == 0) {
		puts("DIMM not present\n");
		return;
	}
	puts("DIMM organization parameters:\n");
	puts("module part name = "); puts(pdimm->mpart); puts("\n");
	puts("rank_density = "); print_uint(pdimm->rank_density);
	puts(" bytes ("); print_uint(pdimm->rank_density / 0x100000);
	puts(" megabytes)\n");
	puts("capacity = "); print_uint(pdimm->capacity); puts(" bytes (");
	print_uint(pdimm->capacity / 0x100000); puts(" megabytes)\n");
	puts("burst_lengths_bitmask = 0x"); print_hex(pdimm->burst_lengths_bitmask);
	puts("\n");
	print_option_table(options, n_opts, pdimm);
#endif /* DEBUG_DIMM */
}

#define CTRL_OPTIONS(x) {#x, offsetof(struct memctl_opt, x), \
        sizeof((struct memctl_opt *)0)->x, 0}
#define CTRL_OPTIONS_CS(x, y) {"cs" #x "_" #y, \
        offsetof(struct memctl_opt, cs_local_opts[x].y), \
        sizeof((struct memctl_opt *)0)->cs_local_opts[x].y, 0}
#define CTRL_OPTIONS_HEX(x) {#x, offsetof(struct memctl_opt, x), \
        sizeof((struct memctl_opt *)0)->x, 1}

void dump_memctl_options(const struct memctl_opt *popts)
{
#ifdef DEBUG_OPTS
	static const struct options_string options[] = {
		CTRL_OPTIONS_CS(0, odt_rd_cfg),
		CTRL_OPTIONS_CS(0, odt_wr_cfg),
#if (CONFIG_CHIP_SELECTS_PER_CTRL > 1)
		CTRL_OPTIONS_CS(1, odt_rd_cfg),
		CTRL_OPTIONS_CS(1, odt_wr_cfg),
#endif
#if (CONFIG_CHIP_SELECTS_PER_CTRL > 2)
		CTRL_OPTIONS_CS(2, odt_rd_cfg),
		CTRL_OPTIONS_CS(2, odt_wr_cfg),
#endif
#if (CONFIG_CHIP_SELECTS_PER_CTRL > 3)
		CTRL_OPTIONS_CS(3, odt_rd_cfg),
		CTRL_OPTIONS_CS(3, odt_wr_cfg),
#endif
		CTRL_OPTIONS_CS(0, odt_rtt_norm),
		CTRL_OPTIONS_CS(0, odt_rtt_wr),
#if (CONFIG_CHIP_SELECTS_PER_CTRL > 1)
		CTRL_OPTIONS_CS(1, odt_rtt_norm),
		CTRL_OPTIONS_CS(1, odt_rtt_wr),
#endif
#if (CONFIG_CHIP_SELECTS_PER_CTRL > 2)
		CTRL_OPTIONS_CS(2, odt_rtt_norm),
		CTRL_OPTIONS_CS(2, odt_rtt_wr),
#endif
#if (CONFIG_CHIP_SELECTS_PER_CTRL > 3)
		CTRL_OPTIONS_CS(3, odt_rtt_norm),
		CTRL_OPTIONS_CS(3, odt_rtt_wr),
#endif
		CTRL_OPTIONS(memctl_interleaving),
		CTRL_OPTIONS(memctl_interleaving_mode),
		CTRL_OPTIONS_HEX(ba_intlv_ctl),
		CTRL_OPTIONS(ecc_mode),
		CTRL_OPTIONS(ecc_init_using_memctl),
		CTRL_OPTIONS(self_refresh_in_sleep),
		CTRL_OPTIONS(dynamic_power),
		CTRL_OPTIONS(data_bus_width),
		CTRL_OPTIONS(data_bus_used),
		CTRL_OPTIONS(burst_length),
		CTRL_OPTIONS(cas_latency_override),
		CTRL_OPTIONS(cas_latency_override_value),
		CTRL_OPTIONS(additive_latency_override),
		CTRL_OPTIONS(additive_latency_override_value),
		CTRL_OPTIONS(clk_adjust),
		CTRL_OPTIONS(cpo_override),
		CTRL_OPTIONS(write_data_delay),
		CTRL_OPTIONS(half_strength_driver_enable),
		/*
		 * These can probably be changed to 2T_EN and 3T_EN
		 * (using a leading numerical character) without problem
		 */
		CTRL_OPTIONS(twot_en),
		CTRL_OPTIONS(threet_en),
		CTRL_OPTIONS(registered_dimm_en),
		CTRL_OPTIONS(mirrored_dimm),
		CTRL_OPTIONS(ap_en),
		CTRL_OPTIONS(x4_en),
		CTRL_OPTIONS(bstopre),
		CTRL_OPTIONS(wrlvl_override),
		CTRL_OPTIONS(wrlvl_sample),
		CTRL_OPTIONS(wrlvl_start),
		CTRL_OPTIONS_HEX(cswl_override),
		CTRL_OPTIONS(rcw_override),
		CTRL_OPTIONS(rcw_1),
		CTRL_OPTIONS(rcw_2),
		CTRL_OPTIONS_HEX(ddr_cdr1),
		CTRL_OPTIONS_HEX(ddr_cdr2),
		CTRL_OPTIONS(tfaw_window_four_activates_ps),
		CTRL_OPTIONS(trwt_override),
		CTRL_OPTIONS(trwt),
		CTRL_OPTIONS(rtt_override),
		CTRL_OPTIONS(rtt_override_value),
		CTRL_OPTIONS(rtt_wr_override_value),
	};
	static const unsigned int n_opts = ARRAY_SIZE(options);

	print_option_table(options, n_opts, popts);
#endif /* DEBUG_OPTS */
}

#define CFG_REGS(x) {#x, offsetof(struct ddr_cfg_regs, x), \
	sizeof((struct ddr_cfg_regs *)0)->x, 1}
#define CFG_REGS_CS(x, y) {"cs" #x "_" #y, \
	offsetof(struct ddr_cfg_regs, cs[x].y), \
	sizeof((struct ddr_cfg_regs *)0)->cs[x].y, 1}

void dump_ddrc_regs(const struct ddr_cfg_regs *ddr)
{
#ifdef DEBUG_REGS
	unsigned int i;
	static const struct options_string options[] = {
		CFG_REGS_CS(0, bnds),
		CFG_REGS_CS(0, config),
		CFG_REGS_CS(0, config_2),
#if (CONFIG_CHIP_SELECTS_PER_CTRL > 1)
		CFG_REGS_CS(1, bnds),
		CFG_REGS_CS(1, config),
		CFG_REGS_CS(1, config_2),
#endif
#if (CONFIG_CHIP_SELECTS_PER_CTRL > 2)
		CFG_REGS_CS(2, bnds),
		CFG_REGS_CS(2, config),
		CFG_REGS_CS(2, config_2),
#endif
#if (CONFIG_CHIP_SELECTS_PER_CTRL > 2)
		CFG_REGS_CS(3, bnds),
		CFG_REGS_CS(3, config),
		CFG_REGS_CS(3, config_2),
#endif
		CFG_REGS(timing_cfg_3),
		CFG_REGS(timing_cfg_0),
		CFG_REGS(timing_cfg_1),
		CFG_REGS(timing_cfg_2),
		CFG_REGS(ddr_sdram_cfg),
		CFG_REGS(ddr_sdram_cfg_2),
		CFG_REGS(ddr_sdram_cfg_3),
		CFG_REGS(ddr_sdram_mode),
		CFG_REGS(ddr_sdram_mode_2),
		CFG_REGS(ddr_sdram_mode_3),
		CFG_REGS(ddr_sdram_mode_4),
		CFG_REGS(ddr_sdram_mode_5),
		CFG_REGS(ddr_sdram_mode_6),
		CFG_REGS(ddr_sdram_mode_7),
		CFG_REGS(ddr_sdram_mode_8),
		CFG_REGS(ddr_sdram_mode_9),
		CFG_REGS(ddr_sdram_mode_10),
		CFG_REGS(ddr_sdram_mode_11),
		CFG_REGS(ddr_sdram_mode_12),
		CFG_REGS(ddr_sdram_mode_13),
		CFG_REGS(ddr_sdram_mode_14),
		CFG_REGS(ddr_sdram_mode_15),
		CFG_REGS(ddr_sdram_mode_16),
		CFG_REGS(ddr_sdram_interval),
		CFG_REGS(ddr_data_init),
		CFG_REGS(ddr_sdram_clk_cntl),
		CFG_REGS(ddr_init_addr),
		CFG_REGS(ddr_init_ext_addr),
		CFG_REGS(timing_cfg_4),
		CFG_REGS(timing_cfg_5),
		CFG_REGS(timing_cfg_6),
		CFG_REGS(timing_cfg_7),
		CFG_REGS(timing_cfg_8),
		CFG_REGS(timing_cfg_9),
		CFG_REGS(ddr_zq_cntl),
		CFG_REGS(ddr_wrlvl_cntl),
		CFG_REGS(ddr_wrlvl_cntl_2),
		CFG_REGS(ddr_wrlvl_cntl_3),
		CFG_REGS(ddr_sr_cntr),
		CFG_REGS(ddr_sdram_rcw_1),
		CFG_REGS(ddr_sdram_rcw_2),
		CFG_REGS(ddr_cdr1),
		CFG_REGS(ddr_cdr2),
		CFG_REGS(dq_map_0),
		CFG_REGS(dq_map_1),
		CFG_REGS(dq_map_2),
		CFG_REGS(dq_map_3),
		CFG_REGS(err_disable),
		CFG_REGS(err_int_en),
		CFG_REGS(ddr_eor),
	};
	static const unsigned int n_opts = ARRAY_SIZE(options);

	print_option_table(options, n_opts, ddr);

	for (i = 0; i < 64; i++) {
		puts("debug_"); print_uint(i + 1);
		puts(" = 0x"); print_hex(ddr->debug[i]);
		puts("\n");
	}

#endif /* DEBUG_REGS */
}
