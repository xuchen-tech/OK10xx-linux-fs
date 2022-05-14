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

int test_fifo_load_op(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	uint64_t msg = 0x12004201;
	uint64_t aad_ptr = 0x200;
	uint64_t src_dma = 0x800;
	uint64_t iv_ptr = 0x02230104;
	uint64_t icv_ptr = 0xf3230d01;
	uint64_t auth_ptr = 0x13422591;
	uint64_t mod = 0x24128820;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);

	FIFOLOAD(p, MSG1, msg, 1, FLUSH1);
	FIFOLOAD(p, MSG1, msg, 2, LAST1);
	FIFOLOAD(p, MSG2, msg, 3, LAST2);
	FIFOLOAD(p, MSG2, msg, 4, 0);

	FIFOLOAD(p, BIT_DATA, msg, 4003, CLASS1 | FLUSH1);
	FIFOLOAD(p, BIT_DATA, msg, 4003, CLASS2 | LAST2 | EXT);

	FIFOLOAD(p, MSGOUTSNOOP, src_dma, 0x212, LAST1 | LAST2 | SGF);
	FIFOLOAD(p, MSGINSNOOP, src_dma, 0x212, LAST1 | LAST2 | SGF);
	FIFOLOAD(p, AAD1, aad_ptr, 800, SGF);
	FIFOLOAD(p, AAD1, aad_ptr, 800, 0);
	FIFOLOAD(p, ICV1, icv_ptr, 12, LAST1);
	FIFOLOAD(p, ICV1, icv_ptr, 12, FLUSH1 | LAST2 | BOTH | EXT);
	FIFOLOAD(p, ICV1, icv_ptr, 12, FLUSH1);
	FIFOLOAD(p, ICV2, icv_ptr, 12, LAST2);
	FIFOLOAD(p, ICV2, icv_ptr, 12, LAST2);
	FIFOLOAD(p, IV1, iv_ptr, 12, LAST1);
	FIFOLOAD(p, IV1, iv_ptr, 12, LAST1);

	FIFOLOAD(p, MSGINSNOOP, 0x616263, 3, LAST1 | LAST2 | IMMED);
	if (rta_sec_era >= RTA_SEC_ERA_3)
		FIFOLOAD(p, IFIFO, auth_ptr, 12, 0);
	SEQFIFOLOAD(p, SKIP, 0, VLF);
	FIFOLOAD(p, PKA, 0, 0, IMMED | COPY);
	SEQFIFOLOAD(p, PKN, 0, 0);

	FIFOLOAD(p, PKA, 0x07, 1, IMMED);
	FIFOLOAD(p, PKB, 0x07, 1, IMMED);
	FIFOLOAD(p, PKA0, 0x07, 1, IMMED);
	FIFOLOAD(p, PKA1, 0x03, 1, IMMED);
	FIFOLOAD(p, PKA2, 0x04, 1, IMMED);
	FIFOLOAD(p, PKA3, 0x12131415, 4, IMMED);
	FIFOLOAD(p, PKB0, 0x07, 1, IMMED);
	FIFOLOAD(p, PKB1, 0x03, 1, IMMED);
	FIFOLOAD(p, PKB2, 0x04, 1, IMMED);
	FIFOLOAD(p, PKB3, 0x12131415, 4, IMMED);
	FIFOLOAD(p, PKN, mod, 12, 0);

	return PROGRAM_FINALIZE(p);
}

int main(int argc, char **argv)
{
	int size;

	pr_debug("FIFOLOAD program\n");
	rta_set_sec_era(RTA_SEC_ERA_3);
	size = test_fifo_load_op(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
