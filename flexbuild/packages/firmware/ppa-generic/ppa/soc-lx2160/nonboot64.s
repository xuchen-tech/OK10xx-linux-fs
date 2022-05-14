//-----------------------------------------------------------------------------
// 
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

#include "boot.h"

//-----------------------------------------------------------------------------

  .global  non_boot_core
  .global  inv_L1_dcache
  .global  get_core_mask_lsb

//-----------------------------------------------------------------------------

non_boot_core:

     // see if this core needs to be disabled
1:
     // read mp affinity reg (MPIDR_EL1) into x0
    Get_MPIDR_EL1 x1, x0

    bl    get_core_mask_lsb
    mov   w3, w0
     // read COREDISABLEDSR
    mov   w0, #COREDISABLEDSR_OFFSET     
    bl    read_reg_dcfg
    tst   w0, w3
    b.ne  core_disable

     // see if this core has been released via core_hold
    mov   x0, x3
    bl    is_core_released_tester
    cbnz  x0, fast_path

     // see if this core has been released via mpidr
    bl    is_core_released_mpidr
    cbnz  x0, secondary_start

     // not released - enter wfe
    wfe
    b    1b 

secondary_start:
     // if we're here, then the core has been released

     // invalidate the L1 icache
    ic iallu
    isb

     // perform the EL3 init
    bl   init_EL3

     // perform base EL2 init
    bl   init_EL2

     // determine start address
    bl   get_exec_addr

     // notify that this core is up
    bl   notify_core_up

     // branch to the address in x0
    br  x0

fast_path:
     // if we're here, then the core has been released

     // perform the EL3 init
    bl   init_EL3

     // perform base EL2 init
    bl   init_EL2

     // determine start address
    bl   get_exec_addr

     // branch to the address in x0
    br  x0

//-----------------------------------------------------------------------------

 // this function returns the bit mask corresponding to the mpidr_el1 value.
 // the mask is returned in w0.
 // this bit mask references the core in the SoC registers such as
 // BRR, COREDISABLEDSR where the LSB represents core0
 // in:   x0  - mpidr_el0 value for the core
 // out:  w0  - core mask
 // uses x0, x1, x2

get_core_mask_lsb:
     // generate a lsb-based mask for the core - this algorithm assumes 2 cores
     // per cluster, and must be adjusted if that is not the case
     // SoC core = ((cluster << 1) + core)
     // mask = (1 << SoC core)
    mov   w1, wzr
    mov   w2, wzr
    bfxil w1, w0, #8, #8  // extract cluster
    bfxil w2, w0, #0, #8  // extract cpu #
    lsl   w1, w1, #1
    add   w1, w1, w2
    mov   w2, #0x1
    lsl   w0, w2, w1
    ret

//-----------------------------------------------------------------------------

 // there is no return from this function
