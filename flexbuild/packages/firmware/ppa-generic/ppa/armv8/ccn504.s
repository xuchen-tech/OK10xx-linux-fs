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

#include "policy.h"

//-----------------------------------------------------------------------------

.global _init_interconnect
.global _manual_L3_flush

//-----------------------------------------------------------------------------

 // CCN-504 defines
.equ  L3_BASE_MN,            0x04000000
.equ  L3_BASE_HNI,           0x04080000
.equ  L3_BASE_HNF,           0x04200000
.equ  HNF_NODE_BASE_ADDR,    0x04200000
.equ  L3_BASE_RNI,           0x04800000
.equ  L3_SDCR_CLR,           0xFFFFFFFF
.equ  SA_AUX_CTRL_OFFSET,    0x0500
.equ  OLYRNF_LIST_OFFSET,    0x180
.equ  OLYRNIDVM_LIST_OFFSET, 0x1A0
.equ  RNI_12_OFFSET,         0x0C0000
.equ  RNI_20_OFFSET,         0x140000
.equ  RNI_AUX_CTL_OFFSET,    0x0500

.equ  RETRY_CNT,             800

.equ  SA_AUX_CTRL_PTB,       0x10
.equ  SA_AUX_CTRL_SDW,       0x200
.equ  RNI_AUX_CTL_WUO,       0x10

.equ  PRR_OFFSET,         0x10
.equ  PSR_OFFSET,         0x18
.equ  HNF_NODE_OFFSET,    0x10000
.equ  P_STATE_MASK,       0xF
.equ  REGION_BITMAP_MASK, 0x3

.equ  HNF_REGION_COUNT, 8
.equ  L3_PSTATE_FULL,   3
.equ  L3_PSTATE_HALF,   2
.equ  L3_PSTATE_SFONLY, 1
.equ  L3_PSTATUS_FULL,        0xC
.equ  L3_PSTATUS_HALF,        0x8
.equ  L3_PSTATUS_FULL_2_HALF, 0xF
.equ  L3_PSTATUS_HALF_2_FULL, 0xB
.equ  L3_PSTATUS_SFONLY,      0x4

 // the tick-count values are dependent upon the value
 // written to the frequency counter
.equ  TICKS_PER_USEC,         12
.equ  TICKS_PER_100_USEC,     1200
.equ  TICKS_PER_200_USEC,     2400
.equ  TICKS_PER_300_USEC,     3600

//-----------------------------------------------------------------------------

 // this function sets snoop mode and dvm domains in the L3 cache
 // in:  none
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7
_init_interconnect:

     // set the WUO bit for system performance
    mov  x0, #POLICY_PERF_WRIOP
    mov  x2, #RNI_12_OFFSET
    cbz  x0, 1f
    mov  x2, #RNI_20_OFFSET
