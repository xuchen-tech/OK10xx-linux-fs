/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

LABEL(aaa);
REFERENCE(ref1_jump_aaa);
REFERENCE(ref2_jump_aaa);
REFERENCE(ref1_job_aaa);
REFERENCE(ref1_shr_aaa);
REFERENCE(ref2_job_aaa);
LABEL(bbb);
REFERENCE(ref1_jump_bbb);
REFERENCE(ref2_jump_bbb);
REFERENCE(ref1_job_bbb);
REFERENCE(ref1_shr_bbb);
LABEL(fff);
REFERENCE(ref1_jump_fff);
REFERENCE(ref1_job_fff);
REFERENCE(ref1_shr_fff);
LABEL(ggg);
REFERENCE(ref1_jump_ggg);
REFERENCE(ref1_job_ggg);
REFERENCE(ref1_shr_ggg);
LABEL(zzz);
REFERENCE(ref2_jump_zzz);
REFERENCE(ref1_jump_zzz);
REFERENCE(ref3_jump_zzz);
REFERENCE(ref1_job_zzz);
REFERENCE(ref1_shr_zzz);
REFERENCE(ref2_job_zzz);
REFERENCE(ref2_shr_zzz);
LABEL(yyy);
REFERENCE(ref1_jump_yyy);
REFERENCE(ref1_job_yyy);
REFERENCE(ref1_shr_yyy);

uint32_t sharehdr = 0x200000;

int jump_cmd_desc1(struct program *p, uint32_t *buff, unsigned buffpos)
{
	REFERENCE(pjump1);
	REFERENCE(pjump2);
	REFERENCE(phdr1);
	REFERENCE(phdr2);
	REFERENCE(phdr3);
	REFERENCE(phdr4);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	SHR_HDR(p, SHR_NEVER, 0, 0);
	{
		SET_LABEL(p, aaa);
		SET_LABEL(p, bbb);
		bbb = 7;

		MOVE(p, CONTEXT2, 0, CONTEXT1, 0, 4, IMMED);
		pjump1 = JUMP(p, aaa, LOCAL_JUMP, ALL_TRUE, 0);
		pjump2 = JUMP(p, bbb, LOCAL_JUMP, ALL_TRUE, 0);
		ref1_jump_fff = JUMP(p, fff, LOCAL_JUMP, ALL_TRUE, 0);
		ref1_jump_ggg = JUMP(p, ggg, LOCAL_JUMP, ALL_TRUE, 0);
		ref1_jump_zzz = JUMP(p, zzz, LOCAL_JUMP, ALL_TRUE, 0);

		MOVE(p, CONTEXT2, 0, CONTEXT1, 0, 24, IMMED);

		phdr1 = JOB_HDR(p, SHR_NEVER, aaa, 0, 0);
		phdr2 = JOB_HDR(p, SHR_NEVER, bbb, 0, 0);
		ref1_job_fff = JOB_HDR(p, SHR_NEVER, fff, 0, 0);
		ref1_job_ggg = JOB_HDR(p, SHR_NEVER, ggg, 0, 0);
		ref1_job_zzz = JOB_HDR(p, SHR_NEVER, zzz, 0, 0);

		phdr3 = SHR_HDR(p, SHR_NEVER, aaa, 0);
		phdr4 = SHR_HDR(p, SHR_NEVER, bbb, 0);
		ref1_shr_fff = SHR_HDR(p, SHR_NEVER, fff, 0);
		ref1_shr_ggg = SHR_HDR(p, SHR_NEVER, ggg, 0);
		ref1_shr_zzz = SHR_HDR(p, SHR_NEVER, zzz, 0);
	}

	PATCH_JUMP(p, pjump1, aaa);
	PATCH_JUMP(p, pjump2, bbb);
	PATCH_HDR(p, phdr1, aaa);
	PATCH_HDR(p, phdr2, bbb);
	PATCH_HDR(p, phdr3, aaa);
	PATCH_HDR(p, phdr4, bbb);

	return PROGRAM_FINALIZE(p);
}

