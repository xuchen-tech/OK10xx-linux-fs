//-----------------------------------------------------------------------------
// 
// Copyright (c) 2015-2016, Freescale Semiconductor
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

//-----------------------------------------------------------------------------

.section .text, "ax"

//-----------------------------------------------------------------------------

#if (LSCH == 3)
#include "lsch3.h"
#else
#include "lsch2.h"
#endif

#include "psci.h"
#include "soc.mac"
#include "smc.h"

//-----------------------------------------------------------------------------

.global _secondary_core_init
.global _cpu_off_exit
.global _start_monitor_el3
.global _mon_core_restart

//-----------------------------------------------------------------------------

.align 16
_start_monitor_el3:
     // save the LR
    mov   x16, x30

#if (!SIMULATOR_BUILD) && (DEBUG_HALT)
debug_stop:
    b  debug_stop
#endif

     // relocate the rela_dyn sections
    adr  x0, _start_monitor_el3
    bl   _relocate_rela

     // clear the bss
    adr  x0, _start_monitor_el3
    bl   _zeroize_bss

     // invalidate the icache
    ic  iallu
    isb

     // invalidate tlb
    tlbi  alle3
    dsb   sy
    isb

     // setup the EL3 vectors
    bl   _set_EL3_vectors

     // soc-specific core init
    bl   _soc_init_percpu

     // perform EL3 init on the core
    bl   _init_core_EL3

     // determine if hw supports el2
    bl   _is_EL2_supported

     // x0 = EL2 support (0=none)
    cbz  x0, 1f
     // perform basic EL2 init on the core
    bl   _init_core_EL2
    b    2f
1:
     // perform basic EL1 init on the core
    bl   _init_core_EL1
2:
     // setup the interconnect
    bl   _init_interconnect
 
     // clean/invalidate the dcache
    mov x0, #0
    bl  _cln_inv_all_dcache
#if (L3_CACHE)
    mov x0, #1
    bl  _manual_L3_flush
#endif

