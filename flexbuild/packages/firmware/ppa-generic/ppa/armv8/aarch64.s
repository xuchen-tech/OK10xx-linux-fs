//-----------------------------------------------------------------------------
// 
// Copyright (c) 2015-2016, Freescale Semiconductor, Inc. All rights reserved.
// Copyright 2017-2018 NXP Semiconductor
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
//-----------------------------------------------------------------------------

.section .text, "ax"

//-----------------------------------------------------------------------------

#include "aarch64.h"
#include "soc.h"
#include "psci.h"
#include "policy.h"
#include "runtime_data.h"

//-----------------------------------------------------------------------------

.global _apply_cpu_errata
.global _cln_inv_L1_dcache
.global _cln_inv_all_dcache
.global _cln_inv_L3_dcache
.global _getCallerEL
.global _get_exit_mode
.global _init_core_EL3
.global _init_core_EL2
.global _init_core_EL1
.global _init_secondary_EL3
.global _init_secondary_EL2
.global _init_secondary_EL1
.global _is_EL2_supported
.global _relocate_rela
.global _set_tcr
.global _set_spsr_4_exit
.global _set_spsr_4_startup
.global _set_EL3_vectors
.global _zeroize_bss
.global _disable_ldstr_pfetch_A72
.global _enable_ldstr_pfetch_A72
.global _disable_ldstr_pfetch_A53
.global _enable_ldstr_pfetch_A53
.global _allow_L1L2err_clear

#if (DEBUG_BUILD)
.global _allow_L1L2err_inject
#endif

//-----------------------------------------------------------------------------

.equ TCR_EL3_IRGN_MASK,      0x300
.equ TCR_EL3_ORGN_MASK,      0xC00
.equ TCR_EL3_IRGN_CACHEABLE, 0x100
.equ TCR_EL3_ORGN_CACHEABLE, 0x400

.equ DCACHE_MASK,            0x6
.equ CTYPE_FIELD_WIDTH,      0x3
 // cache level field is left-shifted by 1, so add 2 for next level
.equ NEXT_CACHE_LEVEL,       0x2

 // the number of sets - 1
 // 1 MB / (16 ways * 64-byte lines) = 1024 sets
.equ L3_MAX_SET,  1023

 // the number of ways - 1
.equ L3_MAX_WAY,  15

 // amount to shift the set #
 // Log2(64-byte line) = 6
.equ L3_SET_SHIFT, 6

 // amount to shift the way #
 // 32 - Log2(16 ways) = 28
.equ L3_WAY_SHIFT, 28

 // 0-based cache level
.equ L3_LEVEL, 2

//-----------------------------------------------------------------------------

 // this function applies any cpu-specific errata (normally comes from ARM)
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4, x5
_apply_cpu_errata:
    mov   x5, x30
    mov   x4, x0

     // x4 = core mask lsb

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

     // branch to the cpu-specific errata
    cmp   x0, #MIDR_PARTNUM_A53
    b.eq  1f
    cmp   x0, #MIDR_PARTNUM_A57
    b.eq  2f
    cmp   x0, #MIDR_PARTNUM_A72
    b.eq  3f
     // we don't recognize the core type - do nothing
    b     4f

1:   // apply a53 errata ------------------------

     // see if we need to apply the dcache cln/invalidate errata
    cmp   x2, #A53_DCACHE_RNPN_START
    b.lt  5f
     // apply the errata - turn dcache cln into dcache cln & inv
    mrs   x0, CPUACTLR_EL1
    orr   x0, x0, #CPUACTLR_ENDCCASCI_EN
    msr   CPUACTLR_EL1, x0

5:
     // see if this core is marked for prefetch disable
    mov   x0, #PREFETCH_DIS_OFFSET
    bl    _get_global_data
    tst   x0, x4
    b.eq  4f
    bl    _disable_ldstr_pfetch_A53
    b     4f

