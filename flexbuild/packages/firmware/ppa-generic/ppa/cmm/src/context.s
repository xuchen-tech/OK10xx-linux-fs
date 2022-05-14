//-----------------------------------------------------------------------------
//
// Copyright (c) 2013-2016, ARM Limited and Contributors. All rights reserved.
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
// Authors:
//  Sumit Garg <sumit.garg@nxp.com>
//  Pankaj Gupta <pankaj.gupta@nxp.com>
//

#include "context_arch64_asm.h"
#include "aarch64.h"
#include "smc.h"

.section .text, "ax"

    .global    el1_sysregs_ctx_save
    .global    el1_sysregs_ctx_restore
    .global    gp_regs_ctx_save_all
    .global    gp_regs_ctx_save_args_n_callee
    .global    gp_regs_ctx_restore_args_n_callee_eret
    .global    gp_regs_ctx_restore_callee_eret
    .global    el3_exit
    .equ       REGISTER_OBFUSCATE, 0xA5A5A5A5A5A5A5A5

 // -----------------------------------------------------
 // The following function strictly follows the AArch64
 // PCS to use x9-x17 (temporary caller-saved registers)
 // to save EL1 system register context. It assumes that
 // 'x0' is pointing to a 'el1_sys_regs' structure where
 // the register context will be saved.
 // -----------------------------------------------------
