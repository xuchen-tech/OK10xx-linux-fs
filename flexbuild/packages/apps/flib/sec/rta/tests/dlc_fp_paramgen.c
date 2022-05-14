/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

/*
  Example job to create DLC/DSA Domain Parameters for q=p

  This runs on SEC Era 2 or later (as long as there is a PKHA)

  Algorithm is modeled after IEEE Std 1363-2000, A.16.1, except:
    -- A substitute method for q generation A.15.7
    -- rmin/rmax are fixed for a  given r_size
    -- pmin/pmax are fixed for a  given q_size
    -- It does not handle the test of k for use with DHC or MQVC

  This will generate values of length 8*size bits for q and r
*/

enum rta_sec_era rta_sec_era;

int r_size = 20;		/* 160 bits */
int q_size = 128;
uint64_t dom_r_addr = 0x51d121510ull;
uint64_t dom_q_addr = 0x7afd02ed0ull;
uint64_t dom_g_addr = 0x6393df32aull;

uint64_t generate_dlc_fp_params_addr = 0x7891c4c75ull;
uint64_t make_X_addr = 0x861776ed8ull;
uint64_t make_q_addr = 0x4b633925aull;
uint64_t make_g_addr = 0x7aa7742f1ull;

int generate_dlc_fp_params(struct program *p, uint32_t *buff)
{
	LABEL(new_r);
	REFERENCE(ref_new_r);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	PROGRAM_SET_36BIT_ADDR(p);

	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		SEQFIFOLOAD(p, PKA, 0, 0);	/* acquire / wake up the PKHA */

		/* 1. Generate a random prime r */
		SET_LABEL(p, new_r);
		LOAD(p, r_size, PKNSZ, 0, 4, IMMED);
		LOAD(p, r_size - 1, PKASZ, 0, 4, IMMED);
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		/* Get random MSB and LSB do abd */
		NFIFOADD(p, PAD, MSG, 2, PAD_RANDOM | LAST1);
		/* Now into math0 */
		MOVE(p, IFIFOABD, 0, MATH0, 4, 2, WAITCOMP | IMMED);
		/* Turn on MSb and LSb */
		MATHB(p, MATH0, OR, 0x80010000, MATH0, 4, IMMED2);
		/* Send them to the ififo */
		MOVE(p, MATH0, 4, IFIFOAB1, 0, 2, IMMED);
		/* Send MSB to pkn */
		NFIFOADD(p, IFIFO, PKN, 1, 0);
		/* and middle random bytes */
		NFIFOADD(p, PAD, PKN, (r_size - 2), PAD_RANDOM | EXT);
		/* Send LSB to pkn (with flush1, so loading finishes) */
		NFIFOADD(p, IFIFO, PKN, 1, FLUSH1);
		LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_NFIFO, 0, IMMED);
		/* Random seed for the Miller-Rabin primality test */
		NFIFOADD(p, PAD, PKA, (r_size - 1),
			 FLUSH1 | PAD_RANDOM | EXT);
		/* Iteration count */
		FIFOLOAD(p, PKB, 0x32, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_PRIMALITY);

		/* If r did not test prime, go try another */
		ref_new_r = JUMP(p, new_r, LOCAL_JUMP, ANY_FALSE, PK_PRIME);
		FIFOSTORE(p, PKN, 0, dom_r_addr, r_size, 0);

		/* Set up maximum number of tries to compute a q from this r */
		MATHB(p, ZERO, ADD, 4096, MATH3, 4, IMMED2);
		JUMP(p, make_X_addr, FAR_JUMP, ALL_TRUE, 0);
	}
	PATCH_JUMP(p, ref_new_r, new_r);

	return PROGRAM_FINALIZE(p);
}