2:   // apply a57 errata ------------------------

#if (!SUPPRESS_SPECTREV4)
     // mitigation for Spectre v4 - disable load-pass-store
    mrs   x0, CPUACTLR_EL1
    orr   x0, x0, #CPUACTLR_DIS_LD_PASS_ST    
    msr   CPUACTLR_EL1, x0
    isb
#endif
    b     4f

3:   // apply a72 errata ------------------------

#if (!SUPPRESS_SPECTREV4)
     // mitigation for Spectre v4 - disable load-pass-store
    mrs   x0, CPUACTLR_EL1
    orr   x0, x0, #CPUACTLR_DIS_LD_PASS_ST    
    msr   CPUACTLR_EL1, x0
    isb
#endif

     // see if this core is marked for prefetch disable
    mov   x0, #PREFETCH_DIS_OFFSET
    bl    _get_global_data
    tst   x0, x4
    b.eq  4f
    bl    _disable_ldstr_pfetch_A72
 
4:   // exit
    mov   x30, x5
    ret

//-----------------------------------------------------------------------------

 // this function cleans, and optionally also invalidates, all levels of dcache
 // in:  x0 = 0, clean only
 //      x0 = 1, clean and invalidate
 // uses x0, x1, x2, x3, x4, x5, x6, x7, x8, x9
_cln_inv_all_dcache:

     // read info on all caches into x8
    mrs   x1, clidr_el1
    mov   x8, xzr
    bfxil x8, x1, #0, #21

     // x9 holds cache level being worked on
    mov   x9, xzr

5:
     // check if current selected cache level has a dcache
    tst   x8, #DCACHE_MASK
    b.eq  2f

     // read info on currently selected data cache into x1
    msr   csselr_el1, x9
    isb
    mrs   x1, ccsidr_el1

     // x2 = (# of ways)-1
     // x3 = (# of sets)-1
     // x4 = Way shift amount
     // x5 = Set shift amount (log2(linesize))
    mov   x2, xzr
    mov   x3, xzr
    bfxil x2, x1, #3, #10
    bfxil x3, x1, #13, #15
    and   x5, x1, #7
    add   x5, x5, #4
    clz   w4, w2

     // x7 is register used for maintenance instruction
     // x6 is the way being worked on

4:   // set-loop
    mov   x6, x2

     // x1 holds the shifted set # and the cache level 
    lsl   x1, x3, x5
    orr   x1, x1, x9
3:   // way-loop
     // insert way #, set #, and cache level into x7
    lsl   x7, x6, x4
    orr   x7, x7, x1
   
     // either perform a clean or a clean and invalidate 
    cbz   x0, 6f
    dc    cisw, x7
    b     1f
    
6:
    dc    csw, x7
    
1:
     // decrement way and check
    subs  x6, x6, #1
    b.ge  3b

     // decrement set and check
    subs  x3, x3, #1
    b.ge  4b

     // increment to next cache level
    add   x9, x9, #NEXT_CACHE_LEVEL
    lsr   x8, x8, #CTYPE_FIELD_WIDTH
    b     5b

2:
    msr   csselr_el1, xzr
    dsb sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function cleans, and optionally also invalidates, the L1 dcache
 // in:  x0 = 0, clean only
 //      x0 = 1, clean and invalidate
 // uses x0, x1, x2, x3, x4, x5, x6, x7
_cln_inv_L1_dcache:

     // set for L1
    msr   csselr_el1, xzr
    isb
     // read the cssidr_el1
    mrs   x1, ccsidr_el1
    mov   x2, xzr
    mov   x3, xzr
    bfxil x2, x1, #3, #10
    bfxil x3, x1, #13, #15

     // x2 = ways-1
     // x3 = sets-1

    clz  w4, w2

     // x2 = ways-1
     // x3 = sets-1
     // x4 = bit position of way # (left-shift amount)

     // extract line-len field
    and  x1, x1, #7
     // generate L=log2(linelength)
    add  x1, x1, #4

     // x1 = L, bit position of set # (left-shift amount)
     // x2 = ways-1
     // x3 = sets-1
     // x4 = bit position of way # (left-shift amount)

