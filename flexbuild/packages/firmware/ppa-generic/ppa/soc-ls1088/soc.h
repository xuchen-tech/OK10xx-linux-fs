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
#define SOC_CORE_RESTART      0x1
#define SOC_CORE_OFF          0x1
#define SOC_CORE_STANDBY      0x1
#define SOC_CORE_PWR_DWN      0x1
#define SOC_CLUSTER_STANDBY   0x1
#define SOC_CLUSTER_PWR_DWN   0x1
#define SOC_SYSTEM_STANDBY    0x0
#define SOC_SYSTEM_PWR_DWN    0x1
#define SOC_SYSTEM_OFF        0x1 
#define SOC_SYSTEM_RESET      0x1 

 // base addresses
#define TZASC_BASE_ADDR    0x01100000
#define TZPC_BASE_ADDR     0x02200000
#define CCI_400_BASE_ADDR  0x04090000

#define GICR_RD_BASE_ADDR  0x06100000
#define GICR_SGI_BASE_ADDR 0x06110000
#define EPU_BASE_ADDR      0x700060000
#define TPMWAKEMR0_ADDR    0x700123c50

 // epu register offsets and values
#define EPU_EPGCR_OFFSET              0x0
#define EPU_EPIMCR10_OFFSET           0x128
#define EPU_EPCTR10_OFFSET            0xa28
#define EPU_EPCCR10_OFFSET            0x828
#define EPU_EPCCR10_VAL               0xb2800000
#define EPU_EPIMCR10_VAL              0xba000000
#define EPU_EPCTR10_VAL               0x0
#define EPU_EPGCR_VAL                 (1 << 31)

 // dcfg register offsets and values
#define DCFG_DEVDISR1_OFFSET          0x70
#define DCFG_DEVDISR3_OFFSET          0x78
#define DCFG_DEVDISR4_OFFSET          0x7c
#define DCFG_DEVDISR1_SEC             (1 << 22)
#define DCFG_DEVDISR3_QBMAIN          (1 << 12)
#define DCFG_DEVDISR4_SPI_QSPI        (1 << 4 | 1 << 5)

 // pmu register offsets and values
#define PMU_PCPW20SR_OFFSET           0x830
#define PMU_CLAINACTSETR_OFFSET       0x1100
#define PMU_CLAINACTCLRR_OFFSET       0x1104
#define PMU_CLSINACTSETR_OFFSET       0x1108
#define PMU_CLSINACTCLRR_OFFSET       0x110C
#define PMU_CLL2FLUSHSETR_OFFSET      0x1110
#define PMU_CLSL2FLUSHCLRR_OFFSET     0x1114
#define PMU_CLL2FLUSHSR_OFFSET        0x1118
#define PMU_POWMGTCSR_OFFSET          0x4000
#define PMU_POWMGTCSR_VAL             (1 << 20)
#define PMU_IDLE_CLUSTER_MASK         0x2
#define PMU_FLUSH_CLUSTER_MASK        0x2
#define PMU_IDLE_CORE_MASK            0xfe

 // reset register bit
#define RSTRQMR_RPTOE_MASK             (1 << 19)

 // tzasc register offsets
#define TZASC_REG_ATTRIB_00_OFFSET    0x00110
#define TZASC_REG_ATTRIB_01_OFFSET    0x10110
#define TZASC_REGID_ACCESS_00_OFFSET  0x00114
#define TZASC_REGID_ACCESS_01_OFFSET  0x10114

 // tzpc register offsets
#define TZPCDECPROT0_SET_OFFSET   0x804
#define TZPCDECPROT1_SET_OFFSET   0x810
#define TZPCDECPROT2_SET_OFFSET   0x81C

 // retry count for cci400 status bit
#define CCI400_PEND_CNT      800

 // timer frequency - 25mhz
#define  COUNTER_FRQ_EL0 0x017D7840

 // OCRAM
#define  OCRAM_SIZE_IN_BYTES 0x20000
#define  OCRAM_MID_ADDR      0x18010000

#define CONFIG_SYSCLK_FREQ	100000000
#define CONFIG_DDRCLK_FREQ	100000000

#define CONFIG_SPD_EEPROM0	0x51

#define CONFIG_SYS_NUM_DDR_CTLRS	1
#define CONFIG_SYS_DIMM_SLOTS_PER_CTLR	1
#define CONFIG_FSL_DDR_BIST

#define CONFIG_SYS_FSL_ERRATUM_A008511
#define CONFIG_SYS_FSL_ERRATUM_A009803
#define CONFIG_SYS_FSL_ERRATUM_A009942
#define CONFIG_SYS_FSL_ERRATUM_A010165

//-----------------------------------------------------------------------------

#endif // _SOC_H