int dlc_fp_make_x(struct program *p, uint32_t *buff)
{
	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	PROGRAM_SET_36BIT_ADDR(p);

	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		/* 2. Generate a random prime q */
		LOAD(p, q_size, PKNSZ, 0, 4, IMMED);
		/*
		 * Start with a random value X with MSb and LSb set
		 * (This is the same 'generate random' recipe as used for r,
		 * but with a different size)
		 */
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		NFIFOADD(p, PAD, MSG, 2, PAD_RANDOM | LAST1);
		MOVE(p, IFIFOABD, 0, MATH0, 4, 2, WAITCOMP | IMMED);
		MATHB(p, MATH0, OR, 0x80010000, MATH0, 4, IMMED2);
		MOVE(p, MATH0, 4, IFIFOAB1, 0, 2, IMMED);
		NFIFOADD(p, IFIFO, PKN, 1, 0);
		NFIFOADD(p, PAD, PKN, (q_size - 2), EXT | PAD_RANDOM);
		NFIFOADD(p, IFIFO, PKN, 1, FLUSH1);
		LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_NFIFO, 0, IMMED);

		/* Let X finish loading into pkn before storing it */
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NIFP);
		FIFOSTORE(p, PKN, 0, dom_q_addr, q_size, 0);
		JUMP(p, make_q_addr, FAR_JUMP, ALL_TRUE, 0);
	}

	return PROGRAM_FINALIZE(p);
}

int dlc_fp_make_q(struct program *p, uint32_t *buff)
{
	LABEL(store_q);
	REFERENCE(ref_store_q);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	PROGRAM_SET_36BIT_ADDR(p);

	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		/* Calculate 2r */
		FIFOLOAD(p, PKA, dom_r_addr, r_size, 0);
		PKHA_OPERATION(p, OP_ALG_PKMODE_CLEARMEM_B);
		PKHA_OPERATION(p, OP_ALG_PKMODE_COPY_NSZ_A_B);
		/* 2r  ( % X) */
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_ADD);
		PKHA_OPERATION(p, OP_ALG_PKMODE_COPY_NSZ_B_N);

		/* Calculate c = X % 2r   ( % X) */
		FIFOLOAD(p, PKA, dom_q_addr, q_size, 0);	/* X */
		/* Set pknsz to byte count of 2*r -always one more than r */
		LOAD(p, r_size + 1, PKNSZ, 0, 4, IMMED);
		/* c = X % 2r  ( % X) */
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_REDUCT);

		/*
		 * q = X - c + 1   ( % X)
		 * This guarantees that (q - 1) % 2r === 0
		 */
		/* X */
		FIFOLOAD(p, PKN, dom_q_addr, q_size, 0);
		PKHA_OPERATION(p, OP_ALG_PKMODE_CLEARMEM_A);
		PKHA_OPERATION(p, OP_ALG_PKMODE_COPY_SSZ_B_A);
		/* c */
		PKHA_OPERATION(p, OP_ALG_PKMODE_COPY_NSZ_N_B);
		/* X - c   ( % X) */
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_SUB_BA);
		FIFOLOAD(p, PKA, 0x01, 1, IMMED);
		/* q = X - c + 1   ( % X) */
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_ADD);

		/* Set up prime test for q */
		PKHA_OPERATION(p, OP_ALG_PKMODE_COPY_NSZ_B_N);
		LOAD(p, q_size - 1, PKASZ, 0, 4, IMMED);
		NFIFOADD(p, PAD, PKA, q_size - 1, PAD_RANDOM | EXT | FLUSH1);
		FIFOLOAD(p, PKB, 0x14, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_PRIMALITY);

		ref_store_q = JUMP(p, store_q, LOCAL_JUMP, ALL_TRUE, PK_PRIME);

		/* Decrement try-X counter */
		MATHB(p, MATH3, SUB, ONE, MATH3, 4, 0);
		/* Go get another X */
		JUMP(p, make_X_addr, FAR_JUMP, ANY_FALSE, MATH_Z);
		/* Start over with a new r */
		JUMP(p, generate_dlc_fp_params_addr, FAR_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, store_q);
		FIFOSTORE(p, PKN, 0, dom_q_addr, q_size, 0);
		JUMP(p, make_g_addr, FAR_JUMP, ALL_TRUE, 0);
	}
	PATCH_JUMP(p, ref_store_q, store_q);

	return PROGRAM_FINALIZE(p);
}