3:   // set-loop
    mov  x5, x2

2:   // way-loop
     // construct the way/set input to the cache op
    lsl  x6, x3, x1
    lsl  x7, x5, x4
    orr  x6, x6, x7
    cbnz x0, 4f
    dc   csw, x6
    b    1f
4:
    dc   cisw, x6
1:
    subs x5, x5, #1
    b.ge 2b

    subs x3, x3, #1
    b.ge 3b

    isb
    ret

//-----------------------------------------------------------------------------

 // this function cleans and optionally invalidates an l3 cache with
 // the following properties:
 //  64 byte line size
 //  16-way set associative
 //  1 MB in size
 // in:  x0 = 0, clean only
 //      x0 = 1, clean and invalidate
 // uses x0, x1, x2, x3, x4, x5
_cln_inv_L3_dcache:
    mov  x1, #L3_MAX_SET

     // put the cache level into x5, left-shifted by 1
    mov x5, #L3_LEVEL
    lsl x5, x5, #1

     // x1 = set #
     // x4 = way #
     // x5 = cache level, left-shifted by 1

1:   // set loop
     // put the cache level into x2
    mov x2, x5

    // put the set # into x2
    orr   x2, x2, x1, lsl #L3_SET_SHIFT

    mov   x4, #L3_MAX_WAY
    
2:   // way loop
     // put all the parameters for cache maintenance into x3
    orr   x3, x2, x4, lsl #L3_WAY_SHIFT
    cbz   x0, 3f

     // perform a clean and invalidate
    dc    cisw, x3
    b     4f
3:
     // clean only
    dc    csw, x3
4:
     // decrement way
    subs  x4, x4, #1
    b.ge  2b

     // decrement set
    subs  x1, x1, #1
    b.ge  1b

    dsb   sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function sets the TCR_EL3 register
 // in:  none
 // out: none
 // uses x0, x1, x2
_set_tcr:

     // read current tcr    
    mrs   x2, tcr_el3
    mov   x0, x2

     // clear the irgn bits
    ldr   x1, =TCR_EL3_IRGN_MASK
    bic   x0, x0, x1
     // clear the orgn bits
    ldr   x1, =TCR_EL3_ORGN_MASK
    bic   x0, x0, x1

     // insert inner cacheable
    ldr   x1, =TCR_EL3_IRGN_CACHEABLE
    orr   x0, x0, x1
     // insert outer cacheable
    ldr   x1, =TCR_EL3_ORGN_CACHEABLE
    orr   x0, x0, x1

     // write back the result if different
    cmp   x0, x2
    b.eq  1f
    msr   tcr_el3, x0
    isb
1:
    ret

//-----------------------------------------------------------------------------

 // this function sets the spsr value for a core exiting
 // from EL3 - if the hw supports EL2, then the core is
 // delivered to EL2, else it is delivered to EL1
 // in:  none
 // out: x0 = #CORE_EL2, if core going to EL2, else
 //      x0 = #CORE_EL1, if core going to EL1
 // uses x0, x1, x2
_set_spsr_4_exit:

     // determine if we have hw support for EL2
    ldr  x1, =ID_AA64PFR0_MASK_EL2
    mrs  x0, id_aa64pfr0_el1
    and  x0, x0, x1

     // x0 = hw support for EL2 (0 means no support)
    cbz  x0, 3f

     // prepare for exit to EL2
    mov  x2, #CORE_EL2

     // aarch32 or aarch64?
    mov  x0, #POLICY_EL2_WIDTH
    cbz  x0, 1f

     // EL2 @ aarch64
    mov  x1, #SPSR_FOR_EL2H
    b    5f

