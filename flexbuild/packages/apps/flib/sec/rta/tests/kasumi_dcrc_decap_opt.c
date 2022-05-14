/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

LABEL(decap_job_seqoutptr);
REFERENCE(ref_decap_job);
LABEL(decap_share_end);

uint64_t desc_addr_1 = 0x00000ac0ull;

int build_shdesc_kasumi_dcrc_decap(struct program *p, uint32_t *buff,
				   unsigned buffpos)
{
	uint32_t key_size = 16;

	LABEL(foo);
	REFERENCE(pmove1);
	REFERENCE(pmove2);
	LABEL(handle_kasumi);
	REFERENCE(pjump1);
	LABEL(process_pdu);
	REFERENCE(pjump2);

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	SHR_HDR(p, SHR_NEVER, 6, 0);
	{
		{	/* 3G RLC DECAP PDB */
			SET_LABEL(p, foo);
			WORD(p, 0);
			WORD(p, 0);
			WORD(p, 0);
			WORD(p, 0x44444444);
			WORD(p, 0x55555555);
		}
		pjump1 = JUMP(p, handle_kasumi, LOCAL_JUMP, ALL_TRUE, MATH_N);
		/*
		 * bring in all the "extra" stuff with an extra word to make
		 * things space out correctly
		 * Start bringing in Preamble and set up PDB for DCRC
		 */
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		SEQFIFOLOAD(p, PKA0, 60, 0);
		LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_NFIFO, 0, IMMED);
		MOVE(p, IFIFOABD, 0, CONTEXT1, 0, 12, IMMED);
		MOVE(p, IFIFOABD, 0, KEY1, 0, key_size, IMMED);
		MOVE(p, IFIFOABD, 0, CONTEXT1, 32, 32, IMMED);
		LOAD(p, key_size, KEY1SZ, 0, 4, IMMED);
		MOVE(p, CONTEXT1, 36, MATH0, 0, 4, WAITCOMP | IMMED);
		/* get PDU len in left 2 bytes */
		MATHB(p, MATH0, LSHIFT, 16, MATH0, 8, IFB | IMMED2);
		pmove1 = MOVE(p, MATH0, 0, DESCBUF, 0, 4, IMMED);
		PROTOCOL(p, OP_TYPE_DECAP_PROTOCOL, OP_PCLID_3G_DCRC,
			 OP_PCL_3G_DCRC_CRC7);
		MATHB(p, ZERO, SUB, ONE, NONE, 4, 0);
		/* re-execute Job Descriptor */
		ref_decap_job = SHR_HDR(p, SHR_NEVER, decap_job_seqoutptr, 0);

		SET_LABEL(p, handle_kasumi);
		MOVE(p, CONTEXT1, 32, MATH0, 0, 32, IMMED);
		pmove2 = MOVE(p, CONTEXT1, 0, DESCBUF, 0, 12, IMMED);
		/* skip over the preamble and the DCRC header */
		MATHB(p, MATH0, ADD, 60, VSEQINSZ, 4, IMMED2);
		SEQFIFOLOAD(p, SKIP, 0, VLF);
		/*
		 * store the Math register stuff back in the Class 2 Context
		 * Register - only need #PDUs and size
		 */
		MOVE(p, MATH1, 0, CONTEXT2, 8, 8, IMMED);
		MOVE(p, MATH3, 0, CONTEXT2, 0, 8, IMMED);
		/* need to clear the Class 1 Context prior to first run! */
		LOAD(p, CLRW_CLR_C1CTX | CLRW_CLR_C1ICV | CLRW_CLR_C1MODE |
		     CLRW_CLR_C1DATAS, CLRW, 0, 4, IMMED);
		LOAD(p, CCTRL_RESET_CHA_KFHA, CCTRL, 0, 4, IMMED);

		SET_LABEL(p, process_pdu);
		MATHB(p, MATH1, ADD, ONE, SEQINSZ, 4, 0);
		PROTOCOL(p, OP_TYPE_DECAP_PROTOCOL, OP_PCLID_3G_RLC_PDU,
			 OP_PCL_3G_RLC_KASUMI);
		/* Give up KFHA so somebody else can use it during DCRC */
		LOAD(p, CLRW_CLR_C1CTX | CLRW_CLR_C1ICV | CLRW_CLR_C1MODE |
		     CLRW_CLR_C1DATAS, CLRW, 0, 4, IMMED);
		LOAD(p, CCTRL_RESET_CHA_KFHA, CCTRL, 0, 4, IMMED);
		MOVE(p, CONTEXT2, 0, MATH0, 0, 16, WAITCOMP | IMMED);
		MATHB(p, MATH0, SUB, ONE, MATH0, 8, 0);
		MOVE(p, MATH0, 0, CONTEXT2, 0, 8, IMMED);
		pjump2 = JUMP(p, process_pdu, LOCAL_JUMP, ANY_FALSE, MATH_Z);
		/*
		 * For ERA < 5: WORKAROUND FOR INPUT FIFO NIBBLE SHIFT
		 * REGISTER BUG
		 * MOVE(p, MATH0, 0, IFIFOAB1, 0, 1, IMMED);
		 * LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_IFIFO, 0, IMMED);
		 * MOVE(p, IFIFOAB2, 0, OFIFO, 0, 1, FLUSH2 | WAITCOMP | IMMED);
		 */
		SET_LABEL(p, decap_share_end);
	}

	PATCH_JUMP(p, pjump1, handle_kasumi);
	PATCH_JUMP(p, pjump2, process_pdu);
	PATCH_MOVE(p, pmove1, foo);
	PATCH_MOVE(p, pmove2, foo);

	return PROGRAM_FINALIZE(p);
}

int build_jbdesc_kasumi_dcrc_decap(struct program *p, uint32_t *buff,
				   int buffpos)
{
	uint32_t input_frame_length = 89;
	uint32_t output_frame_length = 89;
	uint64_t pdu_in_addr_1 = 0x00000b78ull;
	uint64_t pdu_out_addr_1 = 0x098e449dull;

	if (buffpos < 0)
		return -EINVAL;

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_ALWAYS, buffpos, desc_addr_1, REO | SHR);
	{
		SET_LABEL(p, decap_job_seqoutptr);
		SEQOUTPTR(p, pdu_out_addr_1, output_frame_length, EXT);
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

	rta_set_sec_era(RTA_SEC_ERA_2);

	memset(lte_desc, 0xFF, sizeof(lte_desc));
	lte_desc_size =
	    build_shdesc_kasumi_dcrc_decap(&lte_desc_prgm, lte_desc, 0);

	memset(job_desc, 0xFF, sizeof(job_desc));
	job_desc_size =
	    build_jbdesc_kasumi_dcrc_decap(&job_desc_prgm, job_desc,
					   lte_desc_size);

	PATCH_HDR(&lte_desc_prgm, ref_decap_job, decap_job_seqoutptr);

	pr_debug("KASUMI DCRC Decryption program\n");
	pr_debug("size = %d\n", lte_desc_size);
	print_prog((uint32_t *)lte_desc, lte_desc_size);

	pr_debug("size = %d\n", job_desc_size);
	print_prog((uint32_t *)job_desc, job_desc_size);

	return 0;
}
