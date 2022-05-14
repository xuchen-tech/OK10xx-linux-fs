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
#include "smc.h"
#include "aarch64.h"

//-----------------------------------------------------------------------------

  .global _test_psci

//-----------------------------------------------------------------------------

.align 12

.if (CPU_PER_CLUSTER == 2)
.equ  MPIDR_CORE_0,   0x00000000
.equ  MPIDR_CORE_1,   0x00000001
.equ  MPIDR_CORE_2,   0x00000100
.equ  MPIDR_CORE_3,   0x00000101
.else
.equ  MPIDR_CORE_0,   0x00000000
.equ  MPIDR_CORE_1,   0x00000001
.equ  MPIDR_CORE_2,   0x00000002
.equ  MPIDR_CORE_3,   0x00000003
.endif

.equ  CORE_MASK_CPU0, 0x1
.equ  CORE_MASK_CPU1, 0x2
.equ  CORE_MASK_CPU2, 0x4
.equ  CORE_MASK_CPU3, 0x8

.equ  CONTEXT_CORE_0, 0x01234567
.equ  CONTEXT_CORE_1, 0x12345678
.equ  CONTEXT_CORE_2, 0xA9876543
.equ  CONTEXT_CORE_3, 0x10208070

//.equ  PSCI_V_MAJOR,   0x00000001
//.equ  PSCI_V_MINOR,   0x00000000
.equ  PSCI_V_MAJOR,   0x00000000
.equ  PSCI_V_MINOR,   0x00000002

.equ  PSCI_V_MASK,    0xFFFF

.equ  PFETCH_MASK,    0x5  // disable prefetch for cores 0 & 2

//-----------------------------------------------------------------------------

.ltorg

//-----------------------------------------------------------------------------

_test_psci:

 //------------------------------------

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

     // test MIGRATE_INFO
    ldr  x0, =PSCI32_MIGRATE_INFO_TYPE_ID
    smc  0x0
    nop
    nop
    nop
    ldr  x1, =MIGRATE_TYPE_NMIGRATE
    cmp  w0, w1
    b.ne cpu_0_fail_migrate_info

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

     // load a prefetch disable mask
    ldr  x0, =SIP_PREFETCH_DISABLE_64
    mov  x1, #PFETCH_MASK
    smc  0x0
    nop
    nop
    nop
    mov  x1, #SMC_SUCCESS
    cmp  x0, x1
    b.ne cpu_0_fail_prefetch

     // test cpu errata applied to the bootcore
    mov  x0, #CORE_MASK_CPU0
    bl   test_cpu_errata
    cbnz x0, cpu_0_fail_errata
    
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

.if (CPU_MAX_COUNT > 2)

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
2:
     // test AFFINITY_INFO of core 2
     // x0 = function id = 0xC4000004
     // x1 = mpidr       = 0x0002
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

 //------------------------------------
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
3:
     // test AFFINITY_INFO of core 3
     // x0 = function id = 0xC4000004
     // x1 = mpidr       = 0x0003
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

.endif

core_0_stop:
    b  core_0_stop

cpu_0_fail_version:
    b  cpu_0_fail_version

cpu_0_fail_affinity:
    b  cpu_0_fail_affinity

cpu_0_fail_unimplemented:
    b  cpu_0_fail_unimplemented

cpu_0_fail_migrate_info:
    b  cpu_0_fail_migrate_info

cpu_0_fail_errata:
    b cpu_0_fail_errata


 //------------------------------------

core_1a_entry:
    ldr  w9, =CONTEXT_CORE_1
    bl context_id_chk
    mov  x0, #CORE_MASK_CPU1
    bl test_cpu_errata
    cbnz x0, core_1_fail_errata
core_1_pass:
    b core_1_pass
core_1_fail_errata:
    b core_1_fail_errata

.if (CPU_MAX_COUNT > 2)

core_2a_entry:
    ldr  w9, =CONTEXT_CORE_2
    bl context_id_chk
    mov  x0, #CORE_MASK_CPU2
    bl test_cpu_errata
    cbnz x0, core_2_fail_errata
core_2_pass:
    b core_2_pass
core_2_fail_errata:
    b core_2_fail_errata

core_3a_entry:
    ldr  w9, =CONTEXT_CORE_3
    bl context_id_chk
    mov  x0, #CORE_MASK_CPU3
    bl test_cpu_errata
    cbnz x0, core_3_fail_errata
core_3_pass:
    b core_3_pass
core_3_fail_errata:
    b core_3_fail_errata

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

 // in:  x0 = core mask lsb
 // out: x0  = 0, success
 //      x0 != 0, failure
test_cpu_errata:
    mov  x4, x0

     // read midr_el1
    mrs   x1, midr_el1

     // x1 = midr_el1

    mov   x0, xzr
    bfxil x0, x1, #MIDR_PARTNUM_START, #MIDR_PARTNUM_WIDTH

     // x0 = part number (a53, a57, etc)
     // x1 = midr_el1

     // get the rNpN (variant:revision) number
    mov   x2, xzr
    bfxil x2, x1, #MIDR_VARIANT_START, #MIDR_VARIANT_WIDTH
    lsl   x2, x2, #MIDR_REVISION_WIDTH
    bfxil x2, x1, #MIDR_REVISION_START, #MIDR_REVISION_WIDTH

     // x0 = part number (a53, a57, etc)
     // x1 = midr_el1
     // x2 = rNpN
     // x4 = core mask

     // branch to the cpu-specific errata
    cmp   x0, #MIDR_PARTNUM_A53
    b.eq  10f
    cmp   x0, #MIDR_PARTNUM_A57
    b.eq  11f
    cmp   x0, #MIDR_PARTNUM_A72
    b.eq  12f
     // we don't recognize the core type - do nothing
    mov   x0, xzr
    b     13f
    
10:   // check a53 errata ------------------------

     // see if the dcache cln/invalidate errata should have been applied
    cmp   x2, #A53_DCACHE_RNPN_START
    mov   x0, xzr
    b.lt  13f

     // see if the errata was applied
    mrs   x1, CPUACTLR_EL1
    tst   x1, #CPUACTLR_ENDCCASCI_EN
    b.ne  13f
    mov   x0, #1
    b     13f

11:   // check a57 errata ------------------------
    nop
    mov   x0, xzr
    b     13f

12:   // check a72 errata ------------------------

    mov   x0, xzr

     // load 0 or 1 in x1 (0 if core was not marked for prefetch disable)
    mov   x1, #PFETCH_MASK
    tst   x1, x4
    mov   x1, xzr
    b.eq  1f
    mov   x1, #1
1:
    mrs   x2, CPUACTLR_EL1
     // load 0 or 1 in x2 (0 if prefetch is not disabled)
    mov   x3, #CPUACTLR_DIS_LS_HW_PRE
    tst   x2, x3
    mov   x2, xzr
    b.eq  2f
    mov   x2, #1
2:
    eor   x0, x2, x1
13:
    ret

 //------------------------------------

cpu_0_fail_prefetch:
    b  cpu_0_fail_prefetch

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


