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

#include "soc.h"

//-----------------------------------------------------------------------------

.global _init_interconnect

//-----------------------------------------------------------------------------

 // cci-400 register offsets
.equ SNOOP_CNTRL_SLV0,     0x1000
.equ SNOOP_CNTRL_SLV1,     0x2000
.equ SNOOP_CNTRL_SLV2,     0x3000
.equ SNOOP_CNTRL_SLV3,     0x4000
.equ SNOOP_CNTRL_SLV4,     0x5000
.equ SNOOP_STATUS,         0x000C
.equ CONTROL_OVERRIDE,     0x0000
.equ SECURITY_ACCESS,      0x0008

 // cci-400 register bit masks
.equ SNOOP_CNTRL_SNP_EN,   0x1         // enable snooping on this interface
.equ SNOOP_CNTRL_DVM_EN,   0x2         // enable dvm messaging on this interface
.equ SNOOP_CNTRL_SNP_SUPP, 0x40000000  // interface supports snooping
.equ SNOOP_CNTRL_DVM_SUPP, 0x80000000  // interface supports dvm messaging
.equ STATUS_PENDING,       0x1
.equ CNTRL_OVR_TERM_BARR,  0x8         // terminate barriers
.equ SEC_ACCESS_OVRRIDE,   0x1         // security access override

//-----------------------------------------------------------------------------

 // this function performs initialization on the cci-400
 // in:  none
 // out: none
 // uses x0, x1, x2, x3
_init_interconnect:

     // get the base address of the cci-400
    mov  x0, #CCI_400_BASE_ADDR

     // x0 = cci-400 base addr

     // clear barrier termination
    ldr  w2, [x0, #CONTROL_OVERRIDE]
    bic  w2, w2, #CNTRL_OVR_TERM_BARR
    str  w2, [x0, #CONTROL_OVERRIDE]
    dsb sy
    isb

     // set snoop, dvm mode for slave interface 3 (gpp cluster 0)
    mov  x3, #SNOOP_CNTRL_SLV3
    ldr  w1, [x0, x3]
    orr  w1, w1, #SNOOP_CNTRL_SNP_EN
    orr  w1, w1, #SNOOP_CNTRL_DVM_EN
    str  w1, [x0, x3]

     // x0 = cci-400 base addr

     // see if snoop, dvm are needed for slave interface 4 (gpp cluster 1)
    mov  x3, #SNOOP_CNTRL_SLV4
    ldr  w1, [x0, x3]
    mov  w2, w1
    tst  w1, #SNOOP_CNTRL_DVM_SUPP
    b.eq 1f
    orr  w1, w1, #SNOOP_CNTRL_DVM_EN
1:
    tst  w1, #SNOOP_CNTRL_SNP_SUPP
    b.eq 2f
    orr  w1, w1, #SNOOP_CNTRL_SNP_EN
2:
    cmp  w2, w1
    b.eq 3f
    str  w1, [x0, x3]
3:
     // set dvm for slave interface 2 (TCU)
    ldr  w1, [x0, #SNOOP_CNTRL_SLV2]
    orr  w1, w1, #SNOOP_CNTRL_DVM_EN
    str  w1, [x0, #SNOOP_CNTRL_SLV2]
    isb

     // x0 = cci-400 base addr

     // poll on the status register till the changes are applied
    mov  w2, #CCI400_PEND_CNT
4:
    ldr  w1, [x0, #SNOOP_STATUS]
    tst  w1, #STATUS_PENDING
     // if no change pending, exit
    b.eq 5f
     // decrement the retry count
    sub  w2, w2, #1
     // if retries maxed out, exit
    cbz  w2, 5f
     // else loop and try again
    b    4b
5:
    ret

//-----------------------------------------------------------------------------

