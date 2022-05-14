//-----------------------------------------------------------------------------
// 
// Copyright (c) 2016, NXP Semiconductors
// Copyright 2017 NXP Semiconductors
//
// Author York Sun <york.sun@nxp.com>
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
#include "debug.h"

#ifndef CONFIG_SYS_FSL_DDR_MAIN_NUM_CTRLS
#define CONFIG_SYS_FSL_DDR_MAIN_NUM_CTRLS CONFIG_SYS_NUM_DDR_CTLRS
#endif

extern void reset(int);
extern void dump_ddrc(int);
extern void dump_err(int);

#ifndef CONFIG_SYS_FSL_DDR_SDRAM_BASE_PHY
#define CONFIG_SYS_FSL_DDR_SDRAM_BASE_PHY	0
#endif

struct ddr_test_result {
	unsigned int crc32;
	unsigned char pass[16][32];
	unsigned char fail[16][32];
	unsigned char tested[16];
	int count;
};

int center(unsigned int *sdram_clk_cntl,
		 unsigned int *wrlvl_cntl,
		 unsigned int *wrlvl_cntl_2,
		 unsigned int *wrlvl_cntl_3,
		 const int ctrl_num,
		 const int loops)
{
#ifdef CONFIG_SHMOO_DDR
	struct ddr_test_result *table = (void *)(DDR_TEST_TABLE & ~0xf);
	unsigned int clk_adjust = (*sdram_clk_cntl >> 22) & 0x1f;
	unsigned int wrlvl_start = *wrlvl_cntl & 0x1f;
	int x, y;
	unsigned int winner_l = 0, winner_r = 0, winner_y;
	unsigned int winner_u = 0, winner_d = 0, winner_x;
	int max = 0, weight[16];

	debug_int("Center DDRC ", ctrl_num);
	table += ctrl_num;

	x = wrlvl_start;
	y = clk_adjust;

	if ((unsigned long)table + sizeof(struct ddr_test_result) > TOP_OF_OCRAM) {
		debug("Error: ddr test table exceeds OCRAM\n");
		return -EINVAL;
	}
	if (table->crc32 != crc32(0, table->pass,
				  sizeof(struct ddr_test_result) - sizeof(unsigned int))) {
		/* First time run */
		debug("Invalid result table\n");
		return -EINVAL;
	}
	bzero(weight, sizeof(weight));
	for (y = 0; y < 16; y++) {
		weight[y] = 0;
		for (x = 0; x < 32; x++) {
			if (table->pass[y][x] && !table->fail[y][x])
				weight[y] += table->pass[y][x];
		}
		if (weight[y] > max && max < 5 * loops) {	/* above 5 is good */
			max = weight[y];
			winner_l = y;
		}
	}
	if (max > 5 * loops)
		max = 5 * loops;
	winner_r = winner_l;
	for (y = winner_l + 1; y < 16; y++) {
		if (weight[y] >= max && weight[y - 1] >= max)
			winner_r = y;
	}
	winner_y = (winner_l + winner_r) >> 1;
	if ((winner_l + winner_r) >> 1 &&
	    weight[winner_y + 1] > weight[winner_y])
		winner_y += 1;
	debug_int("Winner left is ", winner_l);
	debug_int("Winner right is ", winner_r);
	debug_int("Winner of clk_adjust is ", winner_y);

	if (!winner_y)
		return -EINVAL;

	max = 0;
	for (x = 0; x < 32; x++) {
		if (table->pass[winner_y][x] > max) {
			max = table->pass[winner_y][x];
			winner_d = x;
		}
		if (x > 1 && table->pass[winner_y][x] < max &&
		    table->pass[winner_y][x - 1] == max) {
			winner_u = x - 1;
		}
	}
	winner_x = (winner_u + winner_d) >> 1;
	debug_int("Winner down is ", winner_d);
	debug_int("Winner up is ", winner_u);
	debug_int("Winner of wrlvl_start is ", winner_x);

	if (!winner_x)
		return -EINVAL;

	*sdram_clk_cntl = winner_y << 22;
	*wrlvl_cntl = (*wrlvl_cntl & ~0x1f) | (winner_x & 0x1f);
	*wrlvl_cntl_2 += (winner_x - wrlvl_start) * 0x01010101;
	*wrlvl_cntl_3 += (winner_x - wrlvl_start) * 0x01010101;
#endif	/* CONFIG_SHMOO_DDR */

	return 0;
}

