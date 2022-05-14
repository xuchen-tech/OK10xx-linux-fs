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

#include "soc.h"
#include "io.h"
#include "i2c.h"
#include "types.h"

#define SVR			0x01e000a4
#define SVR_WO_E		0xFFFFFE
#define SVR_LS2088		0x870900
#define SVR_LS2084		0x870910
#define SVR_SOC_VER(svr)	(((svr) >> 8) & SVR_WO_E)

/* Fix me: shall avoid setting register for disabled DDR controller */
static void erratum_a008336(void)
{
	unsigned int *eddrtqcr1;

	eddrtqcr1 = (void *)0x70012c000ULL + 0x800;
	out_le32(eddrtqcr1, 0x63b30002);

	eddrtqcr1 = (void *)0x70012d000ULL + 0x800;
	out_le32(eddrtqcr1, 0x63b30002);
}

static void erratum_a009203(void)
{
	struct ls_i2c *ccsr_i2c = (void *)CONFIG_SYS_I2C_BASE;

	i2c_out(&ccsr_i2c->dbg, I2C_GLITCH_EN);
}

void soc_errata(void)
{
	erratum_a008336();
	erratum_a009203();
}
