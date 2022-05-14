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

.equ  DCFG_BASE_ADDR, 0x01E00000
.equ  PERIPH_BASE,    0x04000000
.equ  L3_BASE_MN,     0x04000000
.equ  L3_BASE_HNI,    0x04080000
.equ  L3_BASE_HNF,    0x04200000
.equ  L3_BASE_RNI,    0x04800000
.equ  L3_SDCR_CLR,    0xFFFFFFFF

//-----------------------------------------------------------------------------

  .section .text, "ax"
 
//-----------------------------------------------------------------------------

#include "soc.h"
#include "psci.h"

//-----------------------------------------------------------------------------

  .global ADDR_BASE_DCFG
  .global ADDR_BASE_RESET
  .global __dead_loop
  .global am_i_boot_core
  .global _reset_vector_el3

//-----------------------------------------------------------------------------

_reset_vector_el3:
     // load the VBAR_EL3 register with the base address of the EL3 vectors
    adr   x0, el3_vector_space
    msr   VBAR_EL3, x0

     // initialize the L2 ram latency
    mrs   x1, S3_1_c11_c0_2
    mov   x2, x1
    and   x3, x1, #0x7
    cmp   x3, #0x2
    b.eq  1f
     // set L2 data ram latency bits [2:0]
    bic   x1, x1, #0x7
    orr   x1, x1, #0x2
1:
    mov   x3, #0
    bfxil x3, x1, #6, #3
    cmp   x3, #0x2
    b.eq  2f
     // set L2 tag ram latency bits [8:6]
    bic   x1, x1, #0x1C0
    orr   x1,  x1, #0x80
2:
     // if we changed the register value, write back the new value
    cmp   x2, x1
    b.eq  3f
    msr   S3_1_c11_c0_2, x1
3:

     // initialize SCTLR_EL3
     // M,   bit [0]  = 0
     // A,   bit [1]  = 0
     // C,   bit [2]  = 0
     // SA,  bit [3]  = 1
     // I,   bit [12] = 0
     // WXN, bit [19] = 0
     // EE,  bit [25] = 0
    mrs  x1, SCTLR_EL3
    mov  x2, #0x8
    orr  x1, x1, x2
    msr  SCTLR_EL3, x1

     // initialize CPUECTLR
     // SMP, bit [6] = 1
    mrs  x2, S3_1_c15_c2_1
    orr  x2, x2, #0x40
    msr S3_1_c15_c2_1, x2

     // initialize CPTR_EL3
     // TFP,   bit [10] = 0
     // TTA,   bit [20] = 0
     // TCPAC, bit [30] = 0
    mrs  x2, CPTR_EL3
    mov  x3, #0x40100000
    orr  x1, x3, #0x0400
    bic  x2, x2, x1
    msr  CPTR_EL3, x2

     // initialize SCR_EL3
     // NS,  bit[0]  = 1
     // IRQ, bit[1]  = 0
     // FIQ, bit[2]  = 0
     // EA,  bit[3]  = 0
     // SMD, bit[7]  = 0
     // HCE, bit[8]  = 1
     // SIF, bit[9]  = 0
     // RW,  bit[10] = 1
     // ST,  bit[11] = 0
     // TWI, bit[12] = 0
     // TWE, bit[13] = 0
    mrs  x1, SCR_EL3
    mov  x2, #0x3A8A
    bic  x1, x1, x2
    mov  x3, #0x501
    orr  x1, x1, x3
    msr  SCR_EL3, x1

     // initialize the value of ESR_EL3 to 0x0
    msr ESR_EL3, xzr

     // synchronize the system register writes
    isb

am_i_boot_core:
     // determine if this is the boot core
     // read mp affinity reg (MPIDR_EL1) into x1
    mrs x1, MPIDR_EL1
     // extract the affinity 0 & 1 fields - bits [15:0]
    mov   x0, xzr
    bfxil x0, x1, #0, #16

     // if x0 == 0, then this core is boot core
     // if x0 != 0, then this core is not boot core
    cbnz   x0, non_boot_core

     //------------------------------------------

     // if we're here, then this is the boot core -

     // x0 contains bits[15:0] of mpidr_el1

