/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;
int shdesc_len = 5;

int pointer_size_1(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, shdesc_len, 200, SHR);
	{
		FIFOLOAD(p, PKN, 0x4, 4, 0);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_EXPO);
	}

	return PROGRAM_FINALIZE(p);
}

int pointer_size_2(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	PROGRAM_SET_36BIT_ADDR(p);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);

	JOB_HDR(p, SHR_NEVER, shdesc_len, 200, SHR);
	{
		FIFOLOAD(p, PKN, 0x4, 4, 0);
		PKHA_OPERATION(p, OP_ALG_PKMODE_MOD_EXPO);
	}

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	rta_set_sec_era(RTA_SEC_ERA_1);

	pr_debug("POINTER_SIZE_1 example program\n");
	size = pointer_size_1(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	pr_debug("POINTER_SIZE_2 example program\n");
	size = pointer_size_2(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
