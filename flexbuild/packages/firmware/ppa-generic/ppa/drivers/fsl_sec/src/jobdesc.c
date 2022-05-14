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

#include "lib.h"
#include "io.h"
#include "sec_hw_specific.h"

#define DESC_LEN_MASK           0x7f
#define DESC_START_SHIFT        16

 // Return Length of desctiptr from first word
uint32_t desc_length(uint32_t *desc)
{
    return desc[0] & DESC_LEN_MASK;
}

 //Update start index in first word of descriptor
void desc_update_start_index(uint32_t *desc, uint32_t index)
{
    desc[0] |= (index << DESC_START_SHIFT);
}

 // Initialize teh descriptor
void desc_init(uint32_t *desc)
{
    *desc = 0;
}

 // Add word in teh descriptor and increment the length
void desc_add_word(uint32_t *desc, uint32_t word)
{
    uint32_t len = desc_length(desc);

     // Add Word at Last
    uint32_t *last = desc + len;
    *last = word;

     // Increase the length
    desc[0] += 1;
}


 // Add Pointer to teh descriptor
void desc_add_ptr(uint32_t *desc, phys_addr_t *ptr)
{
    uint32_t len = desc_length(desc);

     // Add Word at Last 
    phys_addr_t *last = (phys_addr_t *)(desc + len);
#ifdef CONFIG_PHYS_64BIT
    ptr_addr_t *ptr_addr = (ptr_addr_t *)last;
    ptr_addr->m_halfs.high = PHYS_ADDR_HI(ptr);
    ptr_addr->m_halfs.low = PHYS_ADDR_LO(ptr);
#else
    *last = ptr;
#endif

     // Increase the length 
    desc[0] += (uint32_t)(sizeof(phys_addr_t) / sizeof(uint32_t));
}

 // Descriptor to generate Random words 
int cnstr_rng_jobdesc(uint32_t *desc, uint32_t state_handle,
              uint32_t *add_inp, uint32_t add_ip_len,
              uint8_t *out_data, uint32_t len)
{
    phys_addr_t *phys_addr_out = vtop(out_data);

     // Current descriptor support only 64K length
    if (len > 0xffff) {
        return -1;
    }
     // Additional Input not supported by current descriptor
    if (add_ip_len > 0) {
        return -1;
    }
    debug("Constructing descriptor\n");
    desc_init(desc);
     // Class1 Alg Operation,RNG Optype, Generate
    desc_add_word(desc, 0xb0800000);
    desc_add_word(desc, 0x82500000 | (state_handle << ALG_AAI_SH_SHIFT));
    desc_add_word(desc, 0x60340000 | len);
    desc_add_ptr(desc, phys_addr_out);    
    
    return 0;

}

 // Construct descriptor to instantiate RNG
int cnstr_rng_instantiate_jobdesc(uint32_t *desc)
{
    desc_init(desc);
    desc_add_word(desc, 0xb0800000);
     // Class1 Alg Operation,RNG Optype, Instantiate
    desc_add_word(desc, 0x82500004);
     // Wait for done
    desc_add_word(desc, 0xa2000001);
     //Load to clear written
    desc_add_word(desc, 0x10880004);
     //Pri Mode Reg clear
    desc_add_word(desc, 0x00000001);
     // Generate secure keys
    desc_add_word(desc, 0x82501000);

    return 0;
}

 // Construct descriptor to generate hw key blob
int cnstr_hw_encap_blob_jobdesc(uint32_t *desc,
                                uint8_t *key_idnfr, uint32_t key_sz, uint32_t key_class,
                                uint8_t *plain_txt, uint32_t in_sz,
                                uint8_t *enc_blob,  uint32_t out_sz,
                                uint32_t operation)
{
    phys_addr_t *phys_key_idnfr, *phys_addr_in, *phys_addr_out;
    int i = 0;

    phys_key_idnfr = vtop((void *)key_idnfr);
    phys_addr_in   = vtop((void *)plain_txt);
    phys_addr_out  = vtop((void *)enc_blob);

    desc_init(desc);

    desc_add_word(desc, 0xb0800000);

      //Key Identifier
    desc_add_word(desc, (key_class | key_sz));
    desc_add_ptr(desc, phys_key_idnfr);
    //desc_add_word(desc, 0x00000000); //None Hi loc
    //desc_add_word(desc, 0x00000000); //None Lo loc

     //Source Address
    desc_add_word(desc, 0xf0400000);
    desc_add_ptr(desc, phys_addr_in);
    //desc_add_word(desc, 0x00000000); //None Hi loc
    //desc_add_word(desc, 0x00000000); //None Lo loc

     //In Size = 0x10
    desc_add_word(desc, in_sz);

     //Out Address
    desc_add_word(desc, 0xf8400000);
    desc_add_ptr(desc, phys_addr_out);

     //Out Size = 0x10
    desc_add_word(desc, out_sz);

     //Operation
    desc_add_word(desc, operation);

    for(i = 0; i < 15; i++)
        debug_hex("desc word",desc[i]);

    return 0;
}
