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

#ifndef __FSL_DDR_IMMAP_H
#define __FSL_DDR_IMMAP_H
/*
 * DDRC register file for DDRC 5.0 and above
 */
struct ccsr_ddr {
	unsigned int	cs0_bnds;		/* Chip Select 0 Memory Bounds */
	unsigned char	res_04[4];
	unsigned int	cs1_bnds;		/* Chip Select 1 Memory Bounds */
	unsigned char	res_0c[4];
	unsigned int	cs2_bnds;		/* Chip Select 2 Memory Bounds */
	unsigned char	res_14[4];
	unsigned int	cs3_bnds;		/* Chip Select 3 Memory Bounds */
	unsigned char	res_1c[100];
	unsigned int	cs0_config;		/* Chip Select Configuration */
	unsigned int	cs1_config;		/* Chip Select Configuration */
	unsigned int	cs2_config;		/* Chip Select Configuration */
	unsigned int	cs3_config;		/* Chip Select Configuration */
	unsigned char	res_90[48];
	unsigned int	cs0_config_2;		/* Chip Select Configuration 2 */
	unsigned int	cs1_config_2;		/* Chip Select Configuration 2 */
	unsigned int	cs2_config_2;		/* Chip Select Configuration 2 */
	unsigned int	cs3_config_2;		/* Chip Select Configuration 2 */
	unsigned char	res_d0[48];
	unsigned int	timing_cfg_3;		/* SDRAM Timing Configuration 3 */
	unsigned int	timing_cfg_0;		/* SDRAM Timing Configuration 0 */
	unsigned int	timing_cfg_1;		/* SDRAM Timing Configuration 1 */
	unsigned int	timing_cfg_2;		/* SDRAM Timing Configuration 2 */
	unsigned int	sdram_cfg;		/* SDRAM Control Configuration */
	unsigned int	sdram_cfg_2;		/* SDRAM Control Configuration 2 */
	unsigned int	sdram_mode;		/* SDRAM Mode Configuration */
	unsigned int	sdram_mode_2;		/* SDRAM Mode Configuration 2 */
	unsigned int	sdram_md_cntl;		/* SDRAM Mode Control */
	unsigned int	sdram_interval;		/* SDRAM Interval Configuration */
	unsigned int	sdram_data_init;	/* SDRAM Data initialization */
	unsigned char	res_12c[4];
	unsigned int	sdram_clk_cntl;		/* SDRAM Clock Control */
	unsigned char	res_134[20];
	unsigned int	init_addr;		/* training init addr */
	unsigned int	init_ext_addr;		/* training init extended addr */
	unsigned char	res_150[16];
	unsigned int	timing_cfg_4;		/* SDRAM Timing Configuration 4 */
	unsigned int	timing_cfg_5;		/* SDRAM Timing Configuration 5 */
	unsigned int	timing_cfg_6;		/* SDRAM Timing Configuration 6 */
	unsigned int	timing_cfg_7;		/* SDRAM Timing Configuration 7 */
	unsigned int	ddr_zq_cntl;		/* ZQ calibration control*/
	unsigned int	ddr_wrlvl_cntl;		/* write leveling control*/
	unsigned char	reg_178[4];
	unsigned int	ddr_sr_cntr;		/* self refresh counter */
	unsigned int	ddr_sdram_rcw_1;	/* Control Words 1 */
	unsigned int	ddr_sdram_rcw_2;	/* Control Words 2 */
	unsigned char	reg_188[8];
	unsigned int	ddr_wrlvl_cntl_2;	/* write leveling control 2 */
	unsigned int	ddr_wrlvl_cntl_3;	/* write leveling control 3 */
	unsigned char	res_198[0x1a0-0x198];
	unsigned int	ddr_sdram_rcw_3;
	unsigned int	ddr_sdram_rcw_4;
	unsigned int	ddr_sdram_rcw_5;
	unsigned int	ddr_sdram_rcw_6;
	unsigned char	res_1b0[0x200-0x1b0];
	unsigned int	sdram_mode_3;		/* SDRAM Mode Configuration 3 */
	unsigned int	sdram_mode_4;		/* SDRAM Mode Configuration 4 */
	unsigned int	sdram_mode_5;		/* SDRAM Mode Configuration 5 */
	unsigned int	sdram_mode_6;		/* SDRAM Mode Configuration 6 */
	unsigned int	sdram_mode_7;		/* SDRAM Mode Configuration 7 */
	unsigned int	sdram_mode_8;		/* SDRAM Mode Configuration 8 */
	unsigned char	res_218[0x220-0x218];
	unsigned int	sdram_mode_9;		/* SDRAM Mode Configuration 9 */
	unsigned int	sdram_mode_10;		/* SDRAM Mode Configuration 10 */
	unsigned int	sdram_mode_11;		/* SDRAM Mode Configuration 11 */
	unsigned int	sdram_mode_12;		/* SDRAM Mode Configuration 12 */
	unsigned int	sdram_mode_13;		/* SDRAM Mode Configuration 13 */
	unsigned int	sdram_mode_14;		/* SDRAM Mode Configuration 14 */
	unsigned int	sdram_mode_15;		/* SDRAM Mode Configuration 15 */
	unsigned int	sdram_mode_16;		/* SDRAM Mode Configuration 16 */
	unsigned char	res_240[0x250-0x240];
	unsigned int	timing_cfg_8;		/* SDRAM Timing Configuration 8 */
	unsigned int	timing_cfg_9;		/* SDRAM Timing Configuration 9 */
	unsigned char	res_258[0x260-0x258];
	unsigned int	sdram_cfg_3;
	unsigned char	res_264[0x270-0x264];
	unsigned int	sdram_md_cntl_2;
	unsigned char	res_274[0x400-0x274];
	unsigned int	dq_map_0;
	unsigned int	dq_map_1;
	unsigned int	dq_map_2;
	unsigned int	dq_map_3;
	unsigned char	res_410[0xb20-0x410];
	unsigned int	ddr_dsr1;		/* Debug Status 1 */
	unsigned int	ddr_dsr2;		/* Debug Status 2 */
	unsigned int	ddr_cdr1;		/* Control Driver 1 */
	unsigned int	ddr_cdr2;		/* Control Driver 2 */
	unsigned char	res_b30[200];
	unsigned int	ip_rev1;		/* IP Block Revision 1 */
	unsigned int	ip_rev2;		/* IP Block Revision 2 */
	unsigned int	eor;			/* Enhanced Optimization Register */
	unsigned char	res_c04[252];
	unsigned int	mtcr;			/* Memory Test Control Register */
	unsigned char	res_d04[28];
	unsigned int	mtp1;			/* Memory Test Pattern 1 */
	unsigned int	mtp2;			/* Memory Test Pattern 2 */
	unsigned int	mtp3;			/* Memory Test Pattern 3 */
	unsigned int	mtp4;			/* Memory Test Pattern 4 */
	unsigned int	mtp5;			/* Memory Test Pattern 5 */
	unsigned int	mtp6;			/* Memory Test Pattern 6 */
	unsigned int	mtp7;			/* Memory Test Pattern 7 */
	unsigned int	mtp8;			/* Memory Test Pattern 8 */
	unsigned int	mtp9;			/* Memory Test Pattern 9 */
	unsigned int	mtp10;			/* Memory Test Pattern 10 */
	unsigned char	res_d48[184];
	unsigned int	data_err_inject_hi;	/* Data Path Err Injection Mask High */
	unsigned int	data_err_inject_lo;	/* Data Path Err Injection Mask Low */
	unsigned int	ecc_err_inject;		/* Data Path Err Injection Mask ECC */
	unsigned char	res_e0c[20];
	unsigned int	capture_data_hi;	/* Data Path Read Capture High */
	unsigned int	capture_data_lo;	/* Data Path Read Capture Low */
	unsigned int	capture_ecc;		/* Data Path Read Capture ECC */
	unsigned char	res_e2c[20];
	unsigned int	err_detect;		/* Error Detect */
	unsigned int	err_disable;		/* Error Disable */
	unsigned int	err_int_en;
	unsigned int	capture_attributes;	/* Error Attrs Capture */
	unsigned int	capture_address;	/* Error Addr Capture */
	unsigned int	capture_ext_address;	/* Error Extended Addr Capture */
	unsigned int	err_sbe;		/* Single-Bit ECC Error Management */
	unsigned char	res_e5c[164];
	unsigned int	debug[64];		/* debug_1 to debug_64 */
};
#endif /* __FSL_DDR_IMMAP_H */
