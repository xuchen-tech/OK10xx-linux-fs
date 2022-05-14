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

#include "sp_ep_info.h"
#include "context_mgmt.h"
#include "lib.h"
#include "aarch64.h"
#include "sec_hw_specific.h"

 // Global data variables used for SP initialization
static int oen_list[MAX_ALLOWED_OEN];
static int count_trstd_loaded = 0;
static entry_point_info_t sp_ep_info_list[MAX_ALLOWED_OEN];
static sp_ctx_t sp_ctx_per_core[CPU_MAX_COUNT];
static sp_vectors_t *trstd_os_vectors_list[MAX_ALLOWED_OEN];
extern core_context_t ns_core_context[CPU_MAX_COUNT];

 // @brief Get ptr to SP entrypoint info list
 // @retval Ptr to SP entrypoint info list
entry_point_info_t* get_sp_ep_info_list(void)
{
    return sp_ep_info_list;
}

 // @brief Setup and initialize Secure-EL1 payload
 // @param [in] sp_index : SP index
 //        [in] sp_ep_info : SP info structure ptr
 // @retval ret : Result returned from SP
int32_t sp_setup_n_init(int sp_index, entry_point_info_t *sp_ep_info)
{
    uint32_t linear_id;
    uint32_t ep_attr;
    int32_t ret = 0;
    int oen_val_frm_smcfid = 62;
    sp_vectors_t *sp_vector;

    linear_id = get_curr_core_pos();
    sp_ctx_t *sp_crnt_core_ctx = &(sp_ctx_per_core[linear_id]);

     // Associate this context with the cpu specified
    sp_crnt_core_ctx->mpidr = read_mpidr_el1();
    sp_crnt_core_ctx->state = SECURE;

     // Set el1 secure context
    cm_set_context(&(sp_crnt_core_ctx->cpu_ctx), SECURE);

     // Assign entry addr for secondary cores from vector
     // table registered during primary core init.
    if (linear_id != 0) {
        sp_vector = trstd_os_vectors_list[oen_list[oen_val_frm_smcfid - START_OEN]];
        if (sp_vector == NULL)
            return ret;

        sp_ep_info->pc = (uint64_t) &sp_vector->cpu_on_entry;
    }

     // Configure el1 secure contexy
    ep_attr = SECURE | EP_ST_ENABLE;
    if (read_sctlr_el3() & SCTLR_EE_BIT) {
        ep_attr |= EP_EE_BIG;
    }
    sp_ep_info->attr = ep_attr;
    sp_ep_info->spsr = SPSR_64(MODE_EL1, MODE_SP_ELX,
                               DISABLE_ALL_EXCEPTIONS);
    memset(&sp_ep_info->args, 0, sizeof(sp_ep_info->args));

     // Initializes el1 secure context
    cm_init_context(sp_ep_info, SECURE);

     // Restore el1 secure context
    cm_el1_sysregs_context_restore(SECURE);
    cm_set_next_eret_ctx(SECURE);

     // Enter el1 secure context
    ret = secure_el1os_enter_sp(&sp_crnt_core_ctx->c_rt_ctx);

    return ret;
}

 // @brief SMC handler for Trusted OS calls
 // @param [in] smc_fid : SMC function identifier
 //        [in] Arg reg : x1, x2, x3, x4
 //        [in] cookie, handle, flags
 // @retval This function should never return
