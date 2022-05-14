/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

uint64_t share_desc_addr = 0x089c2c80ULL;
uint32_t c1_ctxt_slots = 4;
uint32_t c2_ctxt_slots = 4;
uint32_t max_slots = 8;
uint32_t data_size = 3840;

LABEL(new_source);
REFERENCE(ref1_move_new_source);
REFERENCE(ref2_move_new_source);
LABEL(first);
REFERENCE(ref1_shr_first);
LABEL(last);
REFERENCE(ref1_shr_last);

int build_shdesc_raid_xor_opt(struct program *p, uint32_t *buff,
			      unsigned buffpos)
{
	uint32_t chunk_size = 128;
	uint32_t fetch_limit = 64;

	LABEL(label_2);
	REFERENCE(pmove1);
	REFERENCE(pmove2);
	REFERENCE(pmove3);
	REFERENCE(pjump1);
	REFERENCE(phdr1);
	REFERENCE(phdr2);
	REFERENCE(phdr3);
	LABEL(return_point);
	REFERENCE(pjump2);
	REFERENCE(pjump3);
	REFERENCE(pjump4);
	LABEL(c);
	REFERENCE(phdr4);
	LABEL(store_here);
	REFERENCE(pjump5);

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	SHR_HDR(p, SHR_NEVER, 0, 0);
	{
		SEQLOAD(p, CONTEXT1, 0, c1_ctxt_slots * 16, 0);
		SEQLOAD(p, CONTEXT2, 0, c2_ctxt_slots * 16, 0);
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		pmove1 = MOVE(p, CONTEXT1, 0, DESCBUF, 0, 16, WAITCOMP | IMMED);

		phdr1 = SHR_HDR(p, SHR_NEVER, label_2, 0);

		SET_LABEL(p, label_2);
		SET_LABEL(p, new_source);
		new_source += 1;

		SEQINPTR(p, 0, 0, EXT);
		WORD(p, 0);
		WORD(p, 0);
		SEQFIFOLOAD(p, SKIP, 0, VLF);

		SET_LABEL(p, c);
		SEQFIFOLOAD(p, PKA0, chunk_size, 0);
		MATHB(p, VSEQOUTSZ, ADD, chunk_size, VSEQOUTSZ, 4, IMMED2);
		NFIFOADD(p, IFIFO, MSG, chunk_size, LAST1);

		SET_LABEL(p, return_point);
		MOVE(p, IFIFOABD, 0, MATH0, 0, 32, WAITCOMP | IMMED);
		MATHB(p, MATH0, XOR, OFIFO, MATH0, 8, 0);
		MATHB(p, MATH1, XOR, OFIFO, MATH1, 8, 0);
		MATHB(p, MATH2, XOR, OFIFO, MATH2, 8, 0);
		MATHB(p, MATH3, XOR, OFIFO, MATH3, 8, 0);
		MOVE(p, MATH0, 0, OFIFO, 0, 32, IMMED);
		MATHB(p, VSEQOUTSZ, SUB, 32, VSEQOUTSZ, 4, IMMED2);
		pjump5 = JUMP(p, store_here, LOCAL_JUMP, ALL_TRUE, MATH_Z);
		MATHB(p, VSEQOUTSZ, SUB, fetch_limit, NONE, 4, IMMED2);
		pjump1 = JUMP(p, label_2, LOCAL_JUMP, ALL_TRUE, MATH_Z);
		pjump2 = JUMP(p, return_point, LOCAL_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, store_here);
		SEQFIFOSTORE(p, MSG, 0, chunk_size, 0);
		MATHB(p, SEQOUTSZ, SUB, ONE, NONE, 4, 0);
		JUMP(p, 0, HALT_STATUS, ALL_TRUE, MATH_N);
		MOVE(p, IFIFOABD, 0, OFIFO, 0, chunk_size, IMMED);
		phdr2 = SHR_HDR(p, SHR_NEVER, label_2, 0);

		SET_LABEL(p, first);
		SEQFIFOLOAD(p, SKIP, 0, VLF);
		MATHB(p, SEQINSZ, SUB, ONE, NONE, 4, 0);
		pjump3 = JUMP(p, return_point, LOCAL_JUMP, ALL_TRUE, MATH_N);
		SEQFIFOLOAD(p, PKA0, chunk_size, 0);
		NFIFOADD(p, IFIFO, MSG, chunk_size, LAST1);

		pmove2 = MOVE(p, CONTEXT1, 16, DESCBUF, 0, 16, IMMED);
		MATHB(p, VSEQINSZ, SUB, ONE, NONE, 4, 0);
		pjump4 = JUMP(p, return_point, LOCAL_JUMP, ANY_FALSE, MATH_N);
		MOVE(p, IFIFOABD, 0, OFIFO, 0, 128, IMMED);
		MATHB(p, SEQINSZ, ADD, chunk_size, SEQOUTSZ, 4, IMMED2);
		phdr3 = SHR_HDR(p, SHR_NEVER, label_2, 0);

		SET_LABEL(p, last);
		SEQFIFOLOAD(p, SKIP, 0, VLF);
		MATHB(p, VSEQINSZ, ADD, chunk_size, VSEQINSZ, 4, IMMED2);
		pmove3 = MOVE(p, CONTEXT1, 0, DESCBUF, 0, 16, IMMED);
		phdr4 = SHR_HDR(p, SHR_NEVER, c, 0);
	}

	PATCH_MOVE(p, pmove1, new_source);
	PATCH_MOVE(p, pmove2, new_source);
	PATCH_MOVE(p, pmove3, new_source);
	PATCH_HDR(p, phdr1, label_2);
	PATCH_HDR(p, phdr2, label_2);
	PATCH_HDR(p, phdr3, label_2);
	PATCH_HDR(p, phdr4, c);
	PATCH_JUMP(p, pjump1, label_2);
	PATCH_JUMP(p, pjump2, return_point);
	PATCH_JUMP(p, pjump3, return_point);
	PATCH_JUMP(p, pjump4, return_point);
	PATCH_JUMP(p, pjump5, store_here);

	return PROGRAM_FINALIZE(p);
}

