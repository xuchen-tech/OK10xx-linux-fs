/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

LABEL(encap_job_seqout);
REFERENCE(ref_job_seqout);
LABEL(all_done);
REFERENCE(ref_all_done);
LABEL(encap_job_seqin);
REFERENCE(ref_job_seqin);
LABEL(encap_share_end);

uint64_t desc_addr_1 = 0x00000040ull;

int build_shdesc_kasumi_dcrc_encap(struct program *p, uint32_t *buff,
				   unsigned buffpos)
{
	uint32_t key_size = 16;

	LABEL(foo);
	REFERENCE(pmove1);
	LABEL(get_data);
	REFERENCE(pmove4);
	LABEL(do_dcrc);
	REFERENCE(pjump1);
	LABEL(process_pdu);
	REFERENCE(pjump3);
	LABEL(build_move);
	REFERENCE(pjump4);

	PROGRAM_SET_36BIT_ADDR(p);

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	SHR_HDR(p, SHR_NEVER, 6, 0);
	{
		{	/* 3G RLC ENCAP PDB */
			SET_LABEL(p, foo);
			WORD(p, 0);
			WORD(p, 0);
			WORD(p, 0);
			WORD(p, 0x44444444);
			WORD(p, 0x55555555);
		}
		pjump1 = JUMP(p, do_dcrc, LOCAL_JUMP, ALL_TRUE, MATH_N);

		/* Start bringing in Preamble */
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		SEQFIFOLOAD(p, PKA0, 60, 0);
		LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_NFIFO, 0, IMMED);
		pmove1 = MOVE(p, IFIFOABD, 0, DESCBUF, 0, 12, IMMED);
		MOVE(p, IFIFOABD, 0, KEY1, 0, key_size, IMMED);
		MOVE(p, IFIFOABD, 0, CONTEXT2, 32, 32, IMMED);
		LOAD(p, key_size, KEY1SZ, 0, 4, IMMED);
		MOVE(p, CONTEXT2, 32, MATH0, 0, 32, IMMED);
		/* Get SEQ IN PTR command from Job Descriptor */
		ref_job_seqout = MOVE(p, DESCBUF, 0, MATH2, 0, 12,
				      WAITCOMP | IMMED);
		/* Mask out the bit so SOP becomes SIP; maintain ptr value */
		MATHB(p, MATH2, AND, 0xf7ffffffffffffff, MATH2, 8, IMMED2);
		MATHB(p, SEQINSZ, ADD, MATH3, MATH3, 8, NFU);
		/* swap words */
		MATHB(p, MATH0, SHLD, MATH3, MATH3, 8, 0);
		ref_job_seqin = MOVE(p, MATH2, 0, DESCBUF, 0, 12, IMMED);
		MOVE(p, CONTEXT2, 48, MATH2, 0, 16, WAITCOMP | IMMED);

		/* Build a MOVE command to process input data */
		SET_LABEL(p, build_move);
		/*
		 * RTA does not support generating MATH commands with LEN = 8
		 * and no immediate inlined in the command, thus use WORD(p, ).
		 */
		WORD(p, 0xaa004004);
		MOVE(p, IFIFOABD, 0, OFIFO, 0, 0, IMMED);

		MATHB(p, ZERO, ADD, MATH0, VSEQINSZ, 2, NFU);
		MATHB(p, ZERO, ADD, MATH0, VSEQOUTSZ, 2, NFU);
		MATHB(p, MATH0, SHLD, MATH0, MATH0, 8, NFU);
		pmove4 = MOVE(p, MATH0, 0, DESCBUF, 0, 4, WAITCOMP | IMMED);
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		SEQFIFOLOAD(p, PKA0, 0, VLF);
		SEQFIFOSTORE(p, MSG, 0, 0, VLF);
		LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_NFIFO, 0, IMMED);

		SET_LABEL(p, get_data);
		WORD(p, 0);
		ref_all_done = JUMP(p, all_done, LOCAL_JUMP, ALL_TRUE, MATH_N);

		SET_LABEL(p, process_pdu);
		MATHB(p, ZERO, ADD, MATH1, SEQINSZ, 4, 0);
		PROTOCOL(p, OP_TYPE_ENCAP_PROTOCOL, OP_PCLID_3G_RLC_PDU,
			 OP_PCL_3G_RLC_KASUMI);
		LOAD(p, CLRW_CLR_C1CTX | CLRW_CLR_C1ICV | CLRW_CLR_C1MODE |
		     CLRW_CLR_C1DATAS, CLRW, 0, 4, IMMED);
		LOAD(p, CCTRL_RESET_CHA_KFHA, CCTRL, 0, 4, IMMED);
		MOVE(p, CONTEXT2, 32, MATH0, 0, 32, WAITCOMP | IMMED);
		MATHB(p, MATH3, SUB, ONE, MATH3, 8, 0);
		MOVE(p, MATH3, 0, CONTEXT2, 56, 8, IMMED);
		pjump3 = JUMP(p, process_pdu, LOCAL_JUMP, ANY_FALSE, MATH_Z);
		MATHB(p, MATH0, LSHIFT, 48, MATH0, 8, IFB | IMMED2);
		MOVE(p, MATH0, 0, DESCBUF, 4, 4, IMMED);
		MATHB(p, ZERO, SUB, ONE, NONE, 4, 0);
		MATHB(p, ZERO, ADD, MATH2, SEQINSZ, 4, NFU);
		MATHB(p, ZERO, ADD, MATH2, MATH0, 4, NFU);
		pjump4 = JUMP(p, build_move, LOCAL_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, do_dcrc);
		PROTOCOL(p, OP_TYPE_ENCAP_PROTOCOL, OP_PCLID_3G_DCRC,
			 OP_PCL_3G_DCRC_CRC7);
		SET_LABEL(p, encap_share_end);
	}

	PATCH_JUMP(p, pjump1, do_dcrc);
	PATCH_JUMP(p, pjump3, process_pdu);
	PATCH_JUMP(p, pjump4, build_move);
	PATCH_MOVE(p, pmove1, foo);
	PATCH_MOVE(p, pmove4, get_data);

	return PROGRAM_FINALIZE(p);
}

