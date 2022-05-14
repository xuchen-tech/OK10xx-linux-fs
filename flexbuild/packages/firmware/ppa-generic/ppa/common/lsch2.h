//-----------------------------------------------------------------------------
// 
// Copyright (c) 2016, NXP Semiconductors
// Copyright 2017-2018 NXP Semiconductors
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

#ifndef __LSCH2_H_
#define __LSCH2_H_

//-----------------------------------------------------------------------------

 // collect all of the headers for a chassis-level-2 device
#include "aarch64.h"
#include "gicv2.h"
#include "soc.h"
#include "policy.h"
#include "runtime_data.h"

//-----------------------------------------------------------------------------

 // base addresses
#define SCFG_BASE_ADDR            0x01570000
#define DCFG_BASE_ADDR            0x01EE0000
#define RCPM_BASE_ADDR            0x01EE2000
#define DCSR_RCPM2_BASE           0x20170000
#define SYS_COUNTER_BASE          0x02b00000
#define TIMER_BASE_ADDR           0x02B00000
#define WDT1_BASE                 0x02AD0000
#define WDT3_BASE                 0x02A70000  
#define WDT4_BASE                 0x02A80000
#define WDT5_BASE                 0x02A90000
#define CONFIG_SYS_FSL_DDR_ADDR   0x01080000
#define CONFIG_SYS_I2C_BASE       0x02180000
#define CAAM_BASE_ADDR            0x01700000
#define	ESDHC_BASE_ADDR	          0x01560000  
#define SFP_BASE_ADDR             0x01E80000

 // OCRAM
#define  OCRAM_BASE_ADDR     0x10000000
#define  OCRAM_INIT_RETRY    0x2000
#define  OCRAM_REGION_LOWER  0
#define  OCRAM_REGION_UPPER  1
#define  OCRAM_REGION_ALL    2
#define  TOP_OF_OCRAM        ((OCRAM_BASE_ADDR + OCRAM_SIZE_IN_BYTES) - 1)

 // register offsets
#define SCFG_COREBCR_OFFSET       0x0680
#define SCFG_RETREQCR_OFFSET      0x0424
#define SCFG_COREPMCR_OFFSET      0x042C
#define SCFG_GIC400_ADDR_ALIGN_OFFSET  0x188
#define SCFG_SCRATCHRW2_OFFSET    0x608
#define SCFG_SCRATCHRW3_OFFSET    0x60C

#define SYS_COUNTER_CNTCR_OFFSET  0x0

#define RCPM_PCPH20SETR_OFFSET    0x0D4
#define RCPM_PCPH20CLRR_OFFSET    0x0D8
#define RCPM_POWMGTCSR_OFFSET     0x130
#define RCPM_IPPDEXPCR0_OFFSET    0x140
#define RCPM_POWMGTCSR_LPM20_REQ  0x00100000

#define BOOTLOCPTRL_OFFSET        0x604
#define BOOTLOCPTRH_OFFSET        0x600

#define DCFG_SVR_OFFSET           0x00A4
#define DCFG_BRR_OFFSET           0x00E4
#define DCFG_RSTCR_OFFSET         0x00B0
#define DCFG_COREDISR_OFFSET      0x0094
#define DCFG_RSTRQSR1_OFFSET      0x0C8
#define DCFG_RSTRQMR1_OFFSET      0x0C0

#define DCFG_DEVDISR1_OFFSET      0x70
#define DCFG_DEVDISR2_OFFSET      0x74
#define DCFG_DEVDISR3_OFFSET      0x78
#define DCFG_DEVDISR4_OFFSET      0x7C
#define DCFG_DEVDISR5_OFFSET      0x80

#define RCPM2_IPSTPCR0_OFFSET     0x8
#define RCPM2_IPSTPCR1_OFFSET     0xC
#define RCPM2_IPSTPCR2_OFFSET     0x10
#define RCPM2_IPSTPCR3_OFFSET     0x14
#define RCPM2_IPSTPCR4_OFFSET     0x28

#define RCPM2_IPSTPACKR0_OFFSET   0x18
#define RCPM2_IPSTPACKR1_OFFSET   0x1C
#define RCPM2_IPSTPACKR2_OFFSET   0x20
#define RCPM2_IPSTPACKR3_OFFSET   0x24
#define RCPM2_IPSTPACKR4_OFFSET   0x2C
#define RCPM2_POWMGTDCR_OFFSET    0x0

 // bitfield masks
#define CNTCR_EN_MASK             0x1
#define RSTCR_RESET_REQ           0x2
#define RSTRQSR1_SWRR             0x100000
#define COREPMCR_WFIL2            0x1
#define POWMGTDCR_OVRD_EN         0x80000000

#define SVR_MINOR_REV_MASK        0x0F
#define SVR_MINOR_REV_0           0x00
#define SVR_MINOR_REV_1           0x01
#define SVR_MAJOR_REV_MASK        0xF0
#define SVR_MAJOR_REV_1           0x10
#define SVR_SEC_MASK	          0x100

 // these defines create an interface between the bootloader and the ppa - 
 // the bootloader uses this register to pass the 64-bit load address of a module
 //   base address of the load addr register(s)
#define LOAD_ADDR_BASE  SCFG_BASE_ADDR
 //   lo-order 32-bits of the module load address
#define LOAD_OFFSET_LO  SCFG_SCRATCHRW2_OFFSET
 //   hi-order 32-bits of the module load address
#define LOAD_OFFSET_HI  SCFG_SCRATCHRW3_OFFSET

//-----------------------------------------------------------------------------

#define DDR_TEST_TABLE		(TOP_OF_OCRAM - 4096 * 3 + 1)
#define TOP_OF_STACK		(DDR_TEST_TABLE - 1)

#define CONFIG_SYS_FSL_CCSR_GUR_BE
#define CONFIG_SYS_FSL_CCSR_DDR_BE
#define CONFIG_SYS_FSL_CCSR_SEC_BE
#define CONFIG_SYS_FSL_CCSR_SFP_BE
#define CONFIG_SYS_FSL_SFP_VER_3_2
#define CONFIG_SYS_FSL_CCSR_ESDHC_BE

#define CONFIG_PHYS_64BIT

#ifndef CONFIG_CHIP_SELECTS_PER_CTRL
#define CONFIG_CHIP_SELECTS_PER_CTRL  4
#endif

#define  CAAM_JR0_OFFSET           0x10000
#define  CAAM_JR1_OFFSET           0x20000
#define  CAAM_JR2_OFFSET           0x30000
#define  CAAM_JR3_OFFSET           0x40000


#define FSL_CHASSIS_RCWSR0			          0x01ee0100
#define FSL_CHASSIS_RCWSR0_SYS_PLL_RAT_SHIFT  25
#define FSL_CHASSIS_RCWSR0_SYS_PLL_RAT_MASK	  0x1f
#define FSL_CHASSIS_RCWSR0_MEM_PLL_RAT_SHIFT  16
#define FSL_CHASSIS_RCWSR0_MEM_PLL_RAT_MASK   0x3f
#define FSL_CHASSIS_RCWSR0_MEM2_PLL_RAT_SHIFT 8
#define FSL_CHASSIS_RCWSR0_MEM2_PLL_RAT_MASK  0x3f

#ifndef __ASSEMBLER__

struct sysinfo {
	unsigned long freq_platform;
	unsigned long freq_ddr_pll0;
	unsigned long freq_ddr_pll1;
};

void get_clocks(struct sysinfo *sys);

#endif

#endif /* __LSCH2_H_ */
