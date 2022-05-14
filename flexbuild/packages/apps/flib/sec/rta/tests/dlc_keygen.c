/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

uint8_t modulus[20] = {
	0x9A, 0x27, 0x7B, 0x10, 0x42, 0x3F, 0xEC, 0xDB,
	0x0F, 0x3B, 0x1A, 0x52, 0x2A, 0x61, 0x2D, 0x14,
	0x6E, 0xA3, 0x45, 0x4D
};

uint8_t exponent[20] = {
	0x52, 0x06, 0xF8, 0x66, 0xFF, 0x00, 0x2A, 0x71,
	0x1F, 0xCF, 0x40, 0x4D, 0x58, 0x9F, 0xEC, 0x74,
	0xB1, 0xBB, 0x82, 0xF1
};

int dlc_keygen(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	int field_size = 20;
	uint64_t mod = 0x08049668;	/* I/O address for modulus */
	uint64_t private_key = 0x32244514ull;
	uint64_t public_key = 0x32244514ull;

	LABEL(retry);
	REFERENCE(p1_retry);
	REFERENCE(p2_retry);
	REFERENCE(p3_retry);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	PROGRAM_SET_36BIT_ADDR(p);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);

	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		/* Step A. Load the modulus (prime) we will be using for DH. */
		FIFOLOAD(p, PKN, mod, field_size, 0);	/* N <= Modulus */

		/* Step B. Generate random private key. */
		/* Note that it is 'good practice' to generate eight (8) more
		 * bytes than desired, to avoid skew.  This is obviously not
		 * possible when the desired length wouldn't fit into the PKHA
		 */
		SET_LABEL(p, retry);
		/* Step B.1. Prime 'PKHA A SIZE' register with number of bytes
		 * to expect */
		LOAD(p, field_size + 8, PKASZ, 0, 4, IMMED);

		/* Step B.2. Generate random 'value' for private key */
		NFIFOADD(p, PAD, PKA, (field_size + 8), PAD_RANDOM | EXT);

		/* Step B.3. Reduce the private key down to within the
		 * modulus */
		/* B <= private key */
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_REDUCT);
		/* 'Good practice' says to make sure this value is not 0, 1,
		 * or p-1 ... */
		/* Retry if key is 0 */
		p1_retry = JUMP(p, retry, LOCAL_JUMP, ALL_TRUE, PK_0);

		/* Step B.4. Store the private key for later use. */
		FIFOSTORE(p, PKB, 0, private_key, field_size, 0);

		FIFOLOAD(p, PKA, 0x01, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_ADD);
		/* Retry if key is -1 */
		p2_retry = JUMP(p, retry, LOCAL_JUMP, ALL_TRUE, PK_0);

		FIFOLOAD(p, PKA, 0x02, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_SUB_BA);
		/* Retry if key is 1 */
		p3_retry = JUMP(p, retry, LOCAL_JUMP, ALL_TRUE, PK_0);

		/* Step C.  Generate the public key */
		/* A <= Generator (2) */
		FIFOLOAD(p, PKA, 0x02, 1, IMMED);
		/* E <= Private key */
		PKHA_OPERATION(p, OP_ALG_PKMODE_COPY_NSZ_B_E);
		/* B <= Public Key */
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_EXPO_TEQ);

		FIFOSTORE(p, PKB, 0, public_key, field_size, 0);
	}
	PATCH_JUMP(p, p1_retry, retry);
	PATCH_JUMP(p, p2_retry, retry);
	PATCH_JUMP(p, p3_retry, retry);

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("DLC KEYGEN program\n");
	rta_set_sec_era(RTA_SEC_ERA_1);
	size = dlc_keygen(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
