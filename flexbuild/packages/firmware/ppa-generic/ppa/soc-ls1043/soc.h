//-----------------------------------------------------------------------------
// 
// Copyright (c) 2015, 2016 Freescale Semiconductor, Inc.
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
// Author Rod Dorris <rod.dorris@nxp.com>
// 
//-----------------------------------------------------------------------------

#ifndef _SOC_H
#define	_SOC_H

 // pwr mgmt features supported in the soc-specific code:
 //   value == 0x0  the soc code does not support this feature
 //   value != 0x0  the soc code supports this feature
#define SOC_CORE_RELEASE       0x1
#define SOC_CORE_RESTART       0x1
#define SOC_CORE_OFF           0x1
#define SOC_CORE_STANDBY       0x1
#define SOC_CORE_PWR_DWN       0x1  
#define SOC_CLUSTER_STANDBY    0x1
#define SOC_CLUSTER_PWR_DWN    0x1  
#define SOC_SYSTEM_STANDBY     0x1
#define SOC_SYSTEM_PWR_DWN     0x1 
#define SOC_SYSTEM_OFF         0x1 
#define SOC_SYSTEM_RESET       0x1 

 // set this switch to 1 if you need to keep the debug block
 // clocked during system power-down
#define DEBUG_ACTIVE  0

#define CCI_400_BASE_ADDR         0x01180000

#define PLAT_ARM_TZC_BASE               0x01500000
#define CONFIG_SYS_FSL_CSU_ADDR         0x01510000
#define CSU_SEC_ACCESS_REG_OFFSET       (0x0021C)
#define CCI_TERMINATE_BARRIER_TX        0x8
#define TZASC_BYPASS_MUX_DISABLE        0x04000000

 // retry count for cci400 status bit
#define CCI400_PEND_CNT           0x800

#define IPPDEXPCR_MAC1_1          0x80000000    // DEVDISR2_FMAN1_MAC1
#define IPPDEXPCR_MAC1_2          0x40000000    // DEVDISR2_FMAN1_MAC2
#define IPPDEXPCR_MAC1_3          0x20000000    // DEVDISR2_FMAN1_MAC3
#define IPPDEXPCR_MAC1_4          0x10000000    // DEVDISR2_FMAN1_MAC4
#define IPPDEXPCR_MAC1_5          0x08000000    // DEVDISR2_FMAN1_MAC5
#define IPPDEXPCR_MAC1_6          0x04000000    // DEVDISR2_FMAN1_MAC6
#define IPPDEXPCR_MAC1_9          0x00800000    // DEVDISR2_FMAN1_MAC9
#define IPPDEXPCR_I2C1            0x00080000    // DEVDISR5_I2C_1
#define IPPDEXPCR_LPUART1         0x00040000    // DEVDISR5_LPUART1
#define IPPDEXPCR_FLX_TMR1        0x00020000    // DEVDISR5_FLX_TMR
#define IPPDEXPCR_OCRAM1          0x00010000    // DEVDISR5_OCRAM1
#define IPPDEXPCR_GPIO1           0x00000040    // DEVDISR5_GPIO
#define IPPDEXPCR_FM1             0x00000008    // DEVDISR2_FMAN1

#define IPPDEXPCR_MASK1           0xFC800008    // overrides for DEVDISR2
#define IPPDEXPCR_MASK2           0x000F0040    // overriddes for DEVDISR5

#define IPSTPCR0_VALUE            0xA000C201
#define IPSTPCR1_VALUE            0x00000080
#define IPSTPCR2_VALUE            0x000C0000
#define IPSTPCR3_VALUE            0x38000000
#if (DEBUG_ACTIVE)
  #define IPSTPCR4_VALUE          0x10833BFC
#else
  #define IPSTPCR4_VALUE          0x10A33BFC
#endif

#define DEVDISR1_QE               0x00000001
#define DEVDISR1_SEC              0x00000200
#define DEVDISR1_USB1             0x00004000
#define DEVDISR1_SATA             0x00008000
#define DEVDISR1_USB2             0x00010000
#define DEVDISR1_USB3             0x00020000
#define DEVDISR1_DMA2             0x00400000
#define DEVDISR1_DMA1             0x00800000
#define DEVDISR1_ESDHC            0x20000000
#define DEVDISR1_PBL              0x80000000

