/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

uint64_t pkha_make_rsa_keys_phys = (uint64_t) 0x08eb2f00ul;
uint64_t pkha_make_rsa_p_q_phys = (uint64_t) 0xe6580300ul;
uint64_t pkha_make_rsa_check_pq_phys = (uint64_t) 0xf97183c0ul;
uint64_t pkha_make_rsa_d_n_phys = (uint64_t) 0x88a51280ul;

uint64_t prv_key_p = (uint64_t) 0x9b8a2580ul;
uint64_t prv_key_q = (uint64_t) 0x07c6ca00ul;
uint64_t pub_key_n = (uint64_t) 0xb759c700ul;
uint64_t prv_key_d = (uint64_t) 0xe427fd00ul;
uint64_t max_n = (uint64_t) 0xaee57e00ul;
uint64_t pub_key_e = (uint64_t) 0x9cfe8100ul;

uint16_t e_size = 3;		/* input public key length */
uint16_t n_size = 128;		/* configuration parameter for RSA-nnnn */
uint16_t pq_size = 64;

int jdesc_pkha_make_rsa_p_q(struct program *p, uint32_t *buff, int buffpos)
{
	uint64_t pq_count = (uint64_t) 0x318d7f00ul;

	LABEL(retry);
	REFERENCE(pjump1);
	REFERENCE(pjump5);
	LABEL(short_key);
	REFERENCE(pjump2);
	LABEL(store_q);
	REFERENCE(pjump3);
	LABEL(now_do_q);
	REFERENCE(pjump4);

	if (buffpos < 0)
		return -EINVAL;

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		MATHB(p, ZERO, ADD, MATH0, MATH1, 4, 0); /* try counter */
		MATHB(p, ZERO, ADD, ONE, MATH3, 8, 0);	/* p / q marker */
		SET_LABEL(p, now_do_q);
		SET_LABEL(p, retry);
		MATHB(p, MATH1, SUB, ONE, MATH1, 8, 0);	/* trycounter-- */
		/* fail on too many tries */
		JUMP(p, 0x42, HALT_STATUS, ALL_TRUE, MATH_N);

		/* Get PKHA sizes ready to load values */
		LOAD(p, pq_size, PKNSZ, 0, 4, IMMED);
		LOAD(p, pq_size - 1, PKASZ, 0, 4, IMMED);

		/* Generate 4 MSB and 1 LSB random bytes for our candidate */
		NFIFOADD(p, PAD, MSG, 5, PAD_RANDOM | LAST1);
		MOVE(p, IFIFOABD, 0, MATH2, 0, 5, WAITCOMP | IMMED);
		/* Make it odd */
		MATHB(p, MATH2, OR, 0x8000000001000000, MATH2, 8, IMMED2);
		/* Compare it to sqrt(2) * 2^pq_size ... */
		MATHB(p, MATH2, SUB, 0xb504f333ff000000, NONE, 8, IMMED2);
		pjump1 = JUMP(p, retry, LOCAL_JUMP, ANY_TRUE, MATH_Z | MATH_N);

		/* Put the five bytes into the ififo */
		MOVE(p, MATH2, 0, IFIFOAB1, 0, 4, IMMED);
		MOVE(p, MATH2, 4, IFIFOAB1, 0, 1, IMMED);
		/* And the first four on into pkn */
		NFIFOADD(p, IFIFO, PKN, 4, 0);
		/* skip this next if we're doing very short RSA */
		MATHB(p, SEQOUTSZ, SUB, 5, NONE, 4, IMMED2);
		pjump2 = JUMP(p, short_key, LOCAL_JUMP, ANY_TRUE,
			      MATH_Z | MATH_N);

		/* Generate random 'middle bytes' for our candidate */
		NFIFOADD(p, PAD, PKN, (pq_size - 5), PAD_RANDOM | EXT);
		SET_LABEL(p, short_key);
		NFIFOADD(p, IFIFO, PKN, 1, FLUSH1);
		/* Generate random 'miller-rabin seed' for pka */
		NFIFOADD(p, PAD, PKA, (pq_size - 1),
			 PAD_RANDOM | FLUSH1 | EXT);
		/* Put our 'miller-rabin trial count' into pkb */
		LOAD(p, 1, PKBSZ, 0, 4, IMMED);
		MOVE(p, MATH0, 0, IFIFOAB1, 0, 1, WAITCOMP | IMMED);
		NFIFOADD(p, IFIFO, PKB, 1, FLUSH1);
		/* Crunch */
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_PRIMALITY);
		pjump5 = JUMP(p, retry, LOCAL_JUMP, ANY_FALSE, PK_PRIME);
		/* p / q test */
		MATHB(p, MATH3, SUB, ONE, MATH3, 4, 0);
		pjump3 = JUMP(p, store_q, LOCAL_JUMP, ALL_TRUE, MATH_N);
		FIFOSTORE(p, PKN, 0, prv_key_p, pq_size, 0);
		pjump4 = JUMP(p, now_do_q, LOCAL_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, store_q);
		FIFOSTORE(p, PKN, 0, prv_key_q, pq_size, 0);

		/* pq_count accounting */
		MATHB(p, MATH0, SUB, MATH1, MATH1, 4, 0);
		STORE(p, MATH1, 4, pq_count, 4, 0);
		JUMP(p, pkha_make_rsa_check_pq_phys, FAR_JUMP, ALL_TRUE, 0);
	}
	PATCH_JUMP(p, pjump1, retry);
	PATCH_JUMP(p, pjump2, short_key);
	PATCH_JUMP(p, pjump3, store_q);
	PATCH_JUMP(p, pjump4, now_do_q);
	PATCH_JUMP(p, pjump5, retry);

	return PROGRAM_FINALIZE(p);
}

