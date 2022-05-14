//-----------------------------------------------------------------------------
// 
// Copyright 2016-2018 NXP Semiconductors
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

#ifndef __LSCH3_H_
#define __LSCH3_H_

//-----------------------------------------------------------------------------

 // collect all of the headers for a chassis-level-3 device
#include "aarch64.h"
#include "gicv3.h"
#include "soc.h"
#include "policy.h"
#include "runtime_data.h"

//-----------------------------------------------------------------------------

#define GICD_BASE_ADDR        0x06000000

 // base addresses
#define DCFG_BASE_ADDR        0x01E00000
#define TIMER_BASE_ADDR       0x023E0000
#define RESET_BASE_ADDR       0x01E60000
#define SECMON_BASE_ADDR      0x01E90000
#define SEC_REGFILE_BASE_ADDR 0x01E88000
#define SCFG_BASE_ADDR        0x01E88000
#define PMU_BASE_ADDR         0x01E30000
#define ESDHC_BASE_ADDR       0x02140000
#define SFP_BASE_ADDR         0x01E80000

 // OCRAM
#define  OCRAM_BASE_ADDR     0x18000000
#define  OCRAM_REGION_LOWER  0
#define  OCRAM_REGION_UPPER  1
#define  OCRAM_REGION_ALL    2
#define  OCRAM_INIT_RETRY    0x2000
#define  TOP_OF_OCRAM        ((OCRAM_BASE_ADDR + OCRAM_SIZE_IN_BYTES) - 1)

 // dcfg block register offsets and bitfields
#define DCFG_SCRATCHRW5_OFFSET  0x210
#define DCFG_SCRATCHRW6_OFFSET  0x214
#define DCFG_SCRATCHRW7_OFFSET  0x218
#define DCFG_SVR_OFFSET         0x0A4
#define COREDISABLEDSR_OFFSET   0x990
#define DCFG_COREDISR_OFFSET    0x94
#define COREDISR_OFFSET         0x94
#define BOOTLOCPTRL_OFFSET      0x400
#define BOOTLOCPTRH_OFFSET      0x404
#define RCW_SR1_OFFSET          0x100
#define RCW_SR27_OFFSET         0x168
#define RCWSR1_SYSPLLRAT_MASK   0x7C        

 // reset block register offsets
#define RST_RSTCR_OFFSET      0x0
#define RST_RSTRQMR1_OFFSET   0x10
#define RST_RSTRQSR1_OFFSET   0x18
#define BRR_OFFSET            0x60

 // secure register file offsets and bit masks
#define CORE_HOLD_OFFSET      0x140
#define RSTCNTL_OFFSET        0x180
#define SW_RST_REQ_INIT       0x1

 // secmon register offsets and bitfields
#define SECMON_HPCOMR_OFFSET  0x4
#define SECMON_HPCOMR_NPSWAEN 0x80000000

 // System Counter Offset and Bit Mask
#define SYS_COUNTER_CNTCR_OFFSET	0x0
#define SYS_COUNTER_CNTCR_EN		0x00000001

#define TIMER_CNTCR_OFFSET 0x0

 // timer control bitfields
#define CNTCR_EN_MASK   0x1
#define CNTCR_EN        0x1
#define CNTCR_DIS       0x0

 // bit masks
#define RSTCR_RESET_REQ 0x2
#define RSTRQSR1_SWRR   0x800

#define SVR_SEC_MASK	0x100

#define RESET_SUCCESS   0x0
#define RESET_FAILURE   0x1

 // these defines create an interface between the bootloader and the ppa - 
 // the bootloader uses this register to pass the 64-bit load address of a module
 //   base address of the load addr register(s)
#define LOAD_ADDR_BASE  DCFG_BASE_ADDR
 //   lo-order 32-bits of the module load address
#define LOAD_OFFSET_LO  DCFG_SCRATCHRW5_OFFSET
 //   hi-order 32-bits of the module load address
#define LOAD_OFFSET_HI  DCFG_SCRATCHRW6_OFFSET

//-----------------------------------------------------------------------------

#define DDR_TEST_TABLE		(TOP_OF_OCRAM - 4096 * 3 + 1)
#define TOP_OF_STACK		(DDR_TEST_TABLE - 1)

#define CONFIG_CHIP_SELECTS_PER_CTRL		4
#define CONFIG_PHYS_64BIT

#define CCI_HN_F_0_BASE			0x04200000
#define CCI_HN_F_1_BASE			0x04210000
#define CCN_HN_F_SAM_CTL		0x8
#define CCN_HN_F_SAM_NODEID_MASK	0x7f
#define CCN_HN_F_SAM_NODEID_DDR0	0x4
#define CCN_HN_F_SAM_NODEID_DDR1	0xe
#define CCN_HN_F_SNP_DMN_CTL_OFFSET  0x200
#define CCN_HN_F_SNP_DMN_CTL_SET_OFFSET  0x210
#define CCN_HN_F_SNP_DMN_CTL_CLR_OFFSET  0x220
#define CCN_HN_F_SNP_DMN_CTL_MASK  0x80a00

#define CONFIG_SYS_FSL_DDR_ADDR		0x01080000
#define CONFIG_SYS_FSL_DDR2_ADDR	0x01090000
#define CONFIG_SYS_FSL_DDR3_ADDR	0x08210000

#define CONFIG_SYS_IFC_ADDR		0x02240000
#define CONFIG_SYS_I2C_BASE		0x02000000

#define CAAM_BASE_ADDR           0x08000000
#define CAAM_JR0_OFFSET           0x10000
#define CAAM_JR1_OFFSET           0x20000
#define CAAM_JR2_OFFSET           0x30000
#define CAAM_JR3_OFFSET           0x40000
#define CONFIG_SYS_FSL_CCSR_GUR_LE
#define CONFIG_SYS_FSL_CCSR_DDR_LE
#define CONFIG_SYS_FSL_CCSR_SEC_LE
#define CONFIG_SYS_FSL_CCSR_SFP_LE
#define CONFIG_SYS_FSL_SFP_VER_3_4
#define CONFIG_SYS_FSL_CCSR_ESDHC_LE

#define FSL_CHASSIS_RCWSR0			0x01e00100
#define FSL_CHASSIS_RCWSR0_SYS_PLL_RAT_SHIFT	2
#define FSL_CHASSIS_RCWSR0_SYS_PLL_RAT_MASK	0x1f
#define FSL_CHASSIS_RCWSR0_MEM_PLL_RAT_SHIFT	10
#define FSL_CHASSIS_RCWSR0_MEM_PLL_RAT_MASK	0x3f
#define FSL_CHASSIS_RCWSR0_MEM2_PLL_RAT_SHIFT	18
#define FSL_CHASSIS_RCWSR0_MEM2_PLL_RAT_MASK	0x3f

#ifndef __ASSEMBLER__

struct sysinfo {
	unsigned long freq_platform;
	unsigned long freq_ddr_pll0;
	unsigned long freq_ddr_pll1;
};

void get_clocks(struct sysinfo *sys);

#endif

#endif /* __LSCH3_H_ */
