//-----------------------------------------------------------------------------
// 
// Copyright (c) 2013-2016, Freescale Semiconductor
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

#include "aarch64.h"
#include "smc.h"

//-----------------------------------------------------------------------------

.global smc32_handler
.global _smc_success
.global _smc_failure
.global _smc_unimplemented
.global _smc_invalid
.global _smc_invalid_el

//-----------------------------------------------------------------------------

.equ SIP_REVISION_MAJOR, 0x1
.equ SIP_REVISION_MINOR, 0x0

.equ SIP_ROMUUID_PART1,  0x39080483    // bytes [3:0]
.equ SIP_ROMUUID_PART2,  0xADA93B0E    // bytes [7:4]
.equ SIP_ROMUUID_PART3,  0x1A7841A7    // bytes [11:8]
.equ SIP_ROMUUID_PART4,  0xD3205C97    // bytes [15:12]

 // function classes
.equ  SMC32_ARM_ARCH,   0x80
.equ  SMC32_CPU_SVC,    0x81
.equ  SMC32_SIP_SVC,    0x82
.equ  SMC32_OEM_SVC,    0x83
.equ  SMC32_STD_SVC,    0x84
.equ  SMC32_TRST_APP1,  0xB0
.equ  SMC32_TRST_APP2,  0xB1

 // function counts
.equ  SIP32_FUNCTION_COUNT, 0x4

//-----------------------------------------------------------------------------

smc32_handler:
     // secure monitor smc32 interface lives here
     // Note: this secure monitor only implements "fast calls", thus
     //       interrupts are disabled during execution of the call

     // per the ARM SMC Calling convention:
     // w0     = function id 
     // w1-w5  = function parameters 
     // w6     = session id (optional) 
     // w7     = hypervisor client id
     // w8     = indirect result location register
     // w9-w15 = volatile/scratch

     // mask interrupts
    msr  DAIFset, #0xF

     // extract bits 31:24 to see what class of function this is
    and   w9, w0, #SMC_FCLASS_MASK
    lsr   w9, w9, #SMC_FCLASS_SHIFT
     // extract bits 15:0 (the function number)
    and   w11, w0, #SMC_FNUM_MASK

     // Note: x11 contains the function number

     // is it SMC32: ARM Architecture Call?
    cmp  w9, #SMC32_ARM_ARCH
    b.eq smc32_arch_svc
     // is it SMC32: CPU Service Call?
    cmp  w9, #SMC32_CPU_SVC
    b.eq smc32_no_services
     // is it SMC32: SiP Service Call?
    cmp  w9, #SMC32_SIP_SVC
    b.eq smc32_sip_svc
     // is it SMC32: OEM Service Call?
    cmp  w9, #SMC32_OEM_SVC
    b.eq smc32_no_services
     // is it SMC32: Std Service Call?
    cmp  w9, #SMC32_STD_SVC
    b.eq _smc32_std_svc
     // is it SMC32: Trusted App Call?
    cmp  w9, #SMC32_TRST_APP1
    b.eq _smc_unimplemented
     // is it SMC32: Trusted App Call?
    cmp  w9, #SMC32_TRST_APP2
    b.eq _smc_unimplemented
     // is it SMC32: Trusted OS Call? (multiple ranges)
    lsr  w10, w9, #4
    cmp  w10, #0xB
#if (CNFG_SPD)
    b.eq _smc_trstd_os_handler
#else
    b.eq smc32_no_services
#endif

     // if we are here then we have an unimplemented/unrecognized function
    b _smc_unimplemented

     //------------------------------------------

     // Note: w11 contains the function number

