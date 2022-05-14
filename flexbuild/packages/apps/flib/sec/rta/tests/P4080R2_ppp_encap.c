/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

uint32_t shr_addr = 0x00000B80ul;

LABEL(a);
REFERENCE(ref_jumpa);
LABEL(c);
REFERENCE(ref_jumpc);
LABEL(h);
REFERENCE(ref_jumph);
LABEL(k);
REFERENCE(ref_jumpk);
LABEL(l);
REFERENCE(ref_jumpl);
LABEL(q);
REFERENCE(refq_hdr);
LABEL(s);
REFERENCE(ref1_moves);
REFERENCE(ref2_moves);

int build_shr_desc_ppp_encap(struct program *p, uint32_t *buff,
			     unsigned buffpos)
{
	uint32_t c1_ctx_addr = 0x0972ecc0ul;
	uint32_t c2_ctx_addr = 0x094E03C0ul;

	LABEL(start);
	REFERENCE(pjump1);
	LABEL(b);
	REFERENCE(pjumpb);
	LABEL(d);
	REFERENCE(pjumpd);
	LABEL(do_nfifo);
	REFERENCE(pmove1);
	LABEL(e);
	REFERENCE(pjumpe);
	LABEL(aprm);
	REFERENCE(pjump2);
	LABEL(x);
	REFERENCE(pjumpp);
	LABEL(r);
	REFERENCE(pjumpr);
	REFERENCE(pmoves);
	REFERENCE(pjumps);

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	SHR_HDR(p, SHR_NEVER, 46, 0);
	{
		SET_LABEL(p, start);
		SEQSTORE(p, CONTEXT2, 20, 1, 0);
		MATHB(p, ZERO, ADD, 64, VSEQINSZ, 4, IMMED2);

		SET_LABEL(p, b);
		MATHB(p, SEQINSZ, SUB, 64, MATH0, 8, IFB | IMMED2);
		pjumpd = JUMP(p, d, LOCAL_JUMP, ANY_FALSE, MATH_N);
		/*
		 * RTA does not support generating MATH commands with LEN = 8
		 * and no immediate inlined in the command, thus use WORD(p, ).
		 */
		WORD(p, 0xaa240108);
		NFIFOADD(p, PAD, MSG1, 0, PAD_ZERO | LAST1);
		pmove1 = MOVE(p, MATH1, 0, DESCBUF, 0, 8, WAITCOMP | IMMED);
		MATHB(p, VSEQINSZ, ADD, MATH0, VSEQINSZ, 4, NFU);

		SET_LABEL(p, d);
		SEQFIFOLOAD(p, MSG1, 0, VLF);
		pmoves = MOVE(p, CONTEXT2, 0, DESCBUF, 0, 72, IMMED);
		pjumpe = JUMP(p, e, LOCAL_JUMP, ANY_FALSE, MATH_N);

		SET_LABEL(p, do_nfifo);
		WORD(p, 0);
		WORD(p, 0);

		SET_LABEL(p, e);
		MATHB(p, ZERO, ADD, 8, VSEQOUTSZ, 4, IMMED2);
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		MOVE(p, IFIFOAB2, 0, OFIFO, 0, 64, IMMED);
		pjump2 = JUMP(p, aprm, LOCAL_JUMP, ALL_TRUE, NOP);

		SET_LABEL(p, a);
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);

		SET_LABEL(p, aprm);
		MATHB(p, ZERO, ADD, OFIFO, MATH0, 8, 0);
		pjumps = SHR_HDR(p, SHR_NEVER, b, 0);

		SET_LABEL(p, h);
		MATHB(p, VSEQINSZ, ADD, MATH3, VSEQOUTSZ, 4, 0);
		SEQFIFOSTORE(p, MSG, 0, 0, VLF);
		MATHB(p, MATH3, AND, 7, MATH3, 4, IMMED2);
		pjumpp = JUMP(p, x, LOCAL_JUMP, ALL_TRUE, MATH_Z);
		MOVE(p, IFIFOABD, 0, OFIFO, 0, 8, IMMED);

		SET_LABEL(p, r);
		LOAD(p, 0x7e, IFIFO, 0, 1, IMMED);
		MATHB(p, MATH3, ADD, ONE, MATH3, 4, 0);
		MATHB(p, MATH3, AND, 7, MATH3, 4, IMMED2);
		pjumpr = JUMP(p, r, LOCAL_JUMP, ANY_FALSE, MATH_Z);

		SET_LABEL(p, x);
		MATHB(p, SEQINSZ, SUB, ONE, NONE, 4, 0);
		pjumpb = JUMP(p, b, LOCAL_JUMP, ANY_FALSE, MATH_N);
		JUMP(p, 0, HALT_STATUS, ALL_TRUE, 0);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, q);
		MOVE(p, CONTEXT1, 56, MATH2, 0, 8, WAITCOMP | IMMED);

		SET_LABEL(p, l);
		MATHB(p, MATH1, AND, MATH2, NONE, 1, 0);
		ref_jumpk = JUMP(p, k, LOCAL_JUMP, ALL_TRUE, MATH_Z);

		MOVE(p, CONTEXT2, 4, IFIFOAB1, 0, 1, IMMED);
		/*
		 * RTA does not support generating MATH commands with LEN = 8
		 * and no immediate inlined in the command, thus use WORD(p, ).
		 */
		WORD(p, 0xa8604008);

		SET_LABEL(p, s);
		LOAD(p, c1_ctx_addr, CONTEXT1, 0, 64, 0);
		LOAD(p, c2_ctx_addr, CONTEXT2, 0, 72, 0);
		pjump1 = SHR_HDR(p, SHR_NEVER, start, 0);
		WORD(p, 0);
		WORD(p, 0);
	}

	PATCH_HDR(p, pjump1, start);
	PATCH_JUMP(p, pjumpb, b);
	PATCH_JUMP(p, pjumpd, d);
	PATCH_MOVE(p, pmove1, do_nfifo);
	PATCH_JUMP(p, pjumpe, e);
	PATCH_JUMP(p, pjump2, aprm);
	PATCH_JUMP(p, pjumpp, x);
	PATCH_JUMP(p, pjumpr, r);
	PATCH_HDR(p, pjumps, s);
	PATCH_MOVE(p, pmoves, s);

	return PROGRAM_FINALIZE(p);
}

