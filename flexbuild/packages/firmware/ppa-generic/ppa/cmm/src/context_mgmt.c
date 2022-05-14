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

#include "types.h"
#include "sp_ep_info.h"
#include "context_mgmt.h"
#include "context_arch64.h"
#include "lib.h"
#include "aarch64.h"

 // @brief Get context as per security state
 // @param [in] security_state : SECURE/NONSECURE
 // @retval Ptr to context
void *cm_get_context(uint32_t security_state)
{
    uint64_t v;
    v = read_tpidr_el3();
    return ((core_data_t *)v)->core_context[security_state];
}

 // @brief Get context as per security state
 // @param [in] context : Secure or nonsecure context
 //        [in] security_state : SECURE/NONSECURE
 // @retval void
void cm_set_context(void *context, uint32_t security_state)
{
    uint64_t v;
    v = read_tpidr_el3();
    ((core_data_t *)v)->core_context[security_state] = context;
}

 // @brief Initializes context as per security state
 // @param [in] cxt : Secure or nonsecure context
 //        [in] ep : Ptr to entry point info
 //        [in] security_state : SECURE/NONSECURE
 // @retval void
static void cm_init_context_common(core_context_t *ctx, const entry_point_info_t *ep,
                    uint32_t security_state)
{
    uint32_t scr_el3;
    el3_state_regs_t *state;
    gp_regs_t *gp_regs;
    unsigned long sctlr_elx;

     // Clear any residual register values from the context
    memset(ctx, 0, sizeof(*ctx));

    if (!ep) return;

     // Base the context SCR on the current value, adjust for entry point
     // specific requirements and set trap bits from the IMF
    scr_el3 = read_scr_el3();

     // Unset the below flags
    scr_el3 &= ~(SCR_NS_BIT | SCR_RW_BIT | SCR_FIQ_BIT | SCR_IRQ_BIT |
                 SCR_ST_BIT | SCR_HCE_BIT);

    if (security_state != SECURE) {
        scr_el3 |= SCR_NS_BIT;
    }

    if (GET_RW(ep->spsr) == MODE_RW_64) {
        scr_el3 |= SCR_RW_BIT;
    }

    if (EP_GET_ST(ep->attr)) {
        scr_el3 |= SCR_ST_BIT;
    }

     // Set up SCTLR_ELx for the target exception level:
     // EE bit is taken from the entrypoint attributes
     // M, C and I bits must be zero (as required by PSCI specification)
     //
     // The target exception level is based on the spsr mode requested.
     // If execution is requested to EL2 or hyp mode, HVC is enabled
     // via SCR_EL3.HCE.
     //
     // Always compute the SCTLR_EL1 value and save in the cpu_context
     // - the EL2 registers are set up by cm_preapre_ns_entry() as they
     // are not part of the stored cpu_context
    sctlr_elx = EP_GET_EE(ep->attr) ? SCTLR_EE_BIT : 0;
    if (GET_RW(ep->spsr) == MODE_RW_64) {
        sctlr_elx |= SCTLR_EL1_RES1;
    }

    write_ctx_reg(get_sysregs_ctx(ctx), CPU_CTX_SCTLR_EL1, sctlr_elx);

     // Enable Hypervisor Mode only if SPSR is set for:
     // (MODE_RW_64 & MODE_EL2) or (MODE_RW_64 & MODE32_hyp).
    if ((GET_RW(ep->spsr) == MODE_RW_64
         && GET_EL(ep->spsr) == MODE_EL2)
        || (GET_RW(ep->spsr) != MODE_RW_64
        && GET_M32(ep->spsr) == MODE32_hyp)) {
        scr_el3 |= SCR_HCE_BIT;
    }

     // Populate EL3 state so that we've the right context before doing ERET
    state = (&((core_context_t *) ctx)->el3state_regs_ctx);
    write_ctx_reg(state, CPU_CTX_SCR_EL3, scr_el3);
    write_ctx_reg(state, CPU_CTX_ELR_EL3, ep->pc);
    write_ctx_reg(state, CPU_CTX_SPSR_EL3, ep->spsr);

     // Store the X0-X7 value from the entrypoint into the context
     // Use memcpy as we are in control of the layout of the structures
    gp_regs = (&((core_context_t *) ctx)->gpregs_ctx);
    memcpy(gp_regs, (void *)&ep->args, sizeof(aapcs64_params_t));
}

 // @brief Get and initializes context as per security state
 // @param [in] ep : Ptr to entry point info
 //        [in] security_state : SECURE/NONSECURE
 // @retval void
void cm_init_context(const entry_point_info_t *ep, uint32_t state)
{
    core_context_t *ctx;
    ctx = cm_get_context(state);
    cm_init_context_common(ctx, ep, state);
}

 // @brief Save el1 sysregs as per security state
 // @param [in] security_state : SECURE/NONSECURE
 // @retval void
void cm_el1_sysregs_context_save(uint32_t security_state)
{
    core_context_t *ctx;
    ctx = cm_get_context(security_state);
    el1_sysregs_ctx_save(get_sysregs_ctx(ctx));
}

 // @brief Restore el1 sysregs as per security state
 // @param [in] security_state : SECURE/NONSECURE
 // @retval void
void cm_el1_sysregs_context_restore(uint32_t security_state)
{
    core_context_t *ctx;
    ctx = cm_get_context(security_state);
    el1_sysregs_ctx_restore(get_sysregs_ctx(ctx));
}

 // @brief Set next eret context
 // @param [in] security_state : SECURE/NONSECURE
 // @retval void
void cm_set_next_eret_ctx(uint32_t security_state)
{
    core_context_t *ctx;
    ctx = cm_get_context(security_state);

     // Set SP_EL0 with context pointer
    __asm__ volatile("msr    spsel, #0\n"
                     "mov    sp, %0\n"
                     "msr    spsel, #1\n"
                     : : "r" (ctx));
}
