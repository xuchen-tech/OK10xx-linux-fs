//-----------------------------------------------------------------------------
//
// Copyright (c) 2013-2016, ARM Limited and Contributors. All rights reserved.
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
// Authors:
//  Sumit Garg <sumit.garg@nxp.com>
//  Pankaj Gupta <pankaj.gupta@nxp.com>
//
//-----------------------------------------------------------------------------

#ifndef __ARCH64_H__
#define __ARCH64_H__

 // Thirty Two General Purpose Registers
#define CPU_CTX_GPREGS_OFFSET        0x0
#define CPU_CTX_X0        0x0
#define CPU_CTX_X1        0x8
#define CPU_CTX_X2        0x10
#define CPU_CTX_X3        0x18
#define CPU_CTX_X4        0x20
#define CPU_CTX_X5        0x28
#define CPU_CTX_X6        0x30
#define CPU_CTX_X7        0x38
#define CPU_CTX_X8        0x40
#define CPU_CTX_X9        0x48
#define CPU_CTX_X10        0x50
#define CPU_CTX_X11        0x58
#define CPU_CTX_X12        0x60
#define CPU_CTX_X13        0x68
#define CPU_CTX_X14        0x70
#define CPU_CTX_X15        0x78
#define CPU_CTX_X16        0x80
#define CPU_CTX_X17        0x88
#define CPU_CTX_X18        0x90
#define CPU_CTX_X19        0x98
#define CPU_CTX_X20        0xa0
#define CPU_CTX_X21        0xa8
#define CPU_CTX_X22        0xb0
#define CPU_CTX_X23        0xb8
#define CPU_CTX_X24        0xc0
#define CPU_CTX_X25        0xc8
#define CPU_CTX_X26        0xd0
#define CPU_CTX_X27        0xd8
#define CPU_CTX_X28        0xe0
#define CPU_CTX_X29        0xe8
#define CPU_CTX_LR        0xf0    // x30 saves Link Register
#define CPU_CTX_SP_EL0        0xf8    // x31 saves stack-pointer for EL0
#define CPU_CTX_GPREGS_END        0x100


 // Five EL3 State Registers
#define CPU_CTX_EL3STATE_OFFSET    (CPU_CTX_GPREGS_OFFSET + CPU_CTX_GPREGS_END)
#define CPU_CTX_SCR_EL3            0x0
#define CPU_CTX_RUNTIME_SP        0x8
#define CPU_CTX_SPSR_EL3        0x10
#define CPU_CTX_ELR_EL3            0x18
#define CPU_CTX_EL3STATE_END        0x20

 // Twenty Two EL1 System Registers
#define CPU_CTX_SYSREGS_OFFSET    (CPU_CTX_EL3STATE_OFFSET + CPU_CTX_EL3STATE_END)
#define CPU_CTX_SPSR_EL1        0x0
#define CPU_CTX_ELR_EL1            0x8
#define CPU_CTX_SCTLR_EL1        0x10
#define CPU_CTX_ACTLR_EL1        0x18
#define CPU_CTX_CPACR_EL1        0x20
#define CPU_CTX_CSSELR_EL1        0x28
#define CPU_CTX_SP_EL1            0x30
#define CPU_CTX_ESR_EL1            0x38
#define CPU_CTX_TTBR0_EL1        0x40
#define CPU_CTX_TTBR1_EL1        0x48
#define CPU_CTX_MAIR_EL1        0x50
#define CPU_CTX_AMAIR_EL1        0x58
#define CPU_CTX_TCR_EL1            0x60
#define CPU_CTX_TPIDR_EL1        0x68
#define CPU_CTX_TPIDR_EL0        0x70
#define CPU_CTX_TPIDRRO_EL0        0x78
#define CPU_CTX_PAR_EL1            0x80
#define CPU_CTX_FAR_EL1            0x88
#define CPU_CTX_AFSR0_EL1        0x90
#define CPU_CTX_AFSR1_EL1        0x98
#define CPU_CTX_CONTEXTIDR_EL1        0xa0
#define CPU_CTX_VBAR_EL1        0xa8

 // To support Aarch-32 platform on Arch-64,
 // Arch32 registers needs to be included.
 // For that #define flag 'CPU_CTX_INC_ARCH32_REGS'
 // needs to enabled.
#if CPU_CTX_INCLUDE_AARCH32_REGS
#define CPU_CTX_SPSR_ABT        0xb0
#define CPU_CTX_SPSR_UND        0xb8
#define CPU_CTX_SPSR_IRQ        0xc0
#define CPU_CTX_SPSR_FIQ        0xc8
#define CPU_CTX_DACR32_EL2        0xd0
#define CPU_CTX_IFSR32_EL2        0xd8
#define CPU_CTX_FP_FPEXC32_EL2    0xe0
#define CPU_CTX_TIMER_SYSREGS_OFF        0xf0 // Align to the next 16 byte boundary
#else
#define CPU_CTX_TIMER_SYSREGS_OFF        0xb0
#endif

/*
 * If the timer registers aren't saved and restored, we don't have to reserve
 * space for them in the context
 */
#if NS_TIMER_SWITCH
#define CPU_CTX_CNTP_CTL_EL0	(CPU_CTX_TIMER_SYSREGS_OFF + 0x0)
#define CPU_CTX_CNTP_CVAL_EL0	(CPU_CTX_TIMER_SYSREGS_OFF + 0x8)
#define CPU_CTX_CNTV_CTL_EL0	(CPU_CTX_TIMER_SYSREGS_OFF + 0x10)
#define CPU_CTX_CNTV_CVAL_EL0	(CPU_CTX_TIMER_SYSREGS_OFF + 0x18)
#define CPU_CTX_CNTKCTL_EL1	(CPU_CTX_TIMER_SYSREGS_OFF + 0x20)
#define CPU_CTX_SYSREGS_END	(CPU_CTX_TIMER_SYSREGS_OFF + 0x30) /* Align to the next 16 byte boundary */
#else
#define CPU_CTX_SYSREGS_END	CPU_CTX_TIMER_SYSREGS_OFF
#endif /* __NS_TIMER_SWITCH__ */

#define ARM_CACHE_WRITEBACK_SHIFT    6
#define CACHE_WRITEBACK_GRANULE        (1 << ARM_CACHE_WRITEBACK_SHIFT)

#define GET_RW(mode)        (((mode) >> MODE_RW_SHIFT) & MODE_RW_MASK)
#define GET_EL(mode)        (((mode) >> MODE_EL_SHIFT) & MODE_EL_MASK)
#define GET_SP(mode)        (((mode) >> MODE_SP_SHIFT) & MODE_SP_MASK)
#define GET_M32(mode)        (((mode) >> MODE32_SHIFT) & MODE32_MASK)

#define DISABLE_ALL_EXCEPTIONS \
        (DAIF_FIQ_BIT | DAIF_IRQ_BIT | DAIF_ABT_BIT | DAIF_DBG_BIT)


#define SPSR_64(el, sp, daif)                \
    (MODE_RW_64 << MODE_RW_SHIFT |            \
    ((el) & MODE_EL_MASK) << MODE_EL_SHIFT |    \
    ((sp) & MODE_SP_MASK) << MODE_SP_SHIFT |    \
    (daif) & DAIF_SET_MASK)

#endif //__ARCH64T_H__
