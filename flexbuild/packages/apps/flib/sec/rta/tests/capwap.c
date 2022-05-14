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

int generate_capwap_code(uint32_t *buff, unsigned mdatalen)
{
	struct program prg;
	struct program *p = &prg;
	uint8_t key_imm[] = { 0x12, 0x13, 0x14, 0x15 };
	uintptr_t key_addr = (uintptr_t) &key_imm;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);

	LABEL(encap_iv);
	LABEL(previous_iv);
	LABEL(new_IV_OK);
	LABEL(seqnum);
	LABEL(seqoutptr);
	LABEL(new_seqinptr);
	LABEL(skip_keyloading);
	LABEL(descbuf);

	REFERENCE(ref1);
	REFERENCE(ref2);
	REFERENCE(pjump1);
	REFERENCE(pjump2);

	SHR_HDR(p, SHR_ALWAYS, 0, 0);
	WORD(p, 0x17feff00);	/* type 0x17 / protocol version */
	WORD(p, 0x00010000);	/* Epoch / upper bits of SeqNum */
	WORD(p, seqnum);		/* Lower bits of SeqNum */
	SET_LABEL(p, encap_iv);
	/* Location of the extra, custom part of PDB */
	previous_iv = 16;
	/* All of the IV, both next and previous */
	COPY_DATA(p, ((uint8_t[]){ 00, 00}), 2);

	ref1 = MOVE(p, DESCBUF, 0, MATH0, 0, 16, WAITCOMP | IMMED);
	MATHB(p, MATH0, XOR, 0x0840010000000000, MATH0, 8, IMMED2);
	ref2 = MOVE(p, MATH0, 0, DESCBUF, 0, 8, IMMED);
	MOVE(p, IFIFOABD, 0, OFIFO, 0, mdatalen, IMMED);
	SEQFIFOSTORE(p, MSG, 0, mdatalen, 0);

	pjump2 = JUMP(p, 0, LOCAL_JUMP, ALL_TRUE, SHRD);
	KEY(p, MDHA_SPLIT_KEY, ENC, key_addr, 4, IMMED);
	KEY(p, KEY1, EKT, key_addr, 4, IMMED);
	SET_LABEL(p, skip_keyloading);
	ALG_OPERATION(p, OP_ALG_ALGSEL_AES, OP_ALG_AAI_CTR_MOD128,
		      OP_ALG_AS_INIT, ICV_CHECK_DISABLE, DIR_ENC);
	SET_LABEL(p, new_seqinptr);
	WORD(p, 0x0);
	WORD(p, 0x0);
	SEQFIFOLOAD(p, SKIP, 59, 0);

	SEQLOAD(p, MATH2, 0, 8, 0);
	MOVE(p, DESCBUF, (uint16_t)previous_iv, MATH0, 0, 16, WAITCOMP | IMMED);
	MATHB(p, MATH0, XOR, MATH2, MATH1, 8, 0);

	pjump1 = JUMP(p, 0, LOCAL_JUMP, ALL_FALSE, MATH_Z);
	MATHB(p, MATH1, XOR, MATH3, MATH2, 8, 0);
	SET_LABEL(p, new_IV_OK);
	MOVE(p, MATH0, 0, DESCBUF, (uint16_t)encap_iv, 32, IMMED);
	SEQSTORE(p, DESCBUF, 4, 8, 0);
	seqoutptr = 8;

	PATCH_MOVE(p, ref1, descbuf + seqoutptr);
	PATCH_MOVE(p, ref2, descbuf + new_seqinptr);
	PATCH_JUMP(p, pjump1, new_IV_OK);
	PATCH_JUMP(p, pjump2, skip_keyloading);

	return PROGRAM_FINALIZE(p);
}

int main(int argc, char **argv)
{
	int size;

	pr_debug("CAPWAP program\n");
	rta_set_sec_era(RTA_SEC_ERA_2);
	size = generate_capwap_code(prg_buff, 0);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
