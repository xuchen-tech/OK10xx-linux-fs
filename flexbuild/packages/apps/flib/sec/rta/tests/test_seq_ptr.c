/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;
uint32_t prg_buff[1000];

int test_seq_ptr_op(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	int data_size = 512;
	uint64_t data_in = 0x1234567;
	uint64_t data_out = 0x6123475;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	SEQINPTR(p, data_in, data_size, 0);
	SEQOUTPTR(p, data_out, data_size, 0);
	SEQINPTR(p, 0, 0, RTO);
	SEQOUTPTR(p, 0, 0, RTO);

	return PROGRAM_FINALIZE(p);
}

int main(int argc, char **argv)
{
	int size;

	pr_debug("SEQ PTR program\n");
	rta_set_sec_era(RTA_SEC_ERA_2);
	size = test_seq_ptr_op(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
