/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

int make_prime_test(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	int prime_size = 5;
	uint64_t prime = (uint64_t) 0xeb9f70300ull;

	LABEL(gen);
	REFERENCE(pjump1);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		/* Acquire the PKHA */
		FIFOLOAD(p, PKA, 0, 0, IMMED | COPY);
		/* Write the PKHA N size, getting it ready to load */
		LOAD(p, prime_size, PKNSZ, 0, 4, IMMED);
		/* Write the PKHA A size, getting it ready to load */
		LOAD(p, prime_size - 1, PKASZ, 0, 4, IMMED);
		SET_LABEL(p, gen);
		/* Turn off auto info-fifo entries */
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);

		/*
		   Send six bytes of random padding to DECO AB and then to
		   MATH0. The upper four will be ignored, and the next two
		   will become the first and last bytes of our candidate.
		 */
		NFIFOADD(p, PAD, MSG, 6, PAD_RANDOM | LAST1);
		MOVE(p, ABD, 0, MATH0, 0, 6, WAITCOMP | IMMED);
		/* Turn on MSb of first byte and LSb of last byte */
		MATHB(p, MATH0, OR, 0x80010000, MATH0, 4, IMMED2);
		/* Send the first and last bytes to the input data fifo */
		MOVE(p, MATH0, 4, IFIFOAB1, 0, 2, IMMED);

		/* Send MSB from Input FIFO to PKHA N */
		NFIFOADD(p, IFIFO, PKN, 1, 0);
		/* Send middle bytes to PKHA N */
		NFIFOADD(p, PAD, PKN, (prime_size - 2), PAD_RANDOM | EXT);
		/* Send LSB byte from Input FIFO to PKHA N */
		NFIFOADD(p, IFIFO, PKN, 1, FLUSH1);
		/* Turn on auto info-fifo entries */
		LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_NFIFO, 0, IMMED);
		/*
		 * Now set up other inputs size the PRIME_TEST test
		 * Send random seed to PKHA A
		 */
		NFIFOADD(p, PAD, PKA, (prime_size - 1),
			 PAD_RANDOM | FLUSH1 | EXT);
		/* Miller-Rabin iteration count (either-endian) */
		FIFOLOAD(p, PKB, 0x07, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_PRIMALITY);
		pjump1 = JUMP(p, gen, LOCAL_JUMP, ANY_FALSE, PK_PRIME);
		FIFOSTORE(p, PKN, 0, prime, prime_size, 0);
	}
	PATCH_JUMP(p, pjump1, gen);

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("Make prime example program\n");
	rta_set_sec_era(RTA_SEC_ERA_3);
	size = make_prime_test(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
