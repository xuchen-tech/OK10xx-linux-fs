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

#ifndef __LS2080ARDB_DDR_H__
#define __LS2080ARDB_DDR_H__

struct board_specific_parameters {
	unsigned int n_ranks;
	unsigned int datarate_mhz_high;
	unsigned int rank_gb;
	unsigned int clk_adjust;
	unsigned int wrlvl_start;
	unsigned int wrlvl_ctl_2;
	unsigned int wrlvl_ctl_3;
};

/*
 * These tables contain all valid speeds we want to override with board
 * specific parameters. datarate_mhz_high values need to be in ascending order
 * for each n_ranks group.
 */

static const struct board_specific_parameters udimm0[] = {
	/*
	 * memory controller 0
	 *   num|  hi| rank|  clk| wrlvl |   wrlvl   |  wrlvl
	 * ranks| mhz| GB  |adjst| start |   ctl2    |  ctl3
	 */
	{2,  1350, 0, 8,     6, 0x0708090B, 0x0C0D0E09,},
	{2,  1666, 0, 10,    9, 0x090A0B0E, 0x0F11110C,},
	{2,  1900, 0, 12,  0xA, 0x0B0C0E11, 0x1214140F,},
	{2,  2300, 0, 12,  0xB, 0x0C0D0F12, 0x14161610,},
	{}
};

/* DP-DDR DIMM */
static const struct board_specific_parameters udimm2[] = {
	/*
	 * memory controller 2
	 *   num|  hi| rank|  clk| wrlvl |   wrlvl   |  wrlvl
	 * ranks| mhz| GB  |adjst| start |   ctl2    |  ctl3
	 */
	{2,  1350, 0, 8,   0xd, 0x0C0A0A00, 0x00000009,},
	{2,  1666, 0, 8,   0xd, 0x0C0A0A00, 0x00000009,},
	{2,  1900, 0, 8,   0xe, 0x0D0C0B00, 0x0000000A,},
	{2,  2200, 0, 8,   0xe, 0x0D0C0B00, 0x0000000A,},
	{}
};

static const struct board_specific_parameters rdimm0[] = {
	/*
	 * memory controller 0
	 *   num|  hi| rank|  clk| wrlvl |   wrlvl   |  wrlvl
	 * ranks| mhz| GB  |adjst| start |   ctl2    |  ctl3
	 */
	{2,  1350, 0, 8,     6, 0x0708090B, 0x0C0D0E09,},
	{2,  1666, 0, 8,     7, 0x08090A0C, 0x0D0F100B,},
	{2,  1900, 0, 8,     7, 0x09090B0D, 0x0E10120B,},
	{2,  2200, 0, 8,     8, 0x090A0C0F, 0x1012130C,},
	{}
};

/* DP-DDR DIMM */
static const struct board_specific_parameters rdimm2[] = {
	/*
	 * memory controller 2
	 *   num|  hi| rank|  clk| wrlvl |   wrlvl   |  wrlvl
	 * ranks| mhz| GB  |adjst| start |   ctl2    |  ctl3
	 */
	{2,  1350, 0, 8,     6, 0x0708090B, 0x0C0D0E09,},
	{2,  1666, 0, 8,     7, 0x0B0A090C, 0x0D0F100B,},
	{2,  1900, 0, 8,     7, 0x09090B0D, 0x0E10120B,},
	{2,  2200, 0, 8,     8, 0x090A0C0F, 0x1012130C,},
	{}
};

static const struct board_specific_parameters *udimms[] = {
	udimm0,
	udimm0,
	udimm2,
};

static const struct board_specific_parameters *rdimms[] = {
	rdimm0,
	rdimm0,
	rdimm2,
};

#define CSPR_PORT_SIZE_16	0x00000100
#define CSPR_PORT_SIZE_8	0x00000080
#define CSPR_MSEL_NOR		0x00000000
#define CSPR_MSEL_GPCM		0x00000004
#define CSPR_V			0x00000001

