//-----------------------------------------------------------------------------
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

.align 3

//.equ  PSCI_V_MAJOR,   0x00000001
//.equ  PSCI_V_MINOR,   0x00000000
.equ  PSCI_V_MAJOR,   0x00000000
.equ  PSCI_V_MINOR,   0x00000002

.equ  PSCI_V_MASK,    0xFFFF

//-----------------------------------------------------------------------------

  .section .text, "ax"
 
//-----------------------------------------------------------------------------

.global _test_psci

//-----------------------------------------------------------------------------

#include "soc.h"
#include "psci.h"

//-----------------------------------------------------------------------------

.equ  MPIDR_CORE_0,    0x00000000
.equ  MPIDR_CORE_1,    0x00000001
.equ  MPIDR_CORE_2,    0x00000100
.equ  MPIDR_CORE_3,    0x00000101
.equ  MPIDR_CORE_4,    0x00000200
.equ  MPIDR_CORE_5,    0x00000201
.equ  MPIDR_CORE_6,    0x00000300
.equ  MPIDR_CORE_7,    0x00000301
.equ  MPIDR_CORE_8,    0x00000400
.equ  MPIDR_CORE_9,    0x00000401
.equ  MPIDR_CORE_10,   0x00000500
.equ  MPIDR_CORE_11,   0x00000501
.equ  MPIDR_CORE_12,   0x00000600
.equ  MPIDR_CORE_13,   0x00000601
.equ  MPIDR_CORE_14,   0x00000700
.equ  MPIDR_CORE_15,   0x00000701

.equ  CONTEXT_CORE_1,  0x12345678
.equ  CONTEXT_CORE_2,  0x23456781
.equ  CONTEXT_CORE_3,  0x34567812
.equ  CONTEXT_CORE_4,  0x45678123
.equ  CONTEXT_CORE_5,  0x56781234
.equ  CONTEXT_CORE_6,  0x67812345
.equ  CONTEXT_CORE_7,  0x78123456
.equ  CONTEXT_CORE_8,  0x00008888
.equ  CONTEXT_CORE_9,  0x99990000
.equ  CONTEXT_CORE_10, 0x0A0A0A0A
.equ  CONTEXT_CORE_11, 0xB0B0B0B0
.equ  CONTEXT_CORE_12, 0xCC00CC00
.equ  CONTEXT_CORE_13, 0x00DD00DD
.equ  CONTEXT_CORE_14, 0xEEEEEEEE
.equ  CONTEXT_CORE_15, 0xFFFFFFFF

//-----------------------------------------------------------------------------

_test_psci:
    dsb sy
    isb
    nop

    bl  runTest01
    bl  runTest02
    bl  runTest03
    bl  runTest04
    bl  runTest05
    
cpu_0_stop:
    b  cpu_0_stop

runTest01:
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
    ret

runTest02:
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
    ret

runTest03:
     // test an unimplemented psci function
    ldr  x0, =PSCI32_MIGRATE_ID
    ldr  x1, =MPIDR_CORE_1
    smc  0x0
    nop
    nop
    nop
    ldr  x1, =PSCI_NOT_SUPPORTED
    cmp  w0, w1
    b.ne cpu_0_fail_unimplemented
    ret