smc32_sip_svc:
     // SIP service call COUNT function is 0xFF00
    mov  w10, #SIP_COUNT
    cmp  w10, w11
    b.eq smc32_sip_count

     // SIP service call UUID function is 0xFF01
    mov  w10, #0xFF01
    cmp  w10, w11
    b.eq smc32_sip_UUID

     // SIP service call function reserved 0xFF02

     // SIP service call REVISION function is 0xFF03
    mov  w10, #0xFF03
    cmp  w10, w11
    b.eq smc32_sip_REVISION

     // SIP service call PRNG_32
    mov  w10, #SIP_PRNG
    cmp  w10, w11
    b.eq smc32_sip_PRNG

     // SIP service call RNG_32
    mov  w10, #SIP_RNG
    cmp  w10, w11
    b.eq smc32_sip_RNG

     // SIP service call HW UNQ Key
    mov  w10, #SIP_HW_UNQ_KEY
    cmp  w10, w11
    b.eq smc32_sip_HW_UNQ_KEY

#if (DEBUG_BUILD)
     // SIP service call L1L2_ERR
    mov  w10, #SIP_L1L2_ERR
    cmp  w10, w11
    b.eq smc32_sip_L1L2_ERR_inject
#endif

     // SIP service call L1L2_ERR
    mov  w10, #SIP_L2_CLR
    cmp  w10, w11
    b.eq smc32_sip_L2_ERR_clear

     // if we are here then we have an unimplemented/unrecognized function
    b    _smc_unimplemented

     //------------------------------------------

 // this function returns the number of smc32 SIP functions implemented
 // the count includes *this* function
smc32_sip_count:

    mov  x0, #SIP32_FUNCTION_COUNT
    b    _smc_exit

     //------------------------------------------

 // this function returns the SIP UUID for the secure monitor
 // resident in the bootrom
smc32_sip_UUID:

    ldr  x0, =SIP_ROMUUID_PART1
    ldr  x1, =SIP_ROMUUID_PART2
    ldr  x2, =SIP_ROMUUID_PART3
    ldr  x3, =SIP_ROMUUID_PART4
    b    _smc_exit

     //------------------------------------------

 // this function returns the major and minor revision numbers
 // of this secure monitor
smc32_sip_REVISION:

    ldr  x0, =SIP_REVISION_MAJOR
    ldr  x1, =SIP_REVISION_MINOR
    b    _smc_exit

     //------------------------------------------

 // this is the 32-bit interface to the PRNG function
 // in:  x0 = function id
 //      x1 = 0, 32-bit PRNG requested
 //      x1 = 1, 64-bit PRNG requested
 // out: x0 = 0, success
 //      x0 != 0, failure
 //      x1 = 32-bit PRNG, or hi-order 32-bits of 64-bit PRNG
 //      x2 = lo-order 32-bits of 64-bit PRNG
smc32_sip_PRNG:
     // make sure bits 63:32 in the registers containing input parameters
     // are zeroed-out (input parameters are in x0-x1)
    lsl  x0, x0, #32
    lsl  x1, x1, #32
    lsr  x0, x0, #32
    lsr  x1, x1, #32

    cbz  x1, 1f
     // 64-bit PRNG
    mov  x0, #SIP_PRNG_64BIT
    bl   _get_PRNG
     // hi-order bits in w1
    and  x1, xzr, x0, lsr #32
     // lo-order bits in w2
    mov  w2, w0
     // check if hi-order bit is 0
    cmp w1, #0
    b.ne    2f
     // Check if lo-order bits are also 0
    cmp w2, #0
     // smc_failure if w1 and w2 are 0
    b.eq _smc_failure
    b    2f

1:   // 32-bit PRNG
    mov  x0, #SIP_PRNG_32BIT
    bl   _get_PRNG
     // result in w1
    mov  w1, w0
     // check if rng number is 0
    cmp w1, #0
    b.eq _smc_failure

2:
    b    _smc_success

     //------------------------------------------

 // this is the 32-bit interface to the hw RNG function
 // in:  x0 = function id
 //      x1 = 0, 32-bit hw RNG requested
 //      x1 = 1, 64-bit hw RNG requested
 // out: x0 = 0, success
 //      x0 != 0, failure
 //      x1 = 32-bit RNG, or hi-order 32-bits of 64-bit RNG
 //      x2 = lo-order 32-bits of 64-bit RNG
