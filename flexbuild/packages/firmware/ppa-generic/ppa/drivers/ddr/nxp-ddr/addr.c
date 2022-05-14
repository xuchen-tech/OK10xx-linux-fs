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
#include "plat.h"
#include "ddr.h"

static unsigned long long assign_intlv_addr(struct dimm_params *pdimm,
					    struct memctl_opt *opts,
					    struct ddr_conf *conf,
					    const unsigned long long current_mem_base)
{
	int i;
	int ctlr_density_mul = 0;
	const unsigned long long rank_density = pdimm->rank_density >> opts->dbw_cap_shift;
	unsigned long long total_ctlr_mem;

	debug_int("density, ", rank_density);
	switch (opts->ba_intlv_ctl & FSL_DDR_CS0_CS1_CS2_CS3) {
	case FSL_DDR_CS0_CS1_CS2_CS3:
		ctlr_density_mul = 4;
		break;
	case FSL_DDR_CS0_CS1:
	case FSL_DDR_CS0_CS1_AND_CS2_CS3:
		ctlr_density_mul = 2;
		break;
	case FSL_DDR_CS2_CS3:
	default:
		ctlr_density_mul = 1;
		break;
	}
	debug_int("ctlr density mul ", ctlr_density_mul);
	switch (opts->memctl_interleaving_mode) {
	case FSL_DDR_256B_INTERLEAVING:
	case FSL_DDR_CACHE_LINE_INTERLEAVING:
	case FSL_DDR_PAGE_INTERLEAVING:
	case FSL_DDR_BANK_INTERLEAVING:
	case FSL_DDR_SUPERBANK_INTERLEAVING:
		total_ctlr_mem = 2 * ctlr_density_mul * rank_density;
		break;
	default:
		panic("Unknown interleaving mode");
	}
	conf->base_addr = current_mem_base;
	conf->total_mem = total_ctlr_mem;

	/* overwrite cs_in_use bitmask */
	conf->cs_in_use = (1 << ctlr_density_mul) - 1;
	debug_hex("Overwrite cs_in_use as ", conf->cs_in_use);

	/* Fill addr with each cs in use */
	for (i = 0; i < ctlr_density_mul; i++) {
		conf->cs_base_addr[i] = current_mem_base;
		conf->cs_size[i] = total_ctlr_mem;
		debug_int("CS ", i);
		debug_hex("    base_addr 0x", conf->cs_base_addr[i]);
		debug_hex("    size 0x", conf->cs_size[i]);
	}

	return total_ctlr_mem;
}

static unsigned long long assign_non_intlv_addr(struct dimm_params *pdimm,
				     struct memctl_opt *opts,
				     struct ddr_conf *conf,
				     unsigned long long current_mem_base)
{
	int i;
	const unsigned long long rank_density = pdimm->rank_density >> opts->dbw_cap_shift;
	unsigned long long total_ctlr_mem = 0;

	debug_int("density, ", rank_density);
	conf->base_addr = current_mem_base;

	/* assign each cs */
	switch (opts->ba_intlv_ctl & FSL_DDR_CS0_CS1_CS2_CS3) {
	case FSL_DDR_CS0_CS1_CS2_CS3:
		for (i = 0; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
			conf->cs_base_addr[i] = current_mem_base;
			conf->cs_size[i] = rank_density << 2;
			total_ctlr_mem += rank_density;
		}
		break;
	case FSL_DDR_CS0_CS1:
		for (i = 0; (conf->cs_in_use & (1 << i)) && i < 2; i++) {
			conf->cs_base_addr[i] = current_mem_base;
			conf->cs_size[i] = rank_density << 1;
			total_ctlr_mem += rank_density;
		}
		current_mem_base += total_ctlr_mem;
		for (; (conf->cs_in_use & (1 << i)) && i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
			conf->cs_base_addr[i] = current_mem_base;
			conf->cs_size[i] = rank_density;
			total_ctlr_mem += rank_density;
			current_mem_base += rank_density;
		}
		break;
	case FSL_DDR_CS0_CS1_AND_CS2_CS3:
		for (i = 0; (conf->cs_in_use & (1 << i)) && i < 2; i++) {
			conf->cs_base_addr[i] = current_mem_base;
			conf->cs_size[i] = rank_density << 1;
			total_ctlr_mem += rank_density;
		}
		current_mem_base += total_ctlr_mem;
		total_ctlr_mem += conf->cs_size[0];
		for (; (conf->cs_in_use & (1 << i)) && i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
			conf->cs_base_addr[i] = current_mem_base;
			conf->cs_size[i] = rank_density << 1;
			total_ctlr_mem += rank_density;
		}
		break;
	case FSL_DDR_CS2_CS3:
		for (i = 0; (conf->cs_in_use & (1 << i)) && i < 2; i++) {
			conf->cs_base_addr[i] = current_mem_base;
			conf->cs_size[i] = rank_density;
			current_mem_base += rank_density;
			total_ctlr_mem += rank_density;
		}
		for (; (conf->cs_in_use & (1 << i)) && i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
			conf->cs_base_addr[i] = current_mem_base;
			conf->cs_size[i] = rank_density << 1;
			total_ctlr_mem += rank_density;
		}
		break;
	default:
		for (i = 0; (conf->cs_in_use & (1 << i)) && i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
			conf->cs_base_addr[i] = current_mem_base;
			conf->cs_size[i] = rank_density;
			current_mem_base += rank_density;
			total_ctlr_mem += rank_density;
		}
		break;
	}
	for (i = 0; (conf->cs_in_use & (1 << i)) && i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
		debug_int("CS ", i);
		debug_hex("    base_addr 0x", conf->cs_base_addr[i]);
		debug_hex("    size 0x", conf->cs_size[i]);
	}

	return total_ctlr_mem;
}