1:   // EL2 @ aarch32
    mov  x1, #SPSR32_EL2_LE
    mov  x0, #POLICY_EL2_EE
    cbz  x0, 5f
    mov  x1, #SPSR32_EL2_BE
    b    5f

3:   // prepare for exit to EL1
    mov  x2, #CORE_EL1

     // aarch32 or aarch64?
    mov  x0, #POLICY_EL1_WIDTH
    cbz  x0, 4f

     // EL1 @ aarch64
    mov  x1, #SPSR_FOR_EL1H
    b    5f

4:   // EL1 @ aarch32
    mov  x1, #SPSR32_EL1_LE
    mov  x0, #POLICY_EL1_EE
    cbz  x0, 5f
    mov  x1, #SPSR32_EL1_BE

5:
     // set SPSR_EL3
    msr  spsr_el3, x1
    mov  x0, x2
    isb
    ret

//-----------------------------------------------------------------------------

 // this function sets the spsr value for a core exiting
 // from EL3 - if the hw supports EL2, then the core is
 // delivered to EL2, else it is delivered to EL1
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4, x5
_set_spsr_4_startup:
    mov   x5, x30

     // x0 = core mask lsb

     // get saved spsr
    mov  x1, #SPSR_EL3_DATA
    bl   _getCoreData
    mov  x4, x0

     // x4 = saved spsr

    mov   x0, x4
    bl    _get_exit_mode
    cmp   x0, #AMODE_AARCH64_EL2
    b.eq  4f
    cmp   x0, #AMODE_AARCH32_EL1
    b.eq  1f
    cmp   x0, #AMODE_AARCH64_EL1
    b.eq  3f

     // Aarch32 @ EL2
    mov   x0, #SPSR32_EL2_LE
    b     2f

1:   // Aarch32 @ EL1
    mov   x0, #SPSR32_EL1_LE

2:   // get endianness @ Aarch32
    tst   x4, #SPSR32_E_MASK
    b.eq  5f

     // BE
    orr   x0, x0, #SPSR32_E_BE
    b     5f

3:   // Aarch64 @ EL1
    mov   x0, #SPSR_FOR_EL1H
    b     5f

4:   // Aarch64 @ EL2
    mov   x0, #SPSR_FOR_EL2H

5:  // set SPSR_EL3
    msr  spsr_el3, x0

    isb
    mov  x30, x5
    ret

//-----------------------------------------------------------------------------

 // this function determines if there is hw support for EL2
 // in:  none
 // out: x0 == 0, no EL2 support in hw
 //      x0 == 0x100, the hw supports EL2 @ AArch64
 //      x0 == 0x200, the hw supports EL2 @ AArch64 and AArch32
 // uses x0, x1
_is_EL2_supported:

     // determine if we have hw support for EL2
    ldr  x1, =ID_AA64PFR0_MASK_EL2
    mrs  x0, id_aa64pfr0_el1
    and  x0, x0, x1

    ret

//-----------------------------------------------------------------------------

 // this function determines the exit mode of the core exiting EL3
 //      possible choices are:
 //         Aarch64 @ EL2
 //         Aarch64 @ EL1
 //         Aarch32 @ EL2
 //         Aarch32 @ EL1
 // in:  x0 = spsr of caller
 // out: x0 == 0, Aarch64 @ EL2
 //         == 1, Aarch64 @ EL1
 //         == 2, Aarch32 @ EL2
 //         == 3, Aarch32 @ EL1
 // uses x0, x1, x2
_get_exit_mode:

     // x0 = spsr of caller

     // get the EL2 support
    ldr  x1, =ID_AA64PFR0_MASK_EL2
    mrs  x2, id_aa64pfr0_el1
    and  x2, x2, x1

     // x0 = spsr of caller
     // x2 = EL2 support (0=none, 0x100=Aarch64, 0x200=Aarch64 and Aarch32)

    tst   x0, #SPSR_EL3_M4
    b.eq  1f

     // Aarch32
    mov   x0, #AMODE_AARCH_32
    cmp   x2, #ID_AA64PFR0_EL2_64OR32
    b.eq  4f

