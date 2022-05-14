/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

uint8_t *iv1 = (uint8_t *)0x64;	/* input constant */
uint8_t *ipad = (uint8_t *)0x664;
uint8_t *opad = (uint8_t *)0x12364;

int build_hmacprecomp(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	uint8_t *hmac_key = (uint8_t *)(0x12);	/* input */
	unsigned int hmac_key_len = 18;	/* input */
	uint32_t *hmacprecompute_opad_phys = (uint32_t *)0xaabb2200;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		LOAD(p, (uintptr_t) hmac_key, CONTEXT1, 0, hmac_key_len, 0);
		MATHB(p, ZERO, XOR, 0x3636363636363636, MATH0, 8, IMMED2);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NCP);

		/* Apply the IPAD to the key */
		MOVE(p, CONTEXT1, 0, MATH1, 0, 24, WAITCOMP | IMMED);
		MATHB(p, MATH1, XOR, MATH0, MATH1, 8, 0);
		MATHB(p, MATH2, XOR, MATH0, MATH2, 8, 0);
		MATHB(p, MATH3, XOR, MATH0, MATH3, 8, 0);
		MOVE(p, MATH1, 0, CONTEXT1, 0, 24, WAITCOMP | IMMED);

		MOVE(p, CONTEXT1, 24, MATH1, 0, 24, WAITCOMP | IMMED);
		MATHB(p, MATH1, XOR, MATH0, MATH1, 8, 0);
		MATHB(p, MATH2, XOR, MATH0, MATH2, 8, 0);
		MATHB(p, MATH3, XOR, MATH0, MATH3, 8, 0);
		MOVE(p, MATH1, 0, CONTEXT1, 24, 24, WAITCOMP | IMMED);

		MOVE(p, CONTEXT1, 48, MATH1, 0, 24, WAITCOMP | IMMED);
		MATHB(p, MATH1, XOR, MATH0, MATH1, 8, 0);
		MATHB(p, MATH2, XOR, MATH0, MATH2, 8, 0);
		MATHB(p, MATH3, XOR, MATH0, MATH3, 8, 0);
		MOVE(p, MATH1, 0, CONTEXT1, 48, 24, WAITCOMP | IMMED);

		MOVE(p, CONTEXT1, 72, MATH1, 0, 24, WAITCOMP | IMMED);
		MATHB(p, MATH1, XOR, MATH0, MATH1, 8, 0);
		MATHB(p, MATH2, XOR, MATH0, MATH2, 8, 0);
		MATHB(p, MATH3, XOR, MATH0, MATH3, 8, 0);
		MOVE(p, MATH1, 0, CONTEXT1, 72, 24, WAITCOMP | IMMED);

		/* Init hash IV appropriate to desired algorithm */
		LOAD(p, (uintptr_t) iv1, CONTEXT2, 0, 64, 0);

		/* Process padded key as first half of first block of data */
		MOVE(p, CONTEXT1, 0, IFIFOAB2, 0, 64, IMMED);
		ALG_OPERATION(p, OP_ALG_ALGSEL_SHA512, OP_ALG_AAI_HASH,
			      OP_ALG_AS_UPDATE, 0, 0);

		/* Send IPAD as rest of block */
		MATHB(p, ZERO, XOR, MATH0, MATH1, 8, 0);
		MATHB(p, ZERO, XOR, MATH0, MATH2, 8, 0);
		MATHB(p, ZERO, XOR, MATH0, MATH3, 8, 0);
		MOVE(p, MATH0, 0, IFIFOAB2, 0, 32, IMMED);
		MOVE(p, MATH0, 0, IFIFOAB2, 0, 32, LAST2 | IMMED);

		/* This happens when MDHA is finished */
		STORE(p, CONTEXT2, 0, (uintptr_t) ipad, 64, 0);
		JUMP(p, (uintptr_t) hmacprecompute_opad_phys, FAR_JUMP,
		     ALL_TRUE, 0);
	}

	return PROGRAM_FINALIZE(p);
}

