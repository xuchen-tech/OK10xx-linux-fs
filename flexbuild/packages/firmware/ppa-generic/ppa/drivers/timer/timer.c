//-----------------------------------------------------------------------------
// 
// Copyright (c) 2016, NXP Semiconductors
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
// Author York Sun <york.sun@nxp.com>
// 
//-----------------------------------------------------------------------------

#include "lib.h"
#include "io.h"
#include "soc.h"

unsigned long long get_tbfreq(void)
{
	unsigned long long read;

	asm volatile("mrs %0, cntfrq_el0" : "=r" (read));

	return read;
}

/* Workaround for erratum A008585 is not implemented here */
unsigned long long get_ticks(void)
{
	register unsigned long long read;

	asm volatile("isb" : : : "memory");
	asm volatile("mrs %0, cntpct_el0" : "=r" (read));

	return read;
}

static unsigned long long usec2ticks(unsigned long usec)
{
	return usec * get_tbfreq() / 1000000;
}

static unsigned long long tick2ms(unsigned long long tick)
{
	return tick * 1000 / get_tbfreq();
}

/* return time difference in ms */
unsigned long get_timer(unsigned long start)
{
	return tick2ms(get_ticks()) - start;
}

void timer_init(void)
{
	unsigned int *cntcr = (void *)TIMER_BASE_ADDR;
	unsigned int *cltbenr = (void *)0x01e318a0;
	unsigned long long cntfreq = 25000000;	/* sysclk/4 */

	asm volatile("msr cntfrq_el0, %0" : : "r" (cntfreq) : "memory");
#if (LSCH == 3)
	out_le32(cltbenr, 0xf);
#endif
	out_le32(cntcr, 0x1);
}

void udelay(unsigned long usec)
{
	unsigned long long ticks;

	ticks = get_ticks() + usec2ticks(usec);
	while (get_ticks() < ticks + 1)
		;
}

void mdelay(unsigned long msec)
{
	while(msec--)
		udelay(1000);
}
