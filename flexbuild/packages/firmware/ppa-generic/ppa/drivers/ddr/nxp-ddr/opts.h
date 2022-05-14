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

#ifndef __MEM_OPTS_H__
#define __MEM_OPTS_H__

#define SDRAM_TYPE_DDR4		5	/* sdram_cfg register */

#define DDR_BC4			4	/* burst chop */
#define DDR_OTF			6	/* on-the-fly BC4 and BL8 */
#define DDR_BL8			8	/* burst length 8 */

#define DDR4_RTT_OFF		0
#define DDR4_RTT_60_OHM		1	/* RZQ/4 */
#define DDR4_RTT_120_OHM	2	/* RZQ/2 */
#define DDR4_RTT_40_OHM		3	/* RZQ/6 */
#define DDR4_RTT_240_OHM	4	/* RZQ/1 */
#define DDR4_RTT_48_OHM		5	/* RZQ/5 */
#define DDR4_RTT_80_OHM		6	/* RZQ/3 */
#define DDR4_RTT_34_OHM		7	/* RZQ/7 */

#define FSL_DDR_ODT_NEVER		0x0
#define FSL_DDR_ODT_CS			0x1
#define FSL_DDR_ODT_ALL_OTHER_CS	0x2
#define FSL_DDR_ODT_OTHER_DIMM		0x3
#define FSL_DDR_ODT_ALL			0x4
#define FSL_DDR_ODT_SAME_DIMM		0x5
#define FSL_DDR_ODT_CS_AND_OTHER_DIMM	0x6
#define FSL_DDR_ODT_OTHER_CS_ONSAMEDIMM	0x7

/* define bank(chip select) interleaving mode */
#define FSL_DDR_CS0_CS1			0x40
#define FSL_DDR_CS2_CS3			0x20
#define FSL_DDR_CS0_CS1_AND_CS2_CS3	(FSL_DDR_CS0_CS1 | FSL_DDR_CS2_CS3)
#define FSL_DDR_CS0_CS1_CS2_CS3		(FSL_DDR_CS0_CS1_AND_CS2_CS3 | 0x04)

/* define memory controller interleaving mode */
#define FSL_DDR_CACHE_LINE_INTERLEAVING	0x0
#define FSL_DDR_PAGE_INTERLEAVING	0x1
#define FSL_DDR_BANK_INTERLEAVING	0x2
#define FSL_DDR_SUPERBANK_INTERLEAVING	0x3
#define FSL_DDR_256B_INTERLEAVING	0x8

#define DDR_DATA_BUS_WIDTH_64 0
#define DDR_DATA_BUS_WIDTH_32 1
#define DDR_DATA_BUS_WIDTH_16 2

#define DDR_CSWL_CS0	0x04000001	/* used by ls1043 */
/*
 * Generalized parameters for memory controller configuration,
 * might be a little specific to the FSL memory controller
 */
struct memctl_opt {
	/* registered DIMM support */
	int registered_dimm_en;
	/* data bus width shift for capacity adjustment */
	unsigned int dbw_cap_shift;

	/* Options local to a Chip Select */
	struct cs_local_opts_s {
		unsigned int auto_precharge;
		unsigned int odt_rd_cfg;
		unsigned int odt_wr_cfg;
		unsigned int odt_rtt_norm;
		unsigned int odt_rtt_wr;
	} cs_local_opts[CONFIG_CHIP_SELECTS_PER_CTRL];

	/* Special configurations for chip select */
	int memctl_interleaving;
	unsigned int memctl_interleaving_mode;
	unsigned int ba_intlv_ctl;
	unsigned int addr_hash;

	/* Operational mode parameters */
	int ecc_mode;	 /* Use ECC */
	/* Initialize ECC using memory controller */
	int ecc_init_using_memctl;
	/* SREN - self-refresh during sleep */
	int self_refresh_in_sleep;
	/* SR_IE - Self-refresh interrupt enable */
	int self_refresh_interrupt_en;
	int dynamic_power;	/* DYN_PWR */
	/* memory data width 0 = 64-bit, 1 = 32-bit, 2 = 16-bit */
	unsigned int data_bus_width;
	unsigned int data_bus_used;	/* on individual board */
	unsigned int burst_length;	/* BL4, OTF and BL8 */
	int otf_burst_chop_en;	/* On-The-Fly Burst Chop enable */
	int mirrored_dimm;	/* mirrior DIMMs */
	int quad_rank_present;
	int ap_en;	/* address parity enable */
	int x4_en;	/* enable x4 devices */

	/* Global Timing Parameters */
	int cas_latency_override;
	unsigned int cas_latency_override_value;
	int additive_latency_override;
	unsigned int additive_latency_override_value;

	unsigned int clk_adjust;
	unsigned int cpo_override;
	unsigned int write_data_delay;

	unsigned int cswl_override;
	unsigned int wrlvl_override;
	unsigned int wrlvl_sample;
	unsigned int wrlvl_start;
	unsigned int wrlvl_ctl_2;
	unsigned int wrlvl_ctl_3;

	int half_strength_driver_enable;
	int twot_en;
	int threet_en;
	unsigned int bstopre;
	unsigned int tfaw_window_four_activates_ps;	/* tFAW --  FOUR_ACT */

	/* Rtt impedance */
	int rtt_override;		/* rtt_override enable */
	unsigned int rtt_override_value;	/* Rtt_Nom */
	unsigned int rtt_wr_override_value;	/* Rtt_WR */

	int auto_self_refresh_en;
	unsigned int sr_it;
	int rcw_override;	/* RCW override for RDIMM */
	unsigned int rcw_1;
	unsigned int rcw_2;
	unsigned int ddr_cdr1;
	unsigned int ddr_cdr2;

	int trwt_override;
	unsigned int trwt;	/* read-to-write turnaround */
};

struct mem_ctrl {
	unsigned int num;
	unsigned long mem0_clk;
	unsigned long mem1_clk;
	unsigned long mem2_clk;
};

#endif /* __MEM_OPTS_H__ */
