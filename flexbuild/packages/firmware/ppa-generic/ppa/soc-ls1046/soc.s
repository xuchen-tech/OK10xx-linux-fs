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

#define DAIF_DATA         AUX_01_DATA
#define TIMER_CNTRL_DATA  AUX_02_DATA

#define CPUACTLR_DATA_OFFSET  0x1C

#define IPSTPACK_RETRY_CNT    0x10000
#define DDR_SLEEP_RETRY_CNT   0x10000
#define CPUACTLR_EL1          S3_1_C15_C2_0
#define CPUACTLR_L1PCTL_MASK  0x0000E000

#define DLL_LOCK_MASK   0x3
#define DLL_LOCK_VALUE  0x2

#define ERROR_DDR_SLEEP       -1
#define ERROR_DDR_WAKE        -2
#define ERROR_NO_QUIESCE      -3

//#if DEBUG_BUILD
//#define CORES_DISABLED 0xC
//#endif

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

.global _soc_init_start
.global _soc_init_finish
.global _soc_init_percpu
.global _set_platform_security
.global _soc_exit_boot_svcs
.global _getGICC_BaseAddr
.global _getGICD_BaseAddr

//-----------------------------------------------------------------------------

 // this function performs soc-specific initialization needed on a per-core basis
 // in:  none
 // out: none
 // uses x0
_soc_init_percpu:

    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function puts the calling core into standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x10
_soc_core_entr_stdby:
    mov  x10, x30

     // clean the L1 dcache
    mov  x0, xzr
    bl   _cln_inv_L1_dcache

     // IRQ taken to EL3, set SCR_EL3[IRQ]
    mrs  x0, SCR_EL3
    orr  x0, x0, #SCR_IRQ_MASK
    msr  SCR_EL3, x0

    dsb  sy
    isb
    wfi

    mov  x30, x10
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function performs any necessary cleanup after the calling core has
 // exited standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0,
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
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x9, x10
_soc_core_entr_pwrdn:
    mov  x10, x30

     // X0 = core mask lsb
    mov  x9, x0

     // mask interrupts by setting DAIF[7:4] to 'b1111
    mrs  x1, DAIF
    ldr  x0, =DAIF_SET_MASK
    orr  x1, x1, x0
    msr  DAIF, x1

     // cln/inv L1 dcache
    mov  x0, #1
    bl   _cln_inv_L1_dcache

     // IRQ taken to EL3, set SCR_EL3[IRQ]
    mrs  x0, SCR_EL3
    orr  x0, x0, #SCR_IRQ_MASK
    msr  SCR_EL3, x0

     // disable icache, dcache, mmu @ EL2 & EL1
    mov  x1, #SCTLR_I_C_M_MASK
    mrs  x0, sctlr_el1
    bic  x0, x0, x1
    msr  sctlr_el1, x0

     // disable dcache @ EL3
    mrs  x0, sctlr_el3
    bic  x0, x0, #SCTLR_C_MASK
    msr  sctlr_el3, x0

     // enable CPU retention
    mrs  x6, CPUECTLR_EL1
    orr  x0, x6, #0x1
    msr  CPUECTLR_EL1, x0

     // enable SMPEN
    mrs  x0, CPUECTLR_EL1
    orr  x0, x0, #0x40
    msr  CPUECTLR_EL1, x0

     // Set the RETREQn bit in SCFG_RETREQCR
    mov  x0, x9
    mov  x1, #1
    lsl  x1, x1, x0
     // reverse bit order
    rbit w1, w1
    ldr  x0, =SCFG_RETREQCR_OFFSET
    bl   write_reg_scfg

     // Set the PC_PH20_REQ bit in RCPM_PCPH20SETR
    mov  x0, x9
    mov  x1, #1
    lsl  x1, x1, x0
    mov  x0, #RCPM_PCPH20SETR_OFFSET
    bl   write_reg_rcpm

    dsb  sy
    isb
    wfi

     // restore CPUECTLR_EL1
    msr CPUECTLR_EL1, x6

    mov  x30, x10
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function cleans up after a core exits power-down
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, 
_soc_core_exit_pwrdn:
     // X0 = core mask lsb

     // clear SCR_EL3[IRQ]
    mrs  x0, SCR_EL3
    bic  x0, x0, #SCR_IRQ_MASK
    msr  SCR_EL3, x0

     // invalidate icache
    ic  iallu
    isb
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function puts the cluster into a standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, 
_soc_clstr_entr_stdby:
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function exits the cluster from a standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, 
_soc_clstr_exit_stdby:
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function puts the calling core into a power-down state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, 
_soc_clstr_entr_pwrdn:
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function cleans up after a cluster exits power-down
 // in:  x0 = core mask lsb
 // out: none
 // uses 
