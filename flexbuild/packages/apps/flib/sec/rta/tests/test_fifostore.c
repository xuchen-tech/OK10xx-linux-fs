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

int test_fifo_store_op(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	uint64_t loc = 0x1224445;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	FIFOSTORE(p, MSG, 0, loc, 100000000, 0);
	FIFOSTORE(p, RNG, 0, loc, 32, 0);
	FIFOSTORE(p, RNGOFIFO, 0, 0, 1, 0);
	FIFOSTORE(p, PKA, 0, loc, 10, 0);
	FIFOSTORE(p, PKA0, 0, loc, 10, 0);
	FIFOSTORE(p, PKA1, 0, loc, 10, 0);
	FIFOSTORE(p, PKA2, 0, loc, 40, 0);
	FIFOSTORE(p, PKA3, 0, loc, 10, 0);
	FIFOSTORE(p, PKB, 0, loc, 10, 0);
	FIFOSTORE(p, PKB0, 0, loc, 10, 0);
	FIFOSTORE(p, PKB1, 0, loc, 10, 0);
	FIFOSTORE(p, PKB2, 0, loc, 10, 0);
	FIFOSTORE(p, PKB3, 0, loc, 10, 0);
	FIFOSTORE(p, PKN, 0, loc, 10, 0);
	/* Encrypted (keys) section - normal */
	FIFOSTORE(p, KEY1, 0, loc, 32, 0);
	FIFOSTORE(p, KEY2, 0, loc, 16, 0);
	FIFOSTORE(p, PKE, 0, loc, 10, 0);
	FIFOSTORE(p, AFHA_SBOX, 0, loc, 258, 0);
	FIFOSTORE(p, MDHA_SPLIT_KEY, 0, loc, 128, 0);
	/* Encrypted (keys) section - enhanced */
	FIFOSTORE(p, KEY1, EKT, loc, 32, 0);
	FIFOSTORE(p, KEY2, EKT, loc, 16, 0);
	FIFOSTORE(p, PKE, EKT, loc, 10, 0);
	FIFOSTORE(p, AFHA_SBOX, EKT, loc, 258, 0);
	FIFOSTORE(p, MDHA_SPLIT_KEY, EKT, loc, 128, 0);
	/* Encrypted (keys) section - trusted */
	FIFOSTORE(p, KEY1, TK, loc, 32, 0);
	FIFOSTORE(p, KEY2, TK, loc, 16, 0);
	FIFOSTORE(p, PKE, TK, loc, 10, 0);
	FIFOSTORE(p, AFHA_SBOX, TK, loc, 258, 0);
	FIFOSTORE(p, MDHA_SPLIT_KEY, TK, loc, 128, 0);
	/* Encrypted (keys) section - enhanced, trusted */
	FIFOSTORE(p, KEY1, EKT | TK, loc, 32, 0);
	FIFOSTORE(p, KEY2, EKT | TK, loc, 16, 0);
	FIFOSTORE(p, PKE, EKT | TK, loc, 10, 0);
	FIFOSTORE(p, AFHA_SBOX, EKT | TK, loc, 258, 0);
	FIFOSTORE(p, MDHA_SPLIT_KEY, EKT | TK, loc, 128, 0);
	SEQFIFOSTORE(p, MSG, 0, 12, CONT);
	SEQFIFOSTORE(p, MSG, 0, 0, VLF);
	SEQFIFOSTORE(p, SKIP, 0, 33, 0);
	SEQFIFOSTORE(p, SKIP, 0, 0, VLF);
	SEQFIFOSTORE(p, METADATA, 0, 24, 0);
	SEQFIFOSTORE(p, METADATA, 0, 0, VLF);
	SEQFIFOSTORE(p, METADATA, 0, 13, CLASS1);
	SEQFIFOSTORE(p, METADATA, 0, 0, CLASS1 | VLF);
	SEQFIFOSTORE(p, METADATA, 0, 24, CLASS2);
	SEQFIFOSTORE(p, METADATA, 0, 0, CLASS2 | VLF);
	SEQFIFOSTORE(p, METADATA, 0, 92, BOTH);
	SEQFIFOSTORE(p, METADATA, 0, 0, BOTH | VLF);

	return PROGRAM_FINALIZE(p);
}

int main(int argc, char **argv)
{
	int size;

	pr_debug("FIFOSTORE program\n");
	rta_set_sec_era(RTA_SEC_ERA_5);
	size = test_fifo_store_op(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