3:   // EL1
    orr   x0, x0, #AMODE_EL_1
    b     4f

1:   // Aarch64
    mov   x0, #AMODE_AARCH_64
    cbz   x2, 3b

4:
    ret

//-----------------------------------------------------------------------------

 // in:   none
 // uses: x0, x1, x2
_init_core_EL3:
    mov  x2, x30

     // initialize SCTLR_EL3
     // M,   bit [0]
     // A,   bit [1]
     // C,   bit [2]
     // SA,  bit [3]
     // I,   bit [12]
     // WXN, bit [19]
     // EE,  bit [25]
    mrs  x1, SCTLR_EL3
     // make sure icache is enabled
    orr  x0, x1, #SCTLR_I_MASK
     // make sure SA is enabled
    orr  x0, x0, #SCTLR_SA_MASK
    cmp  x0, x1
    b.eq 1f
     // writeback if we changed the value
    msr  SCTLR_EL3, x0
1:

     // initialize CPUECTLR
    mrs  x1, CPUECTLR_EL1
     // SMP, bit [6] = 1
    orr  x1, x1, #CPUECTLR_SMPEN_EN
    msr CPUECTLR_EL1, x1

     // initialize CPTR_EL3
    msr  CPTR_EL3, xzr

     // determine if hw supports el2
    bl   _is_EL2_supported

     // x0 = EL2 support (0=none)

     // initialize SCR_EL3
    mov  x1, #SCR_EL3_4_EL1_AARCH32
    cbz  x0, 2f
     // set HCE if el2 supported in hw
    orr  x1, x1, #SCR_EL3_HCE_EN
2:
    ldr  x0, =POLICY_EL2_WIDTH
    cbz  x0, 3f
    orr  x1, x1, #SCR_RW_AARCH64
3:
    ldr  x0, =POLICY_SIF_NS
    cbz  x0, 4f
    orr  x1, x1, #SCR_EL3_SIF_DIS
4:
    ldr  x0, =POLICY_FIQ_EL3
    cbz  x0, 5f
    orr  x1, x1, #SCR_EL3_FIQ_EN
5:
    msr  SCR_EL3, x1

     // initialize the value of ESR_EL3 to 0x0
    msr ESR_EL3, xzr

     // set the counter frequency
    ldr  x0, =COUNTER_FRQ_EL0
    msr  cntfrq_el0, x0

     // synchronize the system register writes
    isb

     // some of these bits are potentially cached in the tlb
    tlbi alle3
    dsb  sy
    isb
 
    mov  x30, x2
    ret

 //----------------------------------------------------------------------------

 // this function provides boot core initialization at the EL2 level
 // in:  none
 // out: none
 // uses x0, x1
_init_core_EL2:

     // initialize hcr_el2
    mov  x1, xzr
    mov  x0, #POLICY_EL1_WIDTH
    cbz  x0, 1f
    orr  x1, x1, #HCR_EL2_RW_AARCH64
1:
    msr  hcr_el2, x1

     // initialize sctlr_el2

    ldr  x1, =SCTLR_EL2_RES1
    mov  x0, #POLICY_EL2_EE
    cbz  x0, 2f
    orr  x1, x1, #SCTLR_EE_BE
2:
    msr  sctlr_el2, x1

     // initialize cptr_el2
    ldr  x0, =CPTR_EL2_RES1_MASK
    msr  cptr_el2, x0

     // synchronize the system register writes
    isb

     // some of these bits are potentially cached in the tlb
    tlbi alle3
    dsb  sy
    isb    
    ret

 //----------------------------------------------------------------------------

 // this function provides boot core initialization at the EL1 level
 // in:  none
 // out: none
 // uses x0, x1
