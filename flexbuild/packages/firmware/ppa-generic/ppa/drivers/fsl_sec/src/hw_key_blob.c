//-----------------------------------------------------------------------------
// 
// Copyright 2017 - 2018 NXP Semiconductors
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
//  Pankaj Gupta <pankaj.gupta@nxp.com> 
//
//-----------------------------------------------------------------------------

#include "fsl_sec.h"
#include "sec_hw_specific.h"
#include "jobdesc.h"
#include "lib.h"

//-----------------------------------------------------------------------------

#define BLOB_PROTO_INFO         0x00000002

//-----------------------------------------------------------------------------

 // Callback function after Instantiation decsriptor is submitted to SEC
static void blob_done(uint32_t *desc, uint32_t status, void *arg, void *job_ring)
{
    debug("Desc SUCCESS\n");
    debug_hex("status", status);	
}

 // @brief Submit descriptor to create blob
 // @retval 0 on success
 // @retval -1 on error

int get_hw_unq_key_blob_hw(uint8_t *hw_key, int size)
{
    int ret = 0;
    int i = 0;

    uint32_t key_sz = KEY_IDNFR_SZ_BYTES;
    uint8_t *key_data = NULL;
   
    uint32_t in_sz = 16;
    uint8_t *in_data = NULL;

     //output blob will have 32 bytes key blob in beginning and
     // 16 byte HMAC identifier at end of data blob */
    uint32_t out_sz = in_sz + KEY_BLOB_SIZE + MAC_SIZE;
    uint8_t *out_data = NULL;

    uint32_t operation = CMD_OPERATION |
                         OP_TYPE_ENCAP_PROTOCOL |
                         OP_PCLID_BLOB |
                         BLOB_PROTO_INFO;

     // TBD - Current allocator doesn't have a free function
     // Remove static once free implementation is available
    static struct job_descriptor *jobdesc = NULL;

    key_data = alloc(KEY_IDNFR_SZ_BYTES, 64);

    if (NULL == key_data) {
        debug("get_hw_unq_key_blob_hw: in data buffer allocation failed\n");
        ret = -1;
        goto clean_up;
    }
    memset(key_data, 0xff, KEY_IDNFR_SZ_BYTES);

    in_data = alloc(in_sz, 64);

    if (NULL == in_data) {
        debug("get_hw_unq_key_blob_hw: in data buffer allocation failed\n");
        ret = -1;
        goto clean_up;
    }
    memset(in_data, 0x00, in_sz);

    out_data = alloc(out_sz, 64);

    if (NULL == out_data) {
        debug("get_hw_unq_key_blob_hw: Out data buffer allocation failed\n");
        ret = -1;
        goto clean_up;
    }
    memset(out_data, 0x00, in_sz);

    jobdesc = alloc(sizeof(struct job_descriptor), 64);
    if (NULL == jobdesc) {
            debug("DEC allocation failed\n");
            ret = -1;
            goto clean_up;
    }

    jobdesc->arg = NULL;
    jobdesc->callback = blob_done;

    debug("\nGenerating Master Key Verification Blob.\n");

     // create the hw_rng descriptor
    ret = cnstr_hw_encap_blob_jobdesc(jobdesc->desc,
                                      key_data, key_sz, CLASS_2,
                                      in_data, in_sz,
                                      out_data, out_sz,
                                      operation);

     // Finally, generate the blob 
    ret = run_descriptor_jr(jobdesc);
    if (ret) {
        debug("Error in running hw unq key blob descriptor\n");
        ret = -1;
        goto clean_up;
    }

    //Copying alternate bytes of the Master Key Verification Blob.
    for(i = 0; i< size; i++)
    {
        hw_key[i] = out_data[2*i];
        //debug_hex("hw_key", out_data[2*i]);
        //debug_hex("hw_key", out_data[2*i+1]);
    }


clean_up:
    if (NULL != key_data)
        free(key_data);
    if (NULL != in_data)
        free(in_data);
    if (NULL != out_data)
        free(out_data);
    if (NULL != jobdesc)
        free(jobdesc);
    return ret;
}
