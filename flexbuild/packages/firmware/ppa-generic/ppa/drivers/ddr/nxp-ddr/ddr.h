//-----------------------------------------------------------------------------
//
// Copyright (c) 2016, NXP Semiconductors
// All rights reserved.
//
// Author York Sun <york.sun@nxp.com>
// 
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

#ifndef __DDR_H__
#define __DDR_H__

#include "dimm.h"
#include "opts.h"
#include "regs.h"
#include "utility.h"

struct ddr_conf {
	int in_use;	/* controller in use */
	int dimm_in_use[CONFIG_SYS_DIMM_SLOTS_PER_CTLR];
	int cs_in_use;	/* bitmask, bit 0 for cs0, bit 1 for cs1, etc. */
	unsigned long long cs_base_addr[CONFIG_CHIP_SELECTS_PER_CTRL];
	unsigned long long cs_size[CONFIG_CHIP_SELECTS_PER_CTRL];
	unsigned long long base_addr;
	unsigned long long total_mem;
};

struct ddr_info {
	unsigned long clk;
	struct dimm_params dimms[CONFIG_SYS_NUM_DDR_CTLRS];
	struct memctl_opt opts[CONFIG_SYS_NUM_DDR_CTLRS];
	struct ddr_conf conf[CONFIG_SYS_NUM_DDR_CTLRS];
	struct ddr_cfg_regs ddr_reg[CONFIG_SYS_NUM_DDR_CTLRS];
	unsigned int first_ctrl;
	unsigned int num_ctrls;
	unsigned long long mem_base;
	unsigned int dimm_slots_per_ctrl;
};

int parse_spd(struct ddr_info *priv);
void remove_unused_controllers(struct ddr_info *info);
void populate_memctl_options(struct ddr_info *priv);
void ddr_board_options(struct ddr_info *priv);
int assign_addresses(struct ddr_info *priv);
int compute_ddrc_regs(const unsigned long clk,
		      const struct memctl_opt *popts,
		      const struct ddr_conf *conf,
		      struct ddr_cfg_regs *ddr,
		      const struct dimm_params *dimm_params);
int ddrc_set_regs(const unsigned long clk,
		  const struct ddr_cfg_regs *regs,
		  int ctrl_num, int step);
void ddrc_disable(int ctrl_num);

#endif	/* __DDR_H__ */