int dlc_fp_make_g(struct program *p, uint32_t *buff)
{
	LABEL(h_loop);
	REFERENCE(ref_h_loop);
	LABEL(found_g);
	REFERENCE(ref_found_g);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	PROGRAM_SET_36BIT_ADDR(p);

	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		/*
		 * 3. Let k = (q - 1) / r
		 * Compute 1/r % q  ( % q )
		 */
		FIFOLOAD(p, PKA, dom_r_addr, r_size, 0);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_INV);	/* 1/r */
		FIFOSTORE(p, PKB, 0, dom_g_addr, q_size, 0); /* tmp */

		/* Compute q-1  ( % q ) */
		PKHA_OPERATION(p, OP_ALG_PKMODE_COPY_NSZ_N_A);
		FIFOLOAD(p, PKB, 0x01, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_SUB_AB);

		/* k = q-1 / r  ( % q ) */
		FIFOLOAD(p, PKA, dom_g_addr, q_size, 0);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_MULT);

		PKHA_OPERATION(p, OP_ALG_PKMODE_COPY_NSZ_B_E);

		/*
		 * Algorithm to look for g
		 * h = 2
		 * do {
		 * G = h ^ k mod q
		 * if (G != 1) break
		 * h++
		 * } while true
		 */
		FIFOLOAD(p, PKB, 0x02, 1, IMMED);	/* h = 2 */
		PKHA_OPERATION(p, OP_ALG_PKMODE_CLEARMEM_A);
		/* get q-sized 2 into pka */
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_ADD | OP_ALG_PKMODE_OUT_A);

		/* 5. Choose an integer h, not already chosen, satisfying
		 * 1 < h < q - 1 */
		SET_LABEL(p, h_loop);
		/* save h */
		FIFOSTORE(p, PKB, 0, dom_g_addr, q_size, 0);
		/* 6. Compute g = h^e % q */
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_EXPO);

		/*
		 * 7. If g = 1 then go to step 5
		 * Test g != 1 by calculating checking (g-1) != 0
		 */
		FIFOLOAD(p, PKA, 0x01, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_SUB_BA);
		ref_found_g = JUMP(p, found_g, LOCAL_JUMP, ANY_FALSE, PK_0);

		/* H++ */
		FIFOLOAD(p, PKB, dom_g_addr, q_size, 0);
		FIFOLOAD(p, PKA, 0x01, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_ADD);
		ref_h_loop = JUMP(p, h_loop, LOCAL_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, found_g);

		/* Restore g = g+1 */
		FIFOLOAD(p, PKA, 0x01, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_ADD);

		/* 8. Output q, r, g */
		FIFOSTORE(p, PKB, 0, dom_g_addr, q_size, 0);
	}
	PATCH_JUMP(p, ref_h_loop, h_loop);
	PATCH_JUMP(p, ref_found_g, found_g);

	return PROGRAM_FINALIZE(p);
}

int main(int argc, char **argv)
{
	uint32_t gen_dlc_fp_desc[64];
	uint32_t make_x_desc[64];
	uint32_t make_q_desc[64];
	uint32_t make_g_desc[64];

	struct program gen_dlc_fp_prgm;
	struct program make_x_prgm;
	struct program make_q_prgm;
	struct program make_g_prgm;
	int size;

	rta_set_sec_era(RTA_SEC_ERA_1);

	memset(&gen_dlc_fp_desc, 0, sizeof(gen_dlc_fp_desc));
	memset(&make_x_desc, 0, sizeof(make_x_desc));
	memset(&make_q_desc, 0, sizeof(make_q_desc));
	memset(&make_g_desc, 0, sizeof(make_g_desc));

	pr_debug("Generate DLC FP domain parameters\n");
	size = generate_dlc_fp_params(&gen_dlc_fp_prgm, gen_dlc_fp_desc);
	pr_debug("size = %d\n", size);
	print_prog(gen_dlc_fp_desc, size);

	pr_debug("Make X\n");
	size = dlc_fp_make_x(&make_x_prgm, make_x_desc);
	pr_debug("size = %d\n", size);
	print_prog(make_x_desc, size);

	pr_debug("Make q\n");
	size = dlc_fp_make_q(&make_q_prgm, make_q_desc);
	pr_debug("size = %d\n", size);
	print_prog(make_q_desc, size);

	pr_debug("Make g\n");
	size = dlc_fp_make_g(&make_g_prgm, make_g_desc);
	pr_debug("size = %d\n", size);
	print_prog(make_g_desc, size);

	return 0;
}
