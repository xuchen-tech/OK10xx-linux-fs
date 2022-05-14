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
// Authors:
//  Sumit Garg <sumit.garg@nxp.com>
//  Pankaj Gupta <pankaj.gupta@nxp.com>
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "spd.h"
#include "sp_ep_info.h"
#include "context_arch64.h"
#include "context_mgmt.h"

 // Global data structure for core data
static core_data_t core_data_array[CPU_MAX_COUNT];
core_context_t ns_core_context[CPU_MAX_COUNT];

 // @brief Initializes Secure-EL1 Payload Dispatcher
 // @param [in] addr	SP start address
 // @retval 0 on success
 // @retval -1 on error
int spd_init(unsigned long long addr)
{
    int num_secure_os = 1;
    int index = 0;
    entry_point_info_t *sp_ep_info_list;

    uint32_t linear_id;
    uint64_t core_data_ptr;
    uint64_t scr_el3, sctlr_el3, sctlr_el1;

     // Save EL3 SCR and EL1/EL3 SCTLR reg contents
     // set by non-spd ppa code.
    scr_el3 = read_scr_el3();
    sctlr_el3 = read_sctlr_el3();
    sctlr_el1 = read_sctlr_el1();

     // Get SP info list
    sp_ep_info_list = get_sp_ep_info_list();

     // Update SP entry point info in the list
    sp_ep_info_list[0].state = SECURE;
    sp_ep_info_list[0].pc = addr;

     // Set pointer to core data for context mgmt
    linear_id = get_curr_core_pos();
    core_data_ptr = (uint64_t)&core_data_array[linear_id];
    write_tpidr_el3(core_data_ptr);

     // Setup and initialize SP
    for(; index < num_secure_os; index++) {
        sp_setup_n_init(index, &(sp_ep_info_list[index]));
    }

     // Initialize non-secure contex poiter
    linear_id = get_curr_core_pos();
    cm_set_context(&ns_core_context[linear_id], NON_SECURE);

     // Non-Secure El1/El2 Payload Entry Point info is NULL.
     // till spd module is not setting SCR/SCTLR register..
    cm_init_context(NULL, NON_SECURE);

     // Restore el1 secure context
    cm_el1_sysregs_context_restore(NON_SECURE);
    
     // Restore EL3 SCR and EL1/EL3 SCTLR reg contents
     // set by non-spd ppa code.
    write_scr_el3(scr_el3);
    write_sctlr_el3(sctlr_el3);
    write_sctlr_el1(sctlr_el1);

    return SPD_SUCCESS;
}
