/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

int test_signature_op(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	{
		SIGNATURE(p, SIGN_TYPE_IMM_2);
		SIGNATURE(p, SIGN_TYPE_IMM_3);
		SIGNATURE(p, SIGN_TYPE_IMM_4);
		SIGNATURE(p, SIGN_TYPE_FINAL);
		DWORD(p, 0x0000000000000000);
		DWORD(p, 0x0000000000000000);
		DWORD(p, 0x0000000000000000);
		DWORD(p, 0x0000000000000000);

		SIGNATURE(p, SIGN_TYPE_FINAL_RESTORE);
		DWORD(p, 0x0000000000000000);
		DWORD(p, 0x0000000000000000);
		DWORD(p, 0x0000000000000000);
		DWORD(p, 0x0000000000000000);

		SIGNATURE(p, SIGN_TYPE_FINAL_NONZERO);
		DWORD(p, 0x0000000000000000);
		DWORD(p, 0x0000000000000000);
		DWORD(p, 0x0000000000000000);
		DWORD(p, 0x0000000000000000);

		SIGNATURE(p, SIGN_TYPE_FINAL);
		DWORD(p, 0x123454320002222);
		DWORD(p, 0x123454320002222);
		DWORD(p, 0x123454320002222);
		DWORD(p, 0x123454320002222);

		SIGNATURE(p, SIGN_TYPE_FINAL_RESTORE);
		DWORD(p, 0x123454320002222);
		DWORD(p, 0x123454320002222);
		DWORD(p, 0x123454320002222);
		DWORD(p, 0x123454320002222);

		SIGNATURE(p, SIGN_TYPE_FINAL_NONZERO);
		DWORD(p, 0x123454320002222);
		DWORD(p, 0x123454320002222);
		DWORD(p, 0x123454320002222);
		DWORD(p, 0x123454320002222);
	}

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("SIGNATURE program\n");
	rta_set_sec_era(RTA_SEC_ERA_1);
	size = test_signature_op(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