1:
    mov  x1, #L3_BASE_RNI
    add  x1, x1, x2
    ldr  x0, [x1, #RNI_AUX_CTL_OFFSET]
    orr  x0, x0, #RNI_AUX_CTL_WUO
    str  x0, [x1, #RNI_AUX_CTL_OFFSET]
    
     // terminate the pos barriers in sa_aux_ctl reg
    mov  x1, #L3_BASE_HNI
    ldr  x0, [x1, #SA_AUX_CTRL_OFFSET]
    orr  x0, x0, #SA_AUX_CTRL_PTB
     // set ser_devne_wr
    orr  x0, x0, #SA_AUX_CTRL_SDW
    str  x0, [x1, #SA_AUX_CTRL_OFFSET]

     // read the oly_rnf_nodeid_list register for rnf regions
    mov  x1, #L3_BASE_MN
    ldr  x4, [x1, #OLYRNF_LIST_OFFSET]

     // read the oly_rnidvm_nodeid_list for dvm domains
    mov  x1, #L3_BASE_MN
    ldr  x5, [x1, #OLYRNIDVM_LIST_OFFSET]
     // orr the rnf_nodeid values in with the dvm_nodeid values 
    orr  x5, x5, x4

    mov  x1, #L3_BASE_HNF
    mov  x2, #0x220
    mov  x3, #0x210
    mov  x6, #L3_SDCR_CLR

     // x1 = hnf base address
     // x2 = sdcr_clr offset, dvm_clr offset
     // x3 = sdcr_set offset, dvm_set offset
     // x4 = sdcr node ids
     // x5 = dvm domain ids
     // x6 = sdcr/dvm clr value
     // x7 = region count

    mov  x7, #8
write_sdcr_regs:
    str  x6, [x1, x2]
    str  x4, [x1, x3]
    add  x1, x1, #HNF_NODE_OFFSET
    subs x7, x7, #1
    b.ne write_sdcr_regs   

     // write to dvm_clr, dvm_set regs
    ldr  w1, =L3_BASE_MN
    str  x6, [x1, x2]
    str  x5, [x1, x3]

    dsb  #0xf

    mov  x0, #RETRY_CNT      // retry count for simulator models
    mov  x2, #0x200
poll_mn_ddcr:
    ldr  x6, [x1, x2]
    cmp  x6, x5
    b.eq 1f
    subs x0, x0, #1
    b.ne poll_mn_ddcr
1:
    mov  x1, #L3_BASE_HNF
    mov  x7, #8
2:
    mov  x0, #RETRY_CNT      // retry count for simulator models
poll_hnf_sdcr:
    ldr  x6, [x1, x2]
    cmp  x6, x4
    b.eq 3f
    subs x0, x0, #1
    b.ne poll_hnf_sdcr
3:
    add  x1, x1, #HNF_NODE_OFFSET
    subs x7, x7, #1
    b.ne 2b  

    ret

 //----------------------------------------------------------------------------
 // this function flushes (clean+invalidate) the L3 cache nodes in
 // the CCN-504 device
 // in:  none
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10
_manual_L3_flush:
     // hn-f nodes 0-7
     // base address = ADDR_HNF_NODE_BASE
     // hnf node offset = 0x10000
     // PRR = HN-F P-state Request Register (base+node_offset+0x0010)
     // PSR = HN-F P-state Status  Register (base+node_offset+0x0018)
     // p-state req bits [1:0]:
     //     b'00 = NOL3
     //     b'01 = SFONLY
     //     b'10 = HALF
     //     b'11 = FULL
     // p-state status bits [3:0]:
     //     b'0000 = NOL3
     //     b'0100 = SFONLY
     //     b'1000 = HALF
     //     b'1100 = FULL

     // save the LR
    mov  x10, x30

    mov  x4, #0                   // loop counter
    mov  x5, #L3_BASE_HNF         // HNF node base address
    mov  x9, #0                   // org_state map (2-bits per region)
    mov  x7, #HNF_REGION_COUNT
2:
     // read the hn-f pstate status register and log the pstate
     // into the org_state map
    ldr   x0, [x5, #PSR_OFFSET]

     // check the retention bits for operational status
    tst   x0, #0x30
    b.ne  1f
     // extract the pstate
    and   x0, x0, #P_STATE_MASK
    mov   x1, #L3_PSTATUS_HALF
    cmp   x0, x1
    b.eq  org_state_half

    mov   x1, #L3_PSTATUS_FULL_2_HALF
    cmp   x0, x1
    b.eq  org_state_half

    mov   x1, #L3_PSTATUS_HALF_2_FULL
    cmp   x0, x1
    b.eq  org_state_full

    mov   x1, #L3_PSTATUS_FULL
    cmp   x0, x1
    b.eq  org_state_full

    b     1f        // for all other pstates, we do nothing

org_state_half:
     // insert the half-state into the org_state map
    mov  x1, #L3_PSTATE_HALF
    lsl  x2, x4, #1
    lsl  x1, x1, x2
    orr  x9, x9, x1
    b    set_pstate

org_state_full:
     // insert the full-state into the org_state map
    mov  x1, #L3_PSTATE_FULL
    lsl  x2, x4, #1
    lsl  x1, x1, x2
    orr  x9, x9, x1

set_pstate:
     // set pstate
    mov  x1, #L3_PSTATE_SFONLY
    str  x1, [x5, #PRR_OFFSET]

1:
     // loop control - increment to next HNF region
    add  x5, x5, #HNF_NODE_OFFSET
    add  x4, x4, #1
    cmp  x4, x7
    b.ne 2b

     // exit early if there's nothing to process
    cbz  x9, flush_L3_end

     // create a bitmap with sfonly in each non-null region
    mov  x0, x9
    mov  x1, #HNF_REGION_COUNT
    mov  x2, #0
3:
    mov  x3, #REGION_BITMAP_MASK
    lsl  x4, x2, #1
    lsl  x3, x3, x4

    tst  x0, x3
    b.eq 4f

    bic  x0, x0, x3
    mov  x3, #1
    lsl  x3, x3, x4
    orr  x0, x0, x3
4:
    add  x2, x2, #1
    cmp  x1, x2
    b.gt 3b
    
     // x0 has a bitmap of sfonly-values
    bl   poll_L3_pstate

     // x9 has bitmap of original pstate values
    mov  x0, x9
    bl   reset_pstate

     // x9 has bitmap of original pstate values
    mov  x0, x9
    bl   poll_L3_pstate

flush_L3_end:
     // restore LR and return
    mov x30, x10
    ret

 //----------------------------------------------------------------------------

 // this function resets the L3 hnf region nodes to their original pstate after
 // a flush operation
 // in:  x0 = region pstate bitmap
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7
reset_pstate:
     // get the max number of regions
    mov  x7, #HNF_REGION_COUNT
    mov  x2, x0                   // region bitmap

    mov  x6, #0                   // loop counter
    mov  x5, #L3_BASE_HNF         // pstate request reg offset
1:
     // ...extract the region from the bitmap (2-bits per region)
    mov  x3, #REGION_BITMAP_MASK
    lsl  x4, x6, #1
    lsl  x3, x3, x4
    and  x1, x2, x3
    lsr  x1, x1, x4
    cbz  x1, 2f

     // write the original state (in x1) to the pstate req reg
    str  x1, [x5, #PRR_OFFSET]

2:
     // loop control
    add  x5, x5, #HNF_NODE_OFFSET
    add  x6, x6, #1
    cmp  x7, x6
    b.ne 1b

    ret

 //----------------------------------------------------------------------------

 // this function polls the L3 regions which have a non-null entry in the
 // region bitmap (x0), until they have a pstate corresponding to their
 // 2-bit entry in the bitmap
 // in:  x0, pstate bitmap of regions to poll, each pair of bits contains
 //          the state we are polling that region for
 //          Ex: region 0=bits[1:0], region 1=bits[3:2], region 2=bits[5:4]
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7
poll_L3_pstate:
     // load the timer value reg with the timeout tick count
    mov x3, #TICKS_PER_100_USEC
    msr cntps_tval_el1, x3

     // enable the secure timer
    mov x1, #3
    msr cntps_ctl_el1, x1

     // get the max number of regions
    mov  x7, #HNF_REGION_COUNT
    mov  x2, x0                     // region bitmap
3:
    mov  x6, #0                     // loop counter
    mov  x5, #L3_BASE_HNF           // node base address

1:   // for each region...
     // ...test that the region bits are non-null
    mov  x3, #0x3
    lsl  x4, x6, #1
    lsl  x3, x3, x4
    ands x0, x2, x3                 // extract the desired pstate    
    b.eq 2f

    lsr  x0, x0, x4                 // right-justify pstate bits
    lsl  x0, x0, #2                 // move pstate bits into pstatus position
    mov  x4, x0

     // ...read the pstate status reg
    ldr  x0, [x5, #PSR_OFFSET]

     // extract the status
    and  x0, x0, #P_STATE_MASK
    cmp  x0, x4
    b.ne 2f

     // ...clear the region field in the bitmap
    bic  x2, x2, x3
2:
     // loop control - inner loop
    add  x5, x5, #HNF_NODE_OFFSET
    add  x6, x6, #1
    cmp  x7, x6
    b.ne 1b

     // read the timer til its value is zero (or less)
    mrs  x3, cntps_tval_el1
     // tval is a 32-bit down-count register
    cmp  w3, wzr
    b.le 4f

     // loop control - outer loop
    cbnz x2, 3b
4:
     // disable the secure timer
    mov x1, #2
    msr cntps_ctl_el1, x1

    ret

 //----------------------------------------------------------------------------
 //----------------------------------------------------------------------------

