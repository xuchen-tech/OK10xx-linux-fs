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
// Author Rod Dorris <rod.dorris@nxp.com>
// 
//-----------------------------------------------------------------------------

  .section .text, "ax"
 
//-----------------------------------------------------------------------------

#include "boot.h"

//-----------------------------------------------------------------------------

  .global __dead_loop
  .global am_i_boot_core
  .global init_EL3
  .global init_EL2
  .global read_reg_dcfg
  .global write_reg_dcfg
  .global get_exec_addr

#if (SIMULATOR_BUILD)
    .global _reset_vector_el3
#else
    .global reset_vector_el3:
#endif

//-----------------------------------------------------------------------------

.align 3

#if (SIMULATOR_BUILD)
_reset_vector_el3:
#else
reset_vector_el3:
#endif

     // perform any critical init that must occur early
    bl   early_init

     // see if this is the boot core
    bl   am_i_boot_core
    cbnz x0, non_boot_core

     // if we're here, then this is the boot core -

     // perform EL3 init
    bl   init_EL3

     // determine the SoC personality and configure
    bl   set_personality

     // initialize the interconnect
    bl  init_CCN502

     // perform base EL2 init
    bl   init_EL2

     // see if we have an execution start address
    bl   get_exec_addr

#if (SIMULATOR_BUILD)
    adr  x0, _start_monitor_el3
    b    boot_core_exit
#else
    cbnz x0,  boot_core_exit

     // if we get here then the start address was NULL, so
     // get a start address based on the boot device
    bl   get_boot_device
     // if we still have a NULL address, then shut it down
    cbz  x0, __dead_loop
#endif

boot_core_exit:
     // save the boot address
    mov  x5, x0

     // branch to the boot loader addr
    br  x5

//-----------------------------------------------------------------------------

 // this function gets the lsb mask for the bootcore
 // in:  none
 // out: w0 = lsb mask
 // uses x0, x1