int build_jbdesc_raid_xor_opt(struct program *p, uint32_t *buff, int buffpos)
{
	uint64_t context_ptr = 0x08858d80ULL;
	uint64_t store_ptr = 0x00000040ULL;

	if (buffpos < 0)
		return -EINVAL;

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_ALWAYS, buffpos, share_desc_addr, REO | SHR);
	{
		SEQOUTPTR(p, store_ptr, data_size, EXT);
		SEQINPTR(p, context_ptr, 8 * 16, 0);
	}

	return PROGRAM_FINALIZE(p);
}

int build_more_cmds_raid_xor_opt(struct program *p, uint32_t *buff,
				 unsigned buffpos)
{
	uint64_t load_data_addr[8];
	int i;
	uint32_t num_sources = 8;

	load_data_addr[0] = 0x00000f40ULL;
	load_data_addr[1] = 0x09dd9d00ULL;
	load_data_addr[2] = 0x00001e40ULL;
	load_data_addr[3] = 0x096fcc80ULL;
	load_data_addr[4] = 0x093c41c0ULL;
	load_data_addr[5] = 0x082acf40ULL;
	load_data_addr[6] = 0x08868a80ULL;
	load_data_addr[7] = 0x08093d40ULL;

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	{
		SEQINPTR(p, load_data_addr[0], data_size, EXT);
		ref1_shr_first = SHR_HDR(p, SHR_NEVER, first, 0);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, 0);

		for (i = 1; i < (num_sources - 1); i++)
			if (i < (c1_ctxt_slots - 1)) {
				SEQINPTR(p, load_data_addr[i], data_size,
					 EXT);
				ref1_move_new_source =
					MOVE(p, CONTEXT1, (16 * (i + 1)),
					     DESCBUF, 0, 16, IMMED);
				JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, 0);
				PATCH_MOVE(p, ref1_move_new_source, new_source);
			} else {
				SEQINPTR(p, load_data_addr[i], data_size,
					 EXT);
				ref2_move_new_source =
					MOVE(p, CONTEXT2, (16 * (i + 1)),
					     DESCBUF, 0, 16, IMMED);
				JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, 0);
				PATCH_MOVE(p, ref2_move_new_source, new_source);
			}

		SEQINPTR(p, load_data_addr[i], data_size, EXT);
		ref1_shr_last = SHR_HDR(p, SHR_NEVER, last, 0);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, 0);
	}

	PATCH_HDR(p, ref1_shr_first, first);
	PATCH_HDR(p, ref1_shr_last, last);

	return PROGRAM_FINALIZE(p);
}

int main(int argc, char **argv)
{
	uint32_t sharedesc[64];
	uint32_t jobdesc[10];
	uint32_t context_buf[64];
	int shr_size, job_size, ctx_size;

	struct program shr_desc_prgm;
	struct program job_desc_prgm;
	struct program ctx_buf_prgm;

	rta_set_sec_era(RTA_SEC_ERA_1);

	memset(sharedesc, 0xFF, sizeof(sharedesc));
	shr_size = build_shdesc_raid_xor_opt(&shr_desc_prgm, sharedesc, 0);

	memset(jobdesc, 0xFF, sizeof(jobdesc));
	job_size = build_jbdesc_raid_xor_opt(&job_desc_prgm, jobdesc, shr_size);

	memset(context_buf, 0xFF, sizeof(context_buf));
	ctx_size = build_more_cmds_raid_xor_opt(&ctx_buf_prgm, context_buf, 7);

	pr_debug("raid xor program shared desc\n");
	pr_debug("size = %d\n", shr_size);
	print_prog((uint32_t *)sharedesc, shr_size);

	pr_debug("raid xor program job desc\n");
	pr_debug("size = %d\n", job_size);
	print_prog((uint32_t *)jobdesc, job_size);

	pr_debug("raid xor program job context\n");
	pr_debug("size = %d\n", ctx_size);
	print_prog((uint32_t *)context_buf, ctx_size);

	return 0;
}
