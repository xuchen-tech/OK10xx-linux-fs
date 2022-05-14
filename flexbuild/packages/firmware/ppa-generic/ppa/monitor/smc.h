//-----------------------------------------------------------------------------
// 
// Copyright (c) 2016, Freescale Semiconductor
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

#ifndef _SMC_H
#define	_SMC_H

//-----------------------------------------------------------------------------

.equ REGISTER_OBFUSCATE, 0xA5A5A5A5A5A5A5A5

//-----------------------------------------------------------------------------

#define  SMC_EL2_IS_LE    0x0
#define  SMC_EL2_IS_BE    0x1

#define SMC_AARCH32_MODE  0x1
#define SMC_AARCH64_MODE  0x0

 // function return values
#define  SMC_SUCCESS         0
#define  SMC_UNIMPLEMENTED  -1
#define  SMC_NOT_SUPPORTED  -1
#define  SMC_INVALID        -2
#define  SMC_BAD_PARM       -3
#define  SMC_INVALID_EL     -4
#define  SMC_FAILURE        -5

#define  SMC_VERSION_10     0x10000
#define  SMC_VERSION_11     0x10001

.equ SIP_PRNG_32BIT,  0
.equ SIP_PRNG_64BIT,  1
.equ SIP_RNG_32BIT,   0
.equ SIP_RNG_64BIT,   1

 // mask to extract smc function number from the smc functon id
#define  SMC_FNUM_MASK      0xFFFF
#define  SMC_FCLASS_MASK    0xFF000000
#define  SMC_FCLASS_SHIFT   24

#define  SMC_FEATURES_TABLE_END 0x0FF00000

#define  SMC_FAST_CALL_BIT   0x80000000
#define  SMC_FAST_CALL_FIELD 0x00FF0000
#define  SMC_INTERFACE_BIT   0x40000000

//-----------------------------------------------------------------------------

 // smc sip function id's - these are "fast", non-preemptible functions

 // this function returns the number of implemented smc64-sip functions
#define  SIP64_COUNT_ID     0xC200FF00
 // this function returns the number of implemented smc32-sip functions
#define  SIP32_COUNT_ID     0x8200FF00
#define  SIP_COUNT          (SIP64_COUNT_ID & SMC_FNUM_MASK)

 // this is the 32-bit interface to the PRNG function
 // in:  x0 = function id
 //      x1 = 0, 32-bit PRNG requested
 //      x1 = 1, 64-bit PRNG requested
 // out: x0 = 0, success
 //      x0 != 0, failure
 //      x1 = 32-bit PRNG, or hi-order 32-bits of 64-bit PRNG
 //      x2 = lo-order 32-bits of 64-bit PRNG
#define  SIP_PRNG_32 0x8200FF10

 // this is the 32-bit interface to the hw RNG function
 // in:  x0 = function id
 //      x1 = 0, 32-bit RNG requested
 //      x1 = 1, 64-bit RNG requested
 // out: x0 = 0, success
 //      x0 != 0, failure
 //      x1 = 32-bit PRNG, or hi-order 32-bits of 64-bit PRNG
 //      x2 = lo-order 32-bits of 64-bit PRNG
#define  SIP_RNG_32 0x8200FF11

 // this is the 64-bit interface to the PRNG function
 // in:  x0 = function id
 //      x1 = 0, 32-bit PRNG requested
 //      x1 = 1, 64-bit PRNG requested
 // out: x0 = 0, success
 //      x0 != 0, failure
 //      x1 = 32-or-64-bit PRNG
#define  SIP_PRNG_64 0xC200FF10
#define  SIP_PRNG    (SIP_PRNG_64 & SMC_FNUM_MASK)

 // this is the 64-bit interface to the hw RNG function
 // in:  x0 = function id
 //      x1 = 0, 32-bit hw RNG requested
 //      x1 = 1, 64-bit hw RNG requested
 // out: x0 = 0, success
 //      x0 != 0, failure
 //      x1 = 32-or-64-bit PRNG
#define  SIP_RNG_64 0xC200FF11
#define  SIP_RNG    (SIP_RNG_64 & SMC_FNUM_MASK)

 // this is the 64-bit interface to the MEMBANK function
 // in:  x0 = function id
 //      x1 = memory bank requested (1, 2, 3, etc)
 // out: x0     = -1, failure
 //             = -2, invalid parameter
 //      x0 [0] =  1, valid bank
 //             =  0, invalid bank
 //       [1:2] =  1, ddr  
 //             =  2, sram
 //             =  3, special
 //         [3] =  1, not the last bank
 //             =  0, last bank
 //      x1     =  physical start address (not valid unless x0[0]=1)
 //      x2     =  size in bytes (not valid unless x0[0]=1)
#define  SIP_MEMBANK_64 0xC200FF12
#define  SIP_MEMBANK    (SIP_MEMBANK_64 & SMC_FNUM_MASK)

 // this is the 64-bit interface to the LOAD-STORE PREFETCH DISABLE function
 // in:  x0 = function id
 //      x1 = core mask for cores to have prefetch disabled,
 //           where bit[0] = core0, bit[1] = core1, etc - if bit is set,
 //           then prefetch (CPUACTLR[56]) is disabled for that core.
 // out: none
#define  SIP_PREFETCH_DISABLE_64 0xC200FF13
#define  SIP_PREFETCH            (SIP_PREFETCH_DISABLE_64 & SMC_FNUM_MASK)

 // this is the 32-bit interface to the HUK function
 // in:  x0 = function id
 //      x1 = 0, Buffer Physical Address of the request HUK.
 //      x1 = 1, Size of the HUK
 // out: x0 = 0, success
 //      x0 != 0, failure
 //      x1 = 32-bit PRNG, or hi-order 32-bits of 64-bit PRNG
 //      x2 = lo-order 32-bits of 64-bit PRNG
