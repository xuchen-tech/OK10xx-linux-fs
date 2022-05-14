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
#include "plat.h"
#include "ddr.h"
#include "i2c.h"
#include "debug.h"

#ifdef CONFIG_SYS_DDR_RAW_TIMING
extern int fsl_ddr_get_dimm_params(struct dimm_params *pdimm,
				   unsigned int ctl, unsigned int dimm);
#endif

#define CONFIG_CS_PER_SLOT (CONFIG_CHIP_SELECTS_PER_CTRL / CONFIG_SYS_DIMM_SLOTS_PER_CTLR)

int parse_spd(struct ddr_info *priv)
{
	int i, j, k;
	int ret;
	int spd_addr;
	struct ddr4_spd spd[CONFIG_SYS_DIMM_SLOTS_PER_CTLR];
	int spd_checksum[CONFIG_SYS_DIMM_SLOTS_PER_CTLR];

	for (i = priv->first_ctrl; i < priv->first_ctrl + priv->num_ctrls; i++) {
#ifdef CONFIG_SYS_DDR_RAW_TIMING
		for (j = 0; j < priv->dimm_slots_per_ctrl; j++) {
			ret = fsl_ddr_get_dimm_params(&priv->dimms[i], i, j);
			if (!ret) {
				priv->conf[i].in_use = 1;
				priv->conf[i].dimm_in_use[j] = 1;
			} else if (ret == -EINVAL) {
				debug("Error: RAW timing error on DDR ");
				dbgprint_uint(i);
				debug("\n");
				priv->conf[i].in_use = 0;
				break;
			}
		}
#else
		/* Read in SPD */
		for (j = 0; j < priv->dimm_slots_per_ctrl; j++) {
			spd_addr = get_spd_addr(i, j);	/* return 0 on error */
			if (!spd_addr)
				continue;
			ret = read_spd(spd_addr, &spd[j], sizeof(struct ddr4_spd));
			if (ret) {
				priv->conf[i].dimm_in_use[j] = 0;
				spd_checksum[j] = 0;
				debug_hex("Debug: Reading SPD error at address 0x", spd_addr);
				continue;
			} else {
				debug(".");
				priv->conf[i].dimm_in_use[j] = 1;
				spd_checksum[j] = crc16((unsigned char *)&spd[j],
							sizeof(struct ddr4_spd));
				debug_hex("Calculate checksum for dimm ", j);
				debug_hex("spd checksum = ", spd_checksum[j]);
			}
		}
		/* Compare SPD */
		for (j = 0; j < priv->dimm_slots_per_ctrl; j++) {
			/* Find first non-empty slot */
			if (priv->conf[i].dimm_in_use[j]) {
				debug_int("First dimm in use ", j);
				for (k = j + 1;
				     k < priv->dimm_slots_per_ctrl && priv->conf[i].dimm_in_use[k];
				     k++) {
					if (spd_checksum[j] != spd_checksum[k]) {
						debug("Error: found different DIMMs on the same controller.\n");
						return -EINVAL;
					}
				}
				priv->conf[i].in_use = 1;
				break;	/* done with checking */
			}
		}
		/* Check if all slots are empty */
		if (j >= priv->dimm_slots_per_ctrl)
			continue;

		/* Now we have identical DIMMs, only need to calculate once */
		ret = cal_dimm_params(&spd[j], &priv->dimms[i]);
		if (!ret) {
			dump_dimm_parameters(&priv->dimms[i]);	/* for debug */
		} else {
			debug("Error: SPD calculation error DDR ");
			dbgprint_uint(i);
			debug("\n");
			priv->conf[i].in_use = 0;
			continue;
		}
#endif	/* CONFIG_SYS_DDR_RAW_TIMING */
		for (j = 0; j < priv->dimm_slots_per_ctrl; j++) {
			if (!priv->conf[i].dimm_in_use[j])
				continue;
			switch (priv->dimms[i].n_ranks) {
			case 4:
				if (j) {
					debug("Error: Quad-rank DIMM in wrong slot\n");
					return -EINVAL;
				}
				priv->conf[i].cs_in_use = 0xf;
				break;
			case 2:
				priv->conf[i].cs_in_use |= 0x3 << (j * CONFIG_CS_PER_SLOT);
				break;
			case 1:
				priv->conf[i].cs_in_use |= 0x1 << (j * CONFIG_CS_PER_SLOT);
				break;
			default:
				debug("Error: SPD error\n");
				return -EINVAL;
			}
			debug_hex("Set cs_in_use = ", priv->conf[i].cs_in_use);
		}
		if (priv->dimms[i].registered_dimm) {
			debug("RDIMM ");
        }
		else {
			debug("UDIMM ");
        }
		debug(priv->dimms[i].mpart);
		debug("\n");
	}

	return 0;
}