int jdesc_pkha_make_rsa_check_pq(struct program *p, uint32_t *buff, int buffpos)
{
	LABEL(check_2);
	REFERENCE(pjump1);
	LABEL(do_over);
	REFERENCE(pjump2);
	REFERENCE(pjump3);
	LABEL(pq_ok);
	REFERENCE(pjump4);
	REFERENCE(pjump5);
	REFERENCE(pjump6);

	if (buffpos < 0)
		return -EINVAL;

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_NFIFO, 0, IMMED);
		/* Finish FIFOSTORE of pkn so FIFOLOAD of pkn doesn't get
		 * confused */
		SEQFIFOSTORE(p, MSG, 0, 0, 0);

		/*
		 * Make sure p and q are not 'too close'; they must differ
		 * within the most significant 100 bits.
		 *
		 * Bad when bits are all zero or all 1.
		 */
		FIFOLOAD(p, PKN, max_n, (pq_size + 1), 0);
		FIFOLOAD(p, PKA, prv_key_p, (pq_size), 0);
		FIFOLOAD(p, PKB, prv_key_q, (pq_size), 0);
		/* p - q % fffffffffff */
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_SUB_AB);
		LOAD(p, CCTRL_UNLOAD_PK_B, CCTRL, 0, 4, IMMED);
		/* Get interesting bits */
		MOVE(p, OFIFO, 0, MATH2, 0, 16, WAITCOMP | IMMED);
		/* Trash remaining bits */
		MOVE(p, OFIFO, 0, CONTEXT1, 0, pq_size - 16 + 1,
		     WAITCOMP | IMMED);
		MATHB(p, ZERO, OR, MATH2, NONE, 8, 0);
		pjump1 = JUMP(p, check_2, LOCAL_JUMP, ALL_TRUE, MATH_N);
		pjump2 = JUMP(p, do_over, LOCAL_JUMP, ALL_TRUE, MATH_Z);
		MATHB(p, MATH3, AND, 0xfffffffffff00000, NONE, 8, IMMED2);

		pjump3 = JUMP(p, do_over, LOCAL_JUMP, ALL_TRUE, MATH_Z);
		pjump4 = JUMP(p, pq_ok, LOCAL_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, check_2);
		MATHB(p, MATH2, XOR, 0xffffffffffffffff, NONE, 8, IMMED2);
		pjump5 = JUMP(p, pq_ok, LOCAL_JUMP, ANY_FALSE, MATH_Z);

		MATHB(p, MATH3, AND, 0xfffffffffff00000, MATH3, 8, IMMED2);
		MATHB(p, MATH3, XOR, 0xfffffffffff00000, NONE, 8, IMMED2);
		pjump6 = JUMP(p, pq_ok, LOCAL_JUMP, ANY_FALSE, MATH_Z);

		SET_LABEL(p, do_over);
		JUMP(p, pkha_make_rsa_keys_phys, FAR_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, pq_ok);
		FIFOLOAD(p, PKN, prv_key_q, pq_size, 0);
		JUMP(p, pkha_make_rsa_d_n_phys, FAR_JUMP, ALL_TRUE, 0);
	}
	PATCH_JUMP(p, pjump1, check_2);
	PATCH_JUMP(p, pjump2, do_over);
	PATCH_JUMP(p, pjump3, do_over);
	PATCH_JUMP(p, pjump4, pq_ok);
	PATCH_JUMP(p, pjump5, pq_ok);
	PATCH_JUMP(p, pjump6, pq_ok);

	return PROGRAM_FINALIZE(p);
}