#define IFC_AMASK_MASK		0xFFFF0000
#define IFC_AMASK_SHIFT		16
#define IFC_AMASK(n)		(IFC_AMASK_MASK << (ilog2(n) - IFC_AMASK_SHIFT))
#define CSOR_GPCM_ADM_SHIFT_SHIFT	13
#define CSOR_GPCM_ADM_SHIFT(n)	((n) << CSOR_GPCM_ADM_SHIFT_SHIFT)
#define CSOR_NOR_ADM_SHIFT_SHIFT	13
#define CSOR_NOR_ADM_SHIFT(n)	((n) << CSOR_NOR_ADM_SHIFT_SHIFT)
#define FTIM0_GPCM_TACSE_SHIFT	28
#define FTIM0_GPCM_TACSE(n)	((n) << FTIM0_GPCM_TACSE_SHIFT)
#define FTIM0_GPCM_TEADC_SHIFT	16
#define FTIM0_GPCM_TEADC(n)	((n) << FTIM0_GPCM_TEADC_SHIFT)
#define FTIM0_GPCM_TAVDS_SHIFT	8
#define FTIM0_GPCM_TAVDS(n)	((n) << FTIM0_GPCM_TAVDS_SHIFT)
#define FTIM0_GPCM_TEAHC_SHIFT	0
#define FTIM0_GPCM_TEAHC(n)	((n) << FTIM0_GPCM_TEAHC_SHIFT)
#define FTIM1_GPCM_TACO_SHIFT	24
#define FTIM1_GPCM_TACO(n)	((n) << FTIM1_GPCM_TACO_SHIFT)
#define FTIM1_GPCM_TRAD_SHIFT	8
#define FTIM1_GPCM_TRAD(n)	((n) << FTIM1_GPCM_TRAD_SHIFT)
#define FTIM2_GPCM_TCS_SHIFT	24
#define FTIM2_GPCM_TCS(n)	((n) << FTIM2_GPCM_TCS_SHIFT)
#define FTIM2_GPCM_TCH_SHIFT	18
#define FTIM2_GPCM_TCH(n)	((n) << FTIM2_GPCM_TCH_SHIFT)
#define FTIM2_GPCM_TWP_SHIFT	0
#define FTIM2_GPCM_TWP(n)	((n) << FTIM2_GPCM_TWP_SHIFT)
#define FTIM0_NOR_TACSE_SHIFT	28
#define FTIM0_NOR_TACSE(n)	((n) << FTIM0_NOR_TACSE_SHIFT)
#define FTIM0_NOR_TEADC_SHIFT	16
#define FTIM0_NOR_TEADC(n)	((n) << FTIM0_NOR_TEADC_SHIFT)
#define FTIM0_NOR_TAVDS_SHIFT	8
#define FTIM0_NOR_TAVDS(n)	((n) << FTIM0_NOR_TAVDS_SHIFT)
#define FTIM0_NOR_TEAHC_SHIFT	0
#define FTIM0_NOR_TEAHC(n)	((n) << FTIM0_NOR_TEAHC_SHIFT)
#define FTIM1_NOR_TACO_SHIFT	24
#define FTIM1_NOR_TACO(n)	((n) << FTIM1_NOR_TACO_SHIFT)
#define FTIM1_NOR_TRAD_NOR_SHIFT	8
#define FTIM1_NOR_TRAD_NOR(n)	((n) << FTIM1_NOR_TRAD_NOR_SHIFT)
#define FTIM1_NOR_TSEQRAD_NOR_SHIFT	0
#define FTIM1_NOR_TSEQRAD_NOR(n)	((n) << FTIM1_NOR_TSEQRAD_NOR_SHIFT)
#define FTIM2_NOR_TCS_SHIFT		24
#define FTIM2_NOR_TCS(n)	((n) << FTIM2_NOR_TCS_SHIFT)
#define FTIM2_NOR_TCH_SHIFT		18
#define FTIM2_NOR_TCH(n)	((n) << FTIM2_NOR_TCH_SHIFT)
#define FTIM2_NOR_TWPH_SHIFT	10
#define FTIM2_NOR_TWPH(n)	((n) << FTIM2_NOR_TWPH_SHIFT)
#define FTIM2_NOR_TWP_SHIFT		0
#define FTIM2_NOR_TWP(n)	((n) << FTIM2_NOR_TWP_SHIFT)