smc32_sip_RNG:
     // make sure bits 63:32 in the registers containing input parameters
     // are zeroed-out (input parameters are in x0-x1)
    lsl  x0, x0, #32
    lsl  x1, x1, #32
    lsr  x0, x0, #32
    lsr  x1, x1, #32
    mov  x11, x1

     // For NON-E parts return unimplemented
    bl    _check_sec_disabled
    cbnz  x0, _smc_unimplemented

     // Restore x1
    mov x1, x11
    cbz  x1, 1f
     // 64-bit RNG
    mov  x0, #SIP_RNG_64BIT
    bl   _get_RNG
     // hi-order bits in w1
    and  x1, xzr, x0, lsr #32
     // lo-order bits in w2
    mov  w2, w0
     // check if hi-order bit is 0
    cmp w1, #0
    b.ne    2f
     // Check if lo-order bits are also 0
    cmp w2, #0
     // smc_failure if w1 and w2 are 0
    b.eq _smc_failure
    b   2f

1:   // 32-bit RNG
    mov  x0, #SIP_RNG_32BIT
    bl   _get_RNG
     // result in w1
    mov  w1, w0
     // check if rng number is 0
    cbz  w1, _smc_failure
2:
    b    _smc_success

     //------------------------------------------

#if (DEBUG_BUILD)

 // this is the 32-bit interface to the SIP_ALLOW_L1L2_ERR_32 function
 // in:  x0 = function id
 // out: x0 = SMC_SUCCESS, on success
 //      x0 = SMC_UNIMPLEMENTED, if function not available
smc32_sip_L1L2_ERR_inject:
     // we need to open up access to selected EL2/EL3 registers so that
     // EL1/EL2 code can inject L1/L2 memory errors for testing
    bl  _allow_L1L2err_inject
    b   _smc_success

#endif

     //------------------------------------------

 // this is the 32-bit interface to the SIP_ALLOW_L2_CLR_32 function
 // in:  x0 = function id
 // out: x0 = SMC_SUCCESS, on success
 //      x0 = SMC_UNIMPLEMENTED, if function not available
smc32_sip_L2_ERR_clear:
     // we need to open up access to selected EL2/EL3 registers so that
     // EL1/EL2 code can clear memory errors
    bl  _allow_L1L2err_clear
    b   _smc_success

     //------------------------------------------

     // Note: w11 contains the function number

smc32_arch_svc:
     // ARCH service call VERSION function
    mov  w10, #ARCH_VERSION
    cmp  w10, w11
    b.eq smc32_arch_version

     // ARCH service call FEATURES function
    mov  w10, #ARCH_FEATURES
    cmp  w10, w11
    b.eq smc32_arch_features

     // ARCH service call WORKAROUND_1 function
    mov  w10, #ARCH_WORKAROUND_1
    cmp  w10, w11
    b.eq smc32_arch_workaround1

    b    _smc_unimplemented

     //------------------------------------------

 // this is the 32-bit interface to the arch VERSION function (smc v1.1)
 // in:  x0 = function id
 // out: x0 = SMC_VERSION_11
smc32_arch_version:

     // load the version info
    ldr  x0, =SMC_VERSION_11
    b    _smc_exit

     //------------------------------------------

 // this is the 32-bit interface to the arch FEATURES function (smc v1.1)
 // in:  x0 = function id
 // in:  x1 = function id of ARCH function being queried
 // out: x0 = SMC_SUCCESS, SMC_NOT_SUPPORTED
 // uses x0, x1, x2, x3
smc32_arch_features:

     // extract function range bits [31:16]
    and   w0, w1, #SMC_FCLASS_MASK
    lsr   w0, w0, #SMC_FCLASS_SHIFT
    cmp   w0, #SMC32_ARM_ARCH
    b.ne  _smc_unimplemented

    ldr   x3, =SMC_FEATURES_TABLE_END
     // get the address of the features table
    adr   x0, smc32_arch_features_table