#define  SIP_HW_UNQ_KEY_32 0x8200FF14
#define  SIP_HW_UNQ_KEY (SIP_HW_UNQ_KEY_32 & SMC_FNUM_MASK)

 // this is the 32-bit interface to the SIP_ALLOW_L1L2_ERR_32 function
 // in:  x0 = function id
 // out: x0 = SMC_SUCCESS, on success
 //      x0 = SMC_UNIMPLEMENTED, if function not available
#define  SIP_ALLOW_L1L2_ERR_32 0x8200FF15
#define  SIP_L1L2_ERR          (SIP_ALLOW_L1L2_ERR_32 & SMC_FNUM_MASK)

 // this is the 32-bit interface to the SIP_ALLOW_L2_CLR_32 function
 // in:  x0 = function id
 // out: x0 = SMC_SUCCESS, on success
 //      x0 = SMC_UNIMPLEMENTED, if function not available
#define  SIP_ALLOW_L2_CLR_32   0x8200FF16
#define  SIP_L2_CLR            (SIP_ALLOW_L2_CLR_32 & SMC_FNUM_MASK)

//-----------------------------------------------------------------------------

 // smc arch function id's - these are "fast", non-preemptible functions

 // this function returns the number of implemented smc-arch functions
#define  ARCH32_COUNT_ID    0x8000FF00
#define  ARCH64_COUNT_ID    0xC000FF00
#define  ARCH_COUNT         (ARCH64_COUNT_ID & SMC_FNUM_MASK)

 // this function will return to EL2 @ Aarch32
 // in:  x0 = function id
 //      x1 = start address for EL2 @ Aarch32
 //      x2 = first parameter to EL2
 //      x3 = second parameter to EL2
 //      x4 = 0, EL2 in LE (little-endian)
 //      x4 = 1, EL2 in BE (big-endian)
#define  ARCH_EL2_2_AARCH32_ID  0xC000FF04
#define  ARCH_EL2_2_AARCH32     (ARCH_EL2_2_AARCH32_ID & SMC_FNUM_MASK)

 // this function returns the smc version number
#define  ARCH32_VERSION_ID   0x80000000
#define  ARCH_VERSION        (ARCH32_VERSION_ID & SMC_FNUM_MASK)

 // this function queries for the implementation of other arch32 functions
#define  ARCH32_FEATURES_ID  0x80000001
#define  ARCH_FEATURES       (ARCH32_FEATURES_ID & SMC_FNUM_MASK)

 // this function provides a workaround for CVE-2017-5715
#define  ARCH32_WORKAROUND_1  0x80008000
#define  ARCH32_WKRND_1_UPPER 0x80000000
#define  ARCH32_WKRND_1_LOWER 0x00008000
#define  ARCH_WORKAROUND_1    (ARCH32_WORKAROUND_1 & SMC_FNUM_MASK)

//-----------------------------------------------------------------------------

 // this macro isolates EL3 so that there are fewer visible artifacts
 // between execution levels for speculative execution side channels
.macro EL3_IsolateOnEntry
#if ((CORE == 72) || (CORE == 57))
     // temporarily save a couple of working registers
    stp   x0, x1, [sp, #-16]!

     // disable and re-enable the mmu - this has the side effect
     // of invalidating BTB
    mrs   x0, sctlr_el3
    bic   x1, x0, #SCTLR_M_MASK
    msr   sctlr_el3, x1
    isb

     // re-enable the mmu
    msr   sctlr_el3, x0
    isb

     // restore the temp working regs
    ldp  x0, x1, [sp], #16
#endif
.endm

//-----------------------------------------------------------------------------

 // this macro isolates EL3 so that there are fewer visible artifacts
 // between execution levels for speculative execution side channels
.macro EL3_CkAndIsolate
#if ((CORE == 72) || (CORE == 57))
     // temporarily save a couple of working registers
    stp   x0, x1, [sp, #-16]!

     // provide an extremely fast path for the 
     // SMCCC_ARCH_WORKAROUND_1 function
    mov   w1, #ARCH32_WKRND_1_UPPER
    orr   w1, w1, #ARCH32_WKRND_1_LOWER
    cmp   w0, w1
    b.ne  1f

     // fast processing starts here

     // disable and re-enable the mmu - this has the side effect
     // of invalidating BTB
    mrs   x0, sctlr_el3
    bic   x1, x0, #SCTLR_M_MASK
    msr   sctlr_el3, x1
    isb

     // re-enable the mmu
    msr   sctlr_el3, x0
    isb

     // restore the temp working regs
    ldp  x0, x1, [sp], #16

    tlbi alle3
    eret

1:   // normal processing starts here

     // disable and re-enable the mmu - this has the side effect
     // of invalidating BTB
    mrs   x0, sctlr_el3
    bic   x1, x0, #SCTLR_M_MASK
    msr   sctlr_el3, x1
    isb

     // re-enable the mmu
    msr   sctlr_el3, x0
    isb

     // restore the temp working regs
    ldp  x0, x1, [sp], #16
#endif
.endm

//-----------------------------------------------------------------------------

.macro EL3_ExitToNS
     // invalidate any el3 page translations
    tlbi alle3
.endm

//-----------------------------------------------------------------------------

#endif // _SMC_H
