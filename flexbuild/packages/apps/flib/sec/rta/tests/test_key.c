/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

int test_key_op(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	uint8_t key_imm[] = { 0x12, 0x13, 0x14, 0x15 };
	uintptr_t addr = (uintptr_t) &key_imm;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	KEY(p, MDHA_SPLIT_KEY, ENC, addr, 4, IMMED);
	KEY(p, MDHA_SPLIT_KEY, ENC, addr, 4, 0);
	KEY(p, KEY1, EKT, addr, 4, IMMED);

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("KEY program\n");
	rta_set_sec_era(RTA_SEC_ERA_2);
	size = test_key_op(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