int jump_cmd_desc2(struct program *p, uint32_t *buff, int buffpos)
{
	REFERENCE(pjump1);
	REFERENCE(pjump2);
	REFERENCE(phdr1);
	REFERENCE(phdr2);
	REFERENCE(phdr3);
	REFERENCE(phdr4);
	LABEL(imm_offset);

	if (buffpos < 0)
		return -EINVAL;

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, buffpos, sharehdr, SHR);
	{
		SET_LABEL(p, fff);	/* first instruction in job header */
		MOVE(p, CONTEXT1, 0, CONTEXT2, 0, 8, IMMED);

		ref1_jump_aaa = JUMP(p, aaa, LOCAL_JUMP, ALL_TRUE, 0);
		ref1_jump_bbb = JUMP(p, bbb, LOCAL_JUMP, ALL_TRUE, 0);
		pjump1 = JUMP(p, fff, LOCAL_JUMP, ALL_TRUE, 0);
		pjump2 = JUMP(p, ggg, LOCAL_JUMP, ALL_TRUE, 0);
		ref1_jump_yyy = JUMP(p, yyy, LOCAL_JUMP, ALL_TRUE, 0);
		ref2_jump_zzz = JUMP(p, zzz, LOCAL_JUMP, ALL_TRUE, 0);

		ref1_job_aaa = JOB_HDR(p, SHR_NEVER, aaa, 0, 0);
		ref1_job_bbb = JOB_HDR(p, SHR_NEVER, bbb, 0, 0);
		phdr1 = JOB_HDR(p, SHR_NEVER, fff, 0, 0);
		phdr2 = JOB_HDR(p, SHR_NEVER, ggg, 0, 0);
		ref1_job_yyy = JOB_HDR(p, SHR_NEVER, yyy, 0, 0);
		ref2_job_zzz = JOB_HDR(p, SHR_NEVER, zzz, 0, 0);

		ref1_shr_aaa = SHR_HDR(p, SHR_NEVER, aaa, 0);
		ref1_shr_bbb = SHR_HDR(p, SHR_NEVER, bbb, 0);
		phdr3 = SHR_HDR(p, SHR_NEVER, fff, 0);
		phdr4 = SHR_HDR(p, SHR_NEVER, ggg, 0);
		ref1_shr_yyy = SHR_HDR(p, SHR_NEVER, yyy, 0);
		ref2_shr_zzz = SHR_HDR(p, SHR_NEVER, zzz, 0);

		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, ggg);
		MOVE(p, MATH0, 0, CONTEXT2, 0, 4, IMMED);
		SET_LABEL(p, imm_offset);
		JUMP(p, 10 - imm_offset, LOCAL_JUMP, ALL_TRUE, 0);
		JUMP(p, -2, LOCAL_JUMP, ALL_TRUE, 0);
	}
	PATCH_JUMP(p, pjump1, fff);
	PATCH_JUMP(p, pjump2, ggg);
	PATCH_HDR(p, phdr1, fff);
	PATCH_HDR(p, phdr2, ggg);
	PATCH_HDR(p, phdr3, fff);
	PATCH_HDR(p, phdr4, ggg);

	return PROGRAM_FINALIZE(p);
}

int jump_cmd_desc3(struct program *p, uint32_t *buff, unsigned buffpos)
{
	REFERENCE(pjump1);
	REFERENCE(phdr1);
	REFERENCE(phdr2);

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	{
		SET_LABEL(p, yyy);
		yyy = 63;	/* last word in descbuf [63] */

		ref2_jump_aaa = JUMP(p, aaa, LOCAL_JUMP, ALL_TRUE, 0);
		ref2_jump_bbb = JUMP(p, bbb, LOCAL_JUMP, ALL_TRUE, 0);
		pjump1 = JUMP(p, yyy, LOCAL_JUMP, ALL_TRUE, 0);
		ref3_jump_zzz = JUMP(p, zzz, LOCAL_JUMP, ALL_TRUE, 0);

		ref2_job_aaa = JOB_HDR(p, SHR_NEVER, aaa, 0, 0);
		phdr2 = JOB_HDR(p, SHR_NEVER, yyy, 0, 0);
		phdr1 = JOB_HDR(p, SHR_NEVER, zzz, 0, 0);

		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, zzz);
		MOVE(p, CONTEXT2, 0, CONTEXT1, 0, 44, IMMED);
	}
	PATCH_JUMP(p, pjump1, yyy);
	PATCH_HDR(p, phdr1, zzz);
	PATCH_HDR(p, phdr2, yyy);

	return PROGRAM_FINALIZE(p);
}