_init_core_EL1:

     // initialize sctlr_el1
    ldr   x1, =SCTLR_EL1_RES1
    mov   x0, #POLICY_EL1_EE
    cbz   x0, 2f
    orr   x1, x1, #SCTLR_EE_BE
2:
    msr  sctlr_el1, x1

     // synchronize the system register writes
    isb

     // some of these bits are potentially cached in the tlb
    tlbi alle3
    dsb  sy
    isb    
    ret

 //----------------------------------------------------------------------------

 // this function initializes the EL3 registers on a secondary core out of
 // reset, or any core out of sleep/suspend
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3
_init_secondary_EL3:
    mov  x3, x30

     // x0 = core mask lsb

     // initialize SCTLR_EL3
    ldr   x1, =SCTLR_EL3_RES1
     // make sure icache is enabled
    orr  x1, x1, #SCTLR_I_MASK
     // make sure SA is enabled
    orr  x1, x1, #SCTLR_SA_MASK
    msr  SCTLR_EL3, x1

     // initialize CPUECTLR
    mrs  x1, CPUECTLR_EL1
     // SMP, bit [6] = 1
    orr  x1, x1, #CPUECTLR_SMPEN_EN
    msr CPUECTLR_EL1, x1

     // initialize CPTR_EL3
    msr  CPTR_EL3, xzr

     // x0 = core mask lsb

     // get saved scr_el3
    mov  x1, #SCR_EL3_DATA
    bl   _getCoreData

     // x0 = saved scr_el3

     // initialize SCR_EL3
    msr  SCR_EL3, x0

     // initialize the value of ESR_EL3 to 0x0
    msr ESR_EL3, xzr

     // set the counter frequency
    ldr  x0, =COUNTER_FRQ_EL0
    msr  cntfrq_el0, x0

     // synchronize the system register writes
    isb

     // some of these bits are potentially cached in the tlb
    tlbi alle3
    dsb  sy
    isb
 
    mov  x30, x3
    ret

 //----------------------------------------------------------------------------

 // this function provides some basic core initialization on a secondary
 // core, at the EL2 level
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3, x4, x5
_init_secondary_EL2:
    mov  x5, x30

    mov  x4, x0

     // x4 = core mask lsb

     // load hcr_el2 from saved hcr_el2
    mov  x0, x4
    mov  x1, #HCR_EL2_DATA
    bl   _getCoreData

     // x0 = saved hcr_el2
    msr  hcr_el2, x0

     // get saved sctlr_el2
    mov  x0, x4
    mov  x1, #SCTLR_DATA
    bl   _getCoreData

     // x0 = saved sctlr_el2

     // extract the endianness bit
    and  x0, x0, #SCTLR_EE_MASK

     // insert the RES1 bits
    ldr  x1, =SCTLR_EL2_RES1
    orr  x0, x0, x1

     // initialize sctlr_el2
    msr  sctlr_el2, x0

     // initialize cptr_el2
    ldr  x0, =CPTR_EL2_RES1_MASK
    msr  cptr_el2, x0

     // synchronize the system register writes
    isb

     // some of these bits are potentially cached in the tlb
    tlbi alle3
    dsb  sy
    isb 

    mov  x30, x5   
    ret

 //----------------------------------------------------------------------------

 // this function provides some basic core initialization on a secondary
 // core, at the EL1 level
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, x3

_init_secondary_EL1:
    mov  x3, x30

     // x0 = core mask lsb

     // get saved sctlr_el1
    mov  x1, #SCTLR_DATA
    bl   _getCoreData

     // x0 = saved sctlr_el1

     // extract the endianness bit
    and  x0, x0, #SCTLR_EE_MASK

     // insert the RES1 bits
    ldr  x1, =SCTLR_EL1_RES1
    orr  x0, x0, x1

     // initialize sctlr_el1
    msr  sctlr_el1, x0

     // synchronize the system register writes
    isb

     // some of these bits are potentially cached in the tlb
    tlbi alle3
    dsb  sy
    isb 

    mov  x30, x3   
    ret

 //----------------------------------------------------------------------------

 // this function sets the value in VBAR_EL3
 // out: none
 // uses x0
