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
 * Example AES-CBC encrypt blockcipher-using-sequence pointer
 */
int test_blkcipher_seq(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	void *data_in = (void *)0x1234567;
	void *data_out = (void *)0x6123475;
	void *key_addr = (void *)0x778899AA;
	void *iv = (void *)0x11223344;
	int datasz = 512;
	int ivlen = 16;
	int keylen = 32;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	SEQINPTR(p, (uintptr_t) data_in, datasz, 0);
	SEQOUTPTR(p, (uintptr_t) data_out, datasz, 0);
	LOAD(p, (uintptr_t) iv, CONTEXT1, 0, ivlen, 0);
	KEY(p, KEY1, 0, (uintptr_t) key_addr, keylen, 0);
	MATHB(p, SEQINSZ, ADD, ZERO, VSEQINSZ, 4, 0);
	MATHB(p, SEQOUTSZ, ADD, ZERO, VSEQOUTSZ, 4, 0);
	ALG_OPERATION(p, OP_ALG_ALGSEL_AES, OP_ALG_AAI_CBC, 0, 0, DIR_ENC);
	SEQFIFOLOAD(p, MSG1, 0, VLF | LAST1);
	SEQFIFOSTORE(p, MSG, 0, 0, VLF);

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("BLKCIPHER SEQ program\n");
	rta_set_sec_era(RTA_SEC_ERA_3);
	size = test_blkcipher_seq(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
