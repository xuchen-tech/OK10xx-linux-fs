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

  .section .text, "ax"
 
//-----------------------------------------------------------------------------
    
#include "soc.h"
#include "psci.h"

//-----------------------------------------------------------------------------

  .global _test_psci

//-----------------------------------------------------------------------------

.align 12
.equ  MPIDR_CORE_0,   0x00000000
.equ  MPIDR_CORE_1,   0x00000001
.equ  MPIDR_CORE_2,   0x00000002
.equ  MPIDR_CORE_3,   0x00000003
.equ  MPIDR_CORE_4,   0x00000100
.equ  MPIDR_CORE_5,   0x00000101
.equ  MPIDR_CORE_6,   0x00000102
.equ  MPIDR_CORE_7,   0x00000103

.equ  CONTEXT_CORE_0, 0x01010101
.equ  CONTEXT_CORE_1, 0x12345678
.equ  CONTEXT_CORE_2, 0x23456781
.equ  CONTEXT_CORE_3, 0x34567812
.equ  CONTEXT_CORE_4, 0x45678123
.equ  CONTEXT_CORE_5, 0x56781234
.equ  CONTEXT_CORE_6, 0x67812345
.equ  CONTEXT_CORE_7, 0x78123456

//.equ  PSCI_V_MAJOR,   0x00000001
//.equ  PSCI_V_MINOR,   0x00000000
.equ  PSCI_V_MAJOR,   0x00000000
.equ  PSCI_V_MINOR,   0x00000002

//.equ RCPM_REG_PCPH20SR,      0xD0
//.equ RCPM_PCPH20CLRR_OFFSET, 0xD8
//.equ RCPM_PCPH20SETR_OFFSET, 0x0D4
//.equ SCFG_REG_CORELPMSR,     0x258

.equ PWR_LVL_CORE_STDBY,  0x00000000
.equ PWR_LVL_CORE_PWRDN,  0x00010000
.equ PWR_LVL_CLSTR_STDBY, 0x01000000
.equ PWR_LVL_CLSTR_PWRDN, 0x01010000
.equ PWR_LVL_SYS_STDBY,   0x02000000
.equ PWR_LVL_SYS_PWRDN,   0x02010000

//-----------------------------------------------------------------------------

.ltorg

//-----------------------------------------------------------------------------

_test_psci:

    bl  test01
    bl  test02
    b   core_0_fail

 //------------------------------------

core_0_stop:
    b  core_0_stop

core_1_stop:
    b  core_1_stop

core_0_fail:
    b  core_0_fail

 //------------------------------------

test01:
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
    adr  x2, core_1_stop
    ldr  x3, =CONTEXT_CORE_1
    smc  0x0
    nop
    nop
    nop

    ret

 //------------------------------------

test02:
     // test PSCI_CPU_SUSPEND (core 0)
     // x0 = function id = 0xC4000003
     // x1 = PWR_LVL_SYS_PWRDN
     // x2 = start addr  = core_0_stop
     // x3 = context id  = CONTEXT_CORE_0
    dsb sy
    isb
    nop
    ldr  x0, =PSCI64_CPU_SUSPEND_ID
    ldr  x1, =PWR_LVL_SYS_PWRDN
    adr  x2, core_0_stop
    ldr  x3, =CONTEXT_CORE_0
    smc  0x0
    nop
    nop
    nop

    ret

//-----------------------------------------------------------------------------