int center_wrlvl(const struct ddr_test_result *table,
			  const unsigned int clk_adjust)
{
	int x;
	int winner_l = 0, winner_r = 0, pass = 0, fail = 0;

	for (x = 0; x < 32; x++) {
		if (table->pass[clk_adjust][x] > pass) {
			pass = table->pass[clk_adjust][x];
			winner_l = x;
		}
		if (x > 1 && table->pass[clk_adjust][x] < pass &&
		    table->pass[clk_adjust][x - 1] == pass) {
			winner_r = x - 1;
		}
		if (table->fail[clk_adjust][x] > fail)
			fail = table->fail[clk_adjust][x];
	}

	if (winner_l == 0 && winner_l == 0) {
		if (fail == 0)
			return 0;	/* never run before */
		/* all previous cases failed */
		return -EINVAL;
	}

	return (winner_l + winner_r) / 2;
}

/* Check historic data to decide if new test is needed */
int shmoo_params(unsigned int *sdram_clk_cntl,
		 unsigned int *wrlvl_cntl,
		 unsigned int *wrlvl_cntl_2,
		 unsigned int *wrlvl_cntl_3,
		 const int ctrl_num,
		 const int loops)
{
#ifdef CONFIG_SHMOO_DDR
	struct ddr_test_result *table = (void *)(DDR_TEST_TABLE & ~0xf);
	unsigned int clk_adjust = (*sdram_clk_cntl >> 22) & 0x1f;
	int wrlvl_start = *wrlvl_cntl & 0x1f;
	int wrlvl_save;
	int x, y;

	debug("Shmooing DDRC ");
	dbgprint_uint(ctrl_num);
	table += ctrl_num;

	x = wrlvl_start;
	y = clk_adjust;

	if ((unsigned long)table + sizeof(struct ddr_test_result) > TOP_OF_OCRAM) {
		debug("Error: ddr test table exceeds OCRAM\n");
		return -EINVAL;
	}
	if (table->crc32 != crc32(0, table->pass,
				  sizeof(struct ddr_test_result) - sizeof(unsigned int))) {
		/* First time run */
		bzero(table, sizeof(struct ddr_test_result));
		debug("Zero out ddr result tables\n");
		return 1;
	}

	for (y = clk_adjust; y < 16; y++) {
		if (table->tested[y])
			continue;
		for (x = wrlvl_start; x < 32; x++) {
			if (table->fail[y][x] + table->pass[y][x] >= loops)
				continue;
			if (x > 3 &&
			    table->fail[y][x - 1] >= loops &&
			    table->fail[y][x - 2] >= loops &&
			    table->fail[y][x - 3] >= loops)
				break;
			*wrlvl_cntl_2 += (x - wrlvl_start) * 0x01010101;
			*wrlvl_cntl_3 += (x - wrlvl_start) * 0x01010101;
			*wrlvl_cntl = (*wrlvl_cntl & ~0x1f) | (x & 0x1f);
			*sdram_clk_cntl = y << 22;
			return 1;
		}
		for (x = wrlvl_start - 1; x >= 0; x--) {
			if (table->fail[y][x] + table->pass[y][x] >= loops)
				continue;
			if (x < 29 &&
			    table->fail[y][x + 1] >= loops &&
			    table->fail[y][x + 2] >= loops &&
			    table->fail[y][x + 3] >= loops &&
			    ((table->fail[y][x + 4] >= loops &&
			      table->fail[y][x + 5] >= loops) ||
			      table->pass[y][x + 4] > 0))
				break;
			*wrlvl_cntl_2 -= (wrlvl_start - x) * 0x01010101;
			*wrlvl_cntl_3 -= (wrlvl_start - x) * 0x01010101;
			*wrlvl_cntl = (*wrlvl_cntl & ~0x1f) | (x & 0x1f);
			*sdram_clk_cntl = y << 22;
			return 1;
		}
		/* center wrlvl before doing to next clk_adjust */
		wrlvl_save = wrlvl_start;
		wrlvl_start = center_wrlvl(table, y < 15 ? y : clk_adjust);
		if (!wrlvl_start) {
			wrlvl_start = wrlvl_save;
			debug("Never run\n");
			continue;
		} else if (wrlvl_start < 0) {	/* no passed cases */
			wrlvl_start = wrlvl_save;
			debug("All failed\n");
			return 0;		/* stop here */
		}
		table->tested[y] = 1;	/* avoid testing again */
		*wrlvl_cntl_2 += (wrlvl_start - wrlvl_save) * 0x01010101;
		*wrlvl_cntl_3 += (wrlvl_start - wrlvl_save) * 0x01010101;
	}

	for (y = clk_adjust - 1; y >= 0; y--) {
		if (table->tested[y])
			continue;
		for (x = wrlvl_start; x < 32; x++) {
			if (table->fail[y][x] + table->pass[y][x] >= loops)
				continue;
			if (x > 3 &&
			    table->fail[y][x - 1] >= loops &&
			    table->fail[y][x - 2] >= loops &&
			    table->fail[y][x - 3] >= loops)
				break;
			*wrlvl_cntl_2 += (x - wrlvl_start) * 0x01010101;
			*wrlvl_cntl_3 += (x - wrlvl_start) * 0x01010101;
			*wrlvl_cntl = (*wrlvl_cntl & ~0x1f) | (x & 0x1f);
			*sdram_clk_cntl = y << 22;
			return 1;
		}
		for (x = wrlvl_start - 1; x >= 0; x--) {
			if (table->fail[y][x] + table->pass[y][x] >= loops)
				continue;
			if (x < (32 - 5) &&
			    table->fail[y][x + 1] >= loops &&
			    table->fail[y][x + 2] >= loops &&
			    table->fail[y][x + 3] >= loops &&
			    ((table->fail[y][x + 4] >= loops &&
			      table->fail[y][x + 5] >= loops) ||
			      table->pass[y][x + 4] > 0))
				break;
			*wrlvl_cntl_2 -= (wrlvl_start - x) * 0x01010101;
			*wrlvl_cntl_3 -= (wrlvl_start - x) * 0x01010101;
			*wrlvl_cntl = (*wrlvl_cntl & ~0x1f) | (x & 0x1f);
			*sdram_clk_cntl = y << 22;
			return 1;
		}
		/* center wrlvl before doing to next clk_adjust */
		wrlvl_save = wrlvl_start;
		wrlvl_start = center_wrlvl(table, y);
		if (!wrlvl_start) {
			wrlvl_start = wrlvl_save;
			debug("Never tested\n");
			continue;
		} else if (wrlvl_start < 0) {	/* no passed cases */
			wrlvl_start = wrlvl_save;
			debug("All failed\n");
			return 0;		/* stop here */
		}
		table->tested[y] = 1;	/* avoid testing again */
		*wrlvl_cntl_2 += (wrlvl_start - wrlvl_save) * 0x01010101;
		*wrlvl_cntl_3 += (wrlvl_start - wrlvl_save) * 0x01010101;
	}
#endif

	return 0;
}