int assign_addresses(struct ddr_info *priv) __attribute__ ((weak));

int assign_addresses(struct ddr_info *priv)
{
	int i;
	unsigned long long total_mem, current_mem_base, total_ctlr_mem;
	unsigned int first_ctrl = priv->first_ctrl;
	unsigned int last_ctrl = first_ctrl + priv->num_ctrls - 1;

	/*
	 * If a reduced data width is requested, but the SPD
	 * specifies a physically wider device, adjust the
	 * computed dimm capacities accordingly before
	 * assigning addresses.
	 * 0 = 64-bit, 1 = 32-bit, 2 = 16-bit
	 */
	for (i = first_ctrl; i <= last_ctrl; i++) {

		if (priv->opts[i].data_bus_width >
		    priv->opts[i].data_bus_used) {
			debug("Data bus configuration error\n");
			return -EINVAL;
		}
		priv->opts[i].dbw_cap_shift =
				priv->opts[i].data_bus_used -
				priv->opts[i].data_bus_width;
		debug_int("Controller ", i);
		debug_int("           dbw_cap_shift ",
			  priv->opts[i].dbw_cap_shift);
	}

	current_mem_base = priv->mem_base;
	total_mem = 0;
	total_ctlr_mem = 0;
	debug_int("First ctrl: ", first_ctrl);
	debug_int("memctl_interleaving: ", priv->opts[first_ctrl].memctl_interleaving);
	if (priv->opts[first_ctrl].memctl_interleaving) {
		for (i = first_ctrl; i <= last_ctrl; i++) {
			if (priv->opts[i].memctl_interleaving) {
				total_ctlr_mem = assign_intlv_addr(&priv->dimms[i],
								  &priv->opts[i],
								  &priv->conf[i],
								  current_mem_base);
				debug_int("Controller ", i);
				debug_hex("            base 0x", current_mem_base);
				debug_hex("            total 0x", total_ctlr_mem);
				total_mem = total_ctlr_mem;
			} else {
				/* when 3rd controller not interleaved */
				current_mem_base += total_ctlr_mem;
				debug_int("ctrl ", i);
				total_ctlr_mem = assign_non_intlv_addr(&priv->dimms[i],
								  &priv->opts[i],
								  &priv->conf[i],
								  current_mem_base);
				debug_hex("     total 0x", total_ctlr_mem);
				priv->conf[i].total_mem = total_ctlr_mem;
				total_mem += total_ctlr_mem;
			}
		}
	} else {
		/*
		 * Simple linear assignment if memory
		 * controllers are not interleaved.
		 */
		for (i = first_ctrl; i <= last_ctrl; i++) {
			current_mem_base += total_ctlr_mem;
			debug_int("ctrl ", i);
			total_ctlr_mem = assign_non_intlv_addr(&priv->dimms[i],
								&priv->opts[i],
								&priv->conf[i],
								current_mem_base);
			debug_hex("     total 0x", total_ctlr_mem);
			priv->conf[i].total_mem = total_ctlr_mem;
			total_mem += total_ctlr_mem;
		}
	}
	debug_hex("Total mem by assignment is 0x", total_mem);

	return total_mem;
}
