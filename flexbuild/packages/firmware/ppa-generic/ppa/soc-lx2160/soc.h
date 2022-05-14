//-----------------------------------------------------------------------------
// 
// Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
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
// Author Rod Dorris <rod.dorris@nxp.com>
// 
//-----------------------------------------------------------------------------

#ifndef _SOC_H
#define	_SOC_H

 // pwr mgmt features supported in the soc-specific code:
 //   value == 0x0, the soc code does not support this feature
 //   value != 0x0, the soc code supports this feature
#define SOC_CORE_RELEASE      0x1
#define SOC_CORE_RESTART      0x0
#define SOC_CORE_OFF          0x0
#define SOC_CORE_STANDBY      0x0
#define SOC_CORE_PWR_DWN      0x0
#define SOC_CLUSTER_STANDBY   0x0
#define SOC_CLUSTER_PWR_DWN   0x0  
#define SOC_SYSTEM_STANDBY    0x0
#define SOC_SYSTEM_PWR_DWN    0x0 
#define SOC_SYSTEM_OFF        0x1 
#define SOC_SYSTEM_RESET      0x1 

#define GICR_RD_BASE_ADDR   0x06200000
#define GICR_SGI_BASE_ADDR  0x06210000

#define  OCRAM_SIZE_IN_BYTES 0x40000
#define  OCRAM_MID_ADDR      0x18020000

 // timer frequency - 25mhz
#define  COUNTER_FRQ_EL0 0x017D7840

#define CONFIG_SYS_FSL_IFC_BANK_COUNT	8

#define CONFIG_SYSCLK_FREQ	100000000

 // defines for the ddr driver
#define CONFIG_DDRCLK_FREQ	133333333

#define CONFIG_SPD_EEPROM0	0x51
#define CONFIG_SPD_EEPROM1	0x52
#define CONFIG_SPD_EEPROM2	0x53
#define CONFIG_SPD_EEPROM3	0x54
#define CONFIG_SPD_EEPROM4	0x55
#define CONFIG_SPD_EEPROM5	0x56

#define CONFIG_SYS_NUM_DDR_CTLRS	2
#define CONFIG_SYS_DIMM_SLOTS_PER_CTLR	2
#define CONFIG_FSL_DDR_BIST
#define CONFIG_SYS_FSL_HAS_CCN508
#define CONFIG_SYS_FSL_DDR_MAIN_NUM_CTRLS	2
#define CONFIG_SYS_FSL_DDR_INTLV_256B
#define CONFIG_SYS_FSL_DDR_SDRAM_BASE_PHY	0
#define CONFIG_SYS_DP_DDR_BASE_PHY		0
#define CONFIG_DP_DDR_CTRL			2
#define CONFIG_DP_DDR_NUM_CTRLS			1

 // this is the absolute top frequency (in MHZ) that
 // cluster 3 can be treated as 'normal' - above this
 // threshold, cluster 3 requires special handling....
#define CLUSTER_3_CLK_MAX 600
#define CLUSTER_3_CLK_HI  650
#define CLUSTER_3_CLK_LO  600
#define CLUSTER_3_CORES_MASK 0xC0
#define SYS_PLL_RAT_12    0x30
#define CLUSTER_3_IN_RESET  1
#define CLUSTER_3_NORMAL    0

 // cluster 3 handling no longer based on frequency, but rather on RCW[850],
 // which is bit 18 of RCWSR27
#define CLUSTER_3_RCW_BIT  0x40000

//-----------------------------------------------------------------------------

#endif // _SOC_H
