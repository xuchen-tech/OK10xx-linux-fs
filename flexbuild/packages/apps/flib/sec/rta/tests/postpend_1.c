/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

int postpend(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	uint8_t key_1[40] = {
		0xc8, 0xc1, 0xd7, 0xbf, 0xa4, 0xe3, 0xee, 0x84,
		0xb1, 0x52, 0x37, 0x06, 0x38, 0x97, 0xac, 0x9f,
		0x53, 0xa9, 0x0a, 0xca, 0xc4, 0xc5, 0xe5, 0x9e,
		0x65, 0x94, 0x0e, 0x33, 0x0f, 0xa5, 0x4d, 0x3d,
		0xc7, 0x1e, 0xfe, 0x3e, 0x2f, 0x20, 0x59, 0x08
	};
	uint8_t key_2[16] = {
		0x86, 0xeb, 0x54, 0x5e, 0xec, 0x53, 0x47, 0xb8,
		0x51, 0xa0, 0x53, 0x9e, 0x2f, 0xf6, 0x9a, 0x8d
	};
	uint8_t data_in[15] = {
		0x00, 0xe0, 0x81, 0x58, 0x13, 0xc0, 0x00, 0xe0,
		0x81, 0x58, 0x13, 0xd0, 0x08, 0x00
	};

	LABEL(skip_key_load);
	REFERENCE(pjump1);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	PROGRAM_SET_36BIT_ADDR(p);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);

	SHR_HDR(p, SHR_SERIAL, 15, RIF);
	{
		{	/* IPSEC ESP ENCAP (CBC) PDB */
			WORD(p, 0x0009000D); /* opt word */
			WORD(p, 0x41311751); /* ESN */
			WORD(p, 1); /* seq num */
			DWORD(p, 0x92cd6ce9ab7c728c);
			DWORD(p, 0x153a85cefd12ab79); /* IV */
			WORD(p, 1); /* SPI */
			WORD(p, 20); /* optIPHdrLen */
			DWORD(p, 0xe1a6001458335cb7);
			DWORD(p, 0x55c809f8b44767bb);
			WORD(p, 0x79890a98); /* OptIPHdr */
		}
		pjump1 = JUMP(p, skip_key_load, LOCAL_JUMP, ALL_TRUE, SHRD);
		KEY(p, MDHA_SPLIT_KEY, 0, (uintptr_t) &key_1, 40,
		    IMMED | COPY);
		KEY(p, KEY1, 0, (uintptr_t) &key_2, 16, IMMED | COPY);
		SET_LABEL(p, skip_key_load);
		PROTOCOL(p, OP_TYPE_ENCAP_PROTOCOL, OP_PCLID_IPSEC,
			 OP_PCL_IPSEC_AES_CBC |
			      OP_PCL_IPSEC_HMAC_SHA1_160);
		SEQSTORE(p, (uintptr_t) &data_in, 0, 14, IMMED | COPY);
	}
	PATCH_JUMP(p, pjump1, skip_key_load);

	return PROGRAM_FINALIZE(p);
}

uint32_t prg_buff[1000];

int main(int argc, char **argv)
{
	int size;

	pr_debug("POSTPEND_1 example program\n");
	rta_set_sec_era(RTA_SEC_ERA_4);
	size = postpend(prg_buff);
	pr_debug("size = %d\n", size);
	print_prog(prg_buff, size);

	return 0;
}
