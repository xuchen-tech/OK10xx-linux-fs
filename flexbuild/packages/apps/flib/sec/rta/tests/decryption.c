/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

int test_decryption(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	uint64_t data_in = (uint64_t) 0x82ba84e1cull;
	uint64_t data_out = (uint64_t) 0x582007840ull;
	uint64_t key_data = (uint64_t) 0xdd5fa8880ull;
	uint64_t iv = (uint64_t) 0x8c0b21d80ull;
	int datasz = 1024;
	int ivlen = 16;
	int keylen = 16;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	PROGRAM_SET_36BIT_ADDR(p);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);

	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		LOAD(p, iv, CONTEXT1, 0, ivlen, 0);
		KEY(p, KEY1, 0, key_data, keylen, 0);
		ALG_OPERATION(p, OP_ALG_ALGSEL_AES, OP_ALG_AAI_CBC,
			      OP_ALG_AS_INITFINAL, 0, DIR_DEC);
		FIFOLOAD(p, MSG1, data_in, datasz, LAST1);
		FIFOSTORE(p, MSG, 0, data_out, datasz, EXT);
	}

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("Decryption program\n");
	rta_set_sec_era(RTA_SEC_ERA_1);
	size = test_decryption(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
