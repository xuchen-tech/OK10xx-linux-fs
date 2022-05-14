/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

/*
 *  Test variable length of immediate byte data in a command.
 *  This only works in Style 3.
 */
uint8_t modulus[20] = {
	0x9A, 0x27, 0x7B, 0x10, 0x42, 0x3F, 0xEC, 0xDB,
	0x0F, 0x3B, 0x1A, 0x52, 0x2A, 0x61, 0x2D, 0x14,
	0x6E, 0xA3, 0x45, 0x4D
};				/* [] modulus */

uint8_t secret[20] = {
	0x42, 0x35, 0x1a, 0x45, 0x67, 0x71, 0x54, 0xf4,
	0xb3, 0x2f, 0xc9, 0x98, 0xac, 0x42, 0x11, 0x43,
	0x53, 0xA2, 0x10, 0x08
};				/* [] secret */

int var_test(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	int secret_len = sizeof(secret);
	uint64_t secret_out = 0x3000200;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	PROGRAM_SET_36BIT_ADDR(p);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	/* RSA Encrypt */
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		FIFOLOAD(p, PKN, (uintptr_t) &modulus, (sizeof(modulus)),
			 IMMED | COPY);
		FIFOLOAD(p, PKA, (uintptr_t) &secret, secret_len,
			 IMMED | COPY);
		KEY(p, PKE, 0, 0x03, 1, IMMED);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_EXPO);
		FIFOSTORE(p, PKB, 0, secret_out, (sizeof(modulus)), 0);
	}

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("VAR_1 example program\n");
	rta_set_sec_era(RTA_SEC_ERA_1);
	size = var_test(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
