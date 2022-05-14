//-----------------------------------------------------------------------------
// 
// Copyright (c) 2013-2014, Freescale Semiconductor, Inc.
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
    
#include "aarch64.h"
#include "boot.h"
#include "psci.h"

//-----------------------------------------------------------------------------

  .global _reset_vector_el3
  .global initialize_EL3
  .global get_core_mask_msb
  .global read_reg_scfg
  .global read_reg_dcfg

//-----------------------------------------------------------------------------

.equ  PSCI_CPU_ON,    0xC4000003
.equ  PSCI_VERSION,   0x84000000
.equ  PSCI_CPU_OFF,   0x84000002
.equ  MPIDR_CORE_0,   0x00000000
.equ  MPIDR_CORE_1,   0x00000001
.equ  MPIDR_CORE_2,   0x00000002
.equ  MPIDR_CORE_3,   0x00000003
.equ  MPIDR_CORE_4,   0x00000100

.equ  CONTEXT_CORE_1, 0x12345678
.equ  CONTEXT_CORE_2, 0xA9876543
.equ  CONTEXT_CORE_3, 0x10208070
.equ  CONTEXT_CORE_4, 0x93827160
.equ  CONTEXT_CORE_1_AGAIN, 0x23456789

//.equ  PSCI_V_MAJOR,   0x00000001
//.equ  PSCI_V_MINOR,   0x00000000
.equ  PSCI_V_MAJOR,   0x00000000
.equ  PSCI_V_MINOR,   0x00000002

//.equ RCPM_REG_PCPH20SR,      0xD0
//.equ RCPM_PCPH20CLRR_OFFSET, 0xD8
//.equ RCPM_PCPH20SETR_OFFSET, 0x0D4
//.equ SCFG_REG_CORELPMSR,     0x258

//-----------------------------------------------------------------------------

_reset_vector_el3:

     // load the VBAR_EL3 register with the base address of the EL3 vectors
    adr  x0, el3_vector_space
    msr  VBAR_EL3, x0

     // determine if this is the boot core
    bl   am_i_boot_core
     // if x0 == 0, then this core is boot core
     // if x0 != 0, then this core is not boot core
    cbnz x0, non_boot_core

     //------------------------------------------

     // if we're here, then this is the boot core -
    
     // clear SCRATCHRW7
    mov   w0, #0x618
    mov   w1, wzr
    bl    write_reg_scfg

     // read mp affinity reg (MPIDR_EL1)
    mrs x0, MPIDR_EL1
     // clear this cores bit in COREBCR
    bl    get_core_mask_msb
    mov   w1, w0
    mov   w0, #0x680
    bl    write_reg_scfg

     // set personality
     // read COREDISR
    mov   w0, #0x94     
    bl    read_reg_dcfg
     // write BRR
    mov   w1, w0
    mov   w0, #0xE4
    bl    write_reg_dcfg

     // perform EL3 init
    bl    initialize_EL3

     // branch to the ppa start
    b     _start_monitor_el3

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

 // determine if this is the boot core
 // out: x0  = 0, boot_core
 //      x0 != 0, secondary core
 // uses x0, x1
am_i_boot_core:

     // read mp affinity reg (MPIDR_EL1)
    mrs x1, MPIDR_EL1
     // extract the affinity 0 & 1 fields - bits [15:0]
    mov   x0, xzr
    bfxil x0, x1, #0, #16
    ret

//-----------------------------------------------------------------------------

 // uses: x0, x1, x2
initialize_EL3:
     // initialize SCTLR_EL3
     // M,   bit [0]  = 0
     // A,   bit [1]  = 0
     // C,   bit [2]  = 0
     // SA,  bit [3]  = 1
     // I,   bit [12] = 0
     // WXN, bit [19] = 0
     // EE,  bit [25] = 0
    mov  x2, #0x8
    msr  SCTLR_EL3, x2

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
    mov  x0, #0x40100000
    orr  x1, x0, #0x0400
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
    mov  x1, #0x501
    msr  SCR_EL3, x1

     // initialize the value of ESR_EL3 to 0x0
    msr ESR_EL3, xzr

     // synchronize the system register writes
    isb
    ret

//-----------------------------------------------------------------------------

 // read a register in the SCFG block
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_scfg:
    ldr  w2, =SCFG_BASE_ADDR
    ldr  w1, [x2, x0]
     // swap for BE
    rev  w0, w1
    ret

//-----------------------------------------------------------------------------

 // write a register in the SCFG block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2, x3
write_reg_scfg:
    ldr  w2, =SCFG_BASE_ADDR
     // swap for BE
    rev  w3, w1
    str  w3, [x2, x0]
    ret

//-----------------------------------------------------------------------------

 // read a register in the DCFG block
 // in:  x0 = offset
 // out: w0 = value read
 // uses x0, x1, x2
read_reg_dcfg:
    ldr  w2, =DCFG_BASE_ADDR
    ldr  w1, [x2, x0]
     // swap for BE
    rev  w0, w1
    ret

//-----------------------------------------------------------------------------

 // write a register in the DCFG block
 // in:  x0 = offset
 // in:  w1 = value to write
 // uses x0, x1, x2, x3
write_reg_dcfg:
    ldr  w2, =DCFG_BASE_ADDR
     // swap for BE
    rev  w3, w1
    str  w3, [x2, x0]
    ret

//-----------------------------------------------------------------------------

 // this function returns the bit mask corresponding to the mpidr_el1 value.
 // the mask is returned in w0.
 // this bit mask references the core in the SoC registers such as
 // COREBCR where the MSB represents core0
 // in:   x0 = mpidr_el1 register value
 // out:  w0 = core mask
 // uses x0, x1, x2

get_core_mask_msb:
     // extract the affinity 0 & 1 fields - bits [15:0]
    mov   x1, xzr
    bfxil x1, x0, #0, #16

     // generate a msb-based mask for the core - this algorithm assumes 4 cores
     // per cluster, and must be adjusted if that is not the case
     // SoC core = ((cluster << 2) + core)
     // mask = (0x8000_0000 >> SoC core)
    mov   w1, wzr
    mov   w2, wzr
    bfxil w1, w0, #8, #8  // extract cluster
    bfxil w2, w0, #0, #8  // extract cpu #
    lsl   w1, w1, #2
    add   w1, w1, w2
    mov   w2, #0x80000000
    lsr   w0, w2, w1
    ret

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

GPP_ROMCODE_VERSION:
    .long  0x00020000

//-----------------------------------------------------------------------------

