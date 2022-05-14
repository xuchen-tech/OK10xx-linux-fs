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

#include "fsl_sec.h"
#include "sec_hw_specific.h"
#include "jobdesc.h"
#include "lib.h"

 // Callback function after Instantiation decsriptor is submitted to SEC
static void rng_done(uint32_t *desc, uint32_t status, void *arg, void *job_ring)
{
    debug("Desc SUCCESS\n");
    debug_hex("status", status);	
}

 // Is the HW RNG instantiated?
 // Return code:
 // 0 - Not in the instantiated state
 // 1 - In the instantiated state
 // state_handle - 0 for SH0, 1 for SH1
static int is_hw_rng_instantiated(uint32_t *state_handle)
{
    int ret_code = 0;
    uint32_t rdsta;

    rdsta = sec_in32(CAAM_BASE_ADDR + RNG_REG_RDSTA_OFFSET);

     //Check if either of the two state handles has been instantiated
    if (rdsta & RNG_STATE0_HANDLE_INSTANTIATED) {
        *state_handle = 0;
        ret_code = 1;
    } else if (rdsta & RNG_STATE0_HANDLE_INSTANTIATED) {
        *state_handle = 1;
        ret_code = 1;
    }

    return ret_code;
}

 // @brief Kick the TRNG block of the RNG HW Engine
 // @param [in] ent_delay	Entropy delay to be used
 // 	   By default, the TRNG runs for 200 clocks per sample;
 // 	   1200 clocks per sample generates better entropy.
 // @retval 0 on success
 // @retval -1 on error
static void kick_trng(int ent_delay)
{
    uint32_t  val;

     // put RNG4 into program mode 
    val = sec_in32(CAAM_BASE_ADDR + RNG_REG_RTMCTL_OFFSET);
    val = val | RTMCTL_PRGM;
    sec_out32(CAAM_BASE_ADDR + RNG_REG_RTMCTL_OFFSET, val);

     // rtsdctl bits 0-15 contain "Entropy Delay, which defines the
     //  length (in system clocks) of each Entropy sample taken
    val = sec_in32(CAAM_BASE_ADDR + RNG_REG_RTSDCTL_OFFSET);
    val = (val & ~RTSDCTL_ENT_DLY_MASK) |
              (ent_delay << RTSDCTL_ENT_DLY_SHIFT);
    sec_out32(CAAM_BASE_ADDR + RNG_REG_RTSDCTL_OFFSET, val);
     // min. freq. count, equal to 1/4 of the entropy sample length 
    sec_out32(CAAM_BASE_ADDR + RNG_REG_RTFRQMIN_OFFSET, ent_delay >> 2);
     // disable maximum frequency count 
    sec_out32(CAAM_BASE_ADDR + RNG_REG_RTFRQMAX_OFFSET, RTFRQMAX_DISABLE);

     // select raw sampling in both entropy shifter
     //  and statistical checker
    val = sec_in32(CAAM_BASE_ADDR + RNG_REG_RTMCTL_OFFSET);
    val = val | RTMCTL_SAMP_MODE_RAW_ES_SC;
    sec_out32(CAAM_BASE_ADDR + RNG_REG_RTMCTL_OFFSET, val);

     // put RNG4 into run mode 
    val = sec_in32(CAAM_BASE_ADDR + RNG_REG_RTMCTL_OFFSET);
    val = val & ~RTMCTL_PRGM;
    sec_out32(CAAM_BASE_ADDR + RNG_REG_RTMCTL_OFFSET, val);
}

 // @brief Submit descriptor to instantiate the RNG
 // @retval 0 on success
 // @retval -1 on error
static int instantiate_rng(void)
{
    int ret = 0;
     // TBD - Current allocator doesn't have a free function
     // Remove static once free implementation is available
    static struct job_descriptor *jobdesc = NULL;
    if (!jobdesc) {
	jobdesc = alloc(sizeof(struct job_descriptor), 64);
    	if (!jobdesc) {
       	    debug("DEC allocation failed\n");
            return -1;
	}
    }
    jobdesc->arg = NULL;
    jobdesc->callback = rng_done;

     // create the hw_rng descriptor
    cnstr_rng_instantiate_jobdesc(jobdesc->desc);

     // Finally, generate the requested random data bytes
    ret = run_descriptor_jr(jobdesc);
    if (ret) {
	debug("Error in running descriptor\n");
	ret = -1;	
    }

    free(jobdesc);
    return ret;
}

 // Generate Random Data using HW RNG
 // Parameters:
 // uint8_t* add_input     - user specified optional input byte array
 // uint32_t add_input_len - number of bytes of additional input
 // uint8_t* out           - user specified output byte array
 // uint32_t out_len       - number of bytes to store in output byte array
 // Return code:
 // 0 - SUCCESS
 // -1 - ERROR
