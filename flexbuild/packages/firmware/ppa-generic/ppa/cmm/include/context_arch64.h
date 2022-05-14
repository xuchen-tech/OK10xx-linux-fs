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

#ifndef __CONTEXT_ARCH64_H__
#define __CONTEXT_ARCH64_H__

#include "types.h"
#include "context_arch64_asm.h"

#define DWORD_SHIFT              3
#define CPU_CTX_GPREG_ALL        (CPU_CTX_GPREGS_END >> DWORD_SHIFT)
#define CPU_CTX_SYSREG_ALL       (CPU_CTX_SYSREGS_END >> DWORD_SHIFT)
#define CPU_CTX_EL3STATE_ALL     (CPU_CTX_EL3STATE_END >> DWORD_SHIFT)

#define DEFINE_REG_STRUCT(name, num_regs)    \
    typedef struct name {                    \
        uint64_t _regs[num_regs];            \
    } __attribute__ ((aligned(16))) name##_t

 // Constants to determine the size of individual context structures
DEFINE_REG_STRUCT(gp_regs, CPU_CTX_GPREG_ALL);
DEFINE_REG_STRUCT(el1_sys_regs, CPU_CTX_SYSREG_ALL);
DEFINE_REG_STRUCT(el3_state_regs, CPU_CTX_EL3STATE_ALL);

 // Macros to access members of any of the above structures using their
 // offsets
#define read_ctx_reg(ctx, offset)    ((ctx)->_regs[offset >> DWORD_SHIFT])
#define write_ctx_reg(ctx, offset, val)    (((ctx)->_regs[offset >> DWORD_SHIFT]) \
                     = val)

 // Top-level context structure which is used by EL3 firmware to
 // preserve the state of a core at EL1 in one of the two security
 // states and save enough EL3 meta data to be able to return to that
 // EL and security state. The context management library will be used
 // to ensure that SP_EL3 always points to an instance of this
 // structure at exception entry and exit. Each instance will
 // correspond to either the secure or the non-secure state.
typedef struct core_context {
    gp_regs_t gpregs_ctx;
    el3_state_regs_t el3state_regs_ctx;
    el1_sys_regs_t el1sysregs_ctx;
} core_context_t;

 // Macros to access members of the 'cpu_context_t' structure
#define get_el3state_ctx(h)    (&((core_context_t *) h)->el3state_regs_ctx)
#define get_sysregs_ctx(h)    (&((core_context_t *) h)->el1sysregs_ctx)
#define get_gpregs_ctx(h)    (&((core_context_t *) h)->gpregs_ctx)

void el1_sysregs_ctx_save (el1_sys_regs_t *el1sysregs_ctx);
void el1_sysregs_ctx_restore (el1_sys_regs_t *el1sysregs_ctx);

typedef struct core_data {
    void *core_context[2];
} core_data_t;

#define DEFINE_SYSREG_READ_FUNC(_name, _reg_name)               \
static inline uint64_t read_ ## _name(void)                     \
{                                                               \
    uint64_t v;                                                 \
    __asm__ volatile ("mrs %0, " #_reg_name : "=r" (v));        \
    return v;                                                   \
}

#define DEFINE_SYSREG_WRITE_FUNC(_name, _reg_name)              \
static inline void write_ ## _name(uint64_t v)                  \
{                                                               \
    __asm__ volatile ("msr " #_reg_name ", %0" : : "r" (v));    \
}

DEFINE_SYSREG_READ_FUNC(tpidr_el3, tpidr_el3)
DEFINE_SYSREG_READ_FUNC(mpidr_el1, mpidr_el1)
DEFINE_SYSREG_READ_FUNC(sctlr_el3, sctlr_el3)
DEFINE_SYSREG_READ_FUNC(sctlr_el1, sctlr_el1)
DEFINE_SYSREG_READ_FUNC(scr_el3, scr_el3)
DEFINE_SYSREG_WRITE_FUNC(tpidr_el3, tpidr_el3)
DEFINE_SYSREG_WRITE_FUNC(mpidr_el1, mpidr_el1)
DEFINE_SYSREG_WRITE_FUNC(sctlr_el3, sctlr_el3)
DEFINE_SYSREG_WRITE_FUNC(sctlr_el1, sctlr_el1)
DEFINE_SYSREG_WRITE_FUNC(scr_el3, scr_el3)

#endif //__CONTEXT_ARCH64_H__