get_bootcore_mask:

     // read BRCORENBR
    ldr  x0, =RESET_BASE_ADDR
    ldr  w1, [x0, #BRCORENBR_OFFSET]

    mov  w0, #1
    lsl  w0, w0, w1
    ret

//-----------------------------------------------------------------------------

 // determine if this is the boot core
 // in:  none
 // out: w0  = 0, boot_core
 //      w0 != 0, secondary core
 // uses x0, x1, x2, x3
am_i_boot_core:
    mov  x3, x30

     // read mp affinity reg (MPIDR_EL1)
    Get_MPIDR_EL1 x1, x0

    bl  get_core_mask_lsb
    mov x2, x0

    bl  get_bootcore_mask
    sub w0, w0, w2

    mov  x30, x3
    ret

//-----------------------------------------------------------------------------

 // this function returns a 64-bit execution address of the core in x0
 // out: x0, start address
 // uses x0, x1, x2 
get_exec_addr:
     // get the 64-bit base address of the dcfg block
    ldr  x2, =DCFG_BASE_ADDR

     // read the 32-bit BOOTLOCPTRL register (offset 0x400 in the dcfg block)
    ldr  w0, [x2, #BOOTLOCPTRL_OFFSET]

     // read the 32-bit BOOTLOCPTRH register (offset 0x404 in the dcfg block)
    ldr  w1, [x2, #BOOTLOCPTRH_OFFSET]

     // create a 64-bit BOOTLOCPTR address
    orr  x0, x0, x1, LSL #32
    ret

//-----------------------------------------------------------------------------

 // perform base EL3 initialization on this core
 // in:   none
 // out:  none
 // uses: x0, x1
init_EL3:
     // initialize SCTLR_EL3
     // M,   bit [0]  = 0
     // A,   bit [1]  = 0
     // C,   bit [2]  = 0
     // SA,  bit [3]  = 1
     // I,   bit [12] = 1
     // WXN, bit [19] = 0
     // EE,  bit [25] = 0
    mov  x0, #0x8
     // turn on the icache
    orr  x0, x0, #0x1000
    msr  SCTLR_EL3, x0

     // initialize CPUECTLR
     // SMP, bit [6] = 1
    mrs  x0, S3_1_c15_c2_1
    orr  x0, x0, #0x40
    msr S3_1_c15_c2_1, x0

     // initialize CPTR_EL3
    msr  CPTR_EL3, xzr

     // initialize SCR_EL3
     // NS,   bit[0]  = 1
     // IRQ,  bit[1]  = 0
     // FIQ,  bit[2]  = 0
     // EA,   bit[3]  = 0
     // Res1, bit[4]  = 1
     // Res1, bit[5]  = 1
     // SMD,  bit[7]  = 0
     // HCE,  bit[8]  = 1
     // SIF,  bit[9]  = 0
     // RW,   bit[10] = 1
     // ST,   bit[11] = 0
     // TWI,  bit[12] = 0
     // TWE,  bit[13] = 0
    mov  x1, #0x531
    msr  SCR_EL3, x1

     // initialize the value of ESR_EL3 to 0x0
    msr ESR_EL3, xzr

     // set the timer/counter frequency
    ldr   x1, =TIMER_BASE_ADDR
    ldr   x0, [x1, #CNTFID0_OFFSET]
     // write the counter frequency to CNTFRQ_EL0
    msr   cntfrq_el0, x0

     //----------------------

     // synchronize the system register writes
    isb
    ret

//-----------------------------------------------------------------------------

 // this function performs base init on the EL2 level
 // in:  none
 // out: none
 // uses x0, x1
init_EL2:

    mov  x1, #0x80000000
    msr  hcr_el2, x1

    msr  sctlr_el2, xzr

    mov  x0, #0x33FF
    msr  cptr_el2, x0
    isb
    ret

//-----------------------------------------------------------------------------

 // this function performs any initialization that must be done very early
 // in:  none
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x12
early_init:

     // load the VBAR_EL3 register with the base address of the EL3 vectors
    adr  x0, el3_vector_space
    msr  VBAR_EL3, x0

     // initialize the L2 ram latency
    mrs   x1, S3_1_c11_c0_2
    mov   x2, x1
    mov   x0, #0x1C7
    bic   x1, x1, x0
     // set L2 data ram latency bits [2:0]
    orr   x1, x1, #0x2
     // set L2 tag ram latency bits [8:6]
    orr   x1,  x1, #0x80
     // if we changed the register value, write back the new value
    cmp   x2, x1
    b.eq  1f
    msr   S3_1_c11_c0_2, x1
1:
    isb
    ret

//-----------------------------------------------------------------------------

 // in:  none
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7
init_CCN502:

    ret

//-----------------------------------------------------------------------------

 // establish the personality by reading COREDISABLEDSR and releasing from
 // reset the cores marked to be disabled
 // in:  none
 // out: none
 // uses: x0, x1, x2, x3, x4, x5, x6, x7
set_personality:
    mov  x7, x30

     // read COREDISABLEDSR
    ldr  x1, =DCFG_BASE_ADDR
    ldr  w2, [x1, #COREDISABLEDSR_OFFSET]

     // get the bootcore mask bit
    bl  get_bootcore_mask

     // x0 = bootcore mask

     // clear the bit for the bootcore - we don't allow
     // disabling the bootcore
    bic  w2, w2, w0  

     // exit if there are no cores to disable
    cbz  w2, 3f

    ldr  w3, =MAX_SOC_CORES
    mov  w4, #1

     // x1 = base addr of dcfg block
     // w2 = COREDISABLEDSR
     // w3 = loop count
     // w4 = core mask
2:
    tst  w2, w4
    beq  1f

     // if we are here, then the core indicated by the mask in x4
     // needs to be disabled - to do that we must release it from
     // reset

     // read-modify-write BRR
    ldr  x5, =RESET_BASE_ADDR
    ldr  w6, [x5, #BRR_OFFSET]
    orr  w6, w6, w4
    str  w6, [x5, #BRR_OFFSET]
    isb

     // send sev for any cores trapped in wfe
    sev
    isb
    sev
    isb
1:
     // advance the core bit mask
    lsl  w4, w4, #1

     // decrement the loop count
    subs w3, w3, #1
    bne  2b
3:
    mov  x30, x7
    ret

//-----------------------------------------------------------------------------

 // this function returns the execution start address by determining the
 // boot device
 // in:  none
 // out: x0 = 64-bit start address (base address + offset)
 //      x0 = 0, error return
 // uses x0, x1
get_boot_device:
     // get the 64-bit base address of the dcfg block
    ldr  x1, =DCFG_BASE_ADDR

     // we need the boot location device - this is found in the
     // RCW word bits [264:260], which are in the register
     // RCWSR 9 offset 0x120 in DCFG block

     // read RCWSR9, offset 0x120 in the DCFG block
    ldr  w0, [x1, #RCWSR9_OFFSET]
     // extract 5 bits [8:4]
    mov   w1, wzr
    bfxil w1, w0, #4, #5

     // compare the bits in w1 to determine the boot location device

     // see if boot-loc is pci express #1 (5'b00000)
    cmp  w1, #0x0
    b.eq boot_pciex_1
     // see if boot-loc is the memory complex (5'b10100)
    cmp  w1, #0x14
    b.eq boot_mem_cmplx
     // see if boot-loc is ocram (5'b10101)
    cmp  w1, #0x15
    b.eq boot_ocram
     // see if boot-loc is serial nor (5'b11010)
    cmp  w1, #0x1A
    b.eq boot_ser_nor

     // if we get here then there is some nasty error
    mov  x0, #0
    b    exit_dev_addr

     // get the base address for the specified device
boot_pciex_1:
    ldr  x1, =DEV_PCIEX1_BASE
    b    finish_dev_addr

boot_mem_cmplx:
    ldr  x1, =DEV_MEMCMPLX_BASE
    b    finish_dev_addr

boot_ocram:
    ldr  x1, =DEV_OCRAM_BASE
    b    finish_dev_addr

boot_ser_nor:
    ldr  x1, =DEV_SERNOR_BASE
     // b    finish_dev_addr
     // Note: this is a fall-thru condition, don't
     //       insert anything after this line

finish_dev_addr:
     // 4Kb offset
    mov  x0, #0x1000
     // construct the boot address from the device base address + offset
    orr  x0, x0, x1
exit_dev_addr:
    ret

//-----------------------------------------------------------------------------

 // EL3 exception vectors

   // VBAR_ELn bits [10:0] are RES0
  .align 11
  .global el3_vector_space
el3_vector_space:

   // current EL using SP0 ----------------------

     // synchronous exceptions
    mov  x11, #0x0
    b    __dead_loop

     // IRQ interrupts
  .align 7
     // put the irq vector offset in x3
    mov  x11, #0x80
    b    __dead_loop

     // FIQ interrupts
  .align 7
     // put the fiq vector offset in x3
    mov  x11, #0x100
    b    __dead_loop

     // serror exceptions
  .align 7
     // put the serror vector offset in x3
    mov  x11, #0x180
    b    __dead_loop

   // current EL using SPx ----------------------
  
     // synchronous exceptions
  .align 7
    mov  x11, #0x200
    b    __dead_loop

     // IRQ interrupts
  .align 7
    mov  x11, #0x280
    b    __dead_loop

     // FIQ interrupts
  .align 7
    mov  x11, #0x300
    b    __dead_loop

     // serror exceptions
  .align 7
    mov  x11, #0x380
    b    __dead_loop

   // lower EL using AArch64 --------------------

     // synchronous exceptions
  .align 7
    mov  x11, #0x400
    b    __dead_loop

     // IRQ interrupts
  .align 7
    mov  x11, #0x480
    b    __dead_loop

     // FIQ interrupts
  .align 7
    mov  x11, #0x500
    b    __dead_loop

     // serror exceptions
  .align 7
    mov  x11, #0x580
    b    __dead_loop

   // lower EL using AArch32 --------------------

     // synchronous exceptions
  .align 7
    mov  x11, #0x600
    b    __dead_loop

     // IRQ interrupts
  .align 7
    mov  x11, #0x680
    b    __dead_loop

     // FIQ interrupts
  .align 7
    mov  x11, #0x700
    b    __dead_loop

     // serror exceptions
  .align 7
    mov  x11, #0x780
    b    __dead_loop

     //------------------------------------------

__dead_loop:
    wfe
    b __dead_loop

//-----------------------------------------------------------------------------

 // read a register in the DCFG block
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_dcfg:
     // get base addr of dcfg block
	ldr  x1, =DCFG_BASE_ADDR
    ldr  w2, [x1, x0]
    mov  w0, w2
    ret

//-----------------------------------------------------------------------------

 // write a register in the DCFG block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2
write_reg_dcfg:
	ldr  x2, =DCFG_BASE_ADDR
    str  w1, [x2, x0]
    ret

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

GPP_ROMCODE_VERSION:
    .long  0x00000004

//-----------------------------------------------------------------------------

