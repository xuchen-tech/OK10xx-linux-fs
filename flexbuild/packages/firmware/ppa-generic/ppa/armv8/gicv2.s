//-----------------------------------------------------------------------------
// 
// Copyright (c) 2015-2016, Freescale Semiconductor, Inc. All rights reserved.
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
//-----------------------------------------------------------------------------

.section .text, "ax"

//-----------------------------------------------------------------------------

#include "lsch2.h"
#include "soc.mac"

//-----------------------------------------------------------------------------

.global _gic_init_percpu
.global _gic_init_common

//-----------------------------------------------------------------------------

 // this function performs one-time gic init, executed by the boot core
 // out: none
 // uses x0, x1, x2, x3, x4, [x13, x14, x15]
_gic_init_common:
    mov   x4, x30

    bl   _getGICD_BaseAddr

    mov   w2, #0x3                   // EnableGrp0 | EnableGrp1
    str   w2, [x0, GICD_CTLR_OFFSET] // Secure GICD_CTLR
    ldr   w2, [x0, GICD_TYPER_OFFSET]
    and   w3, w2, #0x1f              // ITLinesNumber
    cbz   w3, 1f                     // No SPIs
    add   x1, x0, (GICD_IGROUPR0_OFFSET + 4)
    mov   w2, #~0                    // Config SPIs as Grp1
0:  
    str   w2, [x1], #0x4
    sub   w3, w3, #0x1
    cbnz  w3, 0b
1:
    mov   x30, x4
    ret

//-----------------------------------------------------------------------------

 // this function performs per-cpu gic init, executed by each core
 // out: none
 // uses x0, x1, x2, x3, x4, [x13, x14, x15]
_gic_init_percpu:
    mov  x4, x30

     // initialize distributor

    bl   _getGICD_BaseAddr

    mov   w2, #~0                         // Config SGIs and PPIs as Grp1
    str   w2, [x0, GICD_IGROUPR0_OFFSET]  // GICD_IGROUPR0
    mov   w2, #0x1                        // Enable SGI 0
    str   w2, [x0, GICD_ISENABLERn_OFFSET]

     // initialize Cpu Interface

    bl   _getGICC_BaseAddr
    mov  x1, x0

    mov   w2, #0x1e7                      // Disable IRQ/FIQ Bypass &
                                          //  enable Ack Group1 Interrupt &
                                          //  enableGrp0 & EnableGrp1
    str   w2, [x1, GICC_CTLR_OFFSET]      // Secure GICC_CTLR
    mov   w2, #0x1 << 7                   // Non-Secure access to GICC_PMR
    str   w2, [x1, GICC_PMR_OFFSET]

    mov  x30, x4
    ret

//-----------------------------------------------------------------------------