void smc_trstd_os_handler(uint32_t smc_fid,
                          uint64_t x1,
                          uint64_t x2,
                          uint64_t x3,
                          uint64_t x4,
                          void *handle,
                          uint64_t state)
{
    int oen_val_frm_smcfid = 62;
    uint32_t linear_id;
    uint32_t security_state = (uint32_t) state;
    sp_ctx_t *sp_crnt_core_ctx;
    sp_vectors_t *sp_vector;
    core_context_t *ctx;

     // Get core position
    linear_id = get_curr_core_pos();

     // Save el1 system registers
    cm_el1_sysregs_context_save(security_state);

    if (security_state == SECURE) {

        switch(smc_fid & TEESMC_TRSTD_OS_FUNC_MASK) {

         // Trusted-OS has finished initialising itself after a cold boot
        case TEESMC_TRSTD_OS_RETURN_ENTRY_DONE:
             // Trstd_os_vectors_list.
            oen_list[oen_val_frm_smcfid - START_OEN] = count_trstd_loaded;

             // Stash the OPTEE entry points information. This is done
             // only once on the primary cpu
            trstd_os_vectors_list[oen_list[oen_val_frm_smcfid - START_OEN]] = (sp_vectors_t *) x1;
            count_trstd_loaded++;

             // Exit from SP and restore EL3 runtime context
            sp_crnt_core_ctx = &(sp_ctx_per_core[linear_id]);
            secure_el1os_exit_sp(sp_crnt_core_ctx->c_rt_ctx, x1);

             // Control should never reach here.
            while(1);

         // Trusted-OS has finished initialising on secondary core
        case TEESMC_TRSTD_OS_RETURN_ON_DONE:
             // Exit from SP and restore EL3 runtime context
            sp_crnt_core_ctx = &(sp_ctx_per_core[linear_id]);
            secure_el1os_exit_sp(sp_crnt_core_ctx->c_rt_ctx, x1);

             // Control should never reach here.
            while(1);

         // Return from Trusted-OS call
        case TEESMC_TRSTD_OS_RETURN_CALL_DONE:
             // Get non-secure context
            ctx = cm_get_context(NON_SECURE);

             // Save return params
            write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X0, x1);
            write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X1, x2);
            write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X2, x3);
            write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X3, x4);

             // Restore el1 secure context
            cm_el1_sysregs_context_restore(NON_SECURE);
            cm_set_next_eret_ctx(NON_SECURE);

            break;
        default:
             // Control should never reach here.
            while(1);
        }
    } else {
         // Handle trusted OS smc calls from non-secure OS
        sp_vector = trstd_os_vectors_list[oen_list[oen_val_frm_smcfid - START_OEN]];

         // Check for sp_vector if NULL means trusted OS uninitialized
         // Return back to non-secure world with unimplemented in x0.
        if (sp_vector == NULL) {
            ctx = cm_get_context(NON_SECURE);
#define  SMC_UNIMPLEMENTED  -1
            write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X0, SMC_UNIMPLEMENTED);

            cm_set_next_eret_ctx(NON_SECURE);
            return;
        }

         // Get secure context
        ctx = cm_get_context(SECURE);

         // Check for fast or standard trusted OS call
        if ((smc_fid & TEESMC_TRSTD_OS_TYPE_MASK) == TEESMC_TRSTD_OS_TYPE_MASK) {
            write_ctx_reg(get_el3state_ctx(ctx), CPU_CTX_ELR_EL3,
                          (uint64_t) &sp_vector->fast_smc_entry);
        } else {
            write_ctx_reg(get_el3state_ctx(ctx), CPU_CTX_ELR_EL3,
                          (uint64_t) &sp_vector->yield_smc_entry);
        }

         // Save parameter list to be passed in secure context
        write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X0, smc_fid);
        write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X1, x1);
        write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X2, x2);
        write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X3, x3);
        write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X4, x4);
        write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X5,
                      read_ctx_reg(get_gpregs_ctx(handle), CPU_CTX_X5));
        write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X6,
                      read_ctx_reg(get_gpregs_ctx(handle), CPU_CTX_X6));
        write_ctx_reg(get_gpregs_ctx(ctx), CPU_CTX_X7,
                      read_ctx_reg(get_gpregs_ctx(handle), CPU_CTX_X7));

         // Restore el1 secure context
        cm_el1_sysregs_context_restore(SECURE);
        cm_set_next_eret_ctx(SECURE);
    }
}
