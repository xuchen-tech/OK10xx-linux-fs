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
#include "dimm.h"
#include "i2c.h"

#define NUM_OF_CTRL	CONFIG_SYS_NUM_DDR_CTLRS
#define NUM_OF_DIMM	CONFIG_SYS_DIMM_SLOTS_PER_CTLR

/* Allow board to override the spd addresses */
unsigned char get_spd_addr(int ctrl, int dimm) __attribute__ ((weak));
unsigned char get_spd_addr(int ctrl, int dimm)
{
	const static unsigned char spd_addr[NUM_OF_CTRL][NUM_OF_DIMM] = {
		[0][0] = CONFIG_SPD_EEPROM0,
#if CONFIG_SYS_DIMM_SLOTS_PER_CTLR > 1
		[0][1] = CONFIG_SPD_EEPROM1,
#endif
#if CONFIG_SYS_NUM_DDR_CTLRS > 1
		[1][0] = CONFIG_SPD_EEPROM2,
#if CONFIG_SYS_DIMM_SLOTS_PER_CTLR > 1
		[1][1] = CONFIG_SPD_EEPROM3,
#endif
#if CONFIG_SYS_NUM_DDR_CTLRS > 2
		[2][0] = CONFIG_SPD_EEPROM4,
#if CONFIG_SYS_DIMM_SLOTS_PER_CTLR > 1
		[2][1] = CONFIG_SPD_EEPROM5,
#endif
#endif /* CONFIG_SYS_NUM_DDR_CTLRS > 2 */
#endif /* CONFIG_SYS_NUM_DDR_CTLRS > 1 */
	};
	if (ctrl >= NUM_OF_CTRL || dimm >= NUM_OF_DIMM)
		return 0;

	return spd_addr[ctrl][dimm];	
}

int read_spd(unsigned char chip, void *buf, int len)
{
	unsigned char dummy = 0;
	int ret;

	if (len < 256) {
		debug("Invalid SPD length\n");
		return -EINVAL;
	}

	i2c_write(SPD_SPA0_ADDRESS, 0, 1, &dummy, 1);
	ret = i2c_read(chip, 0, 1, buf, 256);
	if (!ret) {
		i2c_write(SPD_SPA1_ADDRESS, 0, 1, &dummy, 1);
		ret = i2c_read(chip, 0, 1, buf + 256, min(256, len - 256));
	}
	if (ret)
		bzero(buf, len);

	return ret;
}

int crc16(unsigned char *ptr, int count)
{
	int i;
	int crc = 0;

	while (--count >= 0) {
		crc = crc ^ (int)*ptr++ << 8;
		for (i = 0; i < 8; ++i)
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc = crc << 1;
	}
	return crc & 0xffff;
}

static int ddr4_spd_check(const struct ddr4_spd *spd)
{
	void *p = (void *)spd;
	int csum16;
	int len;
	char crc_lsb;	/* byte 126 */
	char crc_msb;	/* byte 127 */

	len = 126;
	csum16 = crc16(p, len);

	crc_lsb = (char) (csum16 & 0xff);
	crc_msb = (char) (csum16 >> 8);

	if (spd->crc[0] != crc_lsb || spd->crc[1] != crc_msb) {
		debug("SPD checksum unexpected.\n");
		debug("Checksum lsb in SPD = ");
		dbgprint_hex(spd->crc[0]);
		debug(", computed SPD = ");
		dbgprint_hex(crc_lsb);
		debug("\nChecksum msb in SPD = ");
		dbgprint_hex(spd->crc[1]);
		debug(", computed SPD = ");
		dbgprint_hex(crc_msb);
		debug("\n");
		return -EINVAL;
	}

	p = (void *)spd + 128;
	len = 126;
	csum16 = crc16(p, len);

	crc_lsb = (char) (csum16 & 0xff);
	crc_msb = (char) (csum16 >> 8);

	if (spd->mod_section.uc[126] != crc_lsb ||
	    spd->mod_section.uc[127] != crc_msb) {
		debug("SPD module checksum unexpected.\n");
		debug("Checksum lsb in SPD = ");
		dbgprint_hex(spd->mod_section.uc[126]);
		debug(", computed SPD = ");
		dbgprint_hex(crc_lsb);
		debug("\nChecksum msb in SPD = ");
		dbgprint_hex(spd->mod_section.uc[127]);
		debug(", computed SPD = ");
		dbgprint_hex(crc_msb);
		debug("\n");
		return -EINVAL;
	}

	return 0;
}

