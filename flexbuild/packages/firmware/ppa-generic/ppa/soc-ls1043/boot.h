//-----------------------------------------------------------------------------
// 
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

#ifndef _BOOT_H
#define	_BOOT_H

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
#define CCI_400_BASE_ADDR         0x01180000

 // retry count for cci400 status bit
#define CCI400_PEND_CNT           0x800

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

//-----------------------------------------------------------------------------

#endif // _BOOT_H