#define CSPR_PHYS_ADDR(x)	(((unsigned long long)x) & 0xffff0000)

#define QIXIS_BASE_PHYS		0x20000000
#define CONFIG_FLASH_BASE_PHYS	0x0

/* NOR flash */
#define CONFIG_SYS_AMASK0	IFC_AMASK(128*1024*1024)
#define CONFIG_SYS_CSOR0	CSOR_NOR_ADM_SHIFT(12)
#define CONFIG_SYS_CS0_FTIM0	(FTIM0_NOR_TACSE(0x4)	| \
				FTIM0_NOR_TEADC(0x5)	| \
				FTIM0_NOR_TEAHC(0x5))
#define CONFIG_SYS_CS0_FTIM1	(FTIM1_NOR_TACO(0x35)	| \
				FTIM1_NOR_TRAD_NOR(0x1a)|\
				FTIM1_NOR_TSEQRAD_NOR(0x13))
#define CONFIG_SYS_CS0_FTIM2	(FTIM2_NOR_TCS(0x4)	| \
				FTIM2_NOR_TCH(0x4)	| \
				FTIM2_NOR_TWPH(0x0E)	| \
				FTIM2_NOR_TWP(0x1c))
#define CONFIG_SYS_CS0_FTIM3	0x04000000
#define CONFIG_SYS_CSPR0	(CSPR_PHYS_ADDR(CONFIG_FLASH_BASE_PHYS)	\
				| CSPR_PORT_SIZE_16 \
				| CSPR_MSEL_NOR \
				| CSPR_V)
#define CONFIG_SYS_CSPR0_EXT	0
/* QIXIS Timing parameters for IFC CS3 */
#define CONFIG_SYS_AMASK3	IFC_AMASK(64*1024)
#define CONFIG_SYS_CSOR3	CSOR_GPCM_ADM_SHIFT(12)
#define CONFIG_SYS_CS3_FTIM0    (FTIM0_GPCM_TACSE(0x0e)	| \
				FTIM0_GPCM_TEADC(0x0e)	| \
				FTIM0_GPCM_TEAHC(0x0e))
#define CONFIG_SYS_CS3_FTIM1    (FTIM1_GPCM_TACO(0xff)	| \
				FTIM1_GPCM_TRAD(0x3f))
#define CONFIG_SYS_CS3_FTIM2    (FTIM2_GPCM_TCS(0xf)	| \
				FTIM2_GPCM_TCH(0xf)	| \
				FTIM2_GPCM_TWP(0x3E))
#define CONFIG_SYS_CS3_FTIM3    0x0
#define CONFIG_SYS_CSPR3	(CSPR_PHYS_ADDR(QIXIS_BASE_PHYS) \
				| CSPR_PORT_SIZE_8 \
				| CSPR_MSEL_GPCM \
				| CSPR_V)
#define CONFIG_SYS_CSPR3_EXT	0

#define UART_BASE	0x21c0600
#define UART_BAUD_DIV	163     /* 115200 from 600MHz plat clk */

enum ifc_chip_sel {
        IFC_CS0,
        IFC_CS1,
        IFC_CS2,
        IFC_CS3,
        IFC_CS4,
        IFC_CS5,
        IFC_CS6,
        IFC_CS7,
};

enum ifc_ftims {
        IFC_FTIM0,
        IFC_FTIM1,
        IFC_FTIM2,
        IFC_FTIM3,
};

#endif /* __LS2080ARDB_DDR_H__ */
