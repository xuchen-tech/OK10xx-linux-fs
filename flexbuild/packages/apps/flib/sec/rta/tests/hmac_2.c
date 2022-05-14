/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

int hmac_2(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	uint64_t key_data = (uint64_t) 0xab8050640ull;
	int keylen = 61;
	uint64_t msg = (uint64_t) 0x806d8f600ull;
	int msglen = 1601;
	uint64_t icv = (uint64_t) 0xde5ebe080ull;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	PROGRAM_SET_36BIT_ADDR(p);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);

	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		KEY(p, KEY2, 0, key_data, keylen, 0);
		ALG_OPERATION(p, OP_ALG_ALGSEL_SHA256, OP_ALG_AAI_HMAC,
			      OP_ALG_AS_INITFINAL, ICV_CHECK_ENABLE, DIR_DEC);
		FIFOLOAD(p, MSG2, msg, msglen, LAST2 | EXT);
		FIFOLOAD(p, ICV2, icv, 32, LAST2);
	}

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("HMAC_2 example program\n");
	rta_set_sec_era(RTA_SEC_ERA_2);
	size = hmac_2(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
