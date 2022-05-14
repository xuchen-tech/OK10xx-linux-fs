//-----------------------------------------------------------------------------
// 
// Copyright (c) 2016 Freescale Semiconductor, Inc.
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

  .section .text, "ax"

//-----------------------------------------------------------------------------

#include "lsch2.h"
#include "psci.h"
#include "soc.mac"

//-----------------------------------------------------------------------------

#define SWLPM20_WA 1

#define DAIF_DATA  AUX_01_DATA

#define DEVDISR1_MASK_OFFSET  0x0
#define DEVDISR5_MASK_OFFSET  0x8
#define CPUACTLR_DATA_OFFSET  0x10

#define IPSTPACK_RETRY_CNT    0x10000
#define DDR_SLEEP_RETRY_CNT   0x10000
#define CPUACTLR_EL1          S3_1_C15_C2_0
#define CPUACTLR_L1PCTL_MASK  0x0000E000
#define DDR_SDRAM_CFG_2_FRCSR 0x80000000
#define DDR_SDRAM_CFG_2_OFFSET 0x114
#define DDR_CNTRL_BASE_ADDR   0x01080000

#define ERROR_DDR_SLEEP       -1
#define ERROR_DDR_WAKE        -2
#define ERROR_NO_QUIESCE      -3

//-----------------------------------------------------------------------------

.global _soc_sys_reset
.global _soc_ck_disabled
.global _soc_set_start_addr
.global _soc_get_start_addr
.global _soc_core_release
.global _soc_core_entr_stdby
.global _soc_core_exit_stdby
.global _soc_core_entr_pwrdn
.global _soc_core_exit_pwrdn
.global _soc_clstr_entr_stdby
.global _soc_clstr_exit_stdby
.global _soc_clstr_entr_pwrdn
.global _soc_clstr_exit_pwrdn
.global _soc_sys_entr_stdby
.global _soc_sys_exit_stdby
.global _soc_sys_entr_pwrdn
.global _soc_sys_exit_pwrdn
.global _soc_sys_off
.global _soc_core_entr_off
.global _soc_core_exit_off
.global _soc_core_phase1_off
.global _soc_core_phase2_off
.global _soc_core_phase1_clnup
.global _soc_core_phase2_clnup
.global _soc_core_restart

.global _get_current_mask
.global _getCoreData
.global _setCoreData

.global _soc_init_start
.global _soc_init_finish
.global _soc_init_percpu
.global _set_platform_security
.global _soc_exit_boot_svcs
.global _getGICC_BaseAddr
.global _getGICD_BaseAddr

//-----------------------------------------------------------------------------

 // this function performs any soc-specific initialization that is needed on 
 // a per-core basis
 // in:  none
 // out: none
 // uses none
_soc_init_percpu:

    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function puts the calling core into standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0
_soc_core_entr_stdby:

     // X0 = core mask lsb

     // IRQ taken to EL3, set SCR_EL3[IRQ]
    mrs  x0, SCR_EL3
    orr  x0, x0, #SCR_IRQ_MASK
    msr  SCR_EL3, x0

    dsb  sy
    isb
    wfi
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function performs any necessary cleanup after the calling core has
 // exited standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0
_soc_core_exit_stdby:

     // X0 = core mask lsb

     // clear SCR_EL3[IRQ]
    mrs  x0, SCR_EL3
    bic  x0, x0, #SCR_IRQ_MASK
    msr  SCR_EL3, x0

    isb
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function puts the calling core into a power-down state
 // ph20 is defeatured for this device, so pw15 is the lowest core pwr state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1
_soc_core_entr_pwrdn:

     // X0 = core mask lsb

     // mask interrupts by setting DAIF[7:4] to 'b1111
    mrs  x1, DAIF
    ldr  x0, =DAIF_SET_MASK
    orr  x1, x1, x0
    msr  DAIF, x1 

     // IRQ taken to EL3, set SCR_EL3[IRQ]
    mrs  x0, SCR_EL3
    orr  x0, x0, #SCR_IRQ_MASK
    msr  SCR_EL3, x0

    dsb  sy
    isb
    wfi
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function cleans up after a core exits power-down
 // in:  x0 = core mask lsb
 // out: none
 // uses x0