#if ((DDR_INIT) && (DATA_LOC == DATA_IN_DDR))

     // setup a temporary stack in OCRAM for the bootcore so we can run some C code
    ldr  x2, =INITIAL_BC_STACK_ADDR
    mov  sp, x2

     // store the caller's LR on the temp stack
    str  x16, [sp, #-16]!

     // determine address of loadable
    bl  _get_load_addr
     // the load address will be passed as the second parameter to _ppa_main() below
    mov  x1, x0
    adr  x0, _start_monitor_el3
    bl   _ppa_main

     // initialize the smc global data
    bl   _init_global_data
     // initialize the psci data structures
    bl   _initialize_psci

     // start initializing the soc
    bl   _soc_init_start

     // configure GIC
    bl   _gic_init_common
    bl   _get_current_mask
    mov  x8, x0
    bl   _gic_init_percpu

     // apply any cpu-specific errata workarounds
    mov  x0, x8
    bl   _apply_cpu_errata

     // x8 = core mask

     // get the saved LR from the temp stack
    ldr  x16, [sp], #16

     // setup the permanent stack
    mov  x0, x8
    bl   _init_stack_percpu

     // store the caller's LR on the stack
    str  x16, [sp, #-16]!

#else

     // initialize the smc global data
    bl   _init_global_data
     // initialize the psci data structures
    bl   _initialize_psci

     // start initializing the soc
    bl   _soc_init_start

     // configure GIC
    bl   _gic_init_common
    bl   _get_current_mask
    mov  x8, x0
    bl   _gic_init_percpu

     // apply any cpu-specific errata workarounds
    mov  x0, x8
    bl   _apply_cpu_errata

     // x8 = core mask

     // setup the stack
    mov  x0, x8
    bl   _init_stack_percpu

     // store the caller's LR on the stack
    str  x16, [sp, #-16]!

     // determine address of loadable
    bl  _get_load_addr
     // the load address will be passed as the second parameter to _ppa_main() below
    mov  x1, x0
    adr  x0, _start_monitor_el3
    bl   _ppa_main

#endif

     // exit the monitor
    b    monitor_exit_EL3

//-----------------------------------------------------------------------------

 // this function performs the EL3 setup on the secondary cores
_secondary_core_init:
     // save the LR
    mov   x12, x30

     // setup the EL3 vectors
    bl  _set_EL3_vectors

     // get this cores bit mask
    bl   _get_current_mask
    mov  x8, x0

     // x0 = core mask lsb
     // x8 = core mask lsb

     // perform EL3 init on secondary core
    bl   _init_secondary_EL3

     // get saved spsr
    mov  x0, x8
    mov  x1, #SPSR_EL3_DATA
    bl   _getCoreData
    mov  x4, x0

     // x4 = saved spsr

    mov   x0, x4
    bl    _get_exit_mode
    tst   x0, #AMODE_EL_MASK
    b.ne  1f

     // perform EL2 init on secondary core
    mov  x0, x8
    bl   _init_secondary_EL2
    b    2f
1:
     // perform EL1 init on secondary core
    mov  x0, x8
    bl   _init_secondary_EL1
2:
     // set CNTVOFF to 0
    msr  cntvoff_el2, xzr

     // apply any cpu-specific errata workarounds
    mov  x0, x8
    bl   _apply_cpu_errata

     // soc-specific init on secondary core
    bl   _soc_init_percpu

     // since this is a secondary core function, we
     // must be finished boot - trigger the exit
     // boot services call
    bl   exit_boot_svcs

     // perform any secondary-core platform security setup here
     //   configure secure mmu
     //   configure gic
    mov  x0, x8
    bl   _gic_init_percpu

     // setup runtime stack
    mov  x0, x8
    bl   _init_stack_percpu

#if (CNFG_SPD)
     // Intialize spd on secondary cores
    mvn  x0, xzr
    bl   spd_init
#endif

     // exit the EL3 area
    b    _secondary_exit

//-----------------------------------------------------------------------------

 // this function branches to the start address, changing
 // execution level from EL3 to EL2 in the process
 // in:   none
 // uses: x0, x1, x2
monitor_exit_EL3:

     // make sure that the soc initialization tasks completed
    bl   _soc_init_finish

#if (PSCI_TEST)
     // set the exit address to be our test code entry point
    adr  x0, _test_psci
    bl   _soc_set_start_addr
#endif

     // determine start address of bootloader
    bl  _soc_get_start_addr

     // x0 = start address
    
     // set ELR_EL3
    msr  elr_el3, x0

     // set the spsr value for exit
    bl   _set_spsr_4_exit

     // flush dcache
    mov x0, #1
    bl  _cln_inv_all_dcache
#if (L3_CACHE)
    mov x0, #0
    bl  _manual_L3_flush
#endif

     // invalidate the icache
    ic  iallu
    isb

     // invalidate tlb
    tlbi alle3
    dsb  sy
    isb

     // disable dcache and mmu
    mrs  x2, sctlr_el3
    bic  x2, x2, #SCTLR_M_MASK
    bic  x2, x2, #SCTLR_C_MASK
    msr  sctlr_el3, x2
    isb

     // initialize the Platform Security Policy here
    bl   _set_platform_security  

     // restore the LR
    ldr  x30, [sp], #16

     // cleanup
    mov  x0,   #0
    mov  x1,   #0
    mov  x2,   #0
    mov  x3,   #0
    mov  x4,   #0
    mov  x5,   #0
    mov  x6,   #0
    mov  x7,   #0
    mov  x8,   #0
    mov  x9,   #0
    mov  x10,  #0
    mov  x11,  #0
    mov  x12,  #0
    mov  x13,  #0
    mov  x14,  #0
    mov  x15,  #0
    mov  x16,  #0

     // switch to the specified exception level
    clrex
    isb
    eret

//-----------------------------------------------------------------------------

 // this is the exit from the monitor for secondary cores coming out of reset
_secondary_exit:

     // get this cores bit mask
    bl   _get_current_mask
    mov  x6, x0

     // x0 = core mask (lsb)
     // x6 = core mask (lsb)

     // get the start addr
    mov  x1, #START_ADDR_DATA
    bl   _getCoreData

     // x0 = start address
     // x6 = core mask (lsb)

     // load start addr in ELR_EL3
    msr  elr_el3, x0

     // set the core state to released
    mov  x0, x6
    mov  x1, #CORE_STATE_DATA
    mov  x2, #CORE_RELEASED
    bl   _setCoreData

     // x6 = core mask (lsb)

     // setup the spsr
    mov  x0, x6
    bl   _set_spsr_4_startup

     // x6 = core mask lsb

     // get the context id into x0
    mov  x0, x6
    mov  x1, #CNTXT_ID_DATA
    bl   _getCoreData

     // x0 = context id

     // invalidate the icache
    ic  iallu
    isb

     // invalidate tlb
    tlbi  alle3
    dsb   sy
    isb

     // x0 = context id

     // exit EL3
    mov  x1,  #0
    mov  x2,  #0
    mov  x3,  #0
    mov  x4,  #0
    mov  x5,  #0
    mov  x6,  #0

     // clear the exclusive monitor and return from exception
    clrex
    isb
    eret

//-----------------------------------------------------------------------------

 // this is the exit from the monitor for cores coming out of sleep/suspend
_mon_core_restart:

     // get this cores bit mask
    bl   _get_current_mask
    mov  x8, x0

     // x0 = core mask (lsb)
     // x8 = core mask (lsb)

     // perform EL3 init on secondary core
    bl   _init_secondary_EL3

     // get spsr_el3
    mrs   x0, spsr_el3
    bl    _get_exit_mode
    tst   x0, #AMODE_EL_MASK
    b.ne  1f

     // perform EL2 init on core
    mov  x0, x8
    bl   _init_secondary_EL2
    b    2f
1:
     // perform EL1 init on core
    mov  x0, x8
    bl   _init_secondary_EL1
2:
     // get the start addr
    mov  x0, x8
    mov  x1, #START_ADDR_DATA
    bl   _getCoreData

     // x0 = start address
     // x8 = core mask (lsb)

     // load start addr in ELR_EL3
    msr  elr_el3, x0

     // set the core state to released
    mov  x0, x8
    mov  x1, #CORE_STATE_DATA
    mov  x2, #CORE_RELEASED
    bl   _setCoreData

     // x8 = core mask (lsb)

     // setup the spsr
    mov  x0, x8
    bl   _set_spsr_4_startup

     // setup the stack
    mov  x0, x8
    bl   _init_stack_percpu

     // x8 = core mask lsb

     // get the saved link reg
    mov  x0, x8
    mov  x1, #LINK_REG_DATA
    bl   _getCoreData
    mov  x12, x0

     // x8  = core mask (lsb)
     // x12 = saved link reg

     // get the context id into x0
    mov  x0, x8
    mov  x1, #CNTXT_ID_DATA
    bl   _getCoreData

     // x0 = context id

     // invalidate the icache
    ic  iallu
    isb

     // invalidate tlb
    tlbi  alle3
    dsb   sy
    isb

     // x0 = context id

     // exit EL3
    mov  x1,  #0
    mov  x2,  #0
    mov  x3,  #0
    mov  x4,  #0
    mov  x5,  #0
    mov  x6,  #0
    mov  x7,  #0
    mov  x8,  #0

     // restore the LR
    mov  x30, x12

     // clear the exclusive monitor and return from exception
    clrex
    isb
    eret

//-----------------------------------------------------------------------------

 // this function makes any needed changes to the configuration when the end of
 // boot services is triggered.
 // in:  none
 // out: none
 // uses x0, x1, x11
exit_boot_svcs:
    mov  x11, x30

     // read the boot services flag
    mov  x0, #BOOT_SVCS_OSET
    bl   _get_global_data

     // if we have already executed this service, don't execute again
    cbnz x0, 1f

     // set the flag
    mov  x0, #BOOT_SVCS_OSET
    mov  x1, #1
    bl   _set_global_data

     // put any global changes needed at end of boot services here
    
     // call into the soc-specific code for any soc configuration changes needed
    bl  _soc_exit_boot_svcs

1:
    mov  x30, x11
    ret

//-----------------------------------------------------------------------------

