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

#ifndef __CONTEXT_MGMT_H__
#define __CONTEXT_MGMT_H__

#define EP_ST_MASK      0x4
#define EP_GET_ST(x)    (x & EP_ST_MASK)

#define EP_EE_MASK      0x2
#define EP_GET_EE(x)    (x & EP_EE_MASK)

#define EP_ST_ENABLE    0x4
#define EP_EE_BIG       0x2

void *cm_get_context(uint32_t security_state);
void cm_set_context(void *context, uint32_t security_state);
void cm_init_context(const entry_point_info_t *ep, uint32_t state);
void cm_el1_sysregs_context_restore(uint32_t security_state);
void cm_el1_sysregs_context_save(uint32_t security_state);
void cm_set_next_eret_ctx(uint32_t security_state);
void el3_exit(void);

#endif // __CONTEXT_MGMT_H__