int jump_cmd_desc4(struct program *p, uint32_t *buff, unsigned buffpos)
{
	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	{
		JUMP(p, 7 - buffpos, LOCAL_JUMP, ALL_TRUE, 0);
	}

	return PROGRAM_FINALIZE(p);
}

int jump_cmd_desc5(struct program *p, uint32_t *buff, unsigned buffpos)
{
	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	{
		/* class done tests */
		JUMP(p, 0x500, FAR_JUMP, ALL_TRUE, CLASS1 | PK_0 | MATH_N);
		JUMP(p, 0x500, FAR_JUMP, ALL_FALSE, CLASS1 | MATH_Z | MATH_N);
		JUMP(p, 0x500, FAR_JUMP, ALL_TRUE, BOTH | NOP);
		JUMP(p, 0x500, FAR_JUMP, ALL_TRUE, CLASS2 | NOP);
		JUMP(p, 0x500, FAR_JUMP, ALL_TRUE, CLASS2 | NOP);

		JUMP(p, 0, HALT, ALL_TRUE, CALM | NOP | SHRD);
		JUMP(p, 0x42, HALT_STATUS, ANY_FALSE, PK_0 | MATH_Z);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, CLASS1 | CLASS2);
	}

	return PROGRAM_FINALIZE(p);
}

int jump_cmd_desc6(struct program *p, uint32_t *buff, unsigned buffpos)
{
	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, 0, 0, 0);
	{
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NIFP);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NIFP | NIP);
		JUMP(p, -1, LOCAL_JUMP, ALL_TRUE, 0);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NIFP);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NIFP | NIP);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NIFP | SHRD);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, NIFP | SHRD);
		JUMP(p, 1, LOCAL_JUMP, ANY_FALSE, NIFP | SHRD);
		JUMP(p, 1, LOCAL_JUMP, ANY_FALSE, NIFP | SHRD);
		JUMP(p, 1, LOCAL_JUMP, ANY_FALSE, NIFP | SHRD);
		JUMP(p, 1, LOCAL_JUMP, ALL_FALSE, CLASS1 | PK_PRIME | MATH_N);
		JUMP(p, 1, LOCAL_JUMP, ALL_FALSE, CLASS1 | PK_PRIME | MATH_N);
		JUMP(p, 2, LOCAL_JUMP, ANY_FALSE, CLASS1 | JQP);
		JUMP(p, 2, LOCAL_JUMP, ANY_TRUE, MATH_Z | MATH_N);
		JUMP(p, 2, LOCAL_JUMP, ANY_FALSE, MATH_Z | MATH_N);
		JUMP(p, 2, LOCAL_JUMP, ANY_FALSE, MATH_Z | MATH_N);
	}

	return PROGRAM_FINALIZE(p);
}

