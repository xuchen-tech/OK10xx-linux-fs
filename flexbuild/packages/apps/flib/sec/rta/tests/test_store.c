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
uint32_t jd1_buff[64];
uint32_t jd2_buff[64];
uint32_t sd_buff[64];
uintptr_t descwords = (uintptr_t) 0x12889980ull;
int word_size = sizeof(uint32_t);
REFERENCE(pstore2);
REFERENCE(pstore3);
REFERENCE(pstore4);
LABEL(mod_loc2);
LABEL(shr_loc);

int test_store_cmds(uint32_t *buff)
{
	struct program prg;
	struct program *p = &prg;
	uintptr_t ctx = (uintptr_t) 0x55330087ull;
	uintptr_t sgtable = (uintptr_t) 0x22223333ull;
	int ivlen = 16;
	uintptr_t foo = (uintptr_t)0x34000080ull;
	uint8_t abc[] = {0x61, 0x62, 0x63};
	uint here = 32;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	/* Class-independent CCB registers */
	STORE(p, CLRW, 0, foo, 4, 0);
	STORE(p, CCTRL, 0, foo, 4, 0);
	STORE(p, ICTRL, 0, foo, 4, 0);
	STORE(p, CSTAT, 0, foo, 4, 0);

	/* Class 1 CCB registers */
	STORE(p, MODE1, 0, foo, 4, 0);
	STORE(p, KEY1SZ, 0, foo, 4, 0);
	STORE(p, DATA1SZ, 0, foo, 4, 0);
	STORE(p, ICV1SZ, 0, foo, 4, 0);
	/* Class 1 Additional Data Size */
	STORE(p, AAD1SZ, 0, foo, 4, 0);
	STORE(p, IV1SZ, 0, foo, 4, 0);
	STORE(p, PKASZ, 0, foo, 4, 0);
	STORE(p, PKBSZ, 0, foo, 4, 0);
	STORE(p, PKNSZ, 0, foo, 4, 0);
	STORE(p, PKESZ, 0, foo, 4, 0);

	STORE(p, CONTEXT1, 0, sgtable, ivlen, SGF);
	STORE(p, CONTEXT1, 5, foo, 7, 0);

	/* Class 2 CCB registers */
	STORE(p, MODE2, 0, foo, 4, 0);
	STORE(p, KEY2SZ, 0, foo, 4, 0);
	STORE(p, DATA2SZ, 0, foo, 4, 0);
	STORE(p, ICV2SZ, 0, foo, 4, 0);
	STORE(p, CONTEXT2, 0, ctx, 16, 0);
	STORE(p, CONTEXT2, 8, foo, 4, 0);

	/* DECO / SEC registers */
	STORE(p, DJQCTRL, 0, foo, 8, 0);
	/* DECO JQ Descriptor Address */
	STORE(p, DJQDA, 0, foo, 8, 0);
	STORE(p, DSTAT, 0, foo, 8, 0);
	STORE(p, DPID, 0, foo, 8, 0);

	SEQSTORE(p, MATH0, 0, 1, 0);
	SEQSTORE(p, MATH1, 0, 0, VLF);
	STORE(p, MATH2, 0, foo, 4, 0);
	STORE(p, MATH3, 0, foo, 4, 0);
	STORE(p, DESCBUF, 20 * word_size, descwords, 4 * word_size, 0);

	STORE(p, DESCBUF, here, descwords, 8 * word_size, 0);
	SEQSTORE(p, DESCBUF, here, 12 * word_size, 0);

	/* Immediate data */
	STORE(p, 0x010203, 0, foo, 3, IMMED);
	STORE(p, (uintptr_t) &abc, 0, foo, 3, IMMED | COPY);
	SEQSTORE(p, 0x010203, 0, 3, IMMED);
	SEQSTORE(p, 0x01020304050607, 0, 7, IMMED | VLF);
	SEQSTORE(p, (uintptr_t) &abc, 0, 3, IMMED | COPY);

	return PROGRAM_FINALIZE(p);
}