static unsigned long long
compute_ranksize(const struct ddr4_spd *spd)
{
	unsigned long long bsize;

	int nbit_sdram_cap_bsize = 0;
	int nbit_primary_bus_width = 0;
	int nbit_sdram_width = 0;
	int die_count = 0;
	bool package_3ds;

	if ((spd->density_banks & 0xf) <= 7)
		nbit_sdram_cap_bsize = (spd->density_banks & 0xf) + 28;
	if ((spd->bus_width & 0x7) < 4)
		nbit_primary_bus_width = (spd->bus_width & 0x7) + 3;
	if ((spd->organization & 0x7) < 4)
		nbit_sdram_width = (spd->organization & 0x7) + 2;
	package_3ds = (spd->package_type & 0x3) == 0x2;
	if (package_3ds)
		die_count = (spd->package_type >> 4) & 0x7;

	bsize = 1ULL << (nbit_sdram_cap_bsize - 3 +
			 nbit_primary_bus_width - nbit_sdram_width +
			 die_count);

	return bsize;
}

int cal_dimm_params(const struct ddr4_spd *spd, struct dimm_params *pdimm)
{
	int ret;
	int i;
	static const unsigned char udimm_rc_e_dq[18] = {
		0x0c, 0x2c, 0x15, 0x35, 0x15, 0x35, 0x0b, 0x2c, 0x15,
		0x35, 0x0b, 0x35, 0x0b, 0x2c, 0x0b, 0x35, 0x15, 0x36
	};
	int spd_error = 0;
	unsigned char *ptr;

	if (spd->mem_type != SPD_MEMTYPE_DDR4) {
		debug("Error: Not a DDR4 DIMM.\n");
		return -EINVAL;
	}

	ret = ddr4_spd_check(spd);
	if (ret) {
		debug("Error: DIMM SPD checksum mismatch\n");
		return -EINVAL;
	}

	/*
	 * The part name in ASCII in the SPD EEPROM is not null terminated.
	 * Guarantee null termination here by presetting all bytes to 0
	 * and copying the part name in ASCII from the SPD onto it
	 */
	if ((spd->info_size_crc & 0xF) > 2)
		memcpy(pdimm->mpart, spd->mpart, sizeof(pdimm->mpart) - 1);

	/* DIMM organization parameters */
	pdimm->n_ranks = ((spd->organization >> 3) & 0x7) + 1;
	pdimm->rank_density = compute_ranksize(spd);
	pdimm->capacity = pdimm->n_ranks * pdimm->rank_density;
	pdimm->primary_sdram_width = 1 << (3 + (spd->bus_width & 0x7));
	if ((spd->bus_width >> 3) & 0x3)
		pdimm->ec_sdram_width = 8;
	else
		pdimm->ec_sdram_width = 0;
	pdimm->data_width = pdimm->primary_sdram_width
			  + pdimm->ec_sdram_width;
	pdimm->device_width = 1 << ((spd->organization & 0x7) + 2);

	/* These are the types defined by the JEDEC SPD spec */
	switch (spd->module_type & DDR4_SPD_MODULETYPE_MASK) {
	case DDR4_SPD_MODULETYPE_RDIMM:
		/* Registered/buffered DIMMs */
		pdimm->registered_dimm = 1;
		break;

	case DDR4_SPD_MODULETYPE_UDIMM:
	case DDR4_SPD_MODULETYPE_SO_DIMM:
		/* Unbuffered DIMMs */
		if (spd->mod_section.unbuffered.addr_mapping & 0x1)
			pdimm->mirrored_dimm = 1;
		if ((spd->mod_section.unbuffered.mod_height & 0xe0) == 0 &&
		    (spd->mod_section.unbuffered.ref_raw_card == 0x04)) {
			/* Fix SPD error found on DIMMs with raw card E0 */
			for (i = 0; i < 18; i++) {
				if (spd->mapping[i] == udimm_rc_e_dq[i])
					continue;
				spd_error = 1;
				ptr = (unsigned char *)&spd->mapping[i];
				*ptr = udimm_rc_e_dq[i];
			}
			if (spd_error)
				debug("SPD DQ mapping error fixed\n");
		}
		break;

	default:
		debug("Error: unknown module_type 0x");
		dbgprint_hex(spd->module_type);
		debug("\n");
		return -EINVAL;
	}

	/* SDRAM device parameters */
	pdimm->n_row_addr = ((spd->addressing >> 3) & 0x7) + 12;
	pdimm->n_col_addr = (spd->addressing & 0x7) + 9;
	pdimm->bank_addr_bits = (spd->density_banks >> 4) & 0x3;
	pdimm->bank_group_bits = (spd->density_banks >> 6) & 0x3;

	if (pdimm->ec_sdram_width)
		pdimm->edc_config = 0x02;
	else
		pdimm->edc_config = 0x00;

	/* DDR4 spec has BL8 -bit3, BC4 -bit2 */
	pdimm->burst_lengths_bitmask = 0x0c;
	pdimm->row_density = ilog2(pdimm->rank_density);

	/* MTB - medium timebase
	 * The MTB in the SPD spec is 125ps,
	 *
	 * FTB - fine timebase
	 * use 1/10th of ps as our unit to avoid floating point
	 * eg, 10 for 1ps, 25 for 2.5ps, 50 for 5ps
	 */
	if ((spd->timebases & 0xf) == 0x0) {
		pdimm->mtb_ps = 125;
		pdimm->ftb_10th_ps = 10;

	} else {
		debug("Unknown Timebases\n");
		return -EINVAL;
	}

	/* sdram minimum cycle time */
	pdimm->tckmin_x_ps = spd_to_ps(spd->tck_min, spd->fine_tck_min);

	/* sdram max cycle time */
	pdimm->tckmax_ps = spd_to_ps(spd->tck_max, spd->fine_tck_max);

	/*
	 * CAS latency supported
	 * bit0 - CL7
	 * bit4 - CL11
	 * bit8 - CL15
	 * bit12- CL19
	 * bit16- CL23
	 */
	pdimm->caslat_x  = (spd->caslat_b1 << 7)	|
			   (spd->caslat_b2 << 15)	|
			   (spd->caslat_b3 << 23);

	if (spd->caslat_b4 != 0)
		debug("BUG: Unhandled caslat_b4 value\n");

	/*
	 * min CAS latency time
	 */
	pdimm->taa_ps = spd_to_ps(spd->taa_min, spd->fine_taa_min);

	/*
	 * min RAS to CAS delay time
	 */
	pdimm->trcd_ps = spd_to_ps(spd->trcd_min, spd->fine_trcd_min);

	/*
	 * Min Row Precharge Delay Time
	 */
	pdimm->trp_ps = spd_to_ps(spd->trp_min, spd->fine_trp_min);

	/* min active to precharge delay time */
	pdimm->tras_ps = (((spd->tras_trc_ext & 0xf) << 8) +
			  spd->tras_min_lsb) * pdimm->mtb_ps;

	/* min active to actice/refresh delay time */
	pdimm->trc_ps = spd_to_ps((((spd->tras_trc_ext & 0xf0) << 4) +
				   spd->trc_min_lsb), spd->fine_trc_min);
	/* Min Refresh Recovery Delay Time */
	pdimm->trfc1_ps = ((spd->trfc1_min_msb << 8) | (spd->trfc1_min_lsb)) *
		       pdimm->mtb_ps;
	pdimm->trfc2_ps = ((spd->trfc2_min_msb << 8) | (spd->trfc2_min_lsb)) *
		       pdimm->mtb_ps;
	pdimm->trfc4_ps = ((spd->trfc4_min_msb << 8) | (spd->trfc4_min_lsb)) *
			pdimm->mtb_ps;
	/* min four active window delay time */
	pdimm->tfaw_ps = (((spd->tfaw_msb & 0xf) << 8) | spd->tfaw_min) *
			pdimm->mtb_ps;

	/* min row active to row active delay time, different bank group */
	pdimm->trrds_ps = spd_to_ps(spd->trrds_min, spd->fine_trrds_min);
	/* min row active to row active delay time, same bank group */
	pdimm->trrdl_ps = spd_to_ps(spd->trrdl_min, spd->fine_trrdl_min);
	/* min CAS to CAS Delay Time (tCCD_Lmin), same bank group */
	pdimm->tccdl_ps = spd_to_ps(spd->tccdl_min, spd->fine_tccdl_min);

	/* 15ns for all speed bins */
	pdimm->twr_ps = 15000;

	/*
	 * Average periodic refresh interval
	 * tREFI = 7.8 us at normal temperature range
	 */
	pdimm->refresh_rate_ps = 7800000;

	for (i = 0; i < 18; i++)
		pdimm->dq_mapping[i] = spd->mapping[i];

	pdimm->dq_mapping_ors = ((spd->mapping[0] >> 6) & 0x3) == 0 ? 1 : 0;

	return 0;
}