1:
     // load the supported feature
    ldr   w2, [x0]

     // compare this with the TABLE_END value
    cmp   w2, w3
    b.eq  _smc_unimplemented

     // see if we have a match to the feature-under-query (x1)
    cmp   w2, w1
    add   x0, x0, #4
    b.ne  1b

    b    _smc_success

     //------------------------------------------

 // this function provides a callable workaround for CVE-2017-5715,
 // which is Spectre version 2
 // in:  none
 // out: none
smc32_arch_workaround1:

     // disable and re-enable the mmu - this has the side effect
     // of invalidating BTB
    mrs   x0, sctlr_el3
    bic   x1, x0, #SCTLR_M_MASK
    msr   sctlr_el3, x1
    isb

     // re-enable the mmu
    msr   sctlr_el3, x0
    isb

    b    _smc_success

     //------------------------------------------

 // this is the 32-bit interface to the hw HUK function
 // in:  x0 = function id
 //      x1 = TEE OS shared Address for HW Key Buffer
 //      x2 = Size of the HW key
 // out: x0 = 0, success
 //      x0 != 0, failure
smc32_sip_HW_UNQ_KEY:

     // make sure bits 63:32 in the registers containing input parameters
     // are zeroed-out (input parameters are in x0-x1)
    lsl  x0, x0, #32
    lsl  x1, x1, #32
    lsl  x2, x2, #32
    lsr  x0, x0, #32
    lsr  x1, x1, #32
    lsr  x2, x2, #32

     // if the SEC block is not present, return unimplemented
    mov  x3, x1
    mov  x4, x2
    bl   _check_sec_disabled
    cbnz x0, _smc_unimplemented

     // save original function args
    stp  x3, x4, [sp, #-16]!

    mov  x0, #1
     // flush dcache
    bl   _cln_inv_all_dcache

     // Load x0 (physical address of the HUK Buffer)
     // Load x1 (Size of the HUK)
     // Restoring to x0,x1 instead of restoring to x1,x2,
     // so as to be passed as an arguments to _get_hw_unq_key.
     // Take error exit if size is 0x0
    ldp  x0, x1, [sp], #16
    cbz  x1, _smc_failure

     // get the H/W Unique Key
    bl   _get_hw_unq_key
    cbnz w0, _smc_failure

    b    _smc_success

     //------------------------------------------

smc32_no_services:
     // w11 contains the requested function id
    mov   w10, #0xFF00
     // w10 contains the call count function id
    cmp   w10, w11
    b.ne  _smc_unimplemented
     // call count is zero
    mov   w0, #0
    b     _smc_exit

_smc_failure:
    mov   x0, #SMC_FAILURE
    b     _smc_exit
    
_smc_success:
    mov   x0, #SMC_SUCCESS
    b     _smc_exit
    
_smc_unimplemented:
    mov   x0, #SMC_UNIMPLEMENTED
    b     _smc_exit
    
_smc_invalid:
    mov   x0, #SMC_INVALID
    b     _smc_exit
    
_smc_invalid_el:
    mov   x0, #SMC_INVALID_EL
    b     _smc_exit

     //------------------------------------------

__dead_loop:
    wfe
    b __dead_loop

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

smc32_sip_features_table:
    .4byte  SIP32_COUNT_ID
    .4byte  SIP_PRNG_32
    .4byte  SIP_RNG_32
     // table terminating value - must always be last entry in table
    .4byte  SMC_FEATURES_TABLE_END

//-----------------------------------------------------------------------------

smc32_arch_features_table:
    .4byte  ARCH32_VERSION_ID
    .4byte  ARCH32_FEATURES_ID
    .4byte  ARCH32_WORKAROUND_1
     // table terminating value - must always be last entry in table
    .4byte  SMC_FEATURES_TABLE_END

//-----------------------------------------------------------------------------