#define DEVDISR2_FMAN1            0x00000080
#define DEVDISR2_FMAN1_MAC9       0x00800000
#define DEVDISR2_FMAN1_MAC6       0x04000000
#define DEVDISR2_FMAN1_MAC5       0x08000000
#define DEVDISR2_FMAN1_MAC4       0x10000000
#define DEVDISR2_FMAN1_MAC3       0x20000000
#define DEVDISR2_FMAN1_MAC2       0x40000000
#define DEVDISR2_FMAN1_MAC1       0x80000000

#define DEVDISR3_BMAN             0x00040000
#define DEVDISR3_QMAN             0x00080000
#define DEVDISR3_PEX3             0x20000000
#define DEVDISR3_PEX2             0x40000000
#define DEVDISR3_PEX1             0x80000000

#define DEVDISR4_QSPI             0x08000000
#define DEVDISR4_DUART2           0x10000000
#define DEVDISR4_DUART1           0x20000000

#define DEVDISR5_ICMMU            0x00000001
#define DEVDISR5_I2C_1            0x00000002
#define DEVDISR5_I2C_2            0x00000004
#define DEVDISR5_I2C_3            0x00000008
#define DEVDISR5_I2C_4            0x00000010
#define DEVDISR5_WDG_5            0x00000020
#define DEVDISR5_WDG_4            0x00000040
#define DEVDISR5_WDG_3            0x00000080
#define DEVDISR5_DSPI1            0x00000100
#define DEVDISR5_WDG_2            0x00000200
#define DEVDISR5_FLX_TMR          0x00000400
#define DEVDISR5_WDG_1            0x00000800
#define DEVDISR5_LPUART6          0x00001000
#define DEVDISR5_LPUART5          0x00002000
#define DEVDISR5_LPUART3          0x00008000
#define DEVDISR5_LPUART2          0x00010000
#define DEVDISR5_LPUART1          0x00020000
#define DEVDISR5_DBG              0x00200000
#define DEVDISR5_GPIO             0x00400000
#define DEVDISR5_IFC              0x00800000
#define DEVDISR5_OCRAM2           0x01000000
#define DEVDISR5_OCRAM1           0x02000000
#define DEVDISR5_LPUART4          0x10000000
#define DEVDISR5_DDR              0x80000000
#define DEVDISR5_MEM              0x80000000

#define DEVDISR1_VALUE            0xA0C3C201
#define DEVDISR2_VALUE            0xCC0C0080
#define DEVDISR3_VALUE            0xE00C0000
#define DEVDISR4_VALUE            0x38000000
#if (DEBUG_ACTIVE)
  #define DEVDISR5_VALUE          0x10833BFC
#else
  #define DEVDISR5_VALUE          0x10A33BFC
#endif

 // 25mhz
#define  COUNTER_FRQ_EL0  0x017D7840    

 //----------------------------------------------------------------------------

 // base addresses
#define GICD_BASE_ADDR_4K  0x01401000
#define GICC_BASE_ADDR_4K  0x01402000
#define GICD_BASE_ADDR_64K 0x01410000
#define GICC_BASE_ADDR_64K 0x01420000

#define GIC400_ADDR_ALIGN_4KMODE_MASK  0x80000000
#define GIC400_ADDR_ALIGN_4KMODE_EN    0x80000000
#define GIC400_ADDR_ALIGN_4KMODE_DIS   0x0

 // OCRAM
#define  OCRAM_SIZE_IN_BYTES 0x20000
#define  OCRAM_MID_ADDR      0x10010000

 // defines for the ddr driver
#define CONFIG_SYS_FSL_ERRATUM_A009942
#define CONFIG_SYS_FSL_ERRATUM_A009663

#define CONFIG_SYSCLK_FREQ	100000000
#define CONFIG_DDRCLK_FREQ	100000000

#define CONFIG_SYS_NUM_DDR_CTLRS	1
#define CONFIG_SYS_DIMM_SLOTS_PER_CTLR	1
#define CONFIG_FSL_DDR_BIST
#define CONFIG_SYS_DDR_RAW_TIMING

//-----------------------------------------------------------------------------

#endif // _SOC_H