_set_EL3_vectors: 

    adr  x0, _el3_vector_base
    msr  VBAR_EL3, x0
    ret

 //----------------------------------------------------------------------------

 // this function returns the execution level of the caller by
 // reading & decoding spsr_el3
 // in:  x0 = spsr_el3 of caller
 // out: x0 = SPSR_EL1, if caller is EL1
 //      x0 = SPSR_EL2, if caller is EL2
 //      x0 = 0, if caller not EL1 or EL2
 // uses x0, x1
_getCallerEL:
     // x0 = spsr_el3    

     // test spsr_el3 [4] to determine if caller is Aarch64 or Aarch32
    tst   x0, #SPSR_EL3_M4
    b.eq  1f

     // if we are here then the caller is aarch32

     // see if caller is hypervisor
    and   x1, x0, #SPSR32_MODE_MASK
    cmp   x1, SPSR32_MODE_HYP
    b.eq  2f
    cmp   x1, SPSR32_MODE_SUPV
    b.eq  3f

     // must be called from hyp or supv mode
    mov   x0, #0
    b     4f

1:   // caller_is_aarch64
    and   x1, x0, #SPSR_EL_MASK
    cmp   x1, #SPSR_EL2
    b.eq  2f

    cmp   x1, #SPSR_EL1
    b.eq  3f

     // must be called from hyp or supv mode
    mov   x0, #0
    b     4f

2:   // hypervisor mode handled here
    mov   x0, #SPSR_EL2
    b     4f

3:   // supervisor mode handled here
    mov   x0, #SPSR_EL1

4:
    ret

 //----------------------------------------------------------------------------

 // this function disables the load-store prefetch of the calling core
 // Note: this function is for A72 cores ONLY
 // in:  none
 // out: none
 // uses x0
_disable_ldstr_pfetch_A72:

     // 
    mrs   x0, CPUACTLR_EL1
    tst   x0, #CPUACTLR_DIS_LS_HW_PRE
    b.eq  1f
    b     2f

.align 6
1:   // disable prefetch for this A72 core
    dsb   sy
    isb
    orr   x0, x0, #CPUACTLR_DIS_LS_HW_PRE
    msr   CPUACTLR_EL1, x0
    isb

2:
    ret

 //----------------------------------------------------------------------------

 // this function disables the load-store prefetch of the calling core
 // Note: this function is for A53 cores ONLY
 // in:  none
 // out: none
 // uses x0
_disable_ldstr_pfetch_A53:

     // 
    mrs   x0, CPUACTLR_EL1
    tst   x0, #CPUACTLR_L1PCTL_MASK
    b.ne  1f
    b     2f

.align 6
1:   // disable L1 prefetch for this A53 core
    dsb   sy
    isb
    bic   x0, x0, #CPUACTLR_L1PCTL_MASK
    msr   CPUACTLR_EL1, x0
    isb

2:
    ret

 //----------------------------------------------------------------------------

 // this function enables the load-store prefetch of the calling core
 // Note: this function is for A72 cores ONLY
 // in:  none
 // out: none
 // uses x0
_enable_ldstr_pfetch_A72:

    mrs   x0, CPUACTLR_EL1
    tst   x0, #CPUACTLR_DIS_LS_HW_PRE
    b.ne  1f
    b     2f

.align 6
1:   // enable prefetch for this A72 core
    dsb   sy
    isb
    bic   x0, x0, #CPUACTLR_DIS_LS_HW_PRE
    msr   CPUACTLR_EL1, x0
    isb

2:
    ret

 //----------------------------------------------------------------------------

 // this function enables the load-store prefetch of the calling core
 // Note: this function is for A53 cores ONLY
 // in:  none
 // out: none
 // uses x0, x1
