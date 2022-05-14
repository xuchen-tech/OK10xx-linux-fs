//-----------------------------------------------------------------------------
// 
// Copyright (c) 2014-2016 Freescale Semiconductor
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

#include "aarch64.h"
#include "smc.h"

//-----------------------------------------------------------------------------

  .global _el3_vector_base
  .global __el3_dead_loop
  .global _smc_exit
 
//-----------------------------------------------------------------------------

 // EL3 exception vectors

   // VBAR_ELn bits [10:0] are RES0
   // each vector entry is 128-bytes wide,
   // leaving space for 32 instructions
  .align 11
_el3_vector_base:

   // current EL using SP0 ----------------------

     // synchronous exceptions
    b    synch_handler

     // IRQ interrupts
  .align 7  //allocates space for each ISR
     // put the irq vector offset in x3
    mov  x11, #0x80
    b    __el3_dead_loop

     // FIQ interrupts
  .align 7
     // put the fiq vector offset in x3
    mov  x11, #0x100
    b    __el3_dead_loop

     // serror exceptions
  .align 7
     // put the serror vector offset in x3
    mov  x11, #0x180
    b    __el3_dead_loop

   // current EL using SPx ----------------------
  
     // synchronous exceptions
  .align 7
    b  synch_handler

     // IRQ interrupts
  .align 7
    mov  x11, #0x280
    b    __el3_dead_loop

     // FIQ interrupts
  .align 7
    b  synch_handler

     // serror exceptions
  .align 7
    mov  x11, #0x380
    b    __el3_dead_loop

   // lower EL using AArch64 --------------------

     // synchronous exceptions
  .align 7
    EL3_CkAndIsolate
    b  synch_handler

     // IRQ interrupts
  .align 7
    EL3_IsolateOnEntry
    mov  x11, #0x480
    b    __el3_dead_loop

     // FIQ interrupts
  .align 7
    EL3_IsolateOnEntry
    mov  x11, #0x500
    b    __el3_dead_loop

     // serror exceptions
  .align 7
    EL3_IsolateOnEntry
    mov  x11, #0x580
    b    __el3_dead_loop

   // lower EL using AArch32 --------------------

     // synchronous exceptions
  .align 7
    EL3_CkAndIsolate
    b  synch_handler

     // IRQ interrupts
  .align 7
    EL3_IsolateOnEntry
    mov  x11, #0x680
    b    __el3_dead_loop

     // FIQ interrupts
  .align 7
    EL3_IsolateOnEntry
    mov  x11, #0x700
    b    __el3_dead_loop

     // serror exceptions
  .align 7
    EL3_IsolateOnEntry
    mov  x11, #0x780
    b    __el3_dead_loop

     //------------------------------------------

  .align 2
