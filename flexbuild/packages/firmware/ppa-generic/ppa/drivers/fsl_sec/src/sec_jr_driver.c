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
//  Alexandru Porosanu <alexandru.porosanu@nxp.com>
//  Ruchika Gupta <ruchika.gupta@nxp.com> 
//
//-----------------------------------------------------------------------------

#include "sec_hw_specific.h"
#include "lib.h"
#include "sec_jr_driver.h"
 
#define CAAM_TIMEOUT   200000 //ms
 // Job rings used for communication with SEC HW 
struct sec_job_ring_t g_job_rings[MAX_SEC_JOB_RINGS];
 
 // The current state of SEC user space driver 
volatile sec_driver_state_t g_driver_state = SEC_DRIVER_STATE_IDLE;
 
int g_job_rings_no;
 
void *init_job_ring(uint8_t jr_mode,
             uint16_t irq_coalescing_timer,
             uint8_t irq_coalescing_count,
             void *reg_base_addr, uint32_t irq_id)
{
    struct sec_job_ring_t *job_ring = &g_job_rings[g_job_rings_no++];
    int ret = 0;
    void *tmp;

    job_ring->register_base_addr = reg_base_addr;
    job_ring->jr_mode = jr_mode;
    job_ring->irq_fd = irq_id;

     // Allocate mem for input and output ring
    tmp = alloc(SEC_DMA_MEM_INPUT_RING_SIZE, 64);
    job_ring->input_ring = vtop(tmp);

    memset(job_ring->input_ring, 0, SEC_DMA_MEM_INPUT_RING_SIZE);

     // Allocate memory for output ring 
    tmp = alloc(SEC_DMA_MEM_OUTPUT_RING_SIZE, 64);
    job_ring->output_ring = (struct sec_outring_entry *)vtop(tmp);

    memset(job_ring->output_ring, 0, SEC_DMA_MEM_OUTPUT_RING_SIZE);

     // Reset job ring in SEC hw and configure job ring registers
    ret = hw_reset_job_ring(job_ring);
    if (ret) {
         debug("Failed to reset hardware job ring\n");
        return NULL;
    }

    if (jr_mode == SEC_NOTIFICATION_TYPE_IRQ) {
         // Enble IRQ if driver work sin interrupt mode
        debug("Enabling DONE IRQ generation "
             "on job ring   %p");
        ret = jr_enable_irqs(irq_id);
        if (ret) {
            debug("Failed to enable irqs for job ring\n");
            return NULL;
        }
    }
    if (irq_coalescing_timer || irq_coalescing_count) {
        hw_job_ring_set_coalescing_param(job_ring,
             irq_coalescing_timer,
             irq_coalescing_count);

        hw_job_ring_enable_coalescing(job_ring);
        job_ring->coalescing_en = 1;
    }

    job_ring->jr_state = SEC_JOB_RING_STATE_STARTED;

    return job_ring;
}

int sec_release(void)
{
    int i;

     // Validate driver state 
    if (g_driver_state == SEC_DRIVER_STATE_RELEASE) {
        debug("Driver release is already in progress");
        return    SEC_DRIVER_RELEASE_IN_PROGRESS;
    }
     // Update driver state 
    g_driver_state = SEC_DRIVER_STATE_RELEASE;

     // If any descriptors in flight , poll and wait
     // until all descriptors are received and silently discarded.
     
    flush_job_rings();

    for (i = 0; i < g_job_rings_no; i++) {
        shutdown_job_ring(&g_job_rings[i]);
    }
    g_job_rings_no = 0;
    g_driver_state = SEC_DRIVER_STATE_IDLE;

    return SEC_SUCCESS;
}

int sec_jr_lib_init(void)
{
     // Validate driver state 
    if (g_driver_state != SEC_DRIVER_STATE_IDLE) {
        debug("Driver already initialized\n");
        return 0;
    }

    memset(g_job_rings, 0, sizeof(g_job_rings));
    g_job_rings_no = 0;

     // Update driver state 
    g_driver_state = SEC_DRIVER_STATE_STARTED;
    return 0;
}

int dequeue_jr(void *job_ring_handle, int32_t limit)
{
    int ret = 0;
    int notified_descs_no = 0;
    struct sec_job_ring_t *job_ring = (sec_job_ring_t *) job_ring_handle;
    unsigned long start_time, timer;

     // Validate driver state 
    if (g_driver_state != SEC_DRIVER_STATE_STARTED) {
        debug("Driver release is in progress or driver not initialized\n");
        return -1;
    }

     // Validate input arguments 
    if (job_ring == NULL) {
        debug("job_ring_handle is NULL\n");
        return -1;
    }
    if (((limit == 0) || (limit > SEC_JOB_RING_SIZE))) {
        debug("Invalid limit parameter configuration\n");
        return -1;
    }

    debug("JR Polling");
    debug_int("limit[%d] \n", limit);

     // Poll job ring
     // If limit < 0 -> poll JR until no more notifications are available.
     // If limit > 0 -> poll JR until limit is reached. 

     // TBD - Remove the ifdef once split image is enabled and timer code is on
#if (CNFG_TIMER)
    start_time = get_timer(0);
#else
    start_time = 0;
#endif

    while (notified_descs_no == 0) {
         // Run hw poll job ring
        notified_descs_no = hw_poll_job_ring(job_ring, limit);
        if (notified_descs_no < 0) {
            debug("Error polling SEC engine job ring ");
            return notified_descs_no;
        }
        debug_int("Jobs notified[%d]. ", notified_descs_no);

#if (CNFG_TIMER)
        if (get_timer(start_time) >= CAAM_TIMEOUT)
	    break;
#else
	 start_time++;
	 if (start_time == 100)
	     break;
#endif

    }

    if (job_ring->jr_mode == SEC_NOTIFICATION_TYPE_IRQ) {

         // Always enable IRQ generation when in pure IRQ mode
        ret = jr_enable_irqs(job_ring->irq_fd);
        if (ret) {
            debug("Failed to enable irqs for job ring %p");
            return ret;
        }
    }
    return notified_descs_no;
}

int enq_jr_desc(void *job_ring_handle, struct job_descriptor *jobdescr)
{
    struct sec_job_ring_t *job_ring;
    job_ring = (struct sec_job_ring_t *)job_ring_handle;

     // Validate driver state 
    if (g_driver_state != SEC_DRIVER_STATE_STARTED) {
        debug("Driver release is in progress or driver not initialized\n");
        return -1;
     }

    // Check job ring state 
   if (job_ring->jr_state != SEC_JOB_RING_STATE_STARTED) {
        debug("Job ring is currently resetting" \
        "Can use it again after reset is over");
        return -1;
    }

    if (SEC_JOB_RING_IS_FULL(job_ring->pidx, job_ring->cidx,
            SEC_JOB_RING_SIZE, SEC_JOB_RING_SIZE))    {
        debug("Job ring is full\n");
        return -1;
    }
    debug("Before sending desc\n");

     // Set ptr in input ring to current descriptor    
    sec_write_addr(&job_ring->input_ring[job_ring->pidx],
          (phys_addr_t)vtop(jobdescr->desc));
     // Notify HW that a new job is enqueued 
    hw_enqueue_desc_on_job_ring((struct jobring_regs *)job_ring->register_base_addr, 1);

     // increment the producer index for the current job ring 
    job_ring->pidx = SEC_CIRCULAR_COUNTER(job_ring->pidx,
                SEC_JOB_RING_SIZE);

    return 0;
}
