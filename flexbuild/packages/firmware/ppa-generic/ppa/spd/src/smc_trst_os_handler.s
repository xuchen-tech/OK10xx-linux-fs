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
// Author Sumit Garg <sumit.garg@nxp.com>
//
//-----------------------------------------------------------------------------

  .section .text, "ax"

#include "context_arch64_asm.h"

//-----------------------------------------------------------------------------

  .global _smc_trstd_os_handler

//-----------------------------------------------------------------------------

 // This function handles smc calls for trusted os
_smc_trstd_os_handler:
     // Dummy load from stack
    ldr  x10, [sp], #128

     // Get core_data_array ptr from tpidr_el3 register
    mrs  x9, tpidr_el3

     // Parse NS bit from SCR_EL3 to get security state of smc
     // caller.
    mrs  x10, scr_el3
    and  x10, x10, #1

     // Get sec/non-sec context pointer
    add  x9, x9, x10, lsl #3
    ldr  x9, [x9]

     // Save LR, SP_EL0
    mrs  x11, sp_el0
    stp  x30, x11, [x9, #CPU_CTX_GPREGS_OFFSET + CPU_CTX_LR]

     // Save EL3 context registers
    mrs  x12, spsr_el3
    mrs  x13, elr_el3
    mrs  x14, scr_el3
    stp  x12, x13, [x9, #CPU_CTX_EL3STATE_OFFSET + CPU_CTX_SPSR_EL3]
    str  x14, [x9, #CPU_CTX_EL3STATE_OFFSET + CPU_CTX_SCR_EL3]

     // Save context pointer in SP_EL0
    msr  sp_el0, x9

     // Save all general purpose registers using SP_EL0 as
     // context pointer
    msr  spsel, #0
    bl   gp_regs_ctx_save_args_n_callee

     // Restore SP_EL3 stack in sp
    msr  spsel, #1
    dsb  sy
    isb

     // Pass arguments handle register to x5. Also pass security
     // state in x6
    mov  x5, x9
    mov  x6, x10

#if ((DEBUG_BUILD) && (PSCI_TEST))
    b  1f
#endif
    bl   smc_trstd_os_handler
1:
     // Do el3 exit
    b    el3_exit

     // Code flow should never reach here
    b    .