synch_handler:
     // save the volatile registers
     //  save these as pairs of registers to maintain the
     //  required 16-byte alignment on the stack
    stp   x0, x1, [sp, #-16]!
    stp   x2, x3, [sp, #-16]!
    dsb   sy
    isb

     // read the ESR_EL3 register to get exception type
    mrs   x1, ESR_EL3
     // extract the exception type
    mov   x2, xzr
    bfxil w2, w1, #26, #6

     // test if this is a A64 SMC exception
    cmp   w2, #0x17
    b.eq  a64smc_router

     // test if this is a A32 SMC exception
    cmp   w2, #0x13
    b.eq  a32smc_router

     // unhandled exception
    b    __el3_dead_loop

     //------------------------------------------

a64smc_router:
     // mask interrupts
    msr  DAIFset, #0xF

     // test bit [31] - must be '1' for "fast-calls"
    tst   x0, #SMC_FAST_CALL_BIT
#if (CNFG_SPD)
    b.eq  1f
#else
    b.eq  _smc_unimplemented
#endif

     // test bits [23:16] - must be 0x00 for "fast-calls"
    tst   x0, #SMC_FAST_CALL_FIELD
    b.ne  _smc_unimplemented

1:
     // restore the volatile registers
     //  access these as pairs of registers to maintain the
     //  required 16-byte alignment on the stack
    ldp  x2, x3, [sp], #16
    ldp  x0, x1, [sp], #16

     // save the non-volatile aarch64 registers (smc v1.1)
     //  save these as pairs of registers to maintain the
     //  required 16-byte alignment on the stack
    stp  x4,  x5,  [sp, #-16]!
    stp  x6,  x7,  [sp, #-16]!
    stp  x8,  x9,  [sp, #-16]!
    stp  x10, x11, [sp, #-16]!
    stp  x12, x13, [sp, #-16]!
    stp  x14, x15, [sp, #-16]!
    stp  x16, x17, [sp, #-16]!
    stp  x18, x30, [sp, #-16]!
    dsb   sy
    isb

#if (CNFG_SPD)
    tst   x0, #SMC_FAST_CALL_BIT
    b.eq  _smc_trstd_os_handler
#endif

     // test for smc32 or smc64 interface
    tst   x0, #SMC_INTERFACE_BIT
    b.eq  smc32_handler
    b.ne  smc64_handler

     //------------------------------------------

a32smc_router:
#if ((CORE == 72) || (CORE == 57))
     // provide an extremely fast path for the 
     // SMCCC_ARCH_WORKAROUND_1 function
    mov   w1, #ARCH32_WKRND_1_UPPER
    orr   w1, w1, #ARCH32_WKRND_1_LOWER
    cmp   w0, w1
    b.eq  smc_fast_return
#endif

     // test bit [31] - must be '1' for "fast-calls"
    tst   w0, #SMC_FAST_CALL_BIT
    b.eq  _smc_unimplemented

     // test bits [23:16] - must be 0x00 for "fast-calls"
    tst   w0, #SMC_FAST_CALL_FIELD
    b.ne  _smc_unimplemented

     // test for smc32 or smc64 interface
    tst   w0, #SMC_INTERFACE_BIT
    b.ne  _smc_unimplemented

     // smc32 interface called from aarch32
     // mask interrupts
    msr  DAIFset, #0xF

     // restore the volatile registers
     //  access these as pairs of registers to maintain the
     //  required 16-byte alignment on the stack
    ldp  x2, x3, [sp], #16
    ldp  x0, x1, [sp], #16

     // save the non-volatile aarch32 registers
     //  save these as pairs of registers to maintain the
     //  required 16-byte alignment on the stack
    stp  x4,  x5,  [sp, #-16]!
    stp  x6,  x7,  [sp, #-16]!
    stp  x8,  x9,  [sp, #-16]!
    stp  x10, x11, [sp, #-16]!
    stp  x12, x13, [sp, #-16]!
    stp  x14, x15, [sp, #-16]!
    stp  x16, x17, [sp, #-16]!
    stp  x18, x30, [sp, #-16]!
    dsb  sy
    isb
    b    smc32_handler

     //------------------------------------------

__el3_dead_loop:
    wfe
    b __el3_dead_loop

//-----------------------------------------------------------------------------

_smc_exit:

     // x0 = status code

     // restore the aarch32/64 non-volatile registers
    ldp  x18, x30, [sp], #16
    ldp  x16, x17, [sp], #16
    ldp  x14, x15, [sp], #16
    ldp  x12, x13, [sp], #16
    ldp  x10, x11, [sp], #16
    ldp  x8,  x9,  [sp], #16
    ldp  x6,  x7,  [sp], #16
    ldp  x4,  x5,  [sp], #16

     // perform some isolation tasks on exit
    EL3_ExitToNS

     // return from exception
    eret

//-----------------------------------------------------------------------------

 // Note: there is no return value with this fast exit
smc_fast_return:

     // restore the volatile registers
     //  access these as pairs of registers to maintain the
     //  required 16-byte alignment on the stack
    ldp  x2, x3, [sp], #16
    ldp  x0, x1, [sp], #16

     // perform some isolation tasks on exit
    EL3_ExitToNS

     // return from exception
    eret

//-----------------------------------------------------------------------------


