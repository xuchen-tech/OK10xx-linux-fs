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

#ifndef __DDR_REG_H__
#define __DDR_REG_H__

#define SDRAM_CS_CONFIG_EN		0x80000000

/* DDR_SDRAM_CFG - DDR SDRAM Control Configuration
 */
#define SDRAM_CFG_MEM_EN		0x80000000
#define SDRAM_CFG_SREN			0x40000000
#define SDRAM_CFG_ECC_EN		0x20000000
#define SDRAM_CFG_RD_EN			0x10000000
#define SDRAM_CFG_SDRAM_TYPE_MASK	0x07000000
#define SDRAM_CFG_SDRAM_TYPE_SHIFT	24
#define SDRAM_CFG_DYN_PWR		0x00200000
#define SDRAM_CFG_DBW_MASK		0x00180000
#define SDRAM_CFG_DBW_SHIFT		19
#define SDRAM_CFG_32_BW			0x00080000
#define SDRAM_CFG_16_BW			0x00100000
#define SDRAM_CFG_8_BW			0x00180000
#define SDRAM_CFG_8_BE			0x00040000
#define SDRAM_CFG_2T_EN			0x00008000
#define SDRAM_CFG_BI			0x00000001

#define SDRAM_CFG2_FRC_SR		0x80000000
#define SDRAM_CFG2_D_INIT		0x00000010
#define SDRAM_CFG2_AP_EN		0x00000020
#define SDRAM_CFG2_ODT_ONLY_READ	2

#define SDRAM_CFG3_DDRC_RST		0x80000000

#define SDRAM_INTERVAL_BSTOPRE	0x3FFF

#define RD_TO_PRE_MASK		0xf
#define RD_TO_PRE_SHIFT		13
#define WR_DATA_DELAY_MASK	0xf
#define WR_DATA_DELAY_SHIFT	9

/* DDR_MD_CNTL */
#define MD_CNTL_MD_EN		0x80000000
#define MD_CNTL_CS_SEL(x)	(((x) & 0x7) << 28)
#define MD_CNTL_MD_SEL(x)	(((x) & 0xf) << 24)
#define MD_CNTL_CKE(x)		(((x) & 0x3) << 20)

/* DDR_CDR1 */
#define DDR_CDR1_DHC_EN	0x80000000
#define DDR_CDR1_ODT_SHIFT	17
#define DDR_CDR1_ODT_MASK	0x6
#define DDR_CDR2_ODT_MASK	0x1
#define DDR_CDR1_ODT(x) ((x & DDR_CDR1_ODT_MASK) << DDR_CDR1_ODT_SHIFT)
#define DDR_CDR2_ODT(x) (x & DDR_CDR2_ODT_MASK)
#define DDR_CDR2_VREF_OVRD(x)	(0x00008080 | ((((x) - 37) & 0x3F) << 8))
#define DDR_CDR2_VREF_TRAIN_EN	0x00000080
#define DDR_CDR2_VREF_RANGE_2	0x00000040
#define DDR_CDR_ODT_OFF		0x0
#define DDR_CDR_ODT_100ohm	0x1
#define DDR_CDR_ODT_120OHM	0x2
#define DDR_CDR_ODT_80ohm	0x3
#define DDR_CDR_ODT_60ohm	0x4
#define DDR_CDR_ODT_40ohm	0x5
#define DDR_CDR_ODT_50ohm	0x6
#define DDR_CDR_ODT_30ohm	0x7


/* DDR ERR_DISABLE */
#define DDR_ERR_DISABLE_APED	(1 << 8)  /* Address parity error disable */

/* Mode Registers */
#define DDR_MR5_CA_PARITY_LAT_4_CLK	0x1 /* for DDR4-1600/1866/2133 */
#define DDR_MR5_CA_PARITY_LAT_5_CLK	0x2 /* for DDR4-2400 */

/* DEBUG_29 register */
#define DDR_TX_BD_DIS	(1 << 10) /* Transmit Bit Deskew Disable */

#define DDR_INIT_ADDR_EXT_UIA	(1 << 31)

/* Record of register values computed */
struct ddr_cfg_regs {
	struct {
		unsigned int bnds;
		unsigned int config;
		unsigned int config_2;
	} cs[CONFIG_CHIP_SELECTS_PER_CTRL];
	unsigned int timing_cfg_3;
	unsigned int timing_cfg_0;
	unsigned int timing_cfg_1;
	unsigned int timing_cfg_2;
	unsigned int ddr_sdram_cfg;
	unsigned int ddr_sdram_cfg_2;
	unsigned int ddr_sdram_cfg_3;
	unsigned int ddr_sdram_mode;
	unsigned int ddr_sdram_mode_2;
	unsigned int ddr_sdram_mode_3;
	unsigned int ddr_sdram_mode_4;
	unsigned int ddr_sdram_mode_5;
	unsigned int ddr_sdram_mode_6;
	unsigned int ddr_sdram_mode_7;
	unsigned int ddr_sdram_mode_8;
	unsigned int ddr_sdram_mode_9;
	unsigned int ddr_sdram_mode_10;
	unsigned int ddr_sdram_mode_11;
	unsigned int ddr_sdram_mode_12;
	unsigned int ddr_sdram_mode_13;
	unsigned int ddr_sdram_mode_14;
	unsigned int ddr_sdram_mode_15;
	unsigned int ddr_sdram_mode_16;
	unsigned int ddr_sdram_md_cntl;
	unsigned int ddr_sdram_interval;
	unsigned int ddr_data_init;
	unsigned int ddr_sdram_clk_cntl;
	unsigned int ddr_init_addr;
	unsigned int ddr_init_ext_addr;
	unsigned int timing_cfg_4;
	unsigned int timing_cfg_5;
	unsigned int timing_cfg_6;
	unsigned int timing_cfg_7;
	unsigned int timing_cfg_8;
	unsigned int timing_cfg_9;
	unsigned int ddr_zq_cntl;
	unsigned int ddr_wrlvl_cntl;
	unsigned int ddr_wrlvl_cntl_2;
	unsigned int ddr_wrlvl_cntl_3;
	unsigned int ddr_sr_cntr;
	unsigned int ddr_sdram_rcw_1;
	unsigned int ddr_sdram_rcw_2;
	unsigned int ddr_sdram_rcw_3;
	unsigned int ddr_sdram_rcw_4;
	unsigned int ddr_sdram_rcw_5;
	unsigned int ddr_sdram_rcw_6;
	unsigned int dq_map_0;
	unsigned int dq_map_1;
	unsigned int dq_map_2;
	unsigned int dq_map_3;
	unsigned int ddr_eor;
	unsigned int ddr_cdr1;
	unsigned int ddr_cdr2;
	unsigned int err_disable;
	unsigned int err_int_en;
	unsigned int debug[64];
};

#endif /* __DDR_REG_H__ */