/*
 * We need 0x80 Math Register 2. Loading a constant would take 2 words but we
 * didn't have another word to spare. So how to get it? Well, the command at
 * the end of the class 1 context is an absolute jump using the Shared
 * Descriptor Header. When this command is used as an absolute jump, the only
 * parts of the command used are the command field (bits 31:27) and the Start
 * Word (bits 21:16). Therefore, we can "hide" the value 0x80 in the right
 * byte of the command. So, we move the 16 bytes ending with that byte to the
 * Math 2 register, which puts that byte in the right-most byte of Math 2.
 * Then, when we do operations on Math 2, we set the length to 1 byte, wiping
 * all the other bits in the register. And, there you have it - 0x80 using only
 * one word instead of two!
 */
int build_extra_cmds(struct program *p, uint32_t *buff, unsigned buffpos)
{
	REFERENCE(pjumpk);

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	{
		/* imm data for MATH cmd at end of PDB in shared descriptor */
		DWORD(p, 0x2000000000000000);
		MATHB(p, MATH3, ADD, ONE, MATH3, 4, 0);
		MATHB(p, MATH3, AND, 7, NONE, 4, IMMED2);
		pjumpk = JUMP(p, k, LOCAL_JUMP, ANY_FALSE, MATH_Z);
		NFIFOADD(p, IFIFO, MSG, 8, 0);
		MOVE(p, IFIFOABD, 0, OFIFO, 0, 8, IMMED);

		SET_LABEL(p, k);
		MOVE(p, MATH0, 0, IFIFOAB1, 0, 1, IMMED);
		MATHB(p, MATH0, LSHIFT, 8, MATH0, 8, IFB | IMMED2);
		MATHB(p, MATH2, RSHIFT, ONE, MATH2, 1, 0);
		ref_jumpl = JUMP(p, l, LOCAL_JUMP, ANY_FALSE, MATH_Z);
		ref1_moves = MOVE(p, CONTEXT2, 0, DESCBUF, 0, 64,
				  WAITCOMP | IMMED);
		ref_jumpc = SHR_HDR(p, SHR_NEVER, c, 0);
	}
	PATCH_JUMP(p, pjumpk, k);

	return PROGRAM_FINALIZE(p);
}

