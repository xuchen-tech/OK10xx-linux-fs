//-----------------------------------------------------------------------------
// 
// Copyright (c) 2013-2016, Freescale Semiconductor, Inc.
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

  .section .text, "ax"
 
//-----------------------------------------------------------------------------
    
#include "soc.h"
#include "psci.h"
#include "smc.h"

//-----------------------------------------------------------------------------

  .global _test_psci

//-----------------------------------------------------------------------------

.align 3
.equ  PSCI_V_MAJOR,   0x00000000
.equ  PSCI_V_MINOR,   0x00000002

//-----------------------------------------------------------------------------

.ltorg

//-----------------------------------------------------------------------------

.align 3
_test_psci:
    b  _test_psci

     // call psci version to trigger the end of boot services
    ldr  x0, =PSCI_VERSION_ID
    smc  0x0
    nop
    nop
    nop
     // test for return value in x0

     // call the prng 64-bit interface for a 32-bit seed
     // x0 = function id
     // x1 = 0, 32-bit seed returned
     // 32-bit prng seed is returned in x1
    ldr  x0, =SIP_PRNG_64
    mov  x1, xzr
    smc  0x0
    nop
    nop
    nop
     // test for return value in x1

     // call the prng 64-bit interface for a 64-bit seed
     // x0 = function id
     // x1 = 1, 64-bit seed returned
     // 64-bit prng seed is returned in x1
    ldr  x0, =SIP_PRNG_64
    mov  x1, #1
    smc  0x0
    nop
    nop
    nop
     // test for return value in x1

end_test:
    b  end_test

//-----------------------------------------------------------------------------

