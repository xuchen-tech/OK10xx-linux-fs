/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

int test_jump(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	char test_data_char[13] = "My Imm Data\n";
	uint32_t test_data = (uint32_t) &test_data_char;

	LABEL(test1);
	REFERENCE(ptest1);
	LABEL(test2);
	REFERENCE(ptest2);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	{
		SHR_HDR(p, SHR_ALWAYS, 0, 0);
		ptest1 = JUMP(p, test1, LOCAL_JUMP, ALL_TRUE, 0);
		MATHB(p, MATH0, XOR, 0x0840010008880000, MATH3, 8, IMMED2);
		MATHB(p, 0x08400100009990, XOR, MATH1, MATH3, 8, IMMED);
		SET_LABEL(p, test1);
		MATHB(p, 0x0840010000aaa0, XOR, MATH1, MATH3, 8, IMMED);
		SET_LABEL(p, test2);
		MATHB(p, MATH2, XOR, MATH1, MATH3, 4, 0);
		MATHU(p, MATH2, BSWAP, MATH3, 2, NFU);
		ptest2 = JUMP(p, test2, LOCAL_JUMP, ALL_TRUE, 0);
		MATHU(p, MATH0, BSWAP, MATH1, 8, 0);
		LOAD(p, test_data, CONTEXT2, 0, 8, IMMED);

		PATCH_JUMP(p, ptest1, test1);
		PATCH_JUMP(p, ptest2, test2);
	}

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("JUMP program\n");
	rta_set_sec_era(RTA_SEC_ERA_4);
	size = test_jump(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