runTest04:
    // test PSCI_CPU_ON

    //-------
    // (Core 1)     
    ldr  w1, =MPIDR_CORE_1
    adr  x2, cpu_1_pass
    ldr  w3, =CONTEXT_CORE_1
    ldr  w0, =PSCI64_CPU_ON_ID
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_1
1:
     // test AFFINITY_INFO of core 1
     // x0 = function id = 0xC4000004
     // x1 = mpidr       = 0x0001
     // x2 = level       = 0x0
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_1
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 1b

    //-------
    // (Core 2)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_2
    adr  x2, cpu_2_start
    ldr  w3, =CONTEXT_CORE_2
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_2
2:
     // test AFFINITY_INFO of core 2
     // x0 = function id = 0xC4000004
     // x1 = mpidr       = 0x0001
     // x2 = level       = 0x0
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_2
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 2b

    //-------
    // (Core 3)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_3
    adr  x2, cpu_3_start
    ldr  w3, =CONTEXT_CORE_3
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_3
3:
     // test AFFINITY_INFO of core 3
     // x0 = function id = 0xC4000004
     // x1 = mpidr       = 0x0001
     // x2 = level       = 0x0
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_3
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 3b

    //-------
    // (Core 7)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_7
    adr  x2, cpu_7_start
    ldr  w3, =CONTEXT_CORE_7
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_7
4:
     // test AFFINITY_INFO of core 7
     // x0 = function id = 0xC4000004
     // x1 = mpidr       = 0x0001
     // x2 = level       = 0x0
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_7
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 4b

    //-------
    // (Core 5)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_5
    adr  x2, cpu_5_start
    ldr  w3, =CONTEXT_CORE_5
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_5
5:
     // test AFFINITY_INFO of core 5
     // x0 = function id = 0xC4000004
     // x1 = mpidr       = 0x0001
     // x2 = level       = 0x0
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_5
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 5b

    //-------
    // (Core 6)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_6
    adr  x2, cpu_6_start
    ldr  w3, =CONTEXT_CORE_6
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_6
6:
     // test AFFINITY_INFO of core 6
     // x0 = function id = 0xC4000004
     // x1 = mpidr       = 0x0001
     // x2 = level       = 0x0
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_6
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 6b

    //-------
    // (Core 4)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_4
    adr  x2, cpu_4_start
    ldr  w3, =CONTEXT_CORE_4
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_4
7:
     // test AFFINITY_INFO of core 4
     // x0 = function id = 0xC4000004
     // x1 = mpidr       = 0x0001
     // x2 = level       = 0x0
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_4
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 7b
    ret

runTest05:

    // (Core 8)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_8
    adr  x2, cpu_8_start
    ldr  w3, =CONTEXT_CORE_8
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_8
8:
     // test AFFINITY_INFO of core 8
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_8
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 8b

    //-------

    // (Core 9)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_9
    adr  x2, cpu_9_start
    ldr  w3, =CONTEXT_CORE_9
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_9
9:
     // test AFFINITY_INFO of core 9
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_9
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 9b

    //-------

    // (Core 10)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_10
    adr  x2, cpu_10_start
    ldr  w3, =CONTEXT_CORE_10
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_10
10:
     // test AFFINITY_INFO of core 10
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_10
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 10b

    //-------

    // (Core 11)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_11
    adr  x2, cpu_11_start
    ldr  w3, =CONTEXT_CORE_11
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_11
11:
     // test AFFINITY_INFO of core 11
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_11
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 11b

    //-------

    // (Core 12)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_12
    adr  x2, cpu_12_start
    ldr  w3, =CONTEXT_CORE_12
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_12
12:
     // test AFFINITY_INFO of core 12
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_12
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 12b

    //-------

    // (Core 13)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_13
    adr  x2, cpu_13_start
    ldr  w3, =CONTEXT_CORE_13
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_13
13:
     // test AFFINITY_INFO of core 13
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_13
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 13b

    //-------

    // (Core 14)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_14
    adr  x2, cpu_14_start
    ldr  w3, =CONTEXT_CORE_14
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_14
14:
     // test AFFINITY_INFO of core 14
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_14
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 14b

    //-------

    // (Core 15)
    ldr  w0, =PSCI64_CPU_ON_ID
    ldr  w1, =MPIDR_CORE_15
    adr  x2, cpu_15_start
    ldr  w3, =CONTEXT_CORE_15
    smc  0x0
    nop
    nop
    cbnz  x0, cpu_0_error_core_15