int test_store_jd1(uint32_t *buff, unsigned buffpos)
{
	struct program prg;
	struct program *p = &prg;
	REFERENCE(pstore);
	LABEL(mod_loc1);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);

	JOB_HDR(p, SHR_NEVER, buffpos, 0, 0);
	pstore = STORE(p, JOBDESCBUF, mod_loc1, 0, 4 * word_size, 0);
	SET_LABEL(p, mod_loc1);
	MOVE(p, CONTEXT1, 0, CONTEXT2, 0, 1, IMMED);

	PATCH_STORE(p, pstore, mod_loc1);

	return PROGRAM_FINALIZE(p);
}

int test_store_sd(struct program *p, uint32_t *buff, unsigned buffpos)
{
	REFERENCE(pstore);

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);

	SHR_HDR(p, SHR_NEVER, buffpos, 0);
	pstore = STORE(p, SHAREDESCBUF, shr_loc, 0, 4 * word_size, 0);
	SET_LABEL(p, shr_loc);
	MOVE(p, CONTEXT1, 0, CONTEXT2, 0, 1, IMMED);
	MATHB(p, ZERO, ADD, ONE, MATH1, 1, 0);
	MATHB(p, ZERO, ADD, ONE, MATH1, 1, 0);
	MATHB(p, ZERO, ADD, ONE, MATH1, 1, 0);
	pstore2 = STORE(p, JOBDESCBUF, 0, 0, 4 * word_size, 0);

	PATCH_STORE(p, pstore, shr_loc);

	return PROGRAM_FINALIZE(p);
}

int test_store_jd2(struct program *p, uint32_t *buff, int buffpos)
{
	uint64_t shrloc = 0x72650040ull;

	if (buffpos < 0)
		return -EINVAL;

	PROGRAM_CNTXT_INIT(p, buff, 0);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);

	JOB_HDR(p, SHR_NEVER, buffpos, shrloc, SHR);
	MATHB(p, ZERO, ADD, ONE, MATH1, 1, 0);
	SET_LABEL(p, mod_loc2);
	MOVE(p, CONTEXT1, 0, CONTEXT2, 0, 1, IMMED);
	STORE(p, DESCBUF, mod_loc2 * word_size, descwords, 4 * word_size, 0);
	MATHB(p, ZERO, ADD, ONE, MATH1, 1, 0);
	MATHB(p, ZERO, ADD, ONE, MATH1, 1, 0);
	pstore3 = STORE(p, JOBDESCBUF, 0, 0, 4 * word_size, 0);
	pstore4 = STORE(p, SHAREDESCBUF, 0, 0, 4 * word_size, 0);

	return PROGRAM_FINALIZE(p);
}

int main(int argc, char **argv)
{
	struct program share_desc, job_desc;
	int cmd_size, jd1_size, jd2_size, sd_size;

	rta_set_sec_era(RTA_SEC_ERA_3);

	cmd_size = test_store_cmds(prg_buff);
	jd1_size = test_store_jd1(jd1_buff, 0);
	sd_size = test_store_sd(&share_desc, sd_buff, 0);
	jd2_size = test_store_jd2(&job_desc, jd2_buff, sd_size);

	PATCH_STORE(&share_desc, pstore2, mod_loc2);
	PATCH_STORE(&job_desc, pstore3, mod_loc2);
	PATCH_STORE(&job_desc, pstore4, shr_loc);

	pr_debug("STORE commands\n");
	pr_debug("size = %d\n", cmd_size);
	print_prog(prg_buff, cmd_size);

	pr_debug("STORE Job Desc #1\n");
	pr_debug("size = %d\n", jd1_size);
	print_prog(jd1_buff, jd1_size);

	pr_debug("STORE Shared Desc\n");
	pr_debug("size = %d\n", sd_size);
	print_prog(sd_buff, sd_size);

	pr_debug("STORE Job Desc #2\n");
	pr_debug("size = %d\n", jd2_size);
	print_prog(jd2_buff, jd2_size);

	return 0;
}
