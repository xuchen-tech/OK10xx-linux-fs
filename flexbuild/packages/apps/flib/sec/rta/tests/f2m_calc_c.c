/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

int f2m_calc_c_test(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;

	LABEL(bit_search_top);
	REFERENCE(pjump1);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	SHR_HDR(p, SHR_NEVER, 0, 0);
	{
		/*
		 * Compute c = b^(2^(m-2)) for 'b' and input poly q (modulus)
		 * The 'input frame' consists of q followed b.  The output
		 * frame will contain 'c'.  All vectors will be / must be
		 * length of q in bytes.
		 * 1) Compute m and load the Polynomial
		 *    Get byte count / 2 (i.e., q size)
		 */
		MATHB(p, SEQINSZ, RSHIFT, ONE, VSEQINSZ, 4, 0);
		MATHB(p, VSEQINSZ, SUB, ONE, MATH2, 4, 0);

		/* Test for 'too big' (to fit into 4096-bit PKHA memory...) */
		MATHB(p, MATH2, RSHIFT, 9, MATH1, 4, IMMED2);
		JUMP(p, 0x12, HALT_STATUS, ANY_FALSE, MATH_Z);

		/* Direct first byte of polynomial to MATH1 via Class 1 CTX */
		SEQLOAD(p, CONTEXT1, 0, 1, 0);
		/* wait for load to complete */
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NIP);
		MOVE(p, CONTEXT1, 0, MATH1, 0, 1, IMMED);

		/*
		 * Load full polynomial into PKHA N RAM (after recomputing full
		 * byte count)
		 */
		SEQINPTR(p, 0, 1, RTO); /* Back up the to the beginning */
		MATHB(p, MATH2, ADD, ONE, VSEQINSZ, 4, 0);
		SEQFIFOLOAD(p, PKN, 0, VLF);

		/* Start with m = 8 * (poly_size - 1) - 1 */
		MATHB(p, MATH2, LSHIFT, 3, MATH0, 4, IMMED2);
		MATHB(p, MATH2, SUB, ONE, MATH2, 4, 0);
		/* Move first byte from MSB to LSB */
		MATHB(p, MATH1, RSHIFT, 56, MATH1, 8, IFB | IMMED2);
		JUMP(p, 0x13, HALT_STATUS, ALL_TRUE, MATH_Z);

		SET_LABEL(p, bit_search_top);
		MATHB(p, MATH0, ADD, ONE, MATH0, 2, 0);	/* m = m + 1 */
		MATHB(p, MATH1, RSHIFT, ONE, MATH1, 2, 0); /* byte >> 1 */
		pjump1 = JUMP(p, bit_search_top, LOCAL_JUMP, ANY_FALSE, MATH_Z);

		MATHB(p, MATH0, SUB, 3, MATH0, 2, IMMED2);	/* m-2 */
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		LOAD(p, 2, PKESZ, 0, 4, IMMED);
		MATHB(p, MATH0, LSHIFT, 48, MATH0, 8, IFB | IMMED2);
		MOVE(p, MATH0, 0, IFIFOAB1, 0, 2, IMMED);
		NFIFOADD(p, IFIFO, PKE, 2, FLUSH1);
		LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_NFIFO, 0, IMMED);

		/* pke is now 'm-2'; load 2 into pka */
		FIFOLOAD(p, PKA, 0x02, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_EXPO);
		PKHA_OPERATION(p, OP_ALG_PKMODE_COPY_SSZ_B_E);

		/* Now load curve parameter b */
		MATHB(p, SEQINSZ, ADD, ZERO, VSEQINSZ, 4, 0);
		SEQFIFOLOAD(p, PKA, 0, VLF);
		PKHA_OPERATION(p, OP_ALG_PKMODE_F2M_EXP);

		MATHB(p, SEQOUTSZ, ADD, ZERO, VSEQOUTSZ, 4, 0);
		SEQFIFOSTORE(p, PKB, 0, 0, VLF);
	}
	PATCH_JUMP(p, pjump1, bit_search_top);

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("F2M_calc_c example program\n");
	rta_set_sec_era(RTA_SEC_ERA_3);
	size = f2m_calc_c_test(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
