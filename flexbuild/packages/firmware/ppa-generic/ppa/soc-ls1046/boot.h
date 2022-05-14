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

 // base addresses
#define SCFG_BASE_ADDR            0x01570000
#define DCFG_BASE_ADDR            0x01EE0000
#define RCPM_BASE_ADDR            0x01EE2000
#define SYS_COUNTER_BASE          0x02B00000
#define WDT1_BASE                 0x02AD0000
#define WDT3_BASE                 0x02A70000
#define WDT4_BASE                 0x02A80000
#define WDT5_BASE                 0x02A90000
#define TIMER_BASE_ADDR           0x02B00000
#define TZASC_BASE_ADDR           0x01500000
#define CSU_BASE_ADDR             0x01510000
#define CCI_400_BASE_ADDR         0x01180000

 // retry count for cci400 status bit
#define CCI400_PEND_CNT           0x800

 // register offsets
#define SCFG_COREBCR_OFFSET       0x0680
#define SCFG_RETREQCR_OFFSET      0x0424
#define SCFG_COREPMCR_OFFSET      0x042C
#define COREPMCR_WFIL2EN	  0x1

#define SYS_COUNTER_CNTCR_OFFSET  0x0

#define RCPM_PCPH20SETR_OFFSET    0x0D4
#define RCPM_PCPH20CLRR_OFFSET    0x0D8
#define RCPM_POWMGTCSR_OFFSET     0x130
#define RCPM_IPPDEXPCR0_OFFSET    0x140
#define RCPM_POWMGTCSR_LPM20_REQ  0x00100000

#define BOOTLOCPTRL_OFFSET        0x604
#define BOOTLOCPTRH_OFFSET        0x600

#define DCFG_BRR_OFFSET           0x00E4
#define DCFG_SVR_OFFSET           0x00A4
#define DCFG_RSTCR_OFFSET         0x00B0
#define DCFG_COREDISR_OFFSET      0x0094
#define DCFG_RSTRQSR1_OFFSET      0x0C8
#define DCFG_RSTRQMR1_OFFSET      0x0C0

#define SVR_SEC_MASK	          0x100

 // bitfield masks
#define CNTCR_EN_MASK             0x1
#define RSTCR_RESET_REQ           0x2
#define RSTRQSR1_SWRR             0x100000
#define COREPMCR_WFIL2            0x1
#define POWMGTDCR_OVRD_EN         0x80000000

 // 25mhz
#define  COUNTER_FRQ_EL0  0x017D7840    

 //----------------------------------------------------------------------------

 // base addresses
#if (SIMULATOR_BUILD)
#define GICD_BASE_ADDR  0x01401000
#define GICC_BASE_ADDR  0x01402000
#else
#define GICD_BASE_ADDR  0x01410000
#define GICC_BASE_ADDR  0x01420000
#endif

 // OCRAM
#define  OCRAM_BASE_ADDR     0x10000000
#define  OCRAM_MID_ADDR      0x10010000
#define  OCRAM_SIZE_IN_BYTES 0x20000
#define  OCRAM_INIT_RETRY    0x2000
#define  OCRAM_REGION_LOWER  0
#define  OCRAM_REGION_UPPER  1

//-----------------------------------------------------------------------------

#define CPUECTLR_EL1		S3_1_C15_C2_1

#endif // _SOC_H
