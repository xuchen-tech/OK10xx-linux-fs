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

#ifndef __SP_EP_INFO_H__
#define __SP_EP_INFO_H__

#include "context_arch64.h"

#define SECURE          0
#define NON_SECURE      1
#define MAX_ALLOWED_OEN 14
#define START_OEN	50

 // Value of the function number and its mask sent from OPTEE-OS.
#define TEESMC_TRSTD_OS_TYPE_MASK         0x80000000
#define TEESMC_TRSTD_OS_FUNC_MASK         0xFFFF
#define TEESMC_TRSTD_OS_RETURN_ENTRY_DONE 0
#define TEESMC_TRSTD_OS_RETURN_ON_DONE    1
#define TEESMC_TRSTD_OS_RETURN_CALL_DONE  5
typedef struct aapcs64_params {
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t arg6;
    uint64_t arg7;
} aapcs64_params_t;

typedef struct aapcs32_params {
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
} aapcs32_params_t;

typedef struct init_config_info_el1_image {
    uint32_t magic;         // magic value from information
    uint32_t length;        // size of struct in bytes.
    uint64_t version;       // Version of structure
    uint64_t dRamBase;      // NonSecure DRAM start address
    uint64_t dRamSize;      // NonSecure DRAM size
    uint64_t secDRamBase;   // Secure DRAM start address
    uint64_t secDRamSize;   // Secure DRAM size
    uint64_t secIRamBase;   // Secure IRAM base
    uint64_t secIRamSize;   // Secure IRam size
    uint64_t conf_mair_el3; // MAIR_EL3 for memory attributes sharing
    uint32_t RFU1;
    uint32_t MSMPteCount;   // Number of MMU entries for MSM
    uint64_t MSMBase;       // MMU entries for MSM
    uint64_t gic_distributor_base;
    uint64_t gic_cpuinterface_base;
    uint32_t gic_version;
    uint32_t total_number_spi;
    uint32_t ssiq_number;
    uint32_t RFU2;
    uint64_t flags;

}init_config_info_el1_image_t;

 // This structure represents the superset of information needed while
 // switching exception levels. The only two mechanisms to do so are
 // ERET & SMC. Security state is indicated using bit zero of header
 // attribute
typedef struct entry_point_info {
    uint8_t state;      // state (secure or non-secure) entering into
        uint32_t attr;
#ifdef AARCH32
    uint32_t pc;
#else
    uint64_t pc;
#endif
    uint32_t spsr;
#ifdef AARCH32
    aapcs32_params_t args;
#else
    aapcs64_params_t args;
#endif
} entry_point_info_t;

typedef struct sp_ctx {
    uint32_t state; //pstate
    uint64_t mpidr;
    uint64_t c_rt_ctx;
    core_context_t cpu_ctx;
} sp_ctx_t;

typedef struct sp_vectors {
    uint32_t yield_smc_entry;
    uint32_t fast_smc_entry;
    uint32_t cpu_on_entry;
    uint32_t cpu_off_entry;
    uint32_t cpu_resume_entry;
    uint32_t cpu_suspend_entry;
    uint32_t fiq_entry;
    uint32_t system_off_entry;
    uint32_t system_reset_entry;
} sp_vectors_t;

int sp_setup_n_init(int index, entry_point_info_t *sp_ep_info);
entry_point_info_t* get_sp_ep_info_list(void);
int get_curr_core_pos();
int secure_el1os_enter_sp(uint64_t *c_rt_ctx);
int secure_el1os_exit_sp(uint64_t c_rt_ctx, uint64_t x1);

#endif //__EP_INFO_H__
