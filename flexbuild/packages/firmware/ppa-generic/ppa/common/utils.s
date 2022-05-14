//-----------------------------------------------------------------------------
// 
// Copyright (c) 2013-2016 Freescale Semiconductor, Inc.
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

#include "smc.h"
#include "psci.h"

#if (LSCH == 3)
#include "lsch3.h"
#elif (LSCH == 2)
#include "lsch2.h"
#endif

//-----------------------------------------------------------------------------

.global _get_core_mask_lsb
.global _get_cluster_state
.global _core_on_cnt_clstr
.global _is_mpidr_valid
.global _get_ocram_2_init
.global _ocram_init_upper
.global _ocram_init_lower
.global _ocram_init
.global _prep_init_ocram_hi
.global _prep_init_ocram_lo
.global _prep_ocram_init
.global _get_load_addr
.global _check_sec_disabled

//-----------------------------------------------------------------------------

 // This function returns the bit mask corresponding to the mpidr_el1 value.
 // the mask is returned in w0.
 // this bit mask references the core in SoC registers such as
 // BRR, COREDISABLEDSR where the LSB represents core0
 //
#if (SYMMETRICAL_CLUSTERS)
 // This algorithm assumes that all clusters are symmetrical, that
 // each cluster contains the same number of cores. If that is true for
 // this SoC, then this function is appropriate.
 //
 // in:   x0  - mpidr_el1 value for the core
 // out:  w0  = core mask (non-zero)
 //       w0  = 0 for error (bad input mpidr value)
 // uses x0, x1, x2
_get_core_mask_lsb:
     //
     // generate a lsb-based mask for the core
     // SoC core = ((cluster * cpu_per_cluster) + core)
     // mask = (1 << SoC core)
    mov   w1, wzr
    mov   w2, wzr
    bfxil w1, w0, #8, #8  // extract cluster
    bfxil w2, w0, #0, #8  // extract cpu #

    mov   w0, wzr

     // error checking
    cmp   w1, #CLUSTER_COUNT
    b.ge  1f
    cmp   w2, #CPU_PER_CLUSTER
    b.ge  1f

    mov   w0, #CPU_PER_CLUSTER
    mul   w1, w1, w0
    add   w1, w1, w2
    mov   w2, #0x1
    lsl   w0, w2, w1
1:
    ret
#else
     // if the clusters are not symmetrical, then add an appropriate
     // function in the specific device soc.s file.
#endif

//-----------------------------------------------------------------------------

 // this function returns the state of a cluster

#if (SYMMETRICAL_CLUSTERS)
 // in:  x0 = 0-based cluster number
 // out: x0 = AFFINITY_LEVEL_ON (at least one core of the cluster is ON)
 //      x0 = AFFINITY_LEVEL_OFF (all cores of the cluster are [OFF | DISABLED | RESET])
 //      x0 = AFFINITY_LEVEL_PEND (at least one core is ON_PENDING and no cores are ON)
 // uses: x0, x1, x2, x3, x4, x5, x6
_get_cluster_state:
     // save link register
    mov   x6, x30

    mov   x3, #CPU_PER_CLUSTER
    mov   x4, #AFFINITY_LEVEL_OFF
    orr   x5, xzr, x0, LSL #8

     // x3 = loop count
     // x4 = cluster state
     // x5 = mpidr of core
3:
    mov   x0, x5
    bl    _get_core_mask_lsb

     // x0 = core mask lsb

    mov   x1, #CORE_STATE_DATA
    bl    _getCoreData

     // x0 = core state

    mov   x1, #CORE_RELEASED
    cmp   x0, x1
    b.eq  1f

    mov   x1, #CORE_PENDING

    b.ne  4f
    mov   x4, #AFFINITY_LEVEL_PEND
4:
     // increment to next core
    add   x5, x5, #1
     // decrement loop counter
    sub   x3, x3, #1
     // exit if done
    cbz   x3, 2f
    b     3b

1:
    mov   x4, #AFFINITY_LEVEL_ON
2:
    mov   x0, x4
    mov   x30, x6
    ret

