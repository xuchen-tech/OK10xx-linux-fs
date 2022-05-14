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

  .section .text, "ax"
 
//-----------------------------------------------------------------------------
    
#include "soc.h"
#include "psci.h"

//-----------------------------------------------------------------------------

.global _test_psci

//-----------------------------------------------------------------------------

.align 3
.equ  MPIDR_CORE_0,   0x00000000
.equ  MPIDR_CORE_1,   0x00000001
.equ  MPIDR_CORE_2,   0x00000002
.equ  MPIDR_CORE_3,   0x00000003

.equ  CONTEXT_CORE_0, 0x01234567
.equ  CONTEXT_CORE_1, 0x12345678
.equ  CONTEXT_CORE_2, 0xA9876543
.equ  CONTEXT_CORE_3, 0x10208070

//.equ  PSCI_V_MAJOR,   0x00000001
//.equ  PSCI_V_MINOR,   0x00000000
.equ  PSCI_V_MAJOR,   0x00000000
.equ  PSCI_V_MINOR,   0x00000002

.equ  PSCI_V_MASK,    0xFFFF

//-----------------------------------------------------------------------------

_test_psci:


    bl  Test_01

.if (CPU_MAX_COUNT > 2)
    bl  Test_02
    bl  Test_03
.endif

core_0_stop:
    b  core_0_stop

cpu_0_fail_version:
    b  cpu_0_fail_version

cpu_0_fail_affinity:
    b  cpu_0_fail_affinity

cpu_0_error_core_1:
    b  cpu_0_error_core_1

cpu_0_error_core_2:
    b  cpu_0_error_core_2

cpu_0_error_core_3:
    b  cpu_0_error_core_3

 //------------------------------------

core_1a_entry:
    ldr  w9, =CONTEXT_CORE_1
    bl context_id_chk
core_1_pass:
    b core_1_pass

.if (CPU_MAX_COUNT > 2)

core_2a_entry:
    ldr  w9, =CONTEXT_CORE_2
    bl context_id_chk
core_2_pass:
    b core_2_pass

core_3a_entry:
    ldr  w9, =CONTEXT_CORE_3
    bl context_id_chk
core_3_pass:
    b core_3_pass

.endif

 //------------------------------------

Test_01:
     // test PSCI_VERSION
    ldr  x0, =PSCI_VERSION_ID
    smc  0x0
    nop
    nop
    nop
    and  w1, w0, #PSCI_V_MASK
    cmp  w1, #PSCI_V_MINOR
    b.ne cpu_0_fail_version
    lsr  w0, w0, #16
    and  w0, w0, #PSCI_V_MASK
    cmp  w0, #PSCI_V_MAJOR
    b.ne cpu_0_fail_version

     // test AFFINITY_INFO of core 1
     // x1 = mpidr
     // x2 = level
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_1
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_OFF
    cmp  w0, w1
    b.ne cpu_0_fail_affinity

    // test PSCI_CPU_ON

    //-------
     // test PSCI_CPU_ON (core 1)
     // x0 = function id = 0xC4000003
     // x1 = mpidr       = 0x0001
     // x2 = start addr  = core_1a_entry
     // x3 = context id  = CONTEXT_CORE_1
    dsb sy
    isb
    nop
    ldr  x0, =PSCI64_CPU_ON_ID
    ldr  x1, =MPIDR_CORE_1
    adr  x2, core_1a_entry
    ldr  x3, =CONTEXT_CORE_1
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_1
    ret

.if (CPU_MAX_COUNT > 2)

Test_02:
     // test PSCI_CPU_ON (core 2)
     // x0 = function id = 0xC4000003
     // x1 = mpidr       = 0x0002
     // x2 = start addr  = core_2a_entry
     // x3 = context id  = CONTEXT_CORE_2
    nop
    ldr  x0, =PSCI64_CPU_ON_ID
    ldr  x1, =MPIDR_CORE_2
    adr  x2, core_2a_entry
    ldr  x3, =CONTEXT_CORE_2
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_2
    ret

 //------------------------------------

Test_03:
     // test PSCI_CPU_ON (core 3)
     // x0 = function id = 0xC4000003
     // x1 = mpidr       = 0x0003
     // x2 = start addr  = core_3a_entry
     // x3 = context id  = CONTEXT_CORE_3
    nop
    ldr  x0, =PSCI64_CPU_ON_ID
    ldr  x1, =MPIDR_CORE_3
    adr  x2, core_3a_entry
    ldr  x3, =CONTEXT_CORE_3
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_3
    ret

.endif

 //------------------------------------

 // CPU_ON context id check
context_id_chk:
    cmp w0, w9
    b.ne context_chk_fail
    ret
context_chk_fail: 
     // context did not match
    b context_chk_fail

 //------------------------------------



