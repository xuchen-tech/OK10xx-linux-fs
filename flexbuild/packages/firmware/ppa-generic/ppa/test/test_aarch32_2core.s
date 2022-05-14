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

.align 12
//.equ  PSCI_V_MAJOR,   0x00000001
//.equ  PSCI_V_MINOR,   0x00000000
.equ  PSCI_V_MAJOR,   0x00000000
.equ  PSCI_V_MINOR,   0x00000002

//-----------------------------------------------------------------------------

  .section .text, "ax"
 
//-----------------------------------------------------------------------------

.global _test_psci

//-----------------------------------------------------------------------------

#include "soc.h"
#include "psci.h"
#include "smc.h"

//-----------------------------------------------------------------------------

.equ  MPIDR_CORE_0,   0x00000000
.equ  MPIDR_CORE_1,   0x00000001

.equ  CONTEXT_CORE_1, 0x81000000

.equ  PARM1_CORE_0,   0x98765432
.equ  PARM2_CORE_0,   0x76543219
.equ  PARM1_CORE_1,   0x21987654
.equ  PARM2_CORE_1,   0x56789123

.equ  AARCH32_BE,     1
.equ  AARCH32_LE,     0

//-----------------------------------------------------------------------------

_test_psci:
    dsb sy
    isb
    nop

     // test - shift the core into Aarch32
    ldr  x0, =ARCH_EL2_2_AARCH32_ID
    adr  x1, cpu_0_start
    ldr  x2, =PARM1_CORE_0
    ldr  x3, =PARM2_CORE_0
    mov  x4, #AARCH32_LE
    smc  0x0
    nop
    nop
    nop

cpu_0_fail1:
    b  cpu_0_fail1

//-----------------------------------------------------------------------------

.align 12

cpu_0_start:

 // cpu_on(core 1)

 // mov  r4,  #0x00000004
.4byte  0xE3A04004
 // mov  r5,  #0x50000000
.4byte  0xE3A05205
 // mov  r6,  #0x00000006
.4byte  0xE3A06006
 // mov  r7,  #0x70000000
.4byte  0xE3A07207
 // mov  r8,  #0x00008000
.4byte  0xE3A08902
 // mov  r9,  #0x00090000
.4byte  0xE3A09809
 // mov  r10, #0x0000A000
.4byte  0xE3A0AA0A
 // mov  r11, #0x000B0000
.4byte  0xE3A0B80B
 // mov  r12, #0x0000C000
.4byte  0xE3A0C903
 // mov  r14, #0x0000E000
.4byte  0xE3A0EA0E
  // mov  r0, #0x0003
.4byte  0xE3A00003
  // mov  r1, #0x84000000
.4byte  0xE3A01321
  // orr  r0, r0, r1           // function id (aarch32 cpu_on)
.4byte  0xE1800001
  // mov  r1, #0x1             // mpidr for core 1
.4byte  0xE3A01001
  // adr  r2, cpu_1_start      // start address
.4byte  0xE28F2068
  // mov  r3, #0x81000000      // context id
.4byte  0xE3A03481
  // smc  0x0
.4byte  0xE1600070
  // nop
.4byte 0xE320F000
  // nop
.4byte 0xE320F000

 // cmp  r4,  #0x00000004
.4byte  0xE3540004
 // b.ne cpu_0_fail
.4byte  0x1A000013
 // cmp  r5,  #0x50000000
.4byte  0xE3550205
 // b.ne cpu_0_fail
.4byte  0x1A000011
 // cmp  r6,  #0x00000006
.4byte  0xE3560006
 // b.ne cpu_0_fail
.4byte  0x1A00000F
 // cmp  r7,  #0x70000000
.4byte  0xE3570207
 // b.ne cpu_0_fail
.4byte  0x1A00000D
 // cmp  r8,  #0x00008000
.4byte  0xE3580902
 // b.ne cpu_0_fail
.4byte  0x1A00000B
 // cmp  r9,  #0x00090000
.4byte  0xE3590809
 // b.ne cpu_0_fail
.4byte  0x1A000009
 // cmp  r10, #0x0000A000
.4byte  0xE35A0A0A
 // b.ne cpu_0_fail
.4byte  0x1A000007
 // cmp  r11, #0x000B0000
.4byte  0xE35B080B
 // b.ne cpu_0_fail
.4byte  0x1A000005
 // cmp  r12, #0x0000C000
.4byte  0xE35C0903
 // b.ne cpu_0_fail
.4byte  0x1A000003
 // cmp  r14, #0x0000E000
.4byte  0xE35E0A0E
 // b.ne cpu_0_fail
.4byte  0x1A000001

cpu_0_stop:
.4byte  0xE1A00000    // mov r0, r0 (nop)
.4byte  0xEAFFFFFE    // branch-to-self
cpu_0_fail:
.4byte  0xEAFFFFFE    // branch-to-self

cpu_1_start:
 // mov  r0, r0 (nop)
.4byte  0xE1A00000
 // cmp  r0, #0x81000000
.4byte  0xE3500481
 // b.ne cpu_1_fail
.4byte  0x1A000000
cpu_1_stop:
 // b    cpu_1_stop
.4byte  0xEAFFFFFE
cpu_1_fail:
 // b    cpu_1_fail
.4byte  0xEAFFFFFE