int build_jbdesc_kasumi_dcrc_encap(struct program *p, uint32_t *buff,
				   int buffpos)
{
	uint32_t input_frame_length = 2356;
	uint32_t output_frame_length = 2302;
	uint64_t pdu_in_addr_1 = 0x155ull;
	uint64_t pdu_out_addr_1 = 0x095a0967ull;

	if (buffpos < 0)
		return -EINVAL;

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_ALWAYS, buffpos, desc_addr_1, REO | SHR);
	{
		SET_LABEL(p, encap_job_seqout);
		SET_LABEL(p, all_done);
		SEQOUTPTR(p, pdu_out_addr_1, output_frame_length, EXT);
		SET_LABEL(p, encap_job_seqin);
		SEQINPTR(p, pdu_in_addr_1, input_frame_length, EXT);
	}

	return PROGRAM_FINALIZE(p);
}

int main(int argc, char **argv)
{
	uint32_t lte_desc[60];
	uint32_t job_desc[20];
	int lte_desc_size, job_desc_size;
	struct program lte_desc_prgm;
	struct program job_desc_prgm;

	rta_set_sec_era(RTA_SEC_ERA_3);

	memset(lte_desc, 0xFF, sizeof(lte_desc));
	lte_desc_size =
	    build_shdesc_kasumi_dcrc_encap(&lte_desc_prgm, lte_desc, 0);

	memset(job_desc, 0xFF, sizeof(job_desc));
	job_desc_size =
	    build_jbdesc_kasumi_dcrc_encap(&job_desc_prgm, job_desc,
					   lte_desc_size);

	PATCH_JUMP(&lte_desc_prgm, ref_all_done, all_done);
	PATCH_MOVE(&lte_desc_prgm, ref_job_seqout, encap_job_seqout);
	PATCH_MOVE(&lte_desc_prgm, ref_job_seqin, encap_job_seqin);

	pr_debug("KASUMI DCRC Decryption program\n");
	pr_debug("size = %d\n", lte_desc_size);
	print_prog((uint32_t *)lte_desc, lte_desc_size);

	pr_debug("size = %d\n", job_desc_size);
	print_prog((uint32_t *)job_desc, job_desc_size);

	return 0;
}