_soc_core_exit_pwrdn:

     // X0 = core mask lsb

     // clear SCR_EL3[IRQ]
    mrs  x0, SCR_EL3
    bic  x0, x0, #SCR_IRQ_MASK
    msr  SCR_EL3, x0

    isb
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function puts the cluster into a standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2
_soc_clstr_entr_stdby:
    mov  x2, x30

     // x0 = core mask lsb

     // put the final core of the cluster into a standby state
    bl   _soc_core_entr_stdby
    mov  x30, x2
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function exits the cluster from a standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2
_soc_clstr_exit_stdby:
    mov  x2, x30

     // x0 = core mask lsb

    bl   _soc_core_exit_stdby
    mov  x30, x2
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function puts the calling core into a power-down state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x9
_soc_clstr_entr_pwrdn:
    mov   x9, x30

     // x0 = core mask lsb

     // all we need to do is power-down the final core of this cluster
    bl   _soc_core_entr_pwrdn
    mov  x30, x9
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function cleans up after a cluster exits power-down
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4
_soc_clstr_exit_pwrdn:
     mov  x4, x30

     // x0 = core mask lsb

     // cleanup from the core power-down
    bl   _soc_core_exit_pwrdn
    mov  x30, x4
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function puts the system into a standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2
_soc_sys_entr_stdby:
    mov  x2, x30

     // x0 = core mask lsb

     // put the final core of the cluster into a standby state
    bl   _soc_core_entr_stdby
    mov  x30, x2
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function exits the system from a standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2
_soc_sys_exit_stdby:
    mov  x2, x30

     // x0 = core mask lsb

    bl   _soc_core_exit_stdby
    mov  x30, x2
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function puts the calling core, and potentially the soc, into a
 // low-power state
 // in:  x0 = core mask lsb
 // out: x0 = 0, success
 //      x0 < 0, failure
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x13, x16, x17
_soc_sys_entr_pwrdn:
    mov  x10, x30

     // x0 = core mask lsb

     // save DAIF
    mrs  x2, DAIF
    mov  x6, x2
    mov  x1, #DAIF_DATA
    bl   _setCoreData

     // mask interrupts at the core
    mov  x0, #DAIF_SET_MASK
    orr  x6, x6, x0
    msr  DAIF, x6

     // disable icache, dcache, mmu @ EL1
    mov  x1, #SCTLR_I_C_M_MASK
    mrs  x0, sctlr_el1
    bic  x0, x0, x1
    msr  sctlr_el1, x0

     // disable dcache for EL3
    mrs x1, SCTLR_EL3
    bic x1, x1, #SCTLR_C_MASK
     // make sure icache is enabled
    orr x1, x1, #SCTLR_I_MASK      
    msr SCTLR_EL3, x1 
    isb

     // clean/invalidate the dcache
    mov x0, #1
    bl  _cln_inv_all_dcache

     // set WFIL2_EN in SCFG_COREPMCR
    mov  x0, #SCFG_COREPMCR_OFFSET
    mov  x1, #COREPMCR_WFIL2
    bl   write_reg_scfg

     // set OVRD_EN in RCPM2_POWMGTDCR
    mov  x0, #RCPM2_POWMGTDCR_OFFSET
    mov  x1, #POWMGTDCR_OVRD_EN
    bl   write_reg_rcpm2

     // IRQ taken to EL3, set SCR_EL3[IRQ]
    mrs  x0, SCR_EL3
    orr  x0, x0, #SCR_IRQ_MASK
    msr  SCR_EL3, x0

     // read IPPDEXPCR0 @ RCPM_IPPDEXPCR0
    mov  x0, #RCPM_IPPDEXPCR0_OFFSET
    bl   read_reg_rcpm
    mov  w7, w0

     // w7 = IPPDEXPCR

     // get DEVDISR1 mask
#if (DATA_LOC == DATA_IN_DDR)
     // request base address
    mov  x13, #BC_PSCI_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x2, x13
#else
    ldr  x2, =BC_PSCI_BASE
#endif
    add  x2, x2, #AUX_01_DATA

    mov  w5, wzr
    ldr  w6, =IPPDEXPCR_PFE_MASK
    and  w6, w6, w7
    cbz  w6, 10f

    orr  w5, w5, #DEVDISR1_PFE
