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

 // gic init settings
#define GICR_TYPER          0x0008
#define GICR_WAKER          0x0014
#define GICR_IGROUPRn       0x0080
#define GICR_ISENABLERn     0x0100
#define GICR_IGROUPMODRn    0x0d00

//-----------------------------------------------------------------------------

  .section .text, "ax"
  .global  non_boot_core
 
//-----------------------------------------------------------------------------

check_if_released:
     // read mp affinity reg (MPIDR_EL1) into x1
    mrs x1, MPIDR_EL1
     // extract the affinity 0 & 1 fields - bits [15:0]
    mov   x0, xzr
    bfxil x0, x1, #0, #16

non_boot_core:
     // x0 contains bits[15:0] of mpidr_el1

     // put the 64-bit base address of the dcfg block into x2
    adr  x3, ADDR_BASE_DCFG
    ldr  w2, [x3]

     // x2 contains base addr of dcfg block

     // generate a mask for the COREDISR reg - this algorithm assumes 2 cores
     // per cluster, and must be adjusted if that is not the case
     // ((cluster << 1) + core) = SoC core
     // (1 << SoC core) = mask
    mov   x5, xzr
    mov   x6, xzr
    bfxil x5, x0, #8, #8  // extract cluster
    bfxil x6, x0, #0, #8  // extract cpu #
    lsl   x5, x5, #1
    add   x5, x5, x6
    mov   x6, #1
    lsl   x6, x6, x5
    
     // read COREDISR
    ldr  w5, [x2, #0x94]
     // see if this core needs to be disabled
    tst  w5, w6
    b.ne core_disable

     // x0 contains bits[15:0] of mpidr_el1
     // x2 contains base addr of dcfg block

     // see if non-boot cores have been released
     // read the 32-bit SCRATCHRW7 register (offset 0x218 in the dcfg block)
    ldr  w4, [x2, #0x218]

     // if w4 == 0, then the non-boot cores have not been released
     // if w4 != 0, then all non-boot cores have been released
    cbnz  w4, non_boot_exit 

     // if all the cores have not been released, then check to see if
     // this specific core has been released

     // read SCRATCHRW8 [15:0] (offset 0x21C in the dcfg block)
    ldr  w4, [x2, #0x21C]
    and  w4, w4, #0xFFFF

     // cmp the value in SCRATCHRW8 w/bits[15:0] of mpidr_el1
    cmp  w0, w4
     // if they match then this specific core has been released
    b.eq non_boot_exit

     // if not released, wfe
    wfe

     // on wfe wakeup, check again to see if released
    b  check_if_released

     // -----------------------------------------

non_boot_exit:
     // x2 contains base addr of dcfg block

     // read the 32-bit BOOTLOCPTRL register (offset 0x400 in the dcfg block)
    ldr  w3, [x2, #0x400]
     // read the 32-bit BOOTLOCPTRH register (offset 0x404 in the dcfg block)
    ldr  w4, [x2, #0x404]
     // create a 64-bit BOOTLOCPTR address in x3
    orr  x3, x3, x4, LSL #32

     // branch to the address in x3
    br  x3

     //------------------------------------------

core_disable:
     // turn off caches in SCTRL
    mrs  x1, SCTLR_EL3
    mov  x3, #0x1004
    bic  x1, x1, x3
    msr  SCTLR_EL3, x1

     // clean & inv L1 dcache
    bl  flush_L1_dcache 

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
    adr  x2, ADDR_TIMER_BASE
    ldr  w3, [x2]
    mov  w1, #1
    str  w1, [x3, #0x0]

     // set the initial count value
    mov  w1, #0x07000000
    str  w1, [x3, #0x8]

     // enable the timer clocks
    adr  x2, ADDR_PMU_BASE
    ldr  w3, [x2]
    mov  x1, 0xff
    str  x1, [x3, #0x18a0]

     // disable the core - only reset brings the core back from this
    isb
    dsb  #0xF
disable_loop:
    wfi
    b  disable_loop

     // -----------------------------------------

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
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

SMC64_EL3_2_EL2:
    .long  0xC200FF03
SMC64_EL3_2_EL1:
    .long  0xC200FF04

ADDR_TIMER_BASE:
    .long 0x023d0000
ADDR_PMU_BASE:
    .long 0x01e30000

//-----------------------------------------------------------------------------