el1_sysregs_ctx_save:

    mrs    x9, spsr_el1
    mrs    x10, elr_el1
    stp    x9, x10, [x0, #CPU_CTX_SPSR_EL1]

    mrs    x15, sctlr_el1
    mrs    x16, actlr_el1
    stp    x15, x16, [x0, #CPU_CTX_SCTLR_EL1]

    mrs    x17, cpacr_el1
    mrs    x9, csselr_el1
    stp    x17, x9, [x0, #CPU_CTX_CPACR_EL1]

    mrs    x10, sp_el1
    mrs    x11, esr_el1
    stp    x10, x11, [x0, #CPU_CTX_SP_EL1]

    mrs    x12, ttbr0_el1
    mrs    x13, ttbr1_el1
    stp    x12, x13, [x0, #CPU_CTX_TTBR0_EL1]

    mrs    x14, mair_el1
    mrs    x15, amair_el1
    stp    x14, x15, [x0, #CPU_CTX_MAIR_EL1]

    mrs    x16, tcr_el1
    mrs    x17, tpidr_el1
    stp    x16, x17, [x0, #CPU_CTX_TCR_EL1]

    mrs    x9, tpidr_el0
    mrs    x10, tpidrro_el0
    stp    x9, x10, [x0, #CPU_CTX_TPIDR_EL0]

    mrs    x13, par_el1
    mrs    x14, far_el1
    stp    x13, x14, [x0, #CPU_CTX_PAR_EL1]

    mrs    x15, afsr0_el1
    mrs    x16, afsr1_el1
    stp    x15, x16, [x0, #CPU_CTX_AFSR0_EL1]

    mrs    x17, contextidr_el1
    mrs    x9, vbar_el1
    stp    x17, x9, [x0, #CPU_CTX_CONTEXTIDR_EL1]

     // Save AArch32 system registers if the build has instructed so
#if CPU_CTX_INCLUDE_AARCH32_REGS
    mrs    x11, spsr_abt
    mrs    x12, spsr_und
    stp    x11, x12, [x0, #CPU_CTX_SPSR_ABT]

    mrs    x13, spsr_irq
    mrs    x14, spsr_fiq
    stp    x13, x14, [x0, #CPU_CTX_SPSR_IRQ]

    mrs    x15, dacr32_el2
    mrs    x16, ifsr32_el2
    stp    x15, x16, [x0, #CPU_CTX_DACR32_EL2]

    mrs    x17, fpexc32_el2
    str    x17, [x0, #CPU_CTX_FP_FPEXC32_EL2]
#endif

     // Save NS timer registers if the build has instructed so
#if NS_TIMER_SWITCH
    mrs    x10, cntp_ctl_el0
    mrs    x11, cntp_cval_el0
    stp    x10, x11, [x0, #CPU_CTX_CNTP_CTL_EL0]

    mrs    x12, cntv_ctl_el0
    mrs    x13, cntv_cval_el0
    stp    x12, x13, [x0, #CPU_CTX_CNTV_CTL_EL0]

    mrs    x14, cntkctl_el1
    str    x14, [x0, #CPU_CTX_CNTKCTL_EL1]
#endif

    ret

 // -----------------------------------------------------
 // The following function strictly follows the AArch64
 // PCS to use x9-x17 (temporary caller-saved registers)
 // to restore EL1 system register context.  It assumes
 // that 'x0' is pointing to a 'el1_sys_regs' structure
 // from where the register context will be restored
 // -----------------------------------------------------
el1_sysregs_ctx_restore:

    ldp    x9, x10, [x0, #CPU_CTX_SPSR_EL1]
    msr    spsr_el1, x9
    msr    elr_el1, x10

    ldp    x15, x16, [x0, #CPU_CTX_SCTLR_EL1]
    msr    sctlr_el1, x15
    msr    actlr_el1, x16

    ldp    x17, x9, [x0, #CPU_CTX_CPACR_EL1]
    msr    cpacr_el1, x17
    msr    csselr_el1, x9

    ldp    x10, x11, [x0, #CPU_CTX_SP_EL1]
    msr    sp_el1, x10
    msr    esr_el1, x11

    ldp    x12, x13, [x0, #CPU_CTX_TTBR0_EL1]
    msr    ttbr0_el1, x12
    msr    ttbr1_el1, x13

    ldp    x14, x15, [x0, #CPU_CTX_MAIR_EL1]
    msr    mair_el1, x14
    msr    amair_el1, x15

    ldp    x16, x17, [x0, #CPU_CTX_TCR_EL1]
    msr    tcr_el1, x16
    msr    tpidr_el1, x17

    ldp    x9, x10, [x0, #CPU_CTX_TPIDR_EL0]
    msr    tpidr_el0, x9
    msr    tpidrro_el0, x10

    ldp    x13, x14, [x0, #CPU_CTX_PAR_EL1]
    msr    par_el1, x13
    msr    far_el1, x14

    ldp    x15, x16, [x0, #CPU_CTX_AFSR0_EL1]
    msr    afsr0_el1, x15
    msr    afsr1_el1, x16

    ldp    x17, x9, [x0, #CPU_CTX_CONTEXTIDR_EL1]
    msr    contextidr_el1, x17
    msr    vbar_el1, x9

     // Restore AArch32 system registers if the build has instructed so
#if CPU_CTX_INCLUDE_AARCH32_REGS
    ldp    x11, x12, [x0, #CPU_CTX_SPSR_ABT]
    msr    spsr_abt, x11
    msr    spsr_und, x12

    ldp    x13, x14, [x0, #CPU_CTX_SPSR_IRQ]
    msr    spsr_irq, x13
    msr    spsr_fiq, x14

    ldp    x15, x16, [x0, #CPU_CTX_DACR32_EL2]
    msr    dacr32_el2, x15
    msr    ifsr32_el2, x16

    ldr    x17, [x0, #CPU_CTX_FP_FPEXC32_EL2]
    msr    fpexc32_el2, x17
#endif
     // Restore NS timer registers if the build has instructed so
#if NS_TIMER_SWITCH
    ldp    x10, x11, [x0, #CPU_CTX_CNTP_CTL_EL0]
    msr    cntp_ctl_el0, x10
    msr    cntp_cval_el0, x11

    ldp    x12, x13, [x0, #CPU_CTX_CNTV_CTL_EL0]
    msr    cntv_ctl_el0, x12
    msr    cntv_cval_el0, x13

    ldr    x14, [x0, #CPU_CTX_CNTKCTL_EL1]
    msr    cntkctl_el1, x14
#endif

     // No explict ISB required here as ERET covers it
    ret

 // -----------------------------------------------------
 // Save all the GP registers when
 // a world switch occurs.
 // Except: LR & SP_EL0 will be store prior to calling it.
 // -----------------------------------------------------
gp_regs_ctx_save_all:
    stp    x0, x1, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X0]
    stp    x2, x3, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X2]
    stp    x4, x5, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X4]
    stp    x6, x7, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X6]
    stp    x8, x9, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X8]
    stp    x10, x11, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X10]
    stp    x12, x13, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X12]
    stp    x14, x15, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X14]
    stp    x16, x17, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X16]
    stp    x18, x19, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X18]
    stp    x20, x21, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X20]
    stp    x22, x23, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X22]
    stp    x24, x25, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X24]
    stp    x26, x27, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X26]
    stp    x28, x29, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X28]
    str    x30, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_LR]
    ret

 // -----------------------------------------------------
 // Restore all the GP registers when
 // a world switch occurs.
 // Except: LR & SP_EL0 will be store prior to calling it.
 // -----------------------------------------------------
gp_regs_ctx_restore_all:
    ldp    x0, x1, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X0]
    ldp    x2, x3, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X2]
    ldp    x4, x5, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X4]
    ldp    x6, x7, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X6]
    ldp    x8, x9, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X8]
    ldp    x10, x11, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X10]
    ldp    x12, x13, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X12]
    ldp    x14, x15, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X14]
    ldp    x16, x17, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X16]
    ldp    x18, x19, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X18]
    ldp    x20, x21, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X20]
    ldp    x22, x23, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X22]
    ldp    x24, x25, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X24]
    ldp    x26, x27, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X26]
    ldp    x28, x29, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X28]
    ret

 // -----------------------------------------------------
 // Only save and restore the callee saved registers when
 // a world switch occurs.
 // -----------------------------------------------------
gp_regs_ctx_save_args_n_callee:
    stp    x0, x1, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X0]
    stp    x2, x3, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X2]
    stp    x4, x5, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X4]
    stp    x6, x7, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X6]
    stp    x18, x19, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X18]
    stp    x20, x21, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X20]
    stp    x22, x23, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X22]
    stp    x24, x25, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X24]
    stp    x26, x27, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X26]
    stp    x28, x29, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X28]
    ret

