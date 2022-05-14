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
//  Ruchika Gupta <ruchika.gupta@nxp.com> 
//
//-----------------------------------------------------------------------------

#ifndef __JOBDESC_H
#define __JOBDESC_H

#include "types.h"

#define KEY_BLOB_SIZE 32
#define MAC_SIZE 16

#define KEY_IDNFR_SZ_BYTES 16
#define CLASS_SHIFT 25
#define CLASS_2	(0x02 << CLASS_SHIFT)

#define CMD_SHIFT               27
#define CMD_OPERATION		(0x10 << CMD_SHIFT)

#define OP_TYPE_SHIFT		24
#define OP_TYPE_ENCAP_PROTOCOL	(0x07 << OP_TYPE_SHIFT)

/* Assuming OP_TYPE = OP_TYPE_UNI_PROTOCOL */
#define OP_PCLID_SHIFT		16
#define OP_PCLID_BLOB		(0x0d << OP_PCLID_SHIFT)

#define BLOB_PROTO_INFO         0x00000002

uint32_t desc_length(uint32_t *desc);

int cnstr_rng_jobdesc(uint32_t *desc, uint32_t state_handle,
		      uint32_t *add_inp, uint32_t add_ip_len,
             	      uint8_t *out_data, uint32_t len);

int cnstr_rng_instantiate_jobdesc(uint32_t *desc);

 // Construct descriptor to generate hw key blob
int cnstr_hw_encap_blob_jobdesc(uint32_t *desc,
                                uint8_t *key_idnfr, uint32_t key_sz, uint32_t key_class,
                                uint8_t *plain_txt, uint32_t in_sz,
                                uint8_t *enc_blob,  uint32_t out_sz,
                                uint32_t operation);

#endif