_soc_clstr_exit_pwrdn:
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function puts the system into a standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, 
_soc_sys_entr_stdby:
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function exits the system from a standby state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, 
_soc_sys_exit_stdby:
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function puts the calling core, and potentially the soc, into a
 // low-power state
 // in:  x0 = core mask lsb
 // out: x0 = 0, success
 //      x0 < 0, failure
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10
_soc_sys_entr_pwrdn:
    mov  x10, x30

     // x0 = core mask lsb

     // save DAIF and mask ints
    mrs  x2, DAIF
    mov  x6, x2
    mov  x1, #DAIF_DATA
    bl   _setCoreData
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

     // IRQ taken to EL3, set SCR_EL3[IRQ]
    mrs  x0, SCR_EL3
    orr  x0, x0, #SCR_IRQ_MASK
    msr  SCR_EL3, x0

     // enable dynamic retention control (CPUECTLR[2:0]) and SMP (CPUECTLR[6])
    mrs  x0, CPUECTLR_EL1
    orr  x0, x0, #CPUECTLR_TIMER_8TICKS
    orr  x0, x0, #CPUECTLR_SMPEN_EN
    msr  CPUECTLR_EL1, x0

     // set WFIL2EN in SCFG_CLUSTERPMCR
    ldr  x0, =SCFG_COREPMCR_OFFSET
    ldr  x1, =COREPMCR_WFIL2EN
    bl   write_reg_scfg

     // request LPM20
    mov  x0, #RCPM_POWMGTCSR_OFFSET
    bl   read_reg_rcpm
    orr  x1, x0, #RCPM_POWMGTCSR_LPM20_REQ
    mov  x0, #RCPM_POWMGTCSR_OFFSET
    bl   write_reg_rcpm

    dsb  sy
    isb
    wfi

    mov  x0, #0
    mov  x30, x10
    ret

//-----------------------------------------------------------------------------

 // part of CPU_SUSPEND
 // this function performs any necessary cleanup after the soc has exited
 // a low-power state
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, 
_soc_sys_exit_pwrdn:
    mov  x10, x30

     // clear SCR_EL3[IRQ]
    mrs  x0, SCR_EL3
    bic  x0, x0, #SCR_IRQ_MASK
    msr  SCR_EL3, x0

    mov  x30, x10
    ret

//-----------------------------------------------------------------------------

 // part of SYSTEM_OFF
 // this function turns off the SoC clocks
 // Note: this function is not intended to return, and the only allowable
 //       recovery is POR
 // in:  none
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x9
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

     // enable dynamic retention control (CPUECTLR[2:0]) and SMP (CPUECTLR[6])
    mrs  x0, CPUECTLR_EL1
    orr  x0, x0, #CPUECTLR_TIMER_8TICKS
    orr  x0, x0, #CPUECTLR_SMPEN_EN
    msr  CPUECTLR_EL1, x0

     // set WFIL2EN in SCFG_CLUSTERPMCR
    ldr  x0, =SCFG_COREPMCR_OFFSET
    ldr  x1, =COREPMCR_WFIL2EN
    bl   write_reg_scfg

     // request LPM20
    mov  x0, #RCPM_POWMGTCSR_OFFSET
    bl   read_reg_rcpm
    orr  x1, x0, #RCPM_POWMGTCSR_LPM20_REQ
    mov  x0, #RCPM_POWMGTCSR_OFFSET
    bl   write_reg_rcpm

    dsb  sy
    isb
1:
    wfi
    b    1b

//-----------------------------------------------------------------------------

 // this function resets the system via SoC-specific methods
 // in:  none
 // out: x0 = PSCI_SUCCESS
 //      x0 = PSCI_INTERNAL_FAILURE
 // uses x0, x1, x2, x3, x4