15:
     // test AFFINITY_INFO of core 15
    ldr  x0, =PSCI64_AFFINITY_INFO_ID
    ldr  x1, =MPIDR_CORE_15
    mov  x2, #0
    smc  0x0
    nop
    nop
    nop
     // test the return value
    ldr  x1, =AFFINITY_LEVEL_ON
    cmp  w0, w1
    b.ne 15b

    //-------

    ret


cpu_0_fail_version:
    b  cpu_0_fail_version

cpu_0_fail_affinity:
    b  cpu_0_fail_affinity

cpu_0_fail_unimplemented:
    b  cpu_0_fail_unimplemented

cpu_1_pass:
    b cpu_1_pass

cpu_2_start:
    ldr  w9, =CONTEXT_CORE_2
    bl context_id_chk
cpu_2_pass:
    b cpu_2_pass

cpu_3_start:
    ldr  w9, =CONTEXT_CORE_3
    bl context_id_chk
cpu_3_pass:
    b cpu_3_pass

cpu_4_start:
    ldr  w9, =CONTEXT_CORE_4
    bl context_id_chk
cpu_4_pass:
    b cpu_4_pass

cpu_5_start:
    ldr  w9, =CONTEXT_CORE_5
    bl context_id_chk
cpu_5_pass:
    b cpu_5_pass

cpu_6_start:
    ldr  w9, =CONTEXT_CORE_6
    bl context_id_chk
cpu_6_pass:
    b cpu_6_pass

cpu_7_start:
    ldr  w9, =CONTEXT_CORE_7
    bl context_id_chk
cpu_7_pass:
    b cpu_7_pass

cpu_8_start:
    ldr  w9, =CONTEXT_CORE_8
    bl context_id_chk
cpu_8_pass:
    b cpu_8_pass

cpu_9_start:
    ldr  w9, =CONTEXT_CORE_9
    bl context_id_chk
cpu_9_pass:
    b cpu_9_pass

cpu_10_start:
    ldr  w9, =CONTEXT_CORE_10
    bl context_id_chk
cpu_10_pass:
    b cpu_10_pass

cpu_11_start:
    ldr  w9, =CONTEXT_CORE_11
    bl context_id_chk
cpu_11_pass:
    b cpu_11_pass

cpu_12_start:
    ldr  w9, =CONTEXT_CORE_12
    bl context_id_chk
cpu_12_pass:
    b cpu_12_pass

cpu_13_start:
    ldr  w9, =CONTEXT_CORE_13
    bl context_id_chk
cpu_13_pass:
    b cpu_13_pass

cpu_14_start:
    ldr  w9, =CONTEXT_CORE_14
    bl context_id_chk
cpu_14_pass:
    b cpu_14_pass

cpu_15_start:
    ldr  w9, =CONTEXT_CORE_15
    bl context_id_chk
cpu_15_pass:
    b cpu_15_pass

 // CPU_ON context id check
context_id_chk:
    cmp w0, w9
    b.ne context_chk_fail
    ret
context_chk_fail: 
     // context did not match
    b context_chk_fail

cpu_0_error_core_1:
    b  cpu_0_error_core_1

cpu_0_error_core_2:
    b  cpu_0_error_core_2

cpu_0_error_core_3:
    b  cpu_0_error_core_3

cpu_0_error_core_4:
    b  cpu_0_error_core_4

cpu_0_error_core_5:
    b  cpu_0_error_core_5

cpu_0_error_core_6:
    b  cpu_0_error_core_6

cpu_0_error_core_7:
    b  cpu_0_error_core_7

cpu_0_error_core_8:
    b  cpu_0_error_core_8

cpu_0_error_core_9:
    b  cpu_0_error_core_9

cpu_0_error_core_10:
    b  cpu_0_error_core_10

cpu_0_error_core_11:
    b  cpu_0_error_core_11

cpu_0_error_core_12:
    b  cpu_0_error_core_12

cpu_0_error_core_13:
    b  cpu_0_error_core_13

cpu_0_error_core_14:
    b  cpu_0_error_core_14

cpu_0_error_core_15:
    b  cpu_0_error_core_15


