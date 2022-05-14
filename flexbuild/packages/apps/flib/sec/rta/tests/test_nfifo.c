/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

int test_nfifo_op(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	/* From Input Data FIFO to 'all' targets */
	NFIFOADD(p, IFIFO, MSG1, 2045, 0);
	NFIFOADD(p, IFIFO, MSG1, 2046, LAST1);
	NFIFOADD(p, IFIFO, MSG1, 2047, FLUSH1);
	NFIFOADD(p, IFIFO, MSG1, 2048, LAST1 | FLUSH1);

	NFIFOADD(p, IFIFO, MSG2, 2043, LAST2);

	NFIFOADD(p, IFIFO, MSG2, 2044, FLUSH2);
	NFIFOADD(p, IFIFO, MSG2, 2045, 0);

	NFIFOADD(p, IFIFO, IV1, 45, 0);
	NFIFOADD(p, IFIFO, IV1, 46, LAST1);
	NFIFOADD(p, IFIFO, IV1, 47, FLUSH1);
	NFIFOADD(p, IFIFO, IV1, 48, LAST1 | FLUSH1);

	NFIFOADD(p, IFIFO, ICV1, 45, 0);
	NFIFOADD(p, IFIFO, ICV1, 46, LAST1);
	NFIFOADD(p, IFIFO, ICV1, 47, FLUSH1);
	NFIFOADD(p, IFIFO, ICV1, 48, LAST1 | FLUSH1);

	NFIFOADD(p, IFIFO, ICV2, 44, LAST2);
	NFIFOADD(p, IFIFO, ICV2, 45, 0);

	NFIFOADD(p, IFIFO, PKA, 512, 0);
	NFIFOADD(p, IFIFO, PKA0, 512, FLUSH1);
	NFIFOADD(p, IFIFO, PKA1, 512, FLUSH1);
	NFIFOADD(p, IFIFO, PKA2, 512, 0);
	NFIFOADD(p, IFIFO, PKA3, 512, FLUSH1);

	NFIFOADD(p, IFIFO, PKB, 512, 0);
	NFIFOADD(p, IFIFO, PKB0, 512, FLUSH1);
	NFIFOADD(p, IFIFO, PKB1, 512, 0);
	NFIFOADD(p, IFIFO, PKB2, 512, FLUSH1);
	NFIFOADD(p, IFIFO, PKB3, 512, 0);

	NFIFOADD(p, IFIFO, PKN, 512, 0);
	NFIFOADD(p, IFIFO, PKE, 512, FLUSH1);

	NFIFOADD(p, IFIFO, AFHA_SBOX, 258, 0);

	/* From Output Data FIFO to 'all' targets */
	NFIFOADD(p, OFIFO, MSG1, 24, 0);
	/* From Outsnooping to 'all' targets (really all types) */
	NFIFOADD(p, MSGOUTSNOOP, MSG, 2045, 0);

	/* From Padding Block to 'all' targets */
	NFIFOADD(p, PAD, AB1, 16, PAD_ZERO);
	NFIFOADD(p, PAD, AB1, 16, PAD_NONZERO);
	NFIFOADD(p, PAD, AB1, 16, PAD_INCREMENT);
	NFIFOADD(p, PAD, AB1, 16, PAD_RANDOM);
	NFIFOADD(p, PAD, AB1, 16, PAD_ZERO_N1);
	NFIFOADD(p, PAD, AB1, 16, PAD_NONZERO_0);
	NFIFOADD(p, PAD, AB1, 16, PAD_N1);
	NFIFOADD(p, PAD, AB1, 16, PAD_NONZERO_N);

	/* From Alt Source to 'all' targets */
	NFIFOADD(p, ALTSOURCE, IV1, 24, FLUSH1);
	NFIFOADD(p, IFIFO, MSG, 2045, EXT);

	/* Boundary padding */
	NFIFOADD(p, IFIFO, ICV1, 45, BP | PAD_ZERO);
	NFIFOADD(p, IFIFO, ICV1, 45, BP | PAD_INCREMENT);
	NFIFOADD(p, PAD, AB1, 16, BP | PAD_NONZERO_N);

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("NFIFO ADD program\n");
	rta_set_sec_era(RTA_SEC_ERA_2);
	size = test_nfifo_op(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