int build_hmacprecomp_opad(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		LOAD(p, CLRW_CLR_C2CTX | CLRW_CLR_C2MODE | CLRW_CLR_C2DATAS,
		     CLRW, 0, 4, IMMED);
		LOAD(p, CCTRL_RESET_CHA_MDHA, CCTRL, 0, 4, IMMED);

		/* Apply OPAD^IPAD to the key */
		MATHB(p, MATH1, XOR, 0x5C5C5C5C5C5C5C5C, MATH1, 8, IMMED2);

		MOVE(p, CONTEXT1, 0, MATH1, 0, 24, WAITCOMP | IMMED);
		MATHB(p, MATH1, XOR, MATH0, MATH1, 8, 0);
		MATHB(p, MATH2, XOR, MATH0, MATH2, 8, 0);
		MATHB(p, MATH3, XOR, MATH0, MATH3, 8, 0);
		MOVE(p, MATH1, 0, CONTEXT1, 0, 24, WAITCOMP | IMMED);

		MOVE(p, CONTEXT1, 24, MATH1, 0, 24, WAITCOMP | IMMED);
		MATHB(p, MATH1, XOR, MATH0, MATH1, 8, 0);
		MATHB(p, MATH2, XOR, MATH0, MATH2, 8, 0);
		MATHB(p, MATH3, XOR, MATH0, MATH3, 8, 0);
		MOVE(p, MATH1, 0, CONTEXT1, 24, 24, WAITCOMP | IMMED);

		MOVE(p, CONTEXT1, 48, MATH1, 0, 24, WAITCOMP | IMMED);
		MATHB(p, MATH1, XOR, MATH0, MATH1, 8, 0);
		MATHB(p, MATH2, XOR, MATH0, MATH2, 8, 0);
		MATHB(p, MATH3, XOR, MATH0, MATH3, 8, 0);
		MOVE(p, MATH1, 0, CONTEXT1, 48, 24, WAITCOMP | IMMED);

		MOVE(p, CONTEXT1, 72, MATH1, 0, 24, WAITCOMP | IMMED);
		MATHB(p, MATH1, XOR, MATH0, MATH1, 8, 0);
		MATHB(p, MATH2, XOR, MATH0, MATH2, 8, 0);
		MATHB(p, MATH3, XOR, MATH0, MATH3, 8, 0);
		MOVE(p, MATH1, 0, CONTEXT1, 72, 24, WAITCOMP | IMMED);

		/* Process padded key as first block of data */
		LOAD(p, (uintptr_t) iv1, CONTEXT2, 0, 64, 0);
		MOVE(p, CONTEXT1, 0, IFIFOAB2, 0, 64, IMMED);
		ALG_OPERATION(p, OP_ALG_ALGSEL_SHA512, OP_ALG_AAI_HASH,
			      OP_ALG_AS_UPDATE, 0, 0);

		/* Send OPAD as rest of block */
		MATHB(p, ZERO, XOR, 0x5C5C5C5C5C5C5C5C, MATH0, 8, IMMED2);
		MATHB(p, ZERO, XOR, MATH0, MATH1, 8, 0);
		MATHB(p, ZERO, ADD, MATH0, MATH2, 8, 0);
		MATHB(p, ZERO, OR, MATH0, MATH3, 8, 0);
		MOVE(p, MATH0, 0, IFIFOAB2, 0, 32, IMMED);
		MOVE(p, MATH0, 0, IFIFOAB2, 0, 32, IMMED);
		MOVE(p, MATH0, 0, IFIFOAB2, 0, 32, LAST2 | IMMED);

		/* This happens when MDHA is finished */
		STORE(p, CONTEXT2, 0, (uintptr_t) opad, 0, 0);
	}

	return PROGRAM_FINALIZE(p);
}

int build_hmac_from_pre(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	uint8_t *innerhash = (uint8_t *)0x28;	/* debug */
	uint8_t *mac = (uint8_t *)12228;
	void *msg = (void *)0x12341234ul;	/* physical ptr */
	unsigned int maclen = 28;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		/* Set up current state of inner hash */
		LOAD(p, (uintptr_t) ipad, CONTEXT2, 0, 64, 0);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NIP);
		LOAD(p, 0x0000000000000080, CONTEXT2, 64, 8, IMMED);

		FIFOLOAD(p, MSG2, (uintptr_t) msg, 0, LAST2);
		ALG_OPERATION(p, OP_ALG_ALGSEL_SHA512, OP_ALG_AAI_HASH,
			      OP_ALG_AS_FINALIZE, 0, 0);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, CLASS2);

		LOAD(p, CLRW_CLR_C2MODE | CLRW_CLR_C2DATAS, CLRW, 0, 4, IMMED);
		LOAD(p, CCTRL_RESET_CHA_MDHA, CCTRL, 0, 4, IMMED);

		STORE(p, CONTEXT2, 0, (uintptr_t) innerhash, maclen, 0);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NOP);

		/* Make inner hash be the message for the outer hash */
		MOVE(p, CONTEXT2, 0, IFIFOAB2, 0, maclen, LAST2 | IMMED);
		/* Set up current state of outer hash */
		LOAD(p, (uintptr_t) opad, CONTEXT2, 0, 64, 0);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NIP);

		LOAD(p, 0x0000000000000080, CONTEXT2, 64, 8, IMMED);
		ALG_OPERATION(p, OP_ALG_ALGSEL_SHA512, OP_ALG_AAI_HASH,
			      OP_ALG_AS_FINALIZE, 0, 0);
		STORE(p, CONTEXT2, 0, (uintptr_t) mac, maclen, 0);
	}

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	rta_set_sec_era(RTA_SEC_ERA_1);

	pr_debug("HMAC_By_Hand #1 JD program\n");
	size = build_hmacprecomp(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	pr_debug("HMAC_By_Hand #2 JD program\n");
	size = build_hmacprecomp_opad(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	pr_debug("HMAC_By_Hand #3 JD program\n");
	size = build_hmac_from_pre(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