int build_more_cmds(struct program *p, uint32_t *buff, unsigned buffpos)
{
	LABEL(g);
	REFERENCE(pjumpg);

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	{
		MATHB(p, MATH0, XOR, 0x7d7d7d7d7d7d7d7d, MATH1, 8, IMMED2);
		MATHU(p, MATH1, ZBYT, MATH1, 8, 0);

		MATHB(p, MATH0, XOR, 0x7e7e7e7e7e7e7e7e, MATH2, 8, IMMED2);
		MATHU(p, MATH2, ZBYT, MATH2, 8, 0);
		MATHB(p, MATH1, OR, MATH2, MATH1, 8, 0);

		pjumpg = JUMP(p, g, LOCAL_JUMP, ALL_TRUE, MATH_Z);

		ref2_moves = MOVE(p, CONTEXT1, 0, DESCBUF, 0, 64,
				  WAITCOMP | IMMED);
		refq_hdr = SHR_HDR(p, SHR_NEVER, q, 0);

		SET_LABEL(p, g);
		MOVE(p, MATH0, 0, IFIFOAB1, 0, 8, IMMED);

		SET_LABEL(p, c);
		LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_NFIFO, 0, IMMED);
		MOVE(p, IFIFOABD, 0, OFIFO, 0, 8, IMMED);
		MATHB(p, VSEQOUTSZ, SUB, ONE, VSEQOUTSZ, 4, 0);
		ref_jumpa = JUMP(p, a, LOCAL_JUMP, ANY_FALSE, MATH_Z);
		ref_jumph = SHR_HDR(p, SHR_NEVER, h, 0);
	}
	PATCH_JUMP(p, pjumpg, g);

	return PROGRAM_FINALIZE(p);
}

int build_jbdesc_ppp_encap(struct program *p, uint32_t *buff, int buffpos)
{
	uint32_t in_addr = 0x09DDDB80ul;
	uint32_t in_len = 1450;
	uint32_t out_addr = 0x00000000ul;
	uint32_t out_len = 2902;

	if (buffpos < 0)
		return -EINVAL;

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	JOB_HDR(p, SHR_NEVER, buffpos, shr_addr, REO | SHR);
	{
		JUMP(p, 3, LOCAL_JUMP, ALL_TRUE, 0);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, 0);
		JUMP(p, 1, LOCAL_JUMP, ALL_TRUE, 0);
		SEQOUTPTR(p, out_addr, out_len, EXT);
		SEQINPTR(p, in_addr, in_len, EXT);
	}

	return PROGRAM_FINALIZE(p);
}

int main(int argc, char **argv)
{
	uint32_t shr[64];
	uint32_t c1_ctx[20];
	uint32_t c2_ctx[20];
	uint32_t job[20];

	int shr_size, job_size, c1_ctx_size, c2_ctx_size;

	struct program shr_desc_prgm;
	struct program job_desc_prgm;
	struct program extra_cmds_prgm;
	struct program more_cmds_prgm;

	rta_set_sec_era(RTA_SEC_ERA_2);

	memset(shr, 0x00, sizeof(shr));
	shr_size = build_shr_desc_ppp_encap(&shr_desc_prgm, shr, 0);

	memset(c1_ctx, 0x00, sizeof(c1_ctx));
	c1_ctx_size = build_extra_cmds(&extra_cmds_prgm, c1_ctx, 46);

	memset(c2_ctx, 0x00, sizeof(c2_ctx));
	c2_ctx_size = build_more_cmds(&more_cmds_prgm, c2_ctx, 46);

	memset(job, 0x00, sizeof(job));
	job_size = build_jbdesc_ppp_encap(&job_desc_prgm, job, shr_size);

	PATCH_MOVE(&extra_cmds_prgm, ref1_moves, s);
	PATCH_MOVE(&more_cmds_prgm, ref2_moves, s);
	PATCH_JUMP(&more_cmds_prgm, ref_jumpa, a);
	PATCH_HDR(&extra_cmds_prgm, ref_jumpc, c);
	PATCH_HDR(&more_cmds_prgm, ref_jumph, h);
	PATCH_JUMP(&shr_desc_prgm, ref_jumpk, k);
	PATCH_JUMP(&extra_cmds_prgm, ref_jumpl, l);
	PATCH_HDR(&more_cmds_prgm, refq_hdr, q);

	pr_debug("PPP decap program shared desc\n");
	pr_debug("size = %d\n", shr_size);
	print_prog((uint32_t *)shr, shr_size);

	pr_debug("context1\n");
	pr_debug("size = %d\n", c1_ctx_size);
	print_prog((uint32_t *)c1_ctx, c1_ctx_size);

	pr_debug("context2\n");
	pr_debug("size = %d\n", c2_ctx_size);
	print_prog((uint32_t *)c2_ctx, c2_ctx_size);

	pr_debug("PPP decap program job desc\n");
	pr_debug("size = %d\n", job_size);
	print_prog((uint32_t *)job, job_size);

	return 0;
}