void show_test_result(const int ctrl_num, const int less)
{
	struct ddr_test_result *table = (void *)(DDR_TEST_TABLE & ~0xf);
	int x, y;

	table += ctrl_num;
	if ((unsigned long)table + sizeof(struct ddr_test_result) > TOP_OF_OCRAM) {
		debug("Error: ddr test table exceeds OCRAM\n");
		return;
	}
	if (table->crc32 != crc32(0, table->pass,
			  	  sizeof(struct ddr_test_result) - sizeof(unsigned int))) {
		debug("CRC error\n");
		return;
	}

	if (less && table->count % 10)
		return;

	debug("Controller "); dbgprint_uint(ctrl_num); debug("\n");
	debug("            clk_adjust\n");
	debug("wrlvl_start 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15\n");
	for (y = 0; y < 32; y++) {

		if (y < 10) {
			debug("    ");
        }
		else {
			debug("   ");
        }

		dbgprint_uint(y);
		debug("    |");
		for (x = 0; x < 16; x++) {
			if (table->fail[x][y] && table->pass[x][y]) {
				debug("  ~ ");
			} else if (table->fail[x][y]) {
				debug(" -");
				dbgprint_uint(table->fail[x][y]);
				if (table->fail[x][y] < 10) {
					debug(" ");
                }
				else {
					debug("");
                }
			} else if (table->pass[x][y]) {
				debug("  ");
				dbgprint_uint(table->pass[x][y]);
				if (table->pass[x][y] < 10) {
					debug(" ");
                }
				else {
					debug("");
                }
			} else {
				debug("    ");
			}
		}
		debug("\n");
	}
}