static int
hw_rng_generate(uint32_t *add_input, uint32_t add_input_len,
           uint8_t *out, uint32_t out_len, uint32_t state_handle)
{
    int ret = 0;
     // TBD - Current allocator doesn't have a free function
     // Remove static once free implementation is available
    static struct job_descriptor *jobdesc = NULL;
    if (!jobdesc) {
	jobdesc = alloc(sizeof(struct job_descriptor), 64);
    	if (!jobdesc) {
       	    debug("DEC allocation failed\n");
            return -1;
	}
    }
    jobdesc->arg = NULL;
    jobdesc->callback = rng_done;

     // create the hw_rng descriptor
    ret = cnstr_rng_jobdesc(jobdesc->desc, state_handle,
		      add_input, add_input_len, out, out_len);
    if (ret) {
        debug("Descriptor construction failed\n");
	ret = -1;
	goto out;
    }
    
     // Finally, generate the requested random data bytes
    ret = run_descriptor_jr(jobdesc);
    if (ret) {
	debug("Error in running descriptor\n");
       ret = -1;
    }

out:
    free(jobdesc);
    return ret;
}

 // this function instantiates the rng
 //
 // Return code:
 //  0 - All is well
 // <0 - Error occurred somewhere
int hw_rng_instantiate(void)
{
    int ret = 0;
    int ent_delay = RTSDCTL_ENT_DLY_MIN;
    uint32_t state_handle;
    ret = is_hw_rng_instantiated(&state_handle); 
    if (ret) {
	debug("RNG already instantiated\n");
	return 0;
    }
    do {
        kick_trng(ent_delay);
        ent_delay += 400;
	 //if instantiate_rng(...) fails, the loop will rerun
         //and the kick_trng(...) function will modfiy the
	 //upper and lower limits of the entropy sampling
	 //interval, leading to a sucessful initialization of
	ret = instantiate_rng();
    } while ((ret == -1) && (ent_delay < RTSDCTL_ENT_DLY_MAX));
    if (ret) {
	debug("RNG: Failed to instantiate RNG\n");
	return ret;
    } else {
	debug("RNG: INSTANTIATED\n");
    }

    // Enable RDB bit so that RNG works faster
   //sec_setbits32(&sec->scfgr, SEC_SCFGR_RDBENABLE);

    return ret;
}

 // Generate random bytes, and stuff them into the bytes buffer
 //
 // If the HW RNG has not already been instantiated,
 //  it will be instantiated before data is generated.
 //
 // Parameters:
 // uint8_t* bytes  - byte buffer large enough to hold the requested random date
 // int    byte_len - number of random bytes to generate
 //
 // Return code:
 //  0 - All is well
 //  ~0 - Error occurred somewhere
int get_rand_bytes_hw(uint8_t *bytes, int byte_len)
{
    int ret_code = 0;
    uint32_t state_handle;
    
     // If this is the first time this routine is called,
     //  then the hash_drbg will not already be instantiated.
     // Therefore, before generating data, instantiate the hash_drbg
    ret_code = is_hw_rng_instantiated(&state_handle); 
    if (!ret_code) {
        debug("Instantiating the HW RNG\n");

         // Instantiate the hw RNG
        ret_code = hw_rng_instantiate();
        if (ret_code) {
            debug("HW RNG Instantiate failed\n");
            return ret_code;
        }
    }
    
     // If the HW RNG is still not instantiated, something must have gone wrong,
     //  it must be in the error state, and we will not generate any random data
    if (!is_hw_rng_instantiated(&state_handle)) {
        debug("HW RNG is in an Error state, and cannot be used\n");
        return -1;
    }

     // Generate a random 256-bit value, as 32 bytes
    ret_code = hw_rng_generate(0, 0, bytes, byte_len, state_handle);
    if (ret_code) {
        debug("HW RNG Generate failed\n");
        return ret_code;
    }

    return ret_code;
}