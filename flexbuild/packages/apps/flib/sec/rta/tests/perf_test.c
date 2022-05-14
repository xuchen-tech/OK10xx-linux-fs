/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

int test_perf(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	int seqnum = 1;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	LABEL(test1);
	LABEL(test2);
	LABEL(encap_iv);
	LABEL(seqoutptr);
	LABEL(new_seqinptr);

	{
		SHR_HDR(p, SHR_ALWAYS, 0, 0);
		WORD(p, 0x17feff00);	/* type 0x17 / protocol version */
		WORD(p, 0x00010000);	/* Epoch / upper bits of SeqNum */
		WORD(p, seqnum);	/* Lower bits of SeqNum */

		JUMP(p, test1, LOCAL_JUMP, ALL_TRUE, 0);
		MATHB(p, MATH0, ADD, 0x0840010008880000, MATH3, 8, IMMED2);
		MATHB(p, 0x08400100009990, XOR, MATH1, MATH3, 8, IMMED);
		SET_LABEL(p, test1);
		MATHB(p, 0x0840010000aaa0, XOR, MATH1, MATH3, 8, IMMED);
		SET_LABEL(p, test2);
		MATHB(p, MATH2, XOR, MATH1, MATH3, 4, 0);
		MATHU(p, MATH2, BSWAP, MATH3, 2, NFU);
		JUMP(p, test2, LOCAL_JUMP, ALL_TRUE, 0);
		MATHU(p, MATH0, BSWAP, MATH1, 8, 0);
		/* SYNC (NO_PEND_INPUT); */

		LOAD(p, 4, ICV1SZ, 0, 4, IMMED);
		LOAD(p, 4, ICV2SZ, 0, 4, IMMED);
		MATHU(p, MATH2, BSWAP, MATH3, 2, NFU);
		MATHB(p, 0x0840010000aaa0, XOR, MATH1, MATH3, 8, IMMED);
		MATHU(p, MATH2, BSWAP, MATH3, 2, NFU);
		WORD(p, 0x17feff00);	/* type 0x17 / protocol version */
		WORD(p, 0x00010000);	/* Epoch / upper bits of SeqNum */
		WORD(p, seqnum);	/* Lower bits of SeqNum */
		JUMP(p, test1, LOCAL_JUMP, ALL_TRUE, 0);
		MATHB(p, MATH0, XOR, 0x0840010008880000, MATH3, 8, IMMED2);
		MATHB(p, 0x08400100009990, XOR, MATH1, MATH3, 8, IMMED);
		MATHB(p, 0x0840010000aaa0, XOR, MATH1, MATH3, 8, IMMED);
		MATHB(p, MATH2, XOR, MATH1, MATH3, 4, 0);
		MATHU(p, MATH2, BSWAP, MATH3, 2, NFU);
		JUMP(p, test2, LOCAL_JUMP, ALL_TRUE, 0);
		MATHU(p, MATH0, BSWAP, MATH1, 8, 0);
		/* SYNC (NO_PEND_INPUT); */
		LOAD(p, 4, ICV2SZ, 0, 4, IMMED);
		LOAD(p, 4, ICV1SZ, 0, 4, IMMED);
		MATHU(p, MATH2, BSWAP, MATH3, 2, NFU);
		MATHU(p, MATH2, BSWAP, MATH3, 2, NFU);
		MATHB(p, 0x0840010000aaa0, XOR, MATH1, MATH3, 8, IMMED);
		MATHU(p, MATH2, BSWAP, MATH3, 2, NFU);

		SET_LABEL(p, encap_iv);
		/* All of the IV, both next and previous */
		COPY_DATA(p, ((uint8_t[]) { 00, 00}), 2);

		MOVE(p, DESCBUF, seqoutptr, MATH0, 0, 16, WAITCOMP | IMMED);
		MATHB(p, MATH0, XOR, 0x0840010000000000, MATH0, 8, IMMED2);
		MOVE(p, MATH0, 0, DESCBUF, new_seqinptr, 8, IMMED);
		SET_LABEL(p, new_seqinptr);
		WORD(p, 0x0);
		WORD(p, 0x0);
		/* NOP(); */
		SEQLOAD(p, MATH2, 0, 8, 0);
		/* JUMP_COND(all[calm], ADD1); */
		MATHB(p, MATH0, XOR, MATH2, NONE, 8, 0);
		MATHB(p, MATH1, XOR, MATH3, NONE, 8, 0);
		/* HALT_COND(all[z], 255); */
		MOVE(p, MATH0, 0, DESCBUF, encap_iv, 32, IMMED);
		seqoutptr = 8;
	}

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("Perf example program\n");
	rta_set_sec_era(RTA_SEC_ERA_4);
	size = test_perf(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
