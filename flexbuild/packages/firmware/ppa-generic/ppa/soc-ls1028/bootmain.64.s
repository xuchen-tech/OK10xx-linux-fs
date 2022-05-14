//-----------------------------------------------------------------------------
// 
// Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
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
 
#include "boot.h"

//-----------------------------------------------------------------------------

#define L2CTLR_EL1   S3_1_c11_c0_2
#define CPUECTLR_EL1 S3_1_c15_c2_1

//-----------------------------------------------------------------------------

  .global _reset_vector_el3
  .global init_EL3
  .global init_EL2
  .global read_reg_dcfg
  .global write_reg_dcfg
  .global get_exec_addr

//-----------------------------------------------------------------------------

#if (SIMULATOR_BUILD)
.global _reset_vector_el3
_reset_vector_el3:
#else
.global reset_vector_el3
reset_vector_el3:
#endif

     // perform any critical init that must occur early
    bl   early_init

     // see if this is the boot core
    bl   am_i_boot_core
    cbnz x0, non_boot_core

     // if we're here, then this is the boot core -

     // perform the EL3 init
    bl   init_EL3

     // setup the cci-400 interconnect
    bl   init_CCI400

     // perform base EL2 init
    bl   init_EL2

     // see if we have an execution start address
    bl   get_exec_addr
    cbnz x0,  boot_core_exit

     // if we get here then the start address was NULL, so
     // get a start address based on the boot device
    bl   get_boot_device
     // if we still have a NULL address, then shut it down
    cbz  x0, __dead_loop

boot_core_exit:

#if (SIMULATOR_BUILD)
     // branch to the ppa start
    b    _start_monitor_el3
#else
     // branch to the boot loader addr
    br   x0
#endif

//-----------------------------------------------------------------------------

 // this function returns a 64-bit execution address of the core in x0
 // out: x0, start address
 // uses x0, x1, x2 
get_exec_addr:
     // get the 64-bit base address of the dcfg block
    mov  w2, #DCFG_BASE_ADDR

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
     // set the reserved bits
    ldr  x0, =SCTLR_EL3_RES1
     // turn on stack alignment
    orr  x0, x0, #SCTLR_SA_MASK
     // turn on icache
    orr  x0, x0, #SCTLR_I_MASK
     // writeback
    msr  SCTLR_EL3, x0

     // initialize CPUECTLR
     // SMP, bit [6] = 1
    mrs  x0, CPUECTLR_EL1
    orr  x0, x0, #CPUECTLR_SMPEN_EN
    msr  CPUECTLR_EL1, x0

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

    mov  x1, #HCR_EL2_RW_AARCH64
    msr  hcr_el2, x1

    msr  sctlr_el2, xzr

    mov  x0, #CPTR_EL2_NO_TRAP
    msr  cptr_el2, x0
    isb
    ret

//-----------------------------------------------------------------------------

 // this function performs any initialization that must be done very early
 // in:  none
 // out: none
 // uses x0, x1, x2
early_init:

     // load the VBAR_EL3 register with the base address of the EL3 vectors
    adr  x0, el3_vector_space
    msr  VBAR_EL3, x0

     // initialize the L2 ram latency
    mrs   x1, L2CTLR_EL1
    mov   x2, x1
     // clear data ram and tag ram bits
    mov   x0, #L2CTRL_RAM_LATENCY_MASK
    bic   x1, x1, x0
     // set L2 data ram latency bits [2:0]
    orr   x1, x1, #L2CTRL_DATA_LATENCY_3
     // set L2 tag ram latency bits [8:6]
    orr   x1,  x1, #L2CTRL_TAG_LATENCY_3
     // if we changed the register value, write back the new value
    cmp   x2, x1
    b.eq  1f
    msr   L2CTLR_EL1, x1
1:
    isb
    ret

//-----------------------------------------------------------------------------

 // this function performs early configuration on the cci-400 interconnect
 // in:  none
 // out: none
 // uses x0, x1, x2, x3
init_CCI400:

     // get the base address of the cci-400
    mov  x0, #CCI_400_BASE_ADDR

     // set snoop, dvm mode for slave interface 3 (gpp cluster 0)
    mov  x3, #SNOOP_CNTRL_SLV3
    ldr  w1, [x0, x3]
    orr  w1, w1, #SNOOP_CNTRL_SNP_EN
    orr  w1, w1, #SNOOP_CNTRL_DVM_EN
    str  w1, [x0, x3]

     // set dvm for slave interface 2 (TCU)
    mov  x3, #SNOOP_CNTRL_SLV2
    ldr  w1, [x0, x3]
    orr  w1, w1, #SNOOP_CNTRL_DVM_EN
    str  w1, [x0, x3]
    isb

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

 // this function returns the execution start address by determining the
 // boot device
 // in:  none
 // out: x0 = 64-bit start address (base address + offset)
 //      x0 = 0, error return
 // uses x0, x1
get_boot_device:
     // get the 64-bit base address of the dcfg block
    mov  x1, #DCFG_BASE_ADDR

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
     // see if boot-loc is memory complex 1 (5'b10100)
    cmp  w1, #0x14
    b.eq boot_mem_cmplx_1
     // see if boot-loc is memory complex 2 (5'b10011)
    cmp  w1, #0x13
    b.eq boot_mem_cmplx_2
     // see if boot-loc is ocram (5'b10101)
    cmp  w1, #0x15
    b.eq boot_ocram
     // see if boot-loc is ifc (5'b11000)
    cmp  w1, #0x18
    b.eq boot_ifc
     // see if boot-loc is serial nor (5'b11010)
    cmp  w1, #0x1A
    b.eq boot_ser_nor

     // if we get here then there is some nasty error
    mov  x0, #0
    b    exit_dev_addr

     // get the base address for the specified device
boot_pciex_1:
    mov  x1, #DEV_PCIEX1_BASE
    b    finish_dev_addr

boot_mem_cmplx_1:
    mov  x1, #DEV_MEMCMPLX_BASE_01
    b    finish_dev_addr

boot_mem_cmplx_2:
    ldr  x1, =DEV_MEMCMPLX_BASE_02
    b    finish_dev_addr

boot_ocram:
    mov  x1, #DEV_OCRAM_BASE
    b    finish_dev_addr

boot_ifc:
    mov  x1, #DEV_IFC_BASE
    b    finish_dev_addr

boot_ser_nor:
    mov  x1, #DEV_SERNOR_BASE
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
	mov  x1, #DCFG_BASE_ADDR
    ldr  w2, [x1, x0]
    mov  w0, w2
    ret

//-----------------------------------------------------------------------------

 // write a register in the DCFG block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2
write_reg_dcfg:
	mov  x2, #DCFG_BASE_ADDR
    str  w1, [x2, x0]
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
    subs w0, w0, w2

    mov  x30, x3
    ret

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
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

GPP_ROMCODE_VERSION:
    .long  0x00000001

//-----------------------------------------------------------------------------

