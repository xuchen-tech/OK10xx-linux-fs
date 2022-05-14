/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

/* Subroutine to populate a Descriptor buffer */
int build_rsa_verify_desc(uint32_t *buff, uint32_t n_len, uint32_t e_len,
			  const uint64_t n, const uint64_t e, const uint64_t f,
			  const uint64_t g)
{
	struct program prg;
	struct program *p = &prg;
	LABEL(pdb_end);

	PROGRAM_SET_36BIT_ADDR(p);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		{	/* RSA Verify */
			WORD(p, ((e_len << 12) + n_len)); /* e len, n len */
			DWORD(p, f);	/* output */
			DWORD(p, g);	/* input */
			DWORD(p, n);	/* modulus */
			DWORD(p, e);	/* public expnenent */
			WORD(p, n_len);	/* g length */
			SET_LABEL(p, pdb_end);
		}
		PROTOCOL(p, OP_TYPE_UNI_PROTOCOL, OP_PCLID_RSAENCRYPT, 0);
	}
	PATCH_HDR(p, 0, pdb_end);

	return PROGRAM_FINALIZE(p);
}

int test_rsa_verify(uint32_t *buff)
{
	uint32_t n_len = 128;
	uint32_t e_len = 3;

	uint64_t buf_phys = 0x200320024ull;
	uint64_t e = (uint64_t) buf_phys;	/* 1st vector in buffer */
	uint64_t n = e + e_len;	/* 2nd                  */
	uint64_t g = n + n_len;	/* 3rd                  */
	uint64_t f = g;		/* 4th overwrites 3rd   */

	return build_rsa_verify_desc(buff, n_len, e_len, n, e, f, g);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("RSA Verify example program\n");
	rta_set_sec_era(RTA_SEC_ERA_2);
	size = test_rsa_verify(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