_enable_ldstr_pfetch_A53:

.align 6
    mrs   x0, CPUACTLR_EL1

     // enable L1 prefetch for this A53 core
    dsb   sy
    isb
    bic   x0, x0, #CPUACTLR_L1PCTL_MASK
    mov   x1, #CPUACTLR_L1PCTL_EN_5
    orr   x0, x0, x1
    msr   CPUACTLR_EL1, x0
    isb

    ret

 //----------------------------------------------------------------------------

 // this function relocates the rela_dyn sections
 // in:  x0 = Base addr where PPA has been loaded
 // out: None
 // uses x0, x1, x2, x3, x4, x5
_relocate_rela:
     // Find relative load address
    ldr  x1,=__PPA_PROG_START__
    subs  x5, x0, x1
    b.eq  1f

     // Fix .rela.dyn relocations
    ldr  x0, =__REL_DYN_START__
    add  x0, x0, x5
    ldr  x1, =__REL_DYN_END__
    add  x1, x1, x5

relfix:
     // x2,x3 --> SRC, info
    ldp   x2, x3, [x0], #16
     // x4 <- addend
    ldr   x4, [x0], #8
    and   x3, x3, #0xffffffff
    cmp   x3, #1027
    bne   relnext

     // rela fix: store addend plus offset at dest location
    add   x2, x2, x5
    add   x4, x4, x5
    str   x4, [x2]
relnext:
    cmp   x0, x1
    b.lo  relfix
1:
    ret

 //----------------------------------------------------------------------------

 // this function zeroizes the bss section
 // in: x0 - Base address where PPA has been loaded
 // out: None
 // uses x1, x2, x5
_zeroize_bss:
    ldr  x1,=__PPA_PROG_START__
    sub  x5, x0, x1

     // Load start and end of bss
    ldr   x1, =__BSS_START__
    ldr   x2, =__BSS_END__
    add   x1, x1, x5
    add   x2, x2, x5
    cmp   x1, x2
    b.eq  2f
1:
    str   xzr, [x1], #8
    cmp   x1, x2
    b.lo  1b
2:
    ret

 //----------------------------------------------------------------------------

#if (DEBUG_BUILD)

 // this function allows EL2/EL1 write access to L2ACTLR & CPUACTLR (A53-only)
 //   for the purpose of injecting L1/L2 memory errors for EDAC drvr testing
 // in:  none
 // out: none
 // uses x0, x1
_allow_L1L2err_inject:

#if (CORE == 53)
    mrs x0, actlr_el3
    mov x1, #(ACTLR_EL3_CPUACTLR | ACTLR_EL3_L2ACTLR)
    orr x0, x0, x1
    msr actlr_el3, x0

    mrs x0, actlr_el2
    mov x1, #(ACTLR_EL2_CPUACTLR | ACTLR_EL2_L2ACTLR)
    orr x0, x0, x1
    msr actlr_el2, x0

    isb
#endif

    ret
#endif

 //----------------------------------------------------------------------------

 // this function allows EL2/EL1 write access to L2ECTLR 
 //   for the purpose of clearing L2 asynch errors
 // in:  none
 // out: none
 // uses x0, x1
_allow_L1L2err_clear:

#if ((CORE == 53) || (CORE == 57) || (CORE == 72))
    mrs x0, actlr_el3
    mov x1, #ACTLR_EL3_L2ECTLR
    orr x0, x0, x1
    msr actlr_el3, x0

    mrs x0, actlr_el2
    mov x1, #ACTLR_EL2_L2ECTLR
    orr x0, x0, x1
    msr actlr_el2, x0

    isb
#endif

    ret

 //----------------------------------------------------------------------------
 //----------------------------------------------------------------------------
 //----------------------------------------------------------------------------
 //----------------------------------------------------------------------------
 //----------------------------------------------------------------------------