core_disable:
     // turn off caches in SCTRL
    mrs  x1, SCTLR_EL3
    mov  x3, #0x1004
    bic  x1, x1, x3
    msr  SCTLR_EL3, x1

     // clear SMP and set retention control in CPUECTLR
    mov  x1, #0x2
    msr  S3_1_c15_c2_1, x1

     // disable interrupts in DAIF
    mov  x3, 0x3c0
    msr  daif, x3

     // set OSDLR_EL1.DK
    mov  x1, #1
    msr  osdlr_el1, x1

     // enable the generic timer counter
    ldr  x3, =TIMER_BASE_ADDR
    mov  w1, #1
    str  w1, [x3, #CNTCR_OFFSET]

     // set the initial count value
    mov  w1, #0x07000000
    str  w1, [x3, #CNTCV_OFFSET]

     // enable the timer clocks
    ldr  x3, =PMU_BASE_ADDR
    mov  w1, 0xff
    str  w1, [x3, #CLTBENR_OFFSET]

     // write 0x0 to SCRATCHRW7
    ldr  x0, =SCRATCHRW7_OFFSET
    mov  x1, xzr
    bl   write_reg_dcfg

     // disable the core - only reset brings the core back from this
    isb
    dsb  #0xF
disable_loop:
    wfi
    b  disable_loop

//-----------------------------------------------------------------------------

 // clean and invalidate the L1 dcache
 // uses x3-x10
flush_L1_dcache:
    dsb  #0xF
     // select the L1 cache id register
	msr	 csselr_el1, xzr
	isb
     // read the cache id register
	mrs	 x6, ccsidr_el1
     // get the L offset
	and	 x10, x6, #7
	add	 x10, x10, #4

     // extract the max way
	mov	 x3, #0x3ff
	and	 x3, x3, x6, lsr #3
     // get the bit position of the ways field
	clz	 w5, w3

     // extract the max set
	mov	 x4, #0x7fff
	and	 x4, x4, x6, lsr #13

	 // x3  -> number of cache ways - 1
	 // x4  -> number of cache sets - 1
	 // x5  -> bit position of ways
	 // x10 -> L offset

loop_set:
     // load the working copy of the max way
	mov	 x6, x3
loop_way:
     // insert #way and level to data reg
	lsl	 x7, x6, x5
	orr	x9, xzr, x7
     // insert #set to data reg
	lsl	 x7, x4, x10
	orr	 x9, x9, x7

     // clean and invalidate
    dc    cisw, x9
     // decrement the way
    subs  x6, x6, #1
	b.ge  loop_way
     // decrement the set
	subs  x4, x4, #1
	b.ge  loop_set

    isb
	ret

//-----------------------------------------------------------------------------

 // invalidate the L1 dcache
 // uses x3-x10
inv_L1_dcache:
    dsb  #0xF
     // select the L1 cache id register
	msr	 csselr_el1, xzr
	isb
     // read the cache id register
	mrs	 x6, ccsidr_el1
     // get the L offset
	and	 x10, x6, #7
	add	 x10, x10, #4

     // extract the max way
	mov	 x3, #0x3ff
	and	 x3, x3, x6, lsr #3
     // get the bit position of the ways field
	clz	 w5, w3

     // extract the max set
	mov	 x4, #0x7fff
	and	 x4, x4, x6, lsr #13

	 // x3  -> number of cache ways - 1
	 // x4  -> number of cache sets - 1
	 // x5  -> bit position of ways
	 // x10 -> L offset

1:
     // load the working copy of the max way
	mov	 x6, x3
2:
     // insert #way and level to data reg
	lsl	 x7, x6, x5
	orr	x9, xzr, x7
     // insert #set to data reg
	lsl	 x7, x4, x10
	orr	 x9, x9, x7

     // invalidate by set/way
    dc    isw, x9
     // decrement the way
    subs  x6, x6, #1
	b.ge  2b
     // decrement the set
	subs  x4, x4, #1
	b.ge  1b

    isb
	ret

//-----------------------------------------------------------------------------

 // read a register in the SECURE REGISTER FILE
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_secreg:
     // get base addr of secreg file
	ldr  x1, =SEC_REGFILE_BASE_ADDR
    ldr  w2, [x1, x0]
    mov  w0, w2
    ret

//-----------------------------------------------------------------------------

 // write a register in the SECURE REGISTER FILE
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2
write_reg_secreg:
	ldr  x2, =SEC_REGFILE_BASE_ADDR
    str  w1, [x2, x0]
    ret

//-----------------------------------------------------------------------------

 // this function checks to see if the core has been released via mpidr
 // value in scratchrw7
 // in: none
 // out: x0  = 0, core is not released
 //      x0 != 0, core is released
 // uses x0, x1, x2
is_core_released_mpidr:
     // get the 64-bit base address of the dcfg block
    ldr  w2, =DCFG_BASE_ADDR
     // read SCRATCHRW7
    ldr  w0, [x2, #SCRATCHRW7_OFFSET]

     // read mp affinity reg (MPIDR_EL1) into x1
    Get_MPIDR_EL1 x1, x2

    // scratchrw7 in x0
    // mpidr in x2

     // cmp the value in SCRATCHRW7 w/bits[15:0] of mpidr_el1
    cmp   w0, w2
    mov   x0, #0
     // if they do not match, this core has not been released
    b.ne  1f
     // ...else this core is released
    mov   x0, #0x1
1:
    ret

//-----------------------------------------------------------------------------

 // this function checks to see if the core has been released via core_hold
 // Note: this is only valid from a testbench/tester setup
 // in:  x0  = core_mask_lsb
 // out: x0  = 0, core is not released
 //      x0 != 0, core is released
 // uses x0, x1, x2
is_core_released_tester:
     // get the 64-bit base address of the secure register file
    ldr  w2, =SEC_REGFILE_BASE_ADDR
     // read CORE_HOLD
    ldr  w1, [x2, #CORE_HOLD_OFFSET]

     // x0 = core_mask
     // x1 = core_hold register

    tst  x0, x1
    mov  x0, #0
     // if the core bit in core_hold is '0', then core is released
    b.ne 1f

     // the core has been released
    mov  x0, #1
1:
    ret

//-----------------------------------------------------------------------------

 // this function sets a register to notify that the core is up and running
 // perform this step last just before branching to the start address
 // in:  none
 // out: none
 // uses x1
notify_core_up:
     // get the 64-bit base address of the dcfg block
    ldr  w1, =DCFG_BASE_ADDR
     // set SCRATCHRW7 to 0x0
    str  wzr, [x1, #SCRATCHRW7_OFFSET]
    ret

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