boot_where:
     // put the 64-bit base address of the dcfg block into x2
    adr  x3, ADDR_BASE_DCFG
    ldr  w2, [x3]

     // x2 contains base addr of dcfg block

     // write mpidr_el1 bits[15:0] to the 32-bit SCRATCHRW8 register (offset 0x21C in the dcfg block)
    str  w0, [x2, #0x21C]

     //------------------------------------------

     // determine the SoC personality and configure

     // read SVR register
    ldr  w0, [x2, #0xA4]
     // extract the lo-order 16 bits
    mov   x1, xzr
    bfxil x1, x0, #0, #16

     // is it the 0x87010110 device?
    mov  x3, #0x0110
    cmp  x1, x3
    b.eq device_with_8_cores

     // is it the 0x87011110 device?
    mov  x3, #0x1110
    cmp  x1, x3
    b.eq device_with_8_cores

     // is it the 0x87010010 device?
    mov  x3, #0x0010
    cmp  x1, x3
    b.eq device_with_8_cores

     // is it the 0x87011010 device?
    mov  x3, #0x1010
    cmp  x1, x3
    b.eq device_with_8_cores

     // is it the 0x87012110 device?
    mov  x3, #0x2110
    cmp  x1, x3
    b.eq device_with_4_seq_cores

     // is it the 0x87013110 device?
    mov  x3, #0x3110
    cmp  x1, x3
    b.eq device_with_4_seq_cores

     // is it the 0x87012010 device?
    mov  x3, #0x2010
    cmp  x1, x3
    b.eq device_with_4_seq_cores

     // is it the 0x87013010 device?
    mov  x3, #0x3010
    cmp  x1, x3
    b.eq device_with_4_seq_cores

     // is it the 0x87010310 device?
    mov  x3, #0x0310
    cmp  x1, x3
    b.eq device_with_6_cores

     // is it the 0x87011310 device?
    mov  x3, #0x1310
    cmp  x1, x3
    b.eq device_with_6_cores

     // is it the 0x87010210 device?
    mov  x3, #0x0210
    cmp  x1, x3
    b.eq device_with_6_cores

     // is it the 0x87011210 device?
    mov  x3, #0x1210
    cmp  x1, x3
    b.eq device_with_6_cores

     // is it the 0x87010510 device?
    mov  x3, #0x0510
    cmp  x1, x3
    b.eq device_with_4_stg_cores

     // is it the 0x87011510 device?
    mov  x3, #0x1510
    cmp  x1, x3
    b.eq device_with_4_stg_cores

     // is it the 0x87010410 device?
    mov  x3, #0x0410
    cmp  x1, x3
    b.eq device_with_4_stg_cores

     // is it the 0x87011410 device?
    mov  x3, #0x1410
    cmp  x1, x3
    b.eq device_with_4_stg_cores

     // is it the 0x87012310 device?
    mov  x3, #0x2310
    cmp  x1, x3
    b.eq device_with_2_cores

     // is it the 0x87013310 device?
    mov  x3, #0x3310
    cmp  x1, x3
    b.eq device_with_2_cores

     // is it the 0x87012210 device?
    mov  x3, #0x2210
    cmp  x1, x3
    b.eq device_with_2_cores

     // is it the 0x87013210 device?
    mov  x3, #0x3210
    cmp  x1, x3
    b.eq device_with_2_cores

     // if we did not recognize the device, then default to an 8-core model
device_with_8_cores:
    adr  x3, COREDISR_FOR_8_CORES
    b    set_coredisr

device_with_2_cores:
    adr  x3, COREDISR_FOR_2_CORES
    b    set_coredisr

device_with_4_seq_cores:
    adr  x3, COREDISR_FOR_4_CORES_SEQ
    b    set_coredisr

device_with_4_stg_cores:
    adr  x3, COREDISR_FOR_4_CORES_STG
    b    set_coredisr

device_with_6_cores:
    adr  x3, COREDISR_FOR_6_CORES
    b    set_coredisr

set_coredisr:
     // x2 contains base addr of dcfg block
    ldr  w1, [x3]
     // load the value in COREDISR
    str  w1, [x2, #0x94]

     //------------------------------------------

     // set snoop mode in the L3 cache
    bl  set_L3_snoop

     //------------------------------------------

     // determine the bootloader address - start by checking
     // if there is a non-null address in the BOOTLOCPTR registers
    ldr  x2, =DCFG_BASE_ADDR
     // x2 contains base addr of dcfg block

     // read the 32-bit BOOTLOCPTRL register (offset 0x400 in the dcfg block)
    ldr  w3, [x2, #0x400]
     // read the 32-bit BOOTLOCPTRH register (offset 0x404 in the dcfg block)
    ldr  w4, [x2, #0x404]
     // create a 64-bit BOOTLOCPTR address in x3
    orr  x3, x3, x4, LSL #32
     // if address is not null, then branch to exit
    cbnz x3,  boot_core_exit

     //------------------------------------------

     // the address in BOOTLOCPTR is null - aargh!
     // we need the boot location device - this is found in the
     // RCW word bits [264:260], which are in the register
     // RCWSR 9 offset 0x120 in RCWSR block

     // x2 contains base addr of dcfg block

     // read RCWSR9, offset 0x120 in the DCFG block
    ldr  w1, [x2, #0x120]
     // extract 5 bits [8:4]
    mov   w4, wzr
    bfxil w4, w1, #4, #5

     // w4 contains boot-loc bits

     // compare the bits in w4 to determine the boot location device

     // see if boot-loc is pci express #1 (5'b00000)
    cmp  w4, #0x0
    b.eq boot_pciex_1
     // see if boot-loc is pci express #2 (5'b00001)
    cmp  w4, #0x1
    b.eq boot_pciex_2
     // see if boot-loc is pci express #3 (5'b00010)
    cmp  w4, #0x2
    b.eq boot_pciex_3
     // see if boot-loc is pci express #4 (5'b00011)
    cmp  w4, #0x3
    b.eq boot_pciex_4
     // see if boot-loc is the memory complex (5'b10100)
    cmp  w4, #0x14
    b.eq boot_mem_cmplx
     // see if boot-loc is ocram (5'b10101)
    cmp  w4, #0x15
    b.eq boot_ocram
     // see if boot-loc is the test port (5'b10111)
    cmp  w4, #0x17
    b.eq boot_tport
     // see if boot-loc is ifc (5'b11000)
    cmp  w4, #0x18
    b.eq boot_ifc
     // see if boot-loc is serial nor (5'b11010)
    cmp  w4, #0x1A
    b.eq boot_ser_nor

     // if we get here then there is some nasty error - branch
     // back and try getting a boot location again
    b  boot_where

     // get the base address for the specified device
boot_pciex_1:
    adr  x5, ADDR_DEV_PCIEX1
    b    finish_dev_addr

boot_pciex_2:
    adr  x5, ADDR_DEV_PCIEX2
    b    finish_dev_addr

boot_pciex_3:
    adr  x5, ADDR_DEV_PCIEX3
    b    finish_dev_addr

boot_pciex_4:
    adr  x5, ADDR_DEV_PCIEX4
    b    finish_dev_addr

boot_mem_cmplx:
    adr  x5, ADDR_DEV_MEMCMPLX
    b    finish_dev_addr

boot_ocram:
    adr  x5, ADDR_DEV_OCRAM
    b    finish_dev_addr

boot_tport:
    adr  x5, ADDR_DEV_TPORT
    b    finish_dev_addr

boot_ifc:
    adr  x5, ADDR_DEV_IFC
    b    finish_dev_addr

boot_ser_nor:
    adr  x5, ADDR_DEV_SERNOR
    // b    finish_dev_addr
     // Note: this is a fall-thru condition, don't
     //       insert anything after this line

finish_dev_addr:
     // put a device address offset of 4Kb into x2
    mov  x2, #0x1000

     // construct the boot address from the device base address + 4Kb offset
    ldr  w6, [x5]
    orr  x3, x2, x6, LSL #8
    // b    boot_core_exit
     // Note: this is a fall-thru condition, don't
     //       insert anything after this line

boot_core_exit:
     // branch to the ppa start
    b     _start_monitor_el3

//-----------------------------------------------------------------------------

 // this function sets snoop mode and dvm domains in the L3 cache
 // in:  none
 // out: none
 // uses x0, x1, x2, x3, x4, x5, x6, x7
set_L3_snoop:

     // terminate the pos barriers in sa_aux_ctl reg
    ldr  w1, =L3_BASE_HNI
    add  x1, x1, #0x500
    ldr  x0, [x1]
    orr  x0, x0, #0x10
    str  x0, [x1]

     // read the oly_rnf_nodeid_list register for rnf regions
    ldr  w1, =L3_BASE_MN
    add  x1, x1, #0x180
    ldr  x4, [x1]

     // read the oly_rnidvm_nodeid_list for dvm domains
    ldr  w1, =L3_BASE_MN
    add  x1, x1, #0x1A0
    ldr  x5, [x1]

    ldr  w1, =L3_BASE_HNF
    mov  x2, #0x220
    mov  x3, #0x210
    ldr  w6, =L3_SDCR_CLR

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
    add  x1, x1, #0x10000
    subs x7, x7, #1
    b.ne write_sdcr_regs   

     // write to dvm_clr, dvm_set regs
    ldr  w1, =L3_BASE_MN
    str  x6, [x1, x2]
    str  x5, [x1, x3]

    dsb  #0xf

    mov  x0, #800      // retry count for simulator models
    mov  x2, #0x200
poll_mn_ddcr:
    ldr  x6, [x1, x2]
    cmp  x6, x5
    b.eq 1f
    subs x0, x0, #1
    b.ne poll_mn_ddcr
1:
    ldr  w1, =L3_BASE_HNF
    mov  x7, #8
2:
    mov  x0, #800      // retry count for simulator models
poll_hnf_sdcr:
    ldr  x6, [x1, x2]
    cmp  x6, x4
    b.eq 3f
    subs x0, x0, #1
    b.ne poll_hnf_sdcr
3:
    add  x1, x1, #0x10000
    subs x7, x7, #1
    b.ne 2b  

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
//-----------------------------------------------------------------------------

GPP_ROMCODE_VERSION:
    .long  0x00010005

ADDR_BASE_DCFG:
    .long  0x01E00000
ADDR_BASE_RESET:
    .long  0x01E60000

 // the following base addresses have been right-shifted by
 // 8-bits so that they fit neatly into a 32-bit word - they
 // get left-shifted back into position when we construct the
 // address above
 
ADDR_DEV_PCIEX1:
    .long  0x10000000
ADDR_DEV_PCIEX2:
    .long  0x12000000
ADDR_DEV_PCIEX3:
    .long  0x14000000
ADDR_DEV_PCIEX4 :      
    .long  0x16000000
ADDR_DEV_MEMCMPLX:
    .long  0x00800000
ADDR_DEV_OCRAM:
    .long  0x00180000
ADDR_DEV_TPORT:
    .long  0x06000000
ADDR_DEV_IFC:
    .long  0x00300000
ADDR_DEV_SERNOR:
    .long  0x00200000

COREDISR_FOR_8_CORES:
    .long  0xFFFFFF00
COREDISR_FOR_4_CORES_SEQ:  // 4 cores sequential
    .long  0xFFFFFFF0
COREDISR_FOR_6_CORES:
    .long  0xFFFFFF0C
COREDISR_FOR_4_CORES_STG:  // 4 cores staggered
    .long  0xFFFFFFAA
COREDISR_FOR_2_CORES:
    .long  0xFFFFFFFC

//-----------------------------------------------------------------------------