gp_regs_ctx_restore_args_n_callee_eret:
    ldp    x0, x1, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X0]
    ldp    x2, x3, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X2]
    b    gp_regs_ctx_restore_callee_eret

gp_regs_ctx_restore_callee_eret:
    ldp    x4, x5, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X4]
    ldp    x6, x7, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X6]
    ldp    x18, x19, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X18]
    ldp    x20, x21, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X20]
    ldp    x22, x23, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X22]
    ldp    x24, x25, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X24]
    ldp    x26, x27, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X26]
    ldp    x28, x29, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_X28]
    ldp    x30, x17, [sp, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_LR]
    msr    spsel, #1
    mrs    x16, sp_el0
    msr    sp_el0, x17

     // Obfuscate out the temporary registers.
     // Except X8. As it is used to relocate R_AARCH64_LD_PREL_LO19
     // against external symbol.
     // `x8' can not be used when making a shared object;
    ldr  x9,  =REGISTER_OBFUSCATE
    mov  x10,  x9
    mov  x11,  x9
    mov  x12,  x9
    mov  x13,  x9
    mov  x14,  x9
    mov  x15,  x9
    mov  x16,  x9
    mov  x17,  x9

     // return from exception
    eret

     // -----------------------------------------------------
     // This routine assumes that the SP_EL3 is pointing to
     // a valid context structure from where the gp regs and
     // other special registers can be retrieved.
     // -----------------------------------------------------
el3_exit:
     // -----------------------------------------------------
     // Switch to SP_EL0
     // -----------------------------------------------------
    msr    spsel, #0

     // -----------------------------------------------------
     // Restore SPSR_EL3, ELR_EL3 and SCR_EL3 prior to ERET
     // -----------------------------------------------------
    ldr    x18, [sp, #CPU_CTX_EL3STATE_OFFSET + CPU_CTX_SCR_EL3]
    ldp    x16, x17, [sp, #CPU_CTX_EL3STATE_OFFSET + CPU_CTX_SPSR_EL3]
    msr    scr_el3, x18
    msr    spsr_el3, x16
    msr    elr_el3, x17

     // Restore saved general purpose registers and return
    b    gp_regs_ctx_restore_args_n_callee_eret
