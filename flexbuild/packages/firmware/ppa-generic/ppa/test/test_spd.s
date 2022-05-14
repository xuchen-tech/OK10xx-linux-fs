//-----------------------------------------------------------------------------
// 
// Copyright (c) 2013-2016, Freescale Semiconductor, Inc.
// Copyright 2017-2018 NXP Semiconductors
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

//-----------------------------------------------------------------------------

  .global _test_psci

//-----------------------------------------------------------------------------

.align 12

//-----------------------------------------------------------------------------

.equ  ARCH_FUNCTION_IN_RANGE,   0x8000FF0F
.equ  ARCH_FUNCTION_OUT_RANGE,  0x89000000

.equ  SPD_DUMMY_FUNCTION,       0x32000000

.equ  REGISTER_x2,   0x00000002
.equ  REGISTER_x3,   0x00000030
.equ  REGISTER_x4,   0x00000400
.equ  REGISTER_x5,   0x00005000
.equ  REGISTER_x6,   0x00060000
.equ  REGISTER_x7,   0x00700000
.equ  REGISTER_x8,   0x08000000
.equ  REGISTER_x9,   0x90000000
.equ  REGISTER_x10,  0x00010000
.equ  REGISTER_x11,  0x00011000
.equ  REGISTER_x12,  0x00012000
.equ  REGISTER_x13,  0x00013000
.equ  REGISTER_x14,  0x00014000
.equ  REGISTER_x15,  0x00015000
.equ  REGISTER_x16,  0x00016000
.equ  REGISTER_x17,  0x00017000
.equ  REGISTER_x18,  0x00018000

_test_psci:

    bl  test_01
    bl  register_ck
    bl  test_02
    bl  register_ck
    bl  test_03
    bl  register_ck

 //------------------------------------

core_0_stop:
    b  .

fail_smc_ver:
    b  .

fail_feature_implemented:
    b  .

fail_bad_feature_in_range:
    b  .

fail_bad_feature_out_range:
    b  .

fail_workaround_1:
    b  .

fail_register_ck:
    b  .

 //------------------------------------

test_01:
     // test SMC_VERSION
    mov   x0, x30
    bl    load_register
    mov   x30, x0
    ldr   x0, =ARCH32_VERSION_ID
    smc   0x0
    nop
    nop
    nop
    cmp   x0, #SMC_NOT_SUPPORTED
    b.eq  fail_smc_ver
    ldr   x1, =SMC_VERSION_11
    cmp   x0, x1
    b.ne  fail_smc_ver

     // test SMC_FEATURES w/implemented function id
    ldr   x0, =ARCH32_FEATURES_ID
    ldr   x1, =ARCH32_WORKAROUND_1
    smc   0x0
    nop
    nop
    nop
    cmp   x0, #SMC_SUCCESS
    b.ne  fail_feature_implemented
    
     // test SMC_FEATURES w/unimplemented function id in-range
    ldr   x0, =ARCH32_FEATURES_ID
    ldr   x1, =ARCH_FUNCTION_IN_RANGE
    smc   0x0
    nop
    nop
    nop
    cmp   x0, #SMC_NOT_SUPPORTED
    b.ne  fail_bad_feature_in_range
    
     // test SMC_FEATURES w/unimplemented function id out-of-range
    ldr   x0, =ARCH32_FEATURES_ID
    ldr   x1, =ARCH_FUNCTION_OUT_RANGE
    smc   0x0
    nop
    nop
    nop
    cmp   x0, #SMC_NOT_SUPPORTED
    b.ne  fail_bad_feature_out_range
    
    ret

 //------------------------------------

test_02:
     // call the spectre mitigation function
    mov   x0, x30
    bl    load_register
    mov   x30, x0
    ldr   x0, =ARCH32_WORKAROUND_1
    smc   0x0
    nop
    nop
    nop

    ret

 //------------------------------------

test_03:
     // call the dummy spd function
    mov   x0, x30
    bl    load_register
    mov   x30, x0
    mov   x0, #SPD_DUMMY_FUNCTION
    smc   0x0
    nop
    nop
    nop

    ret

 //------------------------------------

load_register:

    mov   x4,  #REGISTER_x4
    mov   x5,  #REGISTER_x5
    mov   x6,  #REGISTER_x6
    mov   x7,  #REGISTER_x7
    mov   x8,  #REGISTER_x8
    mov   x9,  #REGISTER_x9
    mov   x10, #REGISTER_x10
    ldr   x11, =REGISTER_x11
    ldr   x12, =REGISTER_x12
    ldr   x13, =REGISTER_x13
    ldr   x14, =REGISTER_x14
    ldr   x15, =REGISTER_x15
    ldr   x16, =REGISTER_x16
    ldr   x17, =REGISTER_x17
    mov   x18, #REGISTER_x18

    ret

 //------------------------------------

register_ck:
    mvn  x0, xzr

    cmp  x4, #REGISTER_x4
    mov  x0, #4
    b.ne fail_register_ck

    cmp  x5, #REGISTER_x5
    mov  x0, #5
    b.ne fail_register_ck

    cmp  x6, #REGISTER_x6
    mov  x0, #6
    b.ne fail_register_ck

    cmp  x7, #REGISTER_x7
    mov  x0, #7
    b.ne fail_register_ck

    ldr  x1, =REGISTER_x8
    cmp  x8, x1
    mov  x0, #8
    b.ne fail_register_ck

    ldr  x1, =REGISTER_x9
    cmp  x9, x1
    mov  x0, #9
    b.ne fail_register_ck

    cmp  x10, #REGISTER_x10
    mov  x0, #10
    b.ne fail_register_ck

    cmp  x11, #REGISTER_x11
    mov  x0, #11
    b.ne fail_register_ck

    cmp  x12, #REGISTER_x12
    mov  x0, #12
    b.ne fail_register_ck

    cmp  x13, #REGISTER_x13
    mov  x0, #13
    b.ne fail_register_ck

    cmp  x14, #REGISTER_x14
    mov  x0, #14
    b.ne fail_register_ck

    cmp  x15, #REGISTER_x15
    mov  x0, #15
    b.ne fail_register_ck

    cmp  x16, #REGISTER_x16
    mov  x0, #16
    b.ne fail_register_ck

    cmp  x17, #REGISTER_x17
    mov  x0, #17
    b.ne fail_register_ck

    cmp  x18, #REGISTER_x18
    mov  x0, #18
    b.ne fail_register_ck

    mov  x0, xzr
    ret

 //------------------------------------