10:
    str  w5, [x2, #DEVDISR1_MASK_OFFSET]

     // get DEVDISR5 mask
    mov  w5, wzr
    mov  w6, #IPPDEXPCR_I2C1
    and  w6, w6, w7
    cbz  w6, 12f

    orr  w5, w5, #DEVDISR5_I2C_1
12:
    mov  w6, #IPPDEXPCR_FLEXTIMER1
    and  w6, w6, w7
    cbz  w6, 14f

    orr  w5, w5, #DEVDISR5_FLEXTIMER
14:
    mov  w6, #IPPDEXPCR_OCRAM1
    and  w6, w6, w7
    cbz  w6, 16f

    orr  w5, w5, #DEVDISR5_OCRAM1
16:
    mov  w6, #IPPDEXPCR_GPIO1
    and  w6, w6, w7
    cbz  w6, 18f

    orr  w5, w5, #DEVDISR5_GPIO
18:
    str  w5, [x2, #DEVDISR5_MASK_OFFSET]

    ldr  w6, [x2, #DEVDISR1_MASK_OFFSET]

     // w6 = DEVDISR1 override mask

     // write IPSTPCR0 - overrides possible
    ldr  x0, =RCPM2_IPSTPCR0_OFFSET
    ldr  w1, =IPSTPCR0_VALUE
    bic  w1, w1, w6
    bl   write_reg_rcpm2

     // write IPSTPCR3 - no overrides
    ldr  x0, =RCPM2_IPSTPCR3_OFFSET
    ldr  w1, =IPSTPCR3_VALUE
    bl   write_reg_rcpm2

     // w5 = DEVDISR5 override mask

     // write IPSTPCR4 - overrides possible
    ldr  x0, =RCPM2_IPSTPCR4_OFFSET
    ldr  w1, =IPSTPCR4_VALUE
    bic  w1, w1, w5
    bl   write_reg_rcpm2

     // w6 = DEVDISR1 override mask

     // poll on IPSTPACK0
    mov  x3, #RCPM2_IPSTPACKR0_OFFSET
    ldr  w4, =IPSTPCR0_VALUE
    bic  w4, w4, w6
    ldr  x7, =IPSTPACK_RETRY_CNT
3:
    mov  x0, x3
    bl   read_reg_rcpm2
    cmp  w0, w4
    b.eq 14f
    sub  x7, x7, #1
    cbnz x7, 3b

14:
     // poll on IPSTPACK3
    mov  x3, #RCPM2_IPSTPACKR3_OFFSET
    ldr  w4, =IPSTPCR3_VALUE
    ldr  x7, =IPSTPACK_RETRY_CNT
6:
    mov  x0, x3
    bl   read_reg_rcpm2
    cmp  w0, w4
    b.eq 17f
    sub  x7, x7, #1
    cbnz x7, 6b

     // w5 = DEVDISR5 override mask
17:
     // poll on IPSTPACK4
    mov  x3, #RCPM2_IPSTPACKR4_OFFSET
    ldr  w4, =IPSTPCR4_VALUE
    bic  w4, w4, w5
    ldr  x7, =IPSTPACK_RETRY_CNT
7:
    mov  x0, x3
    bl   read_reg_rcpm2
    cmp  w0, w4
    b.eq 18f
    sub  x7, x7, #1
    cbnz x7, 7b

18:
#if (DATA_LOC == DATA_IN_DDR)
     // request base address
    mov  x13, #BC_PSCI_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x7, x13
#else
    ldr  x7, =BC_PSCI_BASE
#endif
    add  x7, x7, #AUX_01_DATA

     // w6 = DEVDISR1 override mask
     // x7 = [soc_data_area]

     // save DEVDISR1 and load new value
    mov  x0, #DCFG_DEVDISR1_OFFSET
    bl   read_reg_dcfg
    mov  w13, w0
    mov  x0, #DCFG_DEVDISR1_OFFSET
    ldr  w1, =DEVDISR1_VALUE
    bic  w1, w1, w6
    bl   write_reg_dcfg

     // x7 = [soc_data_area]

     // save DEVDISR4 and load new value
    mov  x0, #DCFG_DEVDISR4_OFFSET
    bl   read_reg_dcfg
    mov  w16, w0
    mov  x0, #DCFG_DEVDISR4_OFFSET
    ldr  w1, =DEVDISR4_VALUE
    bl   write_reg_dcfg

     // w5 = DEVDISR5 override mask

     // save DEVDISR5 and load new value
    mov  x0, #DCFG_DEVDISR5_OFFSET
    bl   read_reg_dcfg
    mov  w17, w0
    mov  x0, #DCFG_DEVDISR5_OFFSET
    ldr  w1, =DEVDISR5_VALUE
    bic  w1, w1, w5
    bl   write_reg_dcfg

     // x7 = [soc_data_area]

     // save cpuactlr and disable data prefetch
    mrs  x0, CPUACTLR_EL1
    str  x0, [x7, #CPUACTLR_DATA_OFFSET]
    bic  x0, x0, #CPUACTLR_L1PCTL_MASK
    msr  CPUACTLR_EL1, x0

     // w5 = DEVDISR5 override mask

     // setup registers for cache-only execution
    ldr  w6, =IPSTPCR4_VALUE
    bic  w5, w6, w5
    mov  x6, #DDR_CNTRL_BASE_ADDR
    mov  x7, #DCSR_RCPM2_BASE
    mov  x8, #DCFG_BASE_ADDR
    dsb sy
    isb

     // w5  = ipstpcr4 (IPSTPCR4_VALUE bic DEVDISR5_MASK)
     // x6  = DDR_CNTRL_BASE_ADDR
     // x7  = DCSR_RCPM2_BASE
     // x8  = DCFG_BASE_ADDR
     // w13 = DEVDISR1 saved value
     // w16 = DEVDISR4 saved value
     // w17 = DEVDISR5 saved value

     // enter the cache-only sequence
    bl   final_pwrdown

     // when we are here, the core has come out of wfi and the SoC is back up

    mov  x30, x10
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function performs any necessary cleanup after the soc has exited
 // a low-power state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1
_soc_sys_exit_pwrdn:

     // x0 = core mask lsb

     // restore cpuactlr_el1
#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #BC_PSCI_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x1, x13
    mov  x30, x0
#else
    ldr  x1, =BC_PSCI_BASE
#endif
    add  x1, x1, #AUX_01_DATA

    ldr  x0, [x1, #CPUACTLR_DATA_OFFSET]
    msr  CPUACTLR_EL1, x0

     // clear SCR_EL3[IRQ]
    mrs  x0, SCR_EL3
    bic  x0, x0, #0x2
    msr  SCR_EL3, x0

    ret

//-----------------------------------------------------------------------------

 // part of SYSTEM_OFF
 // this function turns off the SoC clocks
 // Note: this function is not intended to return, and the only allowable
 //       recovery is POR
 // in:  none
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, [x13, x14, x15]
_soc_sys_off:

     // mask interrupts at the core
    mrs  x1, DAIF
    mov  x0, #DAIF_SET_MASK
    orr  x0, x1, x0
    msr  DAIF, x0

     // disable icache, dcache, mmu @ EL1
    mov  x1, #SCTLR_I_C_M_MASK
    mrs  x0, sctlr_el1
    bic  x0, x0, x1
    msr  sctlr_el1, x0

     // disable dcache for EL3
    mrs x1, SCTLR_EL3
    bic x1, x1, #SCTLR_C_MASK
     // make sure icache is enabled
    orr x1, x1, #SCTLR_I_MASK      
    msr SCTLR_EL3, x1 
    isb

     // set WFIL2_EN in SCFG_COREPMCR
    mov  x0, #SCFG_COREPMCR_OFFSET
    mov  x1, #COREPMCR_WFIL2
    bl   write_reg_scfg

     // set OVRD_EN in RCPM2_POWMGTDCR
    mov  x0, #RCPM2_POWMGTDCR_OFFSET
    mov  x1, #POWMGTDCR_OVRD_EN
    bl   write_reg_rcpm2

     // read IPPDEXPCR0 @ RCPM_IPPDEXPCR0
    mov  x0, #RCPM_IPPDEXPCR0_OFFSET
    bl   read_reg_rcpm
    mov  w7, w0

     // w7 = IPPDEXPCR

     // get DEVDISR1 mask
#if (DATA_LOC == DATA_IN_DDR)
     // request base address
    mov  x13, #BC_PSCI_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x2, x13
#else
    ldr  x2, =BC_PSCI_BASE
#endif
    add  x2, x2, #AUX_01_DATA

    mov  w5, wzr
    ldr  w6, =IPPDEXPCR_PFE_MASK
    and  w6, w6, w7
    cbz  w6, 10f

    orr  w5, w5, #DEVDISR1_PFE
10:
    str  w5, [x2, #DEVDISR1_MASK_OFFSET]

     // get DEVDISR5 mask
    mov  w5, wzr
    mov  w6, #IPPDEXPCR_I2C1
    and  w6, w6, w7
    cbz  w6, 12f

    orr  w5, w5, #DEVDISR5_I2C_1
12:
    mov  w6, #IPPDEXPCR_FLEXTIMER1
    and  w6, w6, w7
    cbz  w6, 14f

    orr  w5, w5, #DEVDISR5_FLEXTIMER
14:
    mov  w6, #IPPDEXPCR_OCRAM1
    and  w6, w6, w7
    cbz  w6, 16f

    orr  w5, w5, #DEVDISR5_OCRAM1
16:
    mov  w6, #IPPDEXPCR_GPIO1
    and  w6, w6, w7
    cbz  w6, 18f

    orr  w5, w5, #DEVDISR5_GPIO
18:
    str  w5, [x2, #DEVDISR5_MASK_OFFSET]

    ldr  w6, [x2, #DEVDISR1_MASK_OFFSET]

     // w6 = DEVDISR1 override mask

     // write IPSTPCR0 - overrides possible
    ldr  x0, =RCPM2_IPSTPCR0_OFFSET
    ldr  w1, =IPSTPCR0_VALUE
    bic  w1, w1, w6
    bl   write_reg_rcpm2

     // write IPSTPCR3 - no overrides
    ldr  x0, =RCPM2_IPSTPCR3_OFFSET
    ldr  w1, =IPSTPCR3_VALUE
    bl   write_reg_rcpm2

     // w5 = DEVDISR5 override mask

     // write IPSTPCR4 - overrides possible
    ldr  x0, =RCPM2_IPSTPCR4_OFFSET
    ldr  w1, =IPSTPCR4_VALUE
    bic  w1, w1, w5
    bl   write_reg_rcpm2

     // w6 = DEVDISR1 override mask

     // poll on IPSTPACK0
    mov  x3, #RCPM2_IPSTPACKR0_OFFSET
    ldr  w4, =IPSTPCR0_VALUE
    bic  w4, w4, w6
    ldr  x7, =IPSTPACK_RETRY_CNT
3:
    mov  x0, x3
    bl   read_reg_rcpm2
    cmp  w0, w4
    b.eq 14f
    sub  x7, x7, #1
    cbnz x7, 3b

14:
     // poll on IPSTPACK3
    mov  x3, #RCPM2_IPSTPACKR3_OFFSET
    ldr  w4, =IPSTPCR3_VALUE
    ldr  x7, =IPSTPACK_RETRY_CNT
6:
    mov  x0, x3
    bl   read_reg_rcpm2
    cmp  w0, w4
    b.eq 17f
    sub  x7, x7, #1
    cbnz x7, 6b

     // w5 = DEVDISR5 override mask
17:
     // poll on IPSTPACK4
    mov  x3, #RCPM2_IPSTPACKR4_OFFSET
    ldr  w4, =IPSTPCR4_VALUE
    bic  w4, w4, w5
    ldr  x7, =IPSTPACK_RETRY_CNT
7:
    mov  x0, x3
    bl   read_reg_rcpm2
    cmp  w0, w4
    b.eq 18f
    sub  x7, x7, #1
    cbnz x7, 7b

18:
#if (DATA_LOC == DATA_IN_DDR)
     // request base address
    mov  x13, #BC_PSCI_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x7, x13
#else
    ldr  x7, =BC_PSCI_BASE
#endif
    add  x7, x7, #AUX_01_DATA

     // w6 = DEVDISR1 override mask
     // x7 = [soc_data_area]

     // DEVDISR1 - load new value
    mov  x0, #DCFG_DEVDISR1_OFFSET
    bl   read_reg_dcfg
    mov  x0, #DCFG_DEVDISR1_OFFSET
    ldr  w1, =DEVDISR1_VALUE
    bic  w1, w1, w6
    bl   write_reg_dcfg

     // x7 = [soc_data_area]

     // DEVDISR4 - load new value
    mov  x0, #DCFG_DEVDISR4_OFFSET
    bl   read_reg_dcfg
    mov  x0, #DCFG_DEVDISR4_OFFSET
    ldr  w1, =DEVDISR4_VALUE
    bl   write_reg_dcfg

     // w5 = DEVDISR5 override mask

     // DEVDISR5 - load new value
    mov  x0, #DCFG_DEVDISR5_OFFSET
    bl   read_reg_dcfg
    mov  x0, #DCFG_DEVDISR5_OFFSET
    ldr  w1, =DEVDISR5_VALUE
    bic  w1, w1, w5
    bl   write_reg_dcfg

     // x7 = [soc_data_area]

     // disable data prefetch
    mrs  x0, CPUACTLR_EL1
    bic  x0, x0, #CPUACTLR_L1PCTL_MASK
    msr  CPUACTLR_EL1, x0

     // w5 = DEVDISR5 override mask

     // setup registers for cache-only execution
    ldr  w6, =IPSTPCR4_VALUE
    bic  w5, w6, w5
    mov  x6, #DDR_CNTRL_BASE_ADDR
    mov  x7, #DCSR_RCPM2_BASE
    mov  x8, #DCFG_BASE_ADDR
    dsb sy
    isb

     // w5  = ipstpcr4 (IPSTPCR4_VALUE bic DEVDISR5_MASK)
     // x6  = DDR_CNTRL_BASE_ADDR
     // x7  = DCSR_RCPM2_BASE
     // x8  = DCFG_BASE_ADDR

     // enter the cache-only sequence - there is no return from this function
    b    final_shutdown

//-----------------------------------------------------------------------------

 // this function resets the system via SoC-specific methods
 // in:  none
 // out: x0 = PSCI_SUCCESS
 //      x0 = PSCI_INTERNAL_FAILURE
 // uses x0, x1, x2, x3, x4
_soc_sys_reset:

    ldr  x2, =DCFG_BASE_ADDR

     // make sure the mask is cleared in the reset request mask register
    mov  w1, wzr
    str  w1, [x2, #DCFG_RSTRQMR1_OFFSET]

     // x2 = DCFG_BASE_ADDR

     // set the reset request
    ldr  w1, =RSTCR_RESET_REQ
    ldr  x4, =DCFG_RSTCR_OFFSET
    rev  w0, w1
    str  w0, [x2, x4]

     // x2 = DCFG_BASE_ADDR
     // x4 = DCFG_RSTCR_OFFSET

     // just in case this address range is mapped as cacheable,
     // flush the write out of the dcaches
    add  x4, x2, x4
    dc   cvac, x4
    dsb  st
    isb

     // x2 = DCFG_BASE_ADDR

     // now poll on the status bit til it goes high
    ldr  w3, =RESET_RETRY_CNT
    ldr  w4, =RSTRQSR1_SWRR
1:
    ldr  w0, [x2, #DCFG_RSTRQSR1_OFFSET]
    rev  w1, w0
     // see if we have exceeded the retry count
    cbz  w3, 2f
     // decrement retry count and test return value
    sub  w3, w3, #1
    tst  w1, w4
    b.eq 1b

     // if the reset occurs, the following code is not expected
     // to execute.....

     // if we are here then the status bit is set
    ldr  x0, =PSCI_SUCCESS
    b    3f
2:
     // signal failure and return
    ldr  x0, =PSCI_INTERNAL_FAILURE
3:
    ret

//-----------------------------------------------------------------------------

 // this function determines if a core is disabled via COREDISR
 // in:  w0  = core_mask_lsb
 // out: w0  = 0, core not disabled
 //      w0 != 0, core disabled
 // uses x0, x1, x2
_soc_ck_disabled:

     // alwayd return 'disabled'
    mov  w0, #0
    ret

//-----------------------------------------------------------------------------

 // part of CPU_ON
 // this function releases a secondary core from reset
 // in:   x0 = core_mask_lsb
 // out:  none
 // uses: x0, x1, x2, x3
_soc_core_release:

    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function programs ARM core registers in preparation for shutting down
 // the core
 // in:   x0 = core_mask_lsb
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8
_soc_core_phase1_off:

    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function programs SoC & GIC registers in preparation for shutting down
 // the core
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6
_soc_core_phase2_off:

    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function performs the final steps to shutdown the core
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4, x5
_soc_core_entr_off:

    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function starts the process of starting a core back up
 // in:  none
 // out: none
 // uses x0, x1
_soc_core_exit_off:

    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function cleans up from phase 1 of the core shutdown sequence
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x3
_soc_core_phase1_clnup:

    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function cleans up from phase 2 of the core shutdown sequence
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4
_soc_core_phase2_clnup:

    ret

//-----------------------------------------------------------------------------

 // part of CPU_ON
 // this function restarts a core shutdown via _soc_core_entr_off
 // in:  x0 = core mask lsb (of the target cpu)
 // out: x0 == 0, on success
 //      x0 != 0, on failure
 // uses x0, x1, x2, x3, x4, x5
_soc_core_restart:

    ret

//-----------------------------------------------------------------------------

 // this function loads a 64-bit execution address of the core in the soc registers
 // BOOTLOCPTRL/H
 // in:  x0, 64-bit address to write to BOOTLOCPTRL/H
 // uses x0, x1, x2, x3 
_soc_set_start_addr:
     // get the 64-bit base address of the scfg block
    ldr  x2, =SCFG_BASE_ADDR

     // write the 32-bit BOOTLOCPTRL register (offset 0x604 in the scfg block)
    mov  x1, x0
    rev  w3, w1
    str  w3, [x2, #BOOTLOCPTRL_OFFSET]

     // write the 32-bit BOOTLOCPTRH register (offset 0x600 in the scfg block)
    lsr  x1, x0, #32
    rev  w3, w1
    str  w3, [x2, #BOOTLOCPTRH_OFFSET]
    ret

//-----------------------------------------------------------------------------

 // this function returns a 64-bit execution address of the core in x0
 // out: x0, address found in BOOTLOCPTRL/H
 // uses x0, x1, x2 
_soc_get_start_addr:
     // get the 64-bit base address of the scfg block
    ldr  x1, =SCFG_BASE_ADDR

     // read the 32-bit BOOTLOCPTRL register (offset 0x604 in the scfg block)
    ldr  w0, [x1, #BOOTLOCPTRL_OFFSET]
     // swap bytes for BE
    rev  w2, w0

     // read the 32-bit BOOTLOCPTRH register (offset 0x600 in the scfg block)
    ldr  w0, [x1, #BOOTLOCPTRH_OFFSET]
    rev  w1, w0
     // create a 64-bit BOOTLOCPTR address
    orr  x0, x2, x1, LSL #32
    ret

//-----------------------------------------------------------------------------

 // this function programs the GIC for a Group0 SGI targeted at the core
 // in:  none
 // out: none
 // uses x0, x1, x2
enableSGI:

     // set interrupt ID 15 to group 0 by setting GICD_IGROUP0[15] to 0 
    ldr  w2, =GICD_BASE_ADDR
    ldr  w0, [x2, #GICD_IGROUPR0_OFFSET]
    ldr  w1, =GICD_IGROUP0_SGI15
    bic  w0, w0, w1
    str  w0, [x2, #GICD_IGROUPR0_OFFSET]

     // x2 = GICD_BASE_ADDR

     // enable forwarding of group 0 interrupts by setting GICD_CTLR[0] to 1
    ldr  w0, [x2, #GICD_CTLR_OFFSET]
    ldr  w1, =GICD_CTLR_EN_GRP0
    orr  w0, w0, w1
    str  w0, [x2, #GICD_CTLR_OFFSET]

     // enable signaling of group 0 by setting GICC_CTLR[1:0] to 0b01 
    ldr    w2, =GICC_BASE_ADDR
    ldr    w0, [x2, #GICC_CTLR_OFFSET]
    ldr    w1, =GICC_CTLR_EN_GRP0
    bfxil  w0, w1, #0, #2
    str    w0, [x2, #GICC_CTLR_OFFSET]

    isb
    ret

//-----------------------------------------------------------------------------

 // this function enables/disables the SoC retention request for the core,
 // using a read-modify-write methodology
 // in:  w0 = core mask (msb)
 //      w1 = set or clear bit specified in core mask (0 = clear, 1 = set)
 // out: none
 // uses x0, x1, x2, x3
retention_ctrl:
    ldr  w2, =SCFG_BASE_ADDR
    ldr  w3, [x2, #SCFG_RETREQCR_OFFSET]

     // byte swap for BE
    rev  w3, w3
    bic  w3, w3, w0
    cmp  w1, #0
    b.eq 1f
    orr  w3, w3, w0
1:
    rev  w3, w3
    str  w3, [x2, #SCFG_RETREQCR_OFFSET]
    ret

//-----------------------------------------------------------------------------

 // this function clears a pending SGI 15 interrupt
 // in:  none
 // out: none
 // uses x0, x1
clearSGI:
    ldr  w1, =GICD_BASE_ADDR
    ldr  w0, =GICD_CPENDSGIR_CLR_MASK
    str  w0, [x1, #GICD_CPENDSGIR3_OFFSET]
    ret

//-----------------------------------------------------------------------------

 // this function returns the lsb bit mask corresponding to the current core
 // the mask is returned in w0.
 // this bit mask references the core in the SoC registers such as
 // BRR, COREDISR where the LSB represents core0
 // in:   none
 // out:  w0 = core mask
 // uses: x0, x1, x2
_get_current_mask:

     // get the cores mpidr value
    mrs  x0, MPIDR_EL1

     // generate a lsb-based mask for the core - this algorithm assumes 4 cores
     // per cluster, and must be adjusted if that is not the case
     // SoC core = ((cluster << 2) + core)
     // mask = (1 << SoC core)
    mov   w1, wzr
    mov   w2, wzr
    bfxil w1, w0, #8, #8  // extract cluster
    bfxil w2, w0, #0, #8  // extract cpu #
    lsl   w1, w1, #2
    add   w1, w1, w2
    mov   w2, #0x1
    lsl   w0, w2, w1
    ret

//-----------------------------------------------------------------------------

 // this function starts the initialization tasks of the soc, using secondary cores
 // if they are available
 // in: 
 // out: 
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11
_soc_init_start:
    mov   x11, x30

     // init the task flags
    bl  _init_task_flags   // 0-1

     // save start address
    bl   _soc_get_start_addr   // 0-2
    mov  x1, x0
    mov  x0, #BOOTLOC_OFFSET
    bl   _set_global_data

     // see if we are initializing ocram
    ldr x0, =POLICY_USING_ECC
    cbz x0, 1f

     // initialize the OCRAM for ECC

     // there are no secondary cores available, so the
     // boot core will have to init upper ocram
    bl  _ocram_init_upper // 0-10
3:
     // there are no secondary cores available, so the
     // boot core will have to init lower ocram
    bl  _ocram_init_lower // 0-10
    b   1f
7:
     // clear bootlocptr
    mov  x0, xzr
    bl    _soc_set_start_addr

1:
    mov   x30, x11
    ret

//-----------------------------------------------------------------------------

 // this function completes the initialization tasks of the soc
 // in: 
 // out: 
 // uses x0, x1, x2, x3, x4
_soc_init_finish:

    ret

//-----------------------------------------------------------------------------

 // this function sets the security mechanisms in the SoC to implement the
 // Platform Security Policy
_set_platform_security:

    ret

//-----------------------------------------------------------------------------

 // this function makes any needed soc-specific configuration changes when boot
 // services end
_soc_exit_boot_svcs:

    ret

//-----------------------------------------------------------------------------

 // this function returns the base address of the gic distributor
 // in:  none 
 // out: x0 = base address of gic distributor
 // uses x0
_getGICD_BaseAddr:
    ldr   x0, =GICD_BASE_ADDR
    ret

//-----------------------------------------------------------------------------

 // this function returns the base address of the gic controller
 // in:  none 
 // out: x0 = base address of gic controller
 // uses x0
_getGICC_BaseAddr:
    ldr   x0, =GICC_BASE_ADDR
    ret

//-----------------------------------------------------------------------------

 // write a register in the SCFG block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2, x3
write_reg_scfg:
    ldr  x2, =SCFG_BASE_ADDR
     // swap for BE
    rev  w3, w1
    str  w3, [x2, x0]
    ret

//-----------------------------------------------------------------------------

 // read a register in the SCFG block
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_scfg:
    ldr  x2, =SCFG_BASE_ADDR
    ldr  w1, [x2, x0]
     // swap for BE
    rev  w0, w1
    ret

//-----------------------------------------------------------------------------

 // write a register in the DCFG block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2, x3
write_reg_dcfg:
    ldr  x2, =DCFG_BASE_ADDR
     // swap for BE
    rev  w3, w1
    str  w3, [x2, x0]
    ret

//-----------------------------------------------------------------------------

 // read a register in the DCFG block
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_dcfg:
    ldr  x2, =DCFG_BASE_ADDR
    ldr  w1, [x2, x0]
     // swap for BE
    rev  w0, w1
    ret

//-----------------------------------------------------------------------------

 // write a register in the RCPM block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2, x3
write_reg_rcpm:
    ldr  x2, =RCPM_BASE_ADDR
     // swap for BE
    rev  w3, w1
    str  w3, [x2, x0]
    ret
//-----------------------------------------------------------------------------

 // read a register in the RCPM block
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_rcpm:
    ldr  x2, =RCPM_BASE_ADDR
    ldr  w1, [x2, x0]
     // swap for BE
    rev  w0, w1
    ret

//-----------------------------------------------------------------------------

 // write a register in the DCSR-RCPM2 block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2, x3
write_reg_rcpm2:
    ldr  x2, =DCSR_RCPM2_BASE
     // swap for BE
    rev  w3, w1
    str  w3, [x2, x0]
    ret
//-----------------------------------------------------------------------------

 // read a register in the DCSR-RCPM2 block
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_rcpm2:
    ldr  x2, =DCSR_RCPM2_BASE
    ldr  w1, [x2, x0]
     // swap for BE
    rev  w0, w1
    ret

//-----------------------------------------------------------------------------

 // write a register in the SYS_COUNTER block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2, x3
write_reg_sys_counter:
    ldr  x2, =SYS_COUNTER_BASE
     // swap for BE
    rev  w3, w1
    str  w3, [x2, x0]
    ret
//-----------------------------------------------------------------------------

 // read a register in the SYS_COUNTER block
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_sys_counter:
    ldr  x2, =SYS_COUNTER_BASE
    ldr  w1, [x2, x0]
     // swap for BE
    rev  w0, w1
    ret

//-----------------------------------------------------------------------------

 // write a register in the GIC400 distributor block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2, x3
write_reg_gicd:
    ldr  x2, =GICD_BASE_ADDR
    str  w1, [x2, x0]
    ret

//-----------------------------------------------------------------------------

 // read a register in the GIC400 distributor block
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_gicd:
    ldr  x2, =GICD_BASE_ADDR
    ldr  w0, [x2, x0]
    ret

//-----------------------------------------------------------------------------

 // write a register in the GIC400 CPU interface block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2, x3
write_reg_gicc:
    ldr  x2, =GICC_BASE_ADDR
    str  w1, [x2, x0]
    ret

//-----------------------------------------------------------------------------

 // read a register in the GIC400 CPU interface block
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_gicc:
    ldr  x2, =GICC_BASE_ADDR
    ldr  w0, [x2, x0]
    ret

//-----------------------------------------------------------------------------

 // read a register in the ddr controller block
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_ddr:
    ldr  x2, =DDR_CNTRL_BASE_ADDR
    ldr  w0, [x2, x0]
    ret

//-----------------------------------------------------------------------------

 // write a register in the ddr controller block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2, x3
write_reg_ddr:
    ldr  x2, =DDR_CNTRL_BASE_ADDR
    str  w1, [x2, x0]
    ret

//-----------------------------------------------------------------------------

 // this is soc initialization task 1
 // this function releases a secondary core to init the upper half of OCRAM
 // in:  x0 = core mask lsb of the secondary core to put to work
 // out: none
 // uses x0, x1, x2, x3, x4, x5
init_task_1:
    mov  x5, x30
    mov  x4, x0

     // set the core state to WORKING_INIT
    mov  x1, #CORE_STATE_DATA
    mov  x2, #CORE_WORKING_INIT
    bl   _setCoreData

     // save the core mask
    mov  x0, x4
    bl   _set_task1_core

     // load bootlocptr with start addr
    adr  x0, _prep_init_ocram_hi
    bl   _soc_set_start_addr

     // release secondary core
    mov  x0, x4
    bl  _soc_core_release

    mov  x30, x5
    ret

//-----------------------------------------------------------------------------

 // this is soc initialization task 2
 // this function releases a secondary core to init the lower half of OCRAM
 // in:  x0 = core mask lsb of the secondary core to put to work
 // out: none
 // uses x0, x1, x2, x3, x4, x5
init_task_2:
    mov  x5, x30
    mov  x4, x0

     // set the core state to WORKING_INIT
    mov  x1, #CORE_STATE_DATA
    mov  x2, #CORE_WORKING_INIT
    bl   _setCoreData

     // save the core mask
    mov  x0, x4
    bl   _set_task2_core

     // load bootlocptr with start addr
    adr  x0, _prep_init_ocram_lo
    bl   _soc_set_start_addr

     // release secondary core
    mov  x0, x4
    bl  _soc_core_release

    mov  x30, x5
    ret

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

 // this function will shutdown ddr and the final core - it will do this
 // by loading itself into the icache and then executing from there
 // in:  w5  = ipstpcr4 (IPSTPCR4_VALUE bic DEVDISR5_MASK)
 //      x6  = DDR_CNTRL_BASE_ADDR
 //      x7  = DCSR_RCPM2_BASE
 //      x8  = DCFG_BASE_ADDR
 //      w13 = DEVDISR1 saved value
 //      w16 = DEVDISR4 saved value
 //      w17 = DEVDISR5 saved value
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x13, x16, x17

 // 4Kb aligned
.align 12
final_pwrdown:

    mov  x0, xzr
    b    touch_line_0
start_line_0:
    mov  x0, #1
    mov  x2, #DDR_SDRAM_CFG_2_FRCSR         // put ddr in self refresh - start
    ldr  w3, [x6, #DDR_SDRAM_CFG_2_OFFSET]
    rev  w4, w3
    orr  w4, w4, w2
    rev  w3, w4
    str  w3, [x6, #DDR_SDRAM_CFG_2_OFFSET]  // put ddr in self refresh - end
    orr  w3, w5, #DEVDISR5_MEM              // quiesce ddr clocks - start
    rev  w4, w3
    str  w4, [x7, #RCPM2_IPSTPCR4_OFFSET]   // quiesce ddr clocks - end

    mov  w3, #DEVDISR5_MEM
    rev  w3, w3                             // polling mask
    mov  x2, #DDR_SLEEP_RETRY_CNT           // poll on ipstpack4 - start
touch_line_0:
    cbz  x0, touch_line_1

start_line_1:
    ldr  w1, [x7, #RCPM2_IPSTPACKR4_OFFSET]
    tst  w1, w3
    b.ne 1f
    subs x2, x2, #1
    b.gt start_line_1                       // poll on ipstpack4 - end

     // if we get here, we have a timeout err
    rev  w4, w5
    str  w4, [x7, #RCPM2_IPSTPCR4_OFFSET]   // re-enable ddr clks interface
    mov  x0, #ERROR_DDR_SLEEP               // load error code
    b    2f
1:
    str  w4, [x8, #DCFG_DEVDISR5_OFFSET]    // disable ddr cntrlr clk in devdisr5
    wfi                                     // stop the final core

    rev  w4, w5
    str  w4, [x8, #DCFG_DEVDISR5_OFFSET]    // re-enable ddr in devdisr5
    str  w4, [x7, #RCPM2_IPSTPCR4_OFFSET]   // re-enable ddr clk in ipstpcr4
    nop
touch_line_1:
    cbz  x0, touch_line_2

start_line_2:
    ldr  w1, [x7, #RCPM2_IPSTPACKR4_OFFSET] // poll on ipstpack4 - start
    tst  w1, w3
    b.eq 2f
    nop
    b    start_line_2                       // poll on ipstpack4 - end
2:
    mov  x2, #DDR_SDRAM_CFG_2_FRCSR         // take ddr out-of self refresh - start
    ldr  w3, [x6, #DDR_SDRAM_CFG_2_OFFSET]
    rev  w4, w3
    bic  w4, w4, w2
    rev  w3, w4
    mov  x1, #DDR_SLEEP_RETRY_CNT           // wait for ddr cntrlr clock - start
3:
    subs x1, x1, #1
    b.gt 3b                                 // wait for ddr cntrlr clock - end
    str  w3, [x6, #DDR_SDRAM_CFG_2_OFFSET]  // take ddr out-of self refresh - end
    rev  w1, w17
touch_line_2:
    cbz  x0, touch_line_3

start_line_3:
    str  w1, [x8, #DCFG_DEVDISR5_OFFSET]    // reset devdisr5
    rev  w1, w16
    str  w1, [x8, #DCFG_DEVDISR4_OFFSET]    // reset devdisr4
    rev  w1, w13
    str  w1, [x8, #DCFG_DEVDISR1_OFFSET]    // reset devdisr1
    str  wzr, [x7, #RCPM2_IPSTPCR4_OFFSET]  // reset ipstpcr4
    str  wzr, [x7, #RCPM2_IPSTPCR3_OFFSET]  // reset ipstpcr3
    str  wzr, [x7, #RCPM2_IPSTPCR0_OFFSET]  // reset ipstpcr0
    b    continue_restart
    nop
    nop
    nop
    nop
    nop
    nop
touch_line_3:
    cbz  x0, start_line_0

 // execute here after ddr is back up
continue_restart:
    mov  x6, x0

     // clear POWMGTDCR
    mov  x1, #DCSR_RCPM2_BASE
    str  wzr, [x1, #RCPM2_POWMGTDCR_OFFSET]

     // clear WFIL2_EN in SCFG_COREPMCR
    mov  x1, #SCFG_BASE_ADDR
    str  wzr, [x1, #SCFG_COREPMCR_OFFSET]

     // if x0 = 1, all is well
     // if x0 < 1, we had an error
    mov  x0, x6
    cmp  x0, #1
    b.ne 4f
    mov  x0, #0
4:
    ret

//-----------------------------------------------------------------------------

 // this function will shutdown ddr and the final core - it will do this
 // by loading itself into the icache and then executing from there
 // Note: there is no return allowed from this function
 // in:  w5  = ipstpcr4 (IPSTPCR4_VALUE bic DEVDISR5_MASK)
 //      x6  = DDR_CNTRL_BASE_ADDR
 //      x7  = DCSR_RCPM2_BASE
 //      x8  = DCFG_BASE_ADDR
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8

 // 4Kb aligned
.align 12
final_shutdown:

    mov  x0, xzr
    b    touch_line0
start_line0:
    mov  x0, #1
    mov  x2, #DDR_SDRAM_CFG_2_FRCSR         // put ddr in self refresh - start
    ldr  w3, [x6, #DDR_SDRAM_CFG_2_OFFSET]
    rev  w4, w3
    orr  w4, w4, w2
    rev  w3, w4
    str  w3, [x6, #DDR_SDRAM_CFG_2_OFFSET]  // put ddr in self refresh - end
    orr  w3, w5, #DEVDISR5_MEM              // quiesce ddr clocks - start
    rev  w4, w3
    str  w4, [x7, #RCPM2_IPSTPCR4_OFFSET]   // quiesce ddr clocks - end

    mov  w3, #DEVDISR5_MEM
    rev  w3, w3                             // polling mask
    mov  x2, #DDR_SLEEP_RETRY_CNT           // poll on ipstpack4 - start
touch_line0:
    cbz  x0, touch_line1

start_line1:
    ldr  w1, [x7, #RCPM2_IPSTPACKR4_OFFSET]
    tst  w1, w3
    b.ne 1f
    subs x2, x2, #1
    b.gt start_line1                       // poll on ipstpack4 - end
    nop
    nop
    nop
    nop
1:
    str  w4, [x8, #DCFG_DEVDISR5_OFFSET]    // disable ddr cntrlr clk in devdisr5
2:
    wfi                                     // stop the final core
    b    2b                                 // stay here until POR
    nop
    nop
    nop
touch_line1:
    cbz  x0, start_line0

//-----------------------------------------------------------------------------

psci_features_table:
    .4byte  PSCI_VERSION_ID         // psci_version
    .4byte  PSCI_FUNC_IMPLEMENTED   // implemented
    .4byte  PSCI_CPU_OFF_ID         // cpu_off
    .4byte  PSCI_FUNC_IMPLEMENTED   // implemented
    .4byte  PSCI64_CPU_ON_ID        // cpu_on
    .4byte  PSCI_FUNC_IMPLEMENTED   // implemented
    .4byte  PSCI_FEATURES_ID        // psci_features
    .4byte  PSCI_FUNC_IMPLEMENTED   // implemented
    .4byte  PSCI64_AFFINITY_INFO_ID // psci_affinity_info
    .4byte  PSCI_FUNC_IMPLEMENTED   // implemented
    .4byte  FEATURES_TABLE_END      // table terminating value - must always be last entry in table

//-----------------------------------------------------------------------------

