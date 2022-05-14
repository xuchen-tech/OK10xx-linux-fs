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
// Authors:
//  Sumit Garg <sumit.garg@nxp.com>
//  Pankaj Gupta <pankaj.gupta@nxp.com>
//
//-----------------------------------------------------------------------------

#include "sp_asm.h"

.section .text, "ax"

.global secure_el1os_enter_sp
.global secure_el1os_exit_sp

 // Enter secure el1 ctx and save el3 runtime ctx
secure_el1os_enter_sp:
     // Make space for the registers that we're going to save
    mov    x3, sp
    str    x3, [x0, #0]
    sub    sp, sp, #SP_C_RT_CTX_SIZE

     // Save callee-saved registers on to the stack
    stp    x19, x20, [sp, #SP_C_RT_CTX_X19]
    stp    x21, x22, [sp, #SP_C_RT_CTX_X21]
    stp    x23, x24, [sp, #SP_C_RT_CTX_X23]
    stp    x25, x26, [sp, #SP_C_RT_CTX_X25]
    stp    x27, x28, [sp, #SP_C_RT_CTX_X27]
    stp    x29, x30, [sp, #SP_C_RT_CTX_X29]
    str    x18, [sp, #SP_C_RT_CTX_X18]

     // flush dcache
    mov    x0, #0
    bl     _cln_inv_all_dcache

     // Everything is setup now. el3_exit() will
     // use the secure context to restore to the
     // general purpose and EL3 system registers to
     // ERET into SP.
    bl    el3_exit

 // Exit secure el1 ctx and restore el3 runtime ctx
secure_el1os_exit_sp:
     // Restore the previous stack
    mov    sp, x0

     // Restore callee-saved registers on to the stack
    ldp    x19, x20, [x0, #(SP_C_RT_CTX_X19 - SP_C_RT_CTX_SIZE)]
    ldp    x21, x22, [x0, #(SP_C_RT_CTX_X21 - SP_C_RT_CTX_SIZE)]
    ldp    x23, x24, [x0, #(SP_C_RT_CTX_X23 - SP_C_RT_CTX_SIZE)]
    ldp    x25, x26, [x0, #(SP_C_RT_CTX_X25 - SP_C_RT_CTX_SIZE)]
    ldp    x27, x28, [x0, #(SP_C_RT_CTX_X27 - SP_C_RT_CTX_SIZE)]
    ldp    x29, x30, [x0, #(SP_C_RT_CTX_X29 - SP_C_RT_CTX_SIZE)]
    ldr    x18, [x0, #(SP_C_RT_CTX_X18 - SP_C_RT_CTX_SIZE)]

     // This should take us back to the instruction
     // after the call to the last secure_el1os_enter_sp().
     // Place the second parameter to x0 so that the
     // caller will see it as a return value from the
     // original entry call
    mov    x0, x1
    ret