#else
     // if the clusters are not symmetrical, then add an appropriate
     // function in the specific device soc.s file.
#endif

//-----------------------------------------------------------------------------

 // this function returns the number of active cores in the given cluster

#if (SYMMETRICAL_CLUSTERS)
 // in:  x0 = cluster number (mpidr format)
 // out: x0 = count of cores running
 // uses x0, x1, x2, x3, x4, x5, x6
_core_on_cnt_clstr:
    mov  x6, x30

    and  x4, x0, #MPIDR_CLUSTER_MASK
    mov  x3, #CPU_PER_CLUSTER
    mov  x5, xzr

     // x3 = loop count
     // x4 = core mpidr
     // x5 = accumulated count of running cores
     // x6 = saved link reg

3:
    mov  x0, x4
    bl   _get_core_mask_lsb

     // x0 = core mask lsb

    mov  x1, #CORE_STATE_DATA
    bl   _getCoreData

     // x0 = core state

    cmp  x0, #CORE_OFF_MAX
    b.le 1f
    add  x5, x5, #1
1:
     // decrement the loop count and exit if finished
    sub  x3, x3, #1
    cbz  x3, 2f

     // increment to the next core
    add  x4, x4, #1
    b    3b

2:
     // xfer the count to the output reg
    mov  x0, x5
    mov  x30, x6
    ret

#else
     // if the clusters are not symmetrical, then add an appropriate
     // function in the specific device soc.s file.
#endif

//-----------------------------------------------------------------------------

 // this function determines if a given mpidr value is valid for this SoC

#if (SYMMETRICAL_CLUSTERS)
 // in:  x0 = mpidr[15:0]
 // out: x0 == 0, mpidr not valid
 //      x0 != 0, mpidr is valid
 // uses x0, x1, x2
_is_mpidr_valid:

     // compare the 0-based affinity 1 field (cluster #) with the number
     // of clusters in this SoC
    and  x1, x0, #MPIDR_AFFINITY1_MASK
    lsr  x1, x1, #MPIDR_AFFINITY1_OFFSET
     // compare to the cluster count of this SoC
    ldr  x2, =CLUSTER_COUNT
    cmp  x2, x1
    b.le 2f

     // compare the 0-based affinity 0 field (core number) with the
     // number of cores per cluster in this SoC
    and  x1, x0, #MPIDR_AFFINITY0_MASK
    mov  x2, #CPU_PER_CLUSTER
    cmp  x2, x1
    b.le 2f

1:
     // the mpidr is valid
    mov  x0, #1
    b    3f
2:
     // the mpidr is not valid
    mov  x0, #0
3:
    ret

#else
     // if the clusters are not symmetrical, then add an appropriate
     // function in the specific device soc.s file.
#endif

//-----------------------------------------------------------------------------

 // this function gets the 64-bit address of a module loaded by the bootloader
 // in:  none
 // out: x0 = 64-bit load address
 // uses x0, x1, x2
_get_load_addr:

#if (CNFG_SPD)
     // get the 64-bit base address of the block
    ldr  x0, =LOAD_ADDR_BASE

     // read the lo-order 32-bits of the address
    ldr  w2, [x0, #LOAD_OFFSET_LO]

     // read the 32-bit hi-order bits of the address
    ldr  w1, [x0, #LOAD_OFFSET_HI]

     // create a 64-bit address
    orr  x0, x2, x1, LSL #32
#else
    mvn  x0, xzr
#endif

    ret

//-----------------------------------------------------------------------------

 // this function checks SVR for sec disabled
 // out: x0  = 0 if SEC enabled
 //      x0 != 0 if SEC disabled
 // uses x0, x1
_check_sec_disabled:
    ldr  x0, =DCFG_BASE_ADDR
    ldr  w1, [x0, #DCFG_SVR_OFFSET]

     // w1 = SVR register

#if (LSCH == 3)
     // chassis 3 SoC registers are LE
    mov  w0, w1
#elif (LSCH == 2)
     // chassis 2 SoC registers are BE
    rev  w0, w1
#endif

    and  w0, w0, #SVR_SEC_MASK
    ret  

//-----------------------------------------------------------------------------

 // this function returns the start address and size of two equal regions
 // of ocram for initialization purposes. If the stack area is in the top
 // of ocram, this is left out of the ocram regions to be initialized
 // in:  x0 = 0, return start addr and size of lower ocram region
 //         = 1, return start addr and size of upper ocram region
 //         = 2, return start addr and size of entire ocram region
 // out: x0 = start address of region
 //      x1 = size of region in bytes
 // uses x0, x1, x2, x3, x4, x5
_get_ocram_2_init:
    mov  x5, x30
    mov  x4, x0

#if (DATA_LOC == DATA_IN_DDR)

    mov  x1, #OCRAM_SIZE_IN_BYTES
    mov  x2, #OCRAM_BASE_ADDR

#else

    ldr  x0, =STACK_BASE_ADDR
    mov  x1, #OCRAM_SIZE_IN_BYTES
    mov  x2, #OCRAM_BASE_ADDR

     // stack and data region is in top of ocram
     // determine the amount of stack/data space
    add  x3, x2, x1
    sub  x3, x3, x0

     // x3 = total stack/data size in bytes

     // adjust the size by subtracting the amount of stack space
    sub  x1, x1, x3

#endif

1:
     // x1 = size of ocram to initialize
     // x2 = OCRAM_BASE_ADDR

    cmp  x4, #2
    b.ne 4f
     // the request is for all of ocram
    mov  x0, x2
    b    3f

4:
     // divide size in half
    lsr  x1, x1, #1

     // determine if the upper or lower region of ocram is requested
    cbz  x4, 2f

     // process the upper region of ocram

     // x1 = size of ocram to initialize
     // x2 = OCRAM_BASE_ADDR
    
     // add size to base addr to get start addr of upper half
    add  x0, x2, x1
    b    3f

2:   // process the lower region of ocram

     // x1 = size of ocram to initialize
     // x2 = OCRAM_BASE_ADDR

     // get start address of lower region
    mov  x0, x2

3:
    mov  x30, x5
    ret

//-----------------------------------------------------------------------------

 // this function initializes the upper-half of OCRAM for ECC checking
 // in:  none
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10
_ocram_init_upper:
    mov  x10, x30

     // set the start flag
    mov  w0, #1
    bl   _set_task1_start

     // get the start address and size of the upper region
    mov  x0, #OCRAM_REGION_UPPER
    bl   _get_ocram_2_init

     // x0 = start address
     // x1 = size in bytes

     // convert bytes to 64-byte chunks (we are using quad load/store pairs)
    lsr  x1, x1, #6

     // x0 = start address
     // x1 = size in 64-byte chunks
1:
     // for each location, read and write-back
    dc   ivac, x0
    isb
    ldp  x2, x3, [x0]
    ldp  x4, x5, [x0, #16]
    ldp  x6, x7, [x0, #32]
    ldp  x8, x9, [x0, #48]
    stp  x2, x3, [x0]
    stp  x4, x5, [x0, #16]
    stp  x6, x7, [x0, #32]
    stp  x8, x9, [x0, #48]
    dc   cvac, x0

    sub  x1, x1, #1
    cbz  x1, 2f
    add  x0, x0, #64
    b    1b

2:
     // set the done flag
    mov  x0, #1
    bl   _set_task1_done

     // restore link register
    mov  x30, x10

     // clean the registers
    mov  x0,  #0
    mov  x1,  #0
    mov  x2,  #0
    mov  x3,  #0
    mov  x4,  #0
    mov  x5,  #0
    mov  x6,  #0
    mov  x7,  #0
    mov  x8,  #0
    mov  x9,  #0
    mov  x10, #0

    ret

//-----------------------------------------------------------------------------

 // this function initializes the lower-half of OCRAM for ECC checking
 // in:  none
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10
_ocram_init_lower:
    mov  x10, x30

     // set the start flag
    mov  x0, #1
    bl   _set_task2_start

     // get the start address and size of the lower region
    mov  x0, #OCRAM_REGION_LOWER
    bl   _get_ocram_2_init

     // x0 = start address
     // x1 = size in bytes

     // convert bytes to 64-byte chunks (using quad load/store pair ops)
    lsr  x1, x1, #6

     // x0 = start address
     // x1 = size in 64-byte chunks
1:
     // for each location, read and write-back
    dc   ivac, x0
    isb
    ldp  x2, x3, [x0]
    ldp  x4, x5, [x0, #16]
    ldp  x6, x7, [x0, #32]
    ldp  x8, x9, [x0, #48]
    stp  x2, x3, [x0]
    stp  x4, x5, [x0, #16]
    stp  x6, x7, [x0, #32]
    stp  x8, x9, [x0, #48]
    dc   cvac, x0

    sub  x1, x1, #1
    cbz  x1, 2f
    add  x0, x0, #64
    b    1b

2:
     // set the done flag
    mov  x0, #1
    bl   _set_task2_done

     // make sure the data accesses are complete
    dsb  sy
    isb

     // restore link register
    mov  x30, x10

     // clean the registers
    mov  x0,  #0
    mov  x1,  #0
    mov  x2,  #0
    mov  x3,  #0
    mov  x4,  #0
    mov  x5,  #0
    mov  x6,  #0
    mov  x7,  #0
    mov  x8,  #0
    mov  x9,  #0
    mov  x10, #0

    ret

//-----------------------------------------------------------------------------

 // this function initializes OCRAM for ECC checking
 // in:  none
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10
_ocram_init:
    mov  x10, x30

     // set the start flag
    mov  x0, #1
    bl   _set_task1_start

     // get the start address and size of the OCRAM
    mov  x0, #OCRAM_REGION_ALL
    bl   _get_ocram_2_init

     // x0 = start address
     // x1 = size in bytes

     // convert bytes to 64-byte chunks (using quad load/store pair ops)
    lsr  x1, x1, #6

     // x0 = start address
     // x1 = size in 64-byte chunks
1:
     // for each location, read and write-back
    dc   ivac, x0
    isb
    ldp  x2, x3, [x0]
    ldp  x4, x5, [x0, #16]
    ldp  x6, x7, [x0, #32]
    ldp  x8, x9, [x0, #48]
    stp  x2, x3, [x0]
    stp  x4, x5, [x0, #16]
    stp  x6, x7, [x0, #32]
    stp  x8, x9, [x0, #48]
    dc   cvac, x0

    sub  x1, x1, #1
    cbz  x1, 2f
    add  x0, x0, #64
    b    1b

2:
     // set the done flag
    mov  x0, #1
    bl   _set_task1_done

     // make sure the data accesses are complete
    dsb  sy
    isb

     // restore link register
    mov  x30, x10

     // clean the registers
    mov  x0,  #0
    mov  x1,  #0
    mov  x2,  #0
    mov  x3,  #0
    mov  x4,  #0
    mov  x5,  #0
    mov  x6,  #0
    mov  x7,  #0
    mov  x8,  #0
    mov  x9,  #0
    mov  x10, #0

    ret

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

 // DO NOT CALL THIS FUNCTION FROM THE BOOT CORE!!
 // this function uses a secondary core to initialize the upper portion of OCRAM
 // the core does not return from this function
_prep_init_ocram_hi:

     // invalidate the icache
    ic  iallu
    isb

     // enable the icache on the secondary core
    mrs  x1, sctlr_el3
    orr  x1, x1, #SCTLR_I_MASK
    msr  sctlr_el3, x1
    isb

     // init the range of ocram
    bl  _ocram_init_upper

     // get the core mask
    mrs  x0, MPIDR_EL1
    bl   _get_core_mask_lsb

     // x0 = core mask lsb

     // turn off icache, mmu
    mrs  x1, sctlr_el3
    bic  x1, x1, #SCTLR_I_MASK
    bic  x1, x1, #SCTLR_M_MASK
    msr  sctlr_el3, x1

     // invalidate the icache
    ic  iallu
    isb

     // wakeup the bootcore - it might be asleep waiting for us to finish
    sev
    isb
    sev
    isb

    mov  x5, x0

     // x5 = core mask lsb

1:
     // see if our state has changed to CORE_PENDING
    mov   x0, x5
    mov   x1, #CORE_STATE_DATA
    bl    _getCoreData

     // x0 = core state

    cmp   x0, #CORE_PENDING
    b.eq  2f
     // if not core_pending, then wfe
    wfe
    b  1b

2:
     // branch to the start code in the monitor
    adr  x0, _secondary_core_init
    br   x0

//-----------------------------------------------------------------------------

 // DO NOT CALL THIS FUNCTION FROM THE BOOT CORE!!
 // this function uses a secondary core to initialize the lower portion of OCRAM
 // the core does not return from this function
_prep_init_ocram_lo:

     // invalidate the icache
    ic  iallu
    isb

     // enable the icache on the secondary core
    mrs  x1, sctlr_el3
    orr  x1, x1, #SCTLR_I_MASK
    msr  sctlr_el3, x1
    isb

     // init the range of ocram
    bl  _ocram_init_lower    // 0-9

     // get the core mask
    mrs  x0, MPIDR_EL1
    bl   _get_core_mask_lsb  // 0-2

     // x0 = core mask lsb

     // turn off icache
    mrs  x1, sctlr_el3
    bic  x1, x1, #SCTLR_I_MASK
    msr  sctlr_el3, x1

     // invalidate tlb
    tlbi  alle3
    dsb   sy
    isb

     // invalidate the icache
    ic  iallu
    isb

     // wakeup the bootcore - it might be asleep waiting for us to finish
    sev
    isb
    sev
    isb

    mov  x5, x0

     // x5 = core mask lsb

1:
     // see if our state has changed to CORE_PENDING
    mov   x0, x5
    mov   x1, #CORE_STATE_DATA
    bl    _getCoreData

     // x0 = core state

    cmp   x0, #CORE_PENDING
    b.eq  2f
     // if not core_pending, then wfe
    wfe
    b  1b

2:
     // branch to the start code in the monitor
    adr  x0, _secondary_core_init
    br   x0

//-----------------------------------------------------------------------------

 // DO NOT CALL THIS FUNCTION FROM THE BOOT CORE!!
 // this function uses a secondary core to initialize OCRAM
 // the core does not return from this function
_prep_ocram_init:

     // get the core mask
    mrs  x0, MPIDR_EL1
    bl   _get_core_mask_lsb  // 0-2
    mov  x11, x0

     // x0  = core mask lsb
     // x11 = core mask lsb

     // set the core state to WORKING_INIT
    mov  x1, #CORE_STATE_DATA
    mov  x2, #CORE_WORKING_INIT
    bl   _setCoreData

     // x11 = core mask lsb

     // invalidate the icache
    ic  iallu
    isb

     // enable the icache on the secondary core
    mrs  x1, sctlr_el3
    orr  x1, x1, #SCTLR_I_MASK
    msr  sctlr_el3, x1
    isb

     // init the range of ocram
    bl  _ocram_init    // 0-10

     // turn off icache
    mrs  x1, sctlr_el3
    bic  x1, x1, #SCTLR_I_MASK
    msr  sctlr_el3, x1

     // invalidate tlb
    tlbi  alle3
    dsb   sy
    isb

     // invalidate the icache
    ic  iallu
    isb

     // x11 = core mask lsb

1:
     // see if our state has changed to CORE_PENDING
    mov   x0, x11
    mov   x1, #CORE_STATE_DATA
    bl    _getCoreData

     // x0 = core state

    cmp   x0, #CORE_PENDING
    b.eq  2f

     // if not core_pending, then change state of core
    mov   x0, x11
    mov   x1, #CORE_STATE_DATA
    mov   x2, #CORE_WFE
    bl    _setCoreData

    wfe
    b  1b

2:
     // branch to the start code in the monitor
    adr  x0, _secondary_core_init
    br   x0

//-----------------------------------------------------------------------------