_soc_sys_reset:

    mov  x2, #DCFG_BASE_ADDR

     // make sure the mask is cleared in the reset request mask register
    mov  w1, wzr
    str  w1, [x2, #DCFG_RSTRQMR1_OFFSET]

     // x2 = DCFG_BASE_ADDR

     // set the reset request
    mov  w1, #RSTCR_RESET_REQ
    mov  x4, #DCFG_RSTCR_OFFSET
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
    mov  w3, #RESET_RETRY_CNT
    mov  w4, #RSTRQSR1_SWRR
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
    mov  x0, #PSCI_SUCCESS
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

     // get base addr of dcfg block
    mov  x1, #DCFG_BASE_ADDR

     // read COREDISR
    ldr  w1, [x1, #DCFG_COREDISR_OFFSET]
    rev  w2, w1

//#if DEBUG_BUILD
//    mov  w2, #CORES_DISABLED
//#endif

     // test core bit
    and  w0, w2, w0
    ret

//-----------------------------------------------------------------------------

 // part of CPU_ON
 // this function releases a secondary core from reset
 // in:   x0 = core_mask_lsb
 // out:  none
 // uses: x0, x1, x2, x3
_soc_core_release:

#if (SIMULATOR_BUILD)
     // x0 = core mask lsb

    mov  w2, w0
    CoreMaskMsb w2, w3

     // x0 = core mask lsb
     // x2 = core mask msb

#else
     // x0 = core mask lsb

    mov  x2, x0

#endif
     // write COREBCR 
    mov   x1, #SCFG_BASE_ADDR
    rev   w3, w2
    str   w3, [x1, #SCFG_COREBCR_OFFSET]
    isb

     // x0 = core mask lsb

     // read-modify-write BRR
    mov  x1, #DCFG_BASE_ADDR
    ldr  w2, [x1, #DCFG_BRR_OFFSET]
    rev  w3, w2
    orr  w3, w3, w0
    rev  w2, w3
    str  w2, [x1, #DCFG_BRR_OFFSET]
    isb

     // send event
    sev
    isb
    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function programs ARM core registers in preparation for shutting down
 // the core
 // in:   x0 = core_mask_lsb
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8
_soc_core_phase1_off:
    mov  x8, x30

     // x0 = core mask lsb
    mov   x5, x0

     // read cpuectlr and save current value
    mrs   x4, CPUECTLR_EL1
    mov   x1, #CPUECTLR_DATA
    mov   x2, x4
    bl    _setCoreData

     // x4 = cpuectlr
     // x5 = core mask lsb

     // set retention control in CPUECTLR
     // make sure smpen bit is set
    bic   x4, x4, #CPUECTLR_RET_MASK
    orr   x4, x4, #CPUECTLR_TIMER_8TICKS
    orr   x4, x4, #CPUECTLR_SMPEN_EN
    msr   CPUECTLR_EL1, x4

     // x5 = core mask lsb

     // save timer control current value
    mov   x6, #SYS_COUNTER_BASE
    ldr   w4, [x6, #SYS_COUNTER_CNTCR_OFFSET]
    mov   w2, w4
    mov   x0, x5
    mov   x1, #TIMER_CNTRL_DATA
    bl    _setCoreData

     // w4 = counter ctl
     // x5 = core mask lsb
     // x6 = sys counter base addr

     // enable the timer
    orr   w4, w4, #CNTCR_EN_MASK
    str   w4, [x6, #SYS_COUNTER_CNTCR_OFFSET]

     // mask interrupts by setting DAIF[7:4] to 'b1111
    mrs  x1, DAIF
    ldr  x0, =DAIF_SET_MASK
    orr  x1, x1, x0
    msr  DAIF, x1 

     // disable dcache, mmu, and icache for EL1 and EL2 by clearing
     // bits 0, 2, and 12 of SCTLR_EL1 and SCTLR_EL2 (MMU, dcache, icache)
    ldr x0, =SCTLR_I_C_M_MASK
    mrs x1, SCTLR_EL1
    bic x1, x1, x0
    msr SCTLR_EL1, x1 

    mrs x1, SCTLR_EL2
    bic x1, x1, x0
    msr SCTLR_EL2, x1 

     // disable only dcache for EL3 by clearing SCTLR_EL3[2] 
    mrs x1, SCTLR_EL3
    ldr x0, =SCTLR_C_MASK
    bic x1, x1, x0      
    msr SCTLR_EL3, x1 
    isb

     // cln/inv L1 dcache
    mov  x0, #1
    bl   _cln_inv_L1_dcache  // 0-7

     // FIQ taken to EL3, set SCR_EL3[FIQ]
    mrs   x0, scr_el3
    orr   x0, x0, #SCR_FIQ_MASK
    msr   scr_el3, x0

    dsb  sy
    isb
    mov  x30, x8               
    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function programs SoC & GIC registers in preparation for shutting down
 // the core
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6
_soc_core_phase2_off:
    mov  x6, x30

     // x0 = core mask lsb

     // disable signaling of ints
    ldr  x5, =GICC_BASE_ADDR
    ldr  w3, [x5, #GICC_CTLR_OFFSET]
    bic  w3, w3, #GICC_CTLR_EN_GRP0
    bic  w3, w3, #GICC_CTLR_EN_GRP1
    str  w3, [x5, #GICC_CTLR_OFFSET]
    dsb  sy
    isb

     // x0 = core mask lsb
     // x3 = GICC_CTRL
     // x5 = GICC_BASE_ADDR

     // set retention control in SCFG_RETREQCR
     // Note: this register is msb 0
    CoreMaskMsb w0, w1

     // x0 = core mask msb
     // x3 = GICC_CTRL
     // x5 = GICC_BASE_ADDR

    mov  x2, #SCFG_BASE_ADDR
    ldr  w1, [x2, #SCFG_RETREQCR_OFFSET]
    rev  w4, w1
    orr  w4, w4, w0
    rev  w1, w4
    str  w1, [x2, #SCFG_RETREQCR_OFFSET]

     // configure the cpu interface
     // x3 = GICC_CTRL
     // x5 = GICC_BASE_ADDR

     // set the priority filter
    ldr  w2, [x5, #GICC_PMR_OFFSET]
    orr  w2, w2, #GICC_PMR_FILTER
    str  w2, [x5, #GICC_PMR_OFFSET]

     // setup GICC_CTLR
    bic  w3, w3, #GICC_CTLR_ACKCTL_MASK
    orr  w3, w3, #GICC_CTLR_FIQ_EN_MASK
    orr  w3, w3, #GICC_CTLR_EOImodeS_MASK
    orr  w3, w3, #GICC_CTLR_CBPR_MASK
    str  w3, [x5, #GICC_CTLR_OFFSET]

     // x3 = GICC_CTRL
     // x4 = core mask lsb

     // setup the banked-per-core GICD registers
    ldr  x5, =GICD_BASE_ADDR

     // define SGI15 as Grp0
    ldr  w2, [x5, #GICD_IGROUPR0_OFFSET]
    bic  w2, w2, #GICD_IGROUP0_SGI15
    str  w2, [x5, #GICD_IGROUPR0_OFFSET]

     // set priority of SGI 15 to highest...
    ldr  w2, [x5, #GICD_IPRIORITYR3_OFFSET]
    bic  w2, w2, #GICD_IPRIORITY_SGI15_MASK
    str  w2, [x5, #GICD_IPRIORITYR3_OFFSET]

     // enable SGI 15
    ldr  w2, [x5, #GICD_ISENABLER0_OFFSET]
    orr  w2, w2, #GICD_ISENABLE0_SGI15
    str  w2, [x5, #GICD_ISENABLER0_OFFSET]

     // x3 = GICC_CTRL

     // enable the cpu interface

    ldr  x5, =GICC_BASE_ADDR
    orr  w3, w3, #GICC_CTLR_EN_GRP0
    str  w3, [x5, #GICC_CTLR_OFFSET]

    dsb  sy
    isb
    mov  x30, x6
    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function performs the final steps to shutdown the core
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6
_soc_core_entr_off:
    mov  x6, x30

     // x0 = core mask lsb
    mov  x5, x0

     // change state of core in data area
    mov  x1, #CORE_STATE_DATA
    mov  x2, #CORE_OFF
    bl   _setCoreData

     // disable EL3 icache by clearing SCTLR_EL3[12]
    mrs  x1, SCTLR_EL3
    ldr  x2, =SCTLR_I_MASK
    bic  x1, x1, x2      
    msr  SCTLR_EL3, x1 

     // invalidate icache
    ic  iallu
    dsb sy
    isb

     // clear any pending SGIs
    ldr  x2, =GICD_CPENDSGIR_CLR_MASK
    ldr  x4, =GICD_BASE_ADDR
    add  x0, x4, #GICD_CPENDSGIR3_OFFSET
    str  w2, [x0]

     // x4 = GICD_BASE_ADDR
     // x5 = core mask (lsb)

     // set ph20 in RCPM_PCPH20SETR
    ldr  x1, =RCPM_BASE_ADDR
    rev  w2, w5
    str  w2, [x1, #RCPM_PCPH20SETR_OFFSET]
    dsb  sy
    isb

3:
     // enter low-power state by executing wfi
    wfi

     // x4 = GICD_BASE_ADDR
     // x5 = core mask (lsb)

     // see if we got hit by SGI 15
    add   x0, x4, #GICD_SPENDSGIR3_OFFSET
    ldr   w2, [x0]
    and   w2, w2, #GICD_SPENDSGIR3_SGI15_MASK
    cbz   w2, 4f

     // clear the pending SGI
    ldr   x2, =GICD_CPENDSGIR_CLR_MASK
    add   x0, x4, #GICD_CPENDSGIR3_OFFSET
    str   w2, [x0]
4:
     // x5 = core mask (lsb)

     // check if core has been turned on
    mov  x0, x5
    mov  x1, #CORE_STATE_DATA
    bl   _getCoreData

    cmp  x0, #CORE_PENDING
    b.ne 3b

     // if we get here, then we have exited the wfi

    mov  x30, x6
    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function starts the process of starting a core back up
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1
_soc_core_exit_off:

    ldr  x1, =GICC_BASE_ADDR

     // read GICC_IAR
    ldr  w0, [x1, #GICC_IAR_OFFSET]

     // write GICC_EIOR - signal end-of-interrupt
    str  w0, [x1, #GICC_EOIR_OFFSET]

     // write GICC_DIR - disable interrupt
    str  w0, [x1, #GICC_DIR_OFFSET]

     // enable icache in SCTLR_EL3
    mrs  x0, SCTLR_EL3
    orr  x0, x0, #SCTLR_I_MASK
    msr  SCTLR_EL3, x0

    dsb sy
    isb
    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function cleans up from phase 1 of the core shutdown sequence
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4
_soc_core_phase1_clnup:
    mov  x4, x30

     // x0 = core mask lsb

     // clr SCR_EL3[FIQ]
    mrs   x1, scr_el3
    bic   x2, x1, #SCR_FIQ_MASK
    msr   scr_el3, x2

     // x0 = core mask lsb
    mov   x3, x0

     // restore CPUECTLR
    mov   x1, #CPUECTLR_DATA
    bl    _getCoreData
    msr   CPUECTLR_EL1, x0

     // restore timer ctrl
    mov   x0, x3
    mov   x1, #TIMER_CNTRL_DATA
    bl    _getCoreData

     // w0 = timer ctrl saved value

    mov   x2, #SYS_COUNTER_BASE
    str   w0, [x2, #SYS_COUNTER_CNTCR_OFFSET]

    isb
    mov  x30, x4
    ret

//-----------------------------------------------------------------------------

 // part of CPU_OFF
 // this function cleans up from phase 2 of the core shutdown sequence
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4
_soc_core_phase2_clnup:
    mov  x4, x30

     // x0 = core mask lsb

     // disable signaling of grp0 ints
    ldr  x2, =GICC_BASE_ADDR
    ldr  w3, [x2, #GICC_CTLR_OFFSET]
    bic  w3, w3, #GICC_CTLR_EN_GRP0
    str  w3, [x2, #GICC_CTLR_OFFSET]

     // w0 = core mask lsb

    CoreMaskMsb w0, w1

     // w0 = core mask msb

     // unset retention request in SCFG_RETREQCR
    mov  x2, #SCFG_BASE_ADDR
    ldr  w1, [x2, #SCFG_RETREQCR_OFFSET]
    rev  w3, w1
    eor  w3, w3, w0
    rev  w1, w3
    str  w1, [x2, #SCFG_RETREQCR_OFFSET]
    
    dsb  sy
    isb
    mov  x30, x4
    ret

//-----------------------------------------------------------------------------

 // part of CPU_ON
 // this function restarts a core shutdown via _soc_core_entr_off
 // in:  x0 = core mask lsb (of the target cpu)
 // out: x0 == 0, on success
 //      x0 != 0, on failure
 // uses x0, x1, x2, x3, x4, x5
_soc_core_restart:
    mov  x5, x30

     // x0 = core mask lsb

     // unset RCPM_PCPH20CLEARR
    ldr   x1, =RCPM_BASE_ADDR
    rev   w2, w0
    str   w2, [x1, #RCPM_PCPH20CLRR_OFFSET]
    dsb sy
    isb

    ldr  x4, =GICD_BASE_ADDR

     // x0 = core mask lsb
     // x4 = GICD_BASE_ADDR

     // enable forwarding of group 0 interrupts by setting GICD_CTLR[0] = 1
    ldr  w1, [x4, #GICD_CTLR_OFFSET]
    orr  w1, w1, #GICD_CTLR_EN_GRP0
    str  w1, [x4, #GICD_CTLR_OFFSET]
    dsb sy
    isb

     // x0 = core mask lsb
     // x4 = GICD_BASE_ADDR

     // fire SGI by writing to GICD_SGIR the following values:
     // [25:24] = 0x0 (forward interrupt to the CPU interfaces specified in CPUTargetList field)
     // [23:16] = core mask lsb[7:0] (forward interrupt to target cpu)
     // [15]    = 0 (forward SGI only if it is configured as group 0 interrupt)
     // [3:0]   = 0xF (interrupt ID = 15)
    lsl  w1, w0, #16
    orr  w1, w1, #0xF
    str  w1, [x4, #GICD_SGIR_OFFSET]
    dsb sy
    isb

     // load '0' on success
    mov  x0, xzr

    mov  x30, x5
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
    bl  _soc_get_start_addr   // 0-2
    mov  x1, x0
    mov  x0, #BOOTLOC_OFFSET
    bl   _set_global_data

     // see if we are initializing ocram
    ldr x0, =POLICY_USING_ECC
    cbz x0, 1f
     // initialize the OCRAM for ECC

     // get a secondary core to initialize ocram
    bl  _find_core      // 0-4
    cbz x0, 2f
    bl  init_task_1     // 0-5   
3:
     // wait til task 1 has started
    bl   _get_task1_start // 0-1
    cbz  x0, 3b
    b    4f
2:
     // there are no secondary cores available, so the
     // boot core will have to init ocram
    bl  _ocram_init // 0-10
4:
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
    mov   x4, x30

     // are we initializing ocram?
    ldr x0, =POLICY_USING_ECC
    cbz x0, 2f

     // if the ocram init is not completed, wait til it is
1:
    bl   _get_task1_done
    cbz  x0, 1b

2:
     // restore bootlocptr
    mov  x0, #BOOTLOC_OFFSET
    bl   _get_global_data
     // x0 = saved start address
    bl   _soc_set_start_addr

    mov  x30, x4
    ret

//-----------------------------------------------------------------------------

 // this function sets the security mechanisms in the SoC to implement the
 // Platform Security Policy
 // in:   none
 // out:  none
 // uses 
_set_platform_security:

#if (!SUPPRESS_TZC) && (CNFG_SPD)
    mov x8, x30
    bl arm_tzc400_setup
    mov  x30, x8
#endif

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
    mov  x2, #SCFG_BASE_ADDR
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
    mov  x2, #SCFG_BASE_ADDR
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
    mov  x2, #DCFG_BASE_ADDR
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
    mov  x2, #DCFG_BASE_ADDR
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

 // write a register in the SYS_COUNTER block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2, x3
write_reg_sys_counter:
    mov  x2, #SYS_COUNTER_BASE
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
    mov  x2, #SYS_COUNTER_BASE
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

 // this is soc initialization task 1
 // this function releases a secondary core
 // in:  x0 = core mask lsb of the secondary core to put to work
 // out: none
 // uses x0, x1, x2, x3, x4, x5
init_task_1:
    mov  x5, x30
    mov  x4, x0

     // x0 = core mask lsb
     // x4 = core mask lsb

     // save the core mask
    bl   _set_task1_core

     // load bootlocptr with start addr
    adr  x0, _prep_ocram_init
    bl   _soc_set_start_addr

     // x4 = core mask lsb

     // release secondary core
    mov  x0, x4
    bl  _soc_core_release

    mov  x30, x5
    ret

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
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