int jdesc_pkha_make_rsa_keys(struct program *p, uint32_t *buff,
			     unsigned buffpos)
{
	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		/* Configure Miller-Rabin test count ||  max random prime
		 * attempts */
		MATHB(p, ZERO, ADD, 0x1000000000004000, MATH0, 8, IMMED2);
		MATHB(p, ZERO, ADD, 10, VSEQINSZ, 4, IMMED2);
		MATHB(p, ZERO, ADD, pq_size, SEQOUTSZ, 4, IMMED2);
		FIFOLOAD(p, PKA, 0, 0, IMMED | COPY);	/* Acquire the PKHA */
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		JUMP(p, pkha_make_rsa_p_q_phys, FAR_JUMP, ALL_TRUE, 0);
	}

	return PROGRAM_FINALIZE(p);
}

int jdesc_pkha_make_rsa_d_n(struct program *p, uint32_t *buff, int buffpos)
{
	LABEL(phi_e_relatively_prime);
	REFERENCE(pjump1);

	if (buffpos < 0)
		return -EINVAL;

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		PKHA_OPERATION(p, OP_ALG_PKMODE_COPY_NSZ_N_B);
		FIFOLOAD(p, PKA, 0x01, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_SUB_BA);
		FIFOSTORE(p, PKB, 0, prv_key_q, pq_size, 0);
		FIFOLOAD(p, PKN, max_n, n_size, 0);
		FIFOLOAD(p, PKB, prv_key_p, pq_size, 0);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_SUB_BA);
		FIFOLOAD(p, PKA, prv_key_q, pq_size, 0);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_MULT);
		PKHA_OPERATION(p, OP_ALG_PKMODE_COPY_SSZ_B_N);
		FIFOLOAD(p, PKA, pub_key_e, e_size, 0);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_GCD);
		pjump1 = JUMP(p, phi_e_relatively_prime, LOCAL_JUMP, ALL_TRUE,
			      PK_GCD_1);
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		JUMP(p, pkha_make_rsa_keys_phys, FAR_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, phi_e_relatively_prime);
		FIFOLOAD(p, PKA, pub_key_e, e_size, 0);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_INV);
		FIFOSTORE(p, PKB, 0, prv_key_d, n_size, 0);
		SEQFIFOSTORE(p, MSG, 0, 0, 0);
		FIFOLOAD(p, PKA, prv_key_q, pq_size, 0);
		FIFOLOAD(p, PKB, 0x01, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_ADD);
		FIFOSTORE(p, PKB, 0, prv_key_q, pq_size, 0);
		FIFOLOAD(p, PKN, max_n, n_size, 0);
		FIFOLOAD(p, PKA, prv_key_p, pq_size, 0);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_MULT);
		FIFOSTORE(p, PKB, 0, pub_key_n, n_size, 0);
	}
	PATCH_JUMP(p, pjump1, phi_e_relatively_prime);

	return PROGRAM_FINALIZE(p);
}

int main(int argc, char **argv)
{
	uint32_t make_rsa_keys[64];
	uint32_t make_rsa_p_q[64];
	uint32_t make_rsa_check_pq[64];
	uint32_t make_rsa_d_n[64];

	int rsa_keys_size, rsa_p_q_size, rsa_check_pq_size, rsa_d_n_size;

	struct program rsa_keys_prgm;
	struct program rsa_p_q_prgm;
	struct program rsa_check_p_q_prgm;
	struct program rsa_d_n_prgm;

	rta_set_sec_era(RTA_SEC_ERA_1);

	memset(make_rsa_keys, 0, sizeof(make_rsa_keys));
	rsa_keys_size =
	    jdesc_pkha_make_rsa_keys(&rsa_keys_prgm, make_rsa_keys, 0);

	memset(make_rsa_p_q, 0, sizeof(make_rsa_p_q));
	rsa_p_q_size =
	    jdesc_pkha_make_rsa_p_q(&rsa_p_q_prgm, make_rsa_p_q, rsa_keys_size);

	memset(make_rsa_check_pq, 0, sizeof(make_rsa_check_pq));
	rsa_check_pq_size =
	    jdesc_pkha_make_rsa_check_pq(&rsa_check_p_q_prgm,
					 make_rsa_check_pq, rsa_p_q_size);

	memset(make_rsa_d_n, 0, sizeof(make_rsa_d_n));
	rsa_d_n_size =
	    jdesc_pkha_make_rsa_d_n(&rsa_d_n_prgm, make_rsa_d_n,
				    rsa_check_pq_size);

	pr_debug("Make RSA KEYS program\n");
	pr_debug("size = %d\n", rsa_keys_size);
	print_prog((uint32_t *)make_rsa_keys, rsa_keys_size);

	pr_debug("size = %d\n", rsa_p_q_size);
	print_prog((uint32_t *)make_rsa_p_q, rsa_p_q_size);

	pr_debug("size = %d\n", rsa_check_pq_size);
	print_prog((uint32_t *)make_rsa_check_pq, rsa_check_pq_size);

	pr_debug("size = %d\n", rsa_d_n_size);
	print_prog((uint32_t *)make_rsa_d_n, rsa_d_n_size);

	return 0;
}