void update_test_result(const unsigned long clk,
			const unsigned char clk_adjust,
			const unsigned char wrlvl_start,
			const int ctrl_num,
			const int result)
{
	struct ddr_test_result *table = (void *)(DDR_TEST_TABLE & ~0xf);

	table += ctrl_num;
	if ((unsigned long)table + sizeof(struct ddr_test_result) > TOP_OF_OCRAM) {
		debug("Error: ddr test table exceeds OCRAM\n");
		return;
	}
	if (result) {
		debug_int("failed for ", clk_adjust);
		table->fail[clk_adjust][wrlvl_start]++;
	} else {
		debug_int("passed for ", clk_adjust);
		table->pass[clk_adjust][wrlvl_start]++;
	}
	table->count++;
	table->crc32 = crc32(0, table->pass,
			     sizeof(struct ddr_test_result) - sizeof(unsigned int));

	show_test_result(ctrl_num, 1);
}

static long long init_ddr_sdram(struct ddr_info *priv)
{
	int ret, more_test = 0;
	int i, j;
	int first_ctrl;
	int last_ctrl;
	long long total_mem = 0;
	unsigned int end;
	
	first_ctrl = priv->first_ctrl;
	last_ctrl = priv->first_ctrl + priv->num_ctrls;

	ret = parse_spd(priv);
	if (ret)
		return ret;

	/* for each controller, calculate spd and populdate options */
	populate_memctl_options(priv);

	/* Assign addresses according to interleaving */
	assign_addresses(priv);

	/* For each controller, calculate registers */
	for (i = first_ctrl; i < last_ctrl; i++) {
		if (!priv->conf[i].in_use) {
			bzero(&priv->ddr_reg[i], sizeof(struct ddr_cfg_regs));
			continue;
		}
		ret = compute_ddrc_regs(
				priv->clk,
				&priv->opts[i],
				&priv->conf[i],
				&priv->ddr_reg[i],
				&priv->dimms[i]);
		if (ret) {
			debug("Error: Calculating DDR register(s) failed\n");
			return ret;
		}
		dump_ddrc_regs(&priv->ddr_reg[i]);
		/* DDR has continuous memoby block. Look for the highest bnds. */
		for (j = 0; j < CONFIG_CHIP_SELECTS_PER_CTRL; j++) {
			if (priv->ddr_reg[i].cs[j].config & 0x80000000) {
				if (priv->ddr_reg[i].cs[j].bnds == 0xffffffff) {
					debug("Warning: Unused bnds but with cs enabled\n");
					continue;
				}
				end = priv->ddr_reg[i].cs[j].bnds & 0xffff;
				if (end > total_mem)
					total_mem = end;
			}
		}
	}
	total_mem = 1 + ((total_mem << 24) | 0xffffffULL) - priv->mem_base;

	/* If we need to assert board reset to DDR, do it here */
#if 0
	for (i = last_ctrl - 1; i >= first_ctrl; i--) {
#else
	for (i = first_ctrl; i < last_ctrl; i++) {
#endif
		debug_int("Setting register for controller ", i);
#ifdef CONFIG_SHMOO_DDR
		while (1) {
			ddrc_disable(i);
			reset(i);	/* reset DIMMs */
			if (ret && i == 2) {
				debug("After reset ...\n");
				dump_err(i);
			}
			more_test = shmoo_params(&priv->ddr_reg[i].ddr_sdram_clk_cntl,
					 &priv->ddr_reg[i].ddr_wrlvl_cntl,
					 &priv->ddr_reg[i].ddr_wrlvl_cntl_2,
					 &priv->ddr_reg[i].ddr_wrlvl_cntl_3,
					 i, CONFIG_DDR_TEST_LOOPS);
			if (!more_test)
				break;

			dbgprint_uint((priv->ddr_reg[i].ddr_sdram_clk_cntl >> 22) & 0x1f);
			debug(" ");
			dbgprint_uint(priv->ddr_reg[i].ddr_wrlvl_cntl & 0x1f);
			debug("\n");

			ret = ddrc_set_regs(priv->clk, &priv->ddr_reg[i], i, 0);
			if (ret && i == 2) {
				debug("After failure ...\n");
				dump_err(i);
			}
		}
		show_test_result(i, 0);
		center(&priv->ddr_reg[i].ddr_sdram_clk_cntl,
			&priv->ddr_reg[i].ddr_wrlvl_cntl,
			&priv->ddr_reg[i].ddr_wrlvl_cntl_2,
			&priv->ddr_reg[i].ddr_wrlvl_cntl_3,
			i, CONFIG_DDR_TEST_LOOPS);
		debug_hex("Center sdram_clk_cntl = 0x", priv->ddr_reg[i].ddr_sdram_clk_cntl);
		debug_hex("Center wrlvl_cntl     = 0x", priv->ddr_reg[i].ddr_wrlvl_cntl);
		debug_hex("Center wrlvl_cntl_2   = 0x", priv->ddr_reg[i].ddr_wrlvl_cntl_2);
		debug_hex("Center wrlvl_cntl_3   = 0x", priv->ddr_reg[i].ddr_wrlvl_cntl_3);
		ddrc_disable(i);
		reset(i);	/* reset all DIMMs */
#endif
		ret = ddrc_set_regs(priv->clk, &priv->ddr_reg[i], i, 0);
		if (i == 2)
			dump_ddrc(i);
	}
#ifdef CONFIG_SHMOO_DDR
	for (i = first_ctrl; i < last_ctrl; i++)
		show_test_result(i, 0);
#endif

	if (ret)
		return ret;

	return total_mem;
}

long long dram_init(struct sysinfo *sys)
{
	long long dram_size;
	struct ddr_info priv;

	bzero(&priv, sizeof(struct ddr_info));
	priv.mem_base = CONFIG_SYS_FSL_DDR_SDRAM_BASE_PHY;
	priv.first_ctrl = 0;
        priv.num_ctrls = CONFIG_SYS_FSL_DDR_MAIN_NUM_CTRLS;
        priv.dimm_slots_per_ctrl = CONFIG_SYS_DIMM_SLOTS_PER_CTLR;
	remove_unused_controllers(&priv);
	priv.clk = get_ddr_freq(sys, priv.first_ctrl);

	debug("Initialize DDR ");
	dbgprint_uint((priv.clk + 500000)/1000000);
	debug("MT/s\n");
	dram_size = init_ddr_sdram(&priv);
	if (dram_size > 0) {
		dbgprint_uint(dram_size >> 30);
		debug(" GB ");
		print_ddr_info(priv.first_ctrl);
	}

	return dram_size;
}

#ifdef CONFIG_DP_DDR_DIMM_SLOTS_PER_CTLR
long long dpddr_init(struct sysinfo *sys)
{
	long long dpddr_size;
	struct ddr_info priv;

	bzero(&priv, sizeof(struct ddr_info));
	priv.mem_base = CONFIG_SYS_DP_DDR_BASE_PHY;
	priv.first_ctrl = CONFIG_DP_DDR_CTRL;
	priv.num_ctrls = CONFIG_DP_DDR_NUM_CTRLS;
	priv.dimm_slots_per_ctrl = CONFIG_DP_DDR_DIMM_SLOTS_PER_CTLR;
	priv.clk = get_ddr_freq(sys, priv.first_ctrl);

	debug("Initialize DP-DDR ");
	dbgprint_uint((priv.clk + 500000)/1000000);
	debug("MT/s\n");
	dpddr_size = init_ddr_sdram(&priv);
	if (dpddr_size > 0) {
		dbgprint_uint(dpddr_size >> 30);
		debug(" GB ");
		print_ddr_info(priv.first_ctrl);
	}

	return dpddr_size;
}
#endif