int main(int argc, char **argv)
{
	uint32_t desc1[64];
	uint32_t desc2[64];
	uint32_t desc3[64];
	uint32_t desc4[50];
	uint32_t desc5[50];
	uint32_t desc6[50];
	int buf1len, buf2len, buf3len, buf4len, buf5len, buf6len;

	struct program desc1_prgm;
	struct program desc2_prgm;
	struct program desc3_prgm;
	struct program desc4_prgm;
	struct program desc5_prgm;
	struct program desc6_prgm;

	rta_set_sec_era(RTA_SEC_ERA_1);

	memset(desc1, 0x00, sizeof(desc1));
	buf1len = jump_cmd_desc1(&desc1_prgm, desc1, 0);

	memset(desc2, 0x00, sizeof(desc2));
	buf2len = jump_cmd_desc2(&desc2_prgm, desc2, buf1len);

	memset(desc3, 0x00, sizeof(desc3));
	buf3len = jump_cmd_desc3(&desc3_prgm, desc3, 55);

	memset(desc4, 0x00, sizeof(desc4));
	buf4len = jump_cmd_desc4(&desc4_prgm, desc4, 55);

	memset(desc5, 0x00, sizeof(desc5));
	buf5len = jump_cmd_desc5(&desc5_prgm, desc5, 0);

	memset(desc6, 0x00, sizeof(desc6));
	buf6len = jump_cmd_desc6(&desc6_prgm, desc6, 0);

	PATCH_JUMP(&desc2_prgm, ref1_jump_aaa, aaa);
	PATCH_JUMP(&desc3_prgm, ref2_jump_aaa, aaa);
	PATCH_JUMP(&desc2_prgm, ref1_jump_bbb, bbb);
	PATCH_JUMP(&desc3_prgm, ref2_jump_bbb, bbb);
	PATCH_JUMP(&desc1_prgm, ref1_jump_fff, fff);
	PATCH_JUMP(&desc1_prgm, ref1_jump_ggg, ggg);
	PATCH_JUMP(&desc2_prgm, ref2_jump_zzz, zzz);
	PATCH_JUMP(&desc1_prgm, ref1_jump_zzz, zzz);
	PATCH_JUMP(&desc3_prgm, ref3_jump_zzz, zzz);

	PATCH_HDR(&desc2_prgm, ref1_job_aaa, aaa);
	PATCH_HDR(&desc2_prgm, ref1_shr_aaa, aaa);
	PATCH_HDR(&desc3_prgm, ref2_job_aaa, aaa);
	PATCH_HDR(&desc2_prgm, ref1_job_bbb, bbb);
	PATCH_HDR(&desc2_prgm, ref1_shr_bbb, bbb);

	PATCH_HDR(&desc1_prgm, ref1_job_fff, fff);
	PATCH_HDR(&desc1_prgm, ref1_shr_fff, fff);

	PATCH_HDR(&desc1_prgm, ref1_job_ggg, ggg);
	PATCH_HDR(&desc1_prgm, ref1_shr_ggg, ggg);

	PATCH_HDR(&desc1_prgm, ref1_job_zzz, zzz);
	PATCH_HDR(&desc1_prgm, ref1_shr_zzz, zzz);
	PATCH_HDR(&desc2_prgm, ref2_job_zzz, zzz);
	PATCH_HDR(&desc2_prgm, ref2_shr_zzz, zzz);

	PATCH_HDR(&desc2_prgm, ref1_job_yyy, yyy);
	PATCH_HDR(&desc2_prgm, ref1_shr_yyy, yyy);
	PATCH_JUMP(&desc2_prgm, ref1_jump_yyy, yyy);

	pr_debug("JUMP commands program\n");
	pr_debug("Jump desc #1\n");
	pr_debug("size = %d\n", buf1len);
	print_prog((uint32_t *)desc1, buf1len);

	pr_debug("Jump desc #2\n");
	pr_debug("size = %d\n", buf2len);
	print_prog((uint32_t *)desc2, buf2len);

	pr_debug("Jump desc #3\n");
	pr_debug("size = %d\n", buf3len);
	print_prog((uint32_t *)desc3, buf3len);

	pr_debug("Jump desc #4\n");
	pr_debug("size = %d\n", buf4len);
	print_prog((uint32_t *)desc4, buf4len);

	pr_debug("Jump desc #5\n");
	pr_debug("size = %d\n", buf5len);
	print_prog((uint32_t *)desc5, buf5len);

	pr_debug("Jump desc #6\n");
	pr_debug("size = %d\n", buf6len);
	print_prog((uint32_t *)desc6, buf6len);

	return 0;
}
