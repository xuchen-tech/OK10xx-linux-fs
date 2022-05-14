/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "flib/rta.h"
#include "test_common.h"

enum rta_sec_era rta_sec_era;

uint32_t shr_addr = 0x089ae500ul;

LABEL(c);
REFERENCE(refc_shr);
LABEL(a);
REFERENCE(ref1_jumpa);
LABEL(h);
REFERENCE(refh_shr);
LABEL(s);
REFERENCE(ref1_moves);
REFERENCE(ref2_moves);
LABEL(t);
REFERENCE(ref_jumpt);
LABEL(k);
REFERENCE(ref_jumpk);
LABEL(q);
REFERENCE(refq_shr);
LABEL(l);
REFERENCE(ref_jumpl);

uint64_t desc_addr_1 = 0x00000ac0ull;

int build_shr_desc_ppp_decap(struct program *p, uint32_t *buff,
			     unsigned buffpos)
{
	uint32_t c1_ctx_addr = 0x08887A00ul;
	uint32_t c2_ctx_addr = 0x09CB4780ul;

	LABEL(b);
	REFERENCE(pjumpb1);
	REFERENCE(pjumpb2);
	LABEL(d);
	REFERENCE(pjumpd);
	LABEL(do_nfifo);
	REFERENCE(pmove1);
	LABEL(e);
	REFERENCE(pjumpe);
	LABEL(x);
	REFERENCE(pjumpp);
	LABEL(r);
	REFERENCE(pjumpr);
	REFERENCE(pmoves);
	REFERENCE(pjumps);

	/*
	 * NOTES ON HOW THIS DESCRIPTOR WORKS
	 * This descriptor is far too large to fit in the Descriptor Buffer.
	 * However, jumping from one descriptor to another isn't practical
	 * because we'd then have to jump back again. To get around this
	 * problem, I use the two context register. The Descriptor Buffer
	 * holds 64 words. Class 1 Context holds 16 words and Class 2 Context
	 * holds 18 words. So, things work like this:
	 *
	 * 1. The Job Descriptor holds the SEQ IN PTR and SEQ OUT PTR commands.
	 * After this runs, the Shared Descriptor starts.
	 *
	 * 2. The Shared Descriptor jumps to the end of the Shared Descriptor,
	 * where the Class 1 and 2 Context Registers are loaded. What we're
	 * putting in there is actually more descriptor. The Class 1 Context
	 * Register holds the compare routine, which detects if there's a 0x7D
	 * byte in the current dword. The Class 2 Context Register holds the
	 * Special Character Handler. (Acually, it holds part of this the rest
	 * of the special handler is in the Shared Descriptor.)
	 *
	 * 3. Start processing the data. Copy Class 1 Context over the end of
	 * the Shared Descriptor, also replacing the entire Job Descriptor.
	 * If there's a special character, copy the Class 2 Context over the
	 * last 18 words of the Descriptor Buffer, replacing the compare
	 * routine. When the special handling is done, copy Class 1 Context
	 * back again. So, we keep copying them back in as needed.
	 *
	 * 4. Care must be taken when the last byte of a dword, especially the
	 * last byte of a burst, is 0x7D. Then it must be remembered for the
	 * next byte that this byte needs to be XORed with 0x20 to flip it
	 * back!
	 *
	 * 5. Since unstuffing shortens the output frame, we can handle 128
	 * bytes at a time.
	 *
	 * NOTE: The first nine words of the special handling routine are in
	 *       the Shared Descriptor. Immediately following these nine words
	 *       is where the context register contents are copied. The Compare
	 *       Routine is self contained. However, the special handler in the
	 *       context is actually the continuation of the first nine words.
	 *
	 * NOTE: This was all set up so that it would work with 36-bit pointers.
	 *       That is, all pointers needing 2 words instead of 1. So, there
	 *       are "empty" words in various places to compensate for the fact
	 *       that these words would contain addresss if 36-bit addressing
	 *       was used instead of 32-bit addressing.
	 */
	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	SHR_HDR(p, SHR_NEVER, 46, 0);
	{
		SET_LABEL(p, b);
		MATHB(p, ZERO, ADD, 128, VSEQINSZ, 4, IMMED2);
		MATHB(p, SEQINSZ, SUB, 128, MATH0, 8, IFB | IMMED2);
		pjumpd = JUMP(p, d, LOCAL_JUMP, ANY_FALSE, MATH_N);
		/*
		 * RTA does not support generating MATH commands with LEN = 8
		 * and no immediate inlined in the command, thus use WORD(p, ).
		 */
		WORD(p, 0xaa240108);
		NFIFOADD(p, PAD, MSG1, 0, PAD_ZERO | LAST1);
		pmove1 = MOVE(p, MATH1, 0, DESCBUF, 0, 8, IMMED);
		MATHB(p, VSEQINSZ, ADD, MATH0, VSEQINSZ, 4, NFU);

		SET_LABEL(p, d);
		SEQFIFOLOAD(p, MSG1, 0, VLF);
		pmoves = MOVE(p, CONTEXT1, 0, DESCBUF, 0, 64, IMMED);
		pjumpe = JUMP(p, e, LOCAL_JUMP, ANY_FALSE, MATH_N);

		SET_LABEL(p, do_nfifo);
		WORD(p, 0);
		WORD(p, 0);

		SET_LABEL(p, e);
		MATHB(p, ZERO, ADD, 16, VSEQOUTSZ, 4, IMMED2);
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		MOVE(p, IFIFOAB1, 0, OFIFO, 0, 128, IMMED);

		SET_LABEL(p, a);
		LOAD(p, 0, DCTRL, LDOFF_DISABLE_AUTO_NFIFO, 0, IMMED);
		pjumps = JUMP(p, s, LOCAL_JUMP, ALL_TRUE, NOP);
		/* End of Main Loop - all 8 dwords have been handled */

		SET_LABEL(p, h);
		LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_NFIFO, 0, IMMED);
		MATHB(p, VSEQINSZ, ADD, MATH2, VSEQOUTSZ, 4, 0);
		SEQFIFOSTORE(p, MSG, 0, 0, VLF);
		MATHB(p, ZERO, ADD, MATH3, NONE, 4, 0);
		pjumpp = JUMP(p, x, LOCAL_JUMP, ALL_TRUE, MATH_Z);
		MATHB(p, MATH3, SUB, 8, MATH3, 8, IFB | IMMED2);
		MOVE(p, IFIFOABD, 0, OFIFO, 0, 8, IMMED);

		SET_LABEL(p, r);
		LOAD(p, 0x7E, IFIFO, 0, 1, IMMED);
		MATHB(p, MATH3, ADD, ONE, MATH3, 4, 0);
		pjumpr = JUMP(p, r, LOCAL_JUMP, ANY_FALSE, MATH_Z);

		SET_LABEL(p, x);
		MATHB(p, SEQINSZ, SUB, ONE, NONE, 4, 0);
		JUMP(p, 0, HALT_STATUS, ALL_TRUE, MATH_N);
		pjumpb1 = JUMP(p, b, LOCAL_JUMP, ALL_TRUE, 0);

		SET_LABEL(p, q);
		MATHB(p, ZERO, ADD, 0x80, MATH2, 4, IMMED2);

		SET_LABEL(p, l);
		MATHB(p, MATH1, AND, MATH2, NONE, 4, 0);
		ref_jumpk = JUMP(p, k, LOCAL_JUMP, ANY_FALSE, MATH_Z);
		MOVE(p, MATH0, 0, IFIFOAB1, 0, 1, IMMED);
		MATHB(p, MATH3, ADD, ONE, MATH3, 8, 0);
		MATHB(p, MATH3, AND, 7, MATH3, 8, IFB | IMMED2);
		ref_jumpt = JUMP(p, t, LOCAL_JUMP, ANY_FALSE, MATH_Z);

		SET_LABEL(p, s);
		LOAD(p, c1_ctx_addr, CONTEXT1, 0, 64, 0);
		LOAD(p, c2_ctx_addr, CONTEXT2, 0, 72, 0);
		pjumpb2 = SHR_HDR(p, SHR_NEVER, b, 0);
		WORD(p, 0);
		WORD(p, 0);
	}

	PATCH_JUMP(p, pjumpb1, b);
	PATCH_HDR(p, pjumpb2, b);
	PATCH_JUMP(p, pjumpd, d);
	PATCH_MOVE(p, pmove1, do_nfifo);
	PATCH_MOVE(p, pmoves, s);
	PATCH_JUMP(p, pjumps, s);
	PATCH_JUMP(p, pjumpe, e);
	PATCH_JUMP(p, pjumpp, x);
	PATCH_JUMP(p, pjumpr, r);

	return PROGRAM_FINALIZE(p);
}

int build_extra_cmds(struct program *p, uint32_t *buff, unsigned buffpos)
{
	LABEL(g);
	REFERENCE(pjumpg);

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	{
		MATHB(p, ZERO, ADD, OFIFO, MATH0, 8, 0);
		MATHB(p, MATH0, XOR, 0x7d7d7d7d7d7d7d7d, MATH1, 8, IMMED2);
		/* in case last of prev dword was special */
		MATHB(p, MATH0, XOR, MATH2, MATH0, 8, 0);
		MATHB(p, ZERO, AND, MATH2, MATH2, 8, 0);
		MATHU(p, MATH1, ZBYT, MATH1, 8, 0);
		pjumpg = JUMP(p, g, LOCAL_JUMP, ALL_TRUE, MATH_Z);
		ref1_moves = MOVE(p, CONTEXT2, 0, DESCBUF, 0, 72,
				  WAITCOMP | IMMED);
		refq_shr = SHR_HDR(p, SHR_NEVER, q, 0);

		SET_LABEL(p, g);
		MOVE(p, MATH0, 0, IFIFOAB1, 0, 8, IMMED);
		LOAD(p, 0, DCTRL, LDOFF_ENABLE_AUTO_NFIFO, 0, IMMED);
		MOVE(p, IFIFOABD, 0, OFIFO, 0, 8, IMMED);

		SET_LABEL(p, c);
		MATHB(p, VSEQOUTSZ, SUB, ONE, VSEQOUTSZ, 4, 0);
		ref1_jumpa = JUMP(p, a, LOCAL_JUMP, ANY_FALSE, MATH_Z);
		refh_shr = SHR_HDR(p, SHR_NEVER, h, 0);
	}
	PATCH_JUMP(p, pjumpg, g);

	return PROGRAM_FINALIZE(p);
}

int build_more_cmds(struct program *p, uint32_t *buff, unsigned buffpos)
{
	REFERENCE(pjumpt);
	LABEL(i);
	REFERENCE(pjumpi);

	PROGRAM_CNTXT_INIT(p, buff, buffpos);
	if (need_bswap)
		PROGRAM_SET_BSWAP(p);
	{
		NFIFOADD(p, IFIFO, MSG, 8, 0);
		MOVE(p, IFIFOABD, 0, OFIFO, 0, 8, IMMED);

		SET_LABEL(p, t);
		MATHB(p, MATH0, LSHIFT, 8, MATH0, 8, IFB | IMMED2);
		MATHB(p, MATH2, RSHIFT, ONE, MATH2, 4, 0);
		ref_jumpl = JUMP(p, l, LOCAL_JUMP, ANY_FALSE, MATH_Z);

		SET_LABEL(p, i);
		ref2_moves = MOVE(p, CONTEXT1, 0, DESCBUF, 0, 64,
				  WAITCOMP | IMMED);
		refc_shr = SHR_HDR(p, SHR_NEVER, c, 0);

		SET_LABEL(p, k);
		MATHB(p, MATH0, XOR, 0x0020000000000000, MATH0, 8, IMMED2);
		MATHB(p, VSEQINSZ, SUB, ONE, VSEQINSZ, 4, 0);
		MATHB(p, MATH2, SUB, ONE, NONE, 8, 0);

		pjumpt = JUMP(p, t, LOCAL_JUMP, ANY_FALSE, MATH_Z);
		MATHB(p, MATH2, LSHIFT, 61, MATH2, 8, IFB | IMMED2);
		pjumpi = JUMP(p, i, LOCAL_JUMP, ALL_TRUE, 0);
	}
	PATCH_JUMP(p, pjumpt, t);
	PATCH_JUMP(p, pjumpi, i);

	return PROGRAM_FINALIZE(p);
}

int build_jbdesc_ppp_decap(struct program *p, uint32_t *buff, int buffpos)
{
	uint32_t in_addr = 0x00000040ul;
	uint32_t in_len = 1450;
	uint32_t out_addr = 0x0818fe00ul;
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
	shr_size = build_shr_desc_ppp_decap(&shr_desc_prgm, shr, 0);

	memset(c1_ctx, 0x00, sizeof(c1_ctx));
	c1_ctx_size = build_extra_cmds(&extra_cmds_prgm, c1_ctx,  46);

	memset(c2_ctx, 0x00, sizeof(c2_ctx));
	c2_ctx_size = build_more_cmds(&more_cmds_prgm, c2_ctx, 46);

	memset(job, 0x00, sizeof(job));
	job_size = build_jbdesc_ppp_decap(&job_desc_prgm, job, shr_size);

	PATCH_HDR(&more_cmds_prgm, refc_shr, c);
	PATCH_JUMP(&extra_cmds_prgm, ref1_jumpa, a);
	PATCH_HDR(&extra_cmds_prgm, refh_shr, h);
	PATCH_MOVE(&extra_cmds_prgm, ref1_moves, s);
	PATCH_MOVE(&more_cmds_prgm, ref2_moves, s);
	PATCH_JUMP(&shr_desc_prgm, ref_jumpt, t);
	PATCH_JUMP(&shr_desc_prgm, ref_jumpk, k);
	PATCH_HDR(&extra_cmds_prgm, refq_shr, q);
	PATCH_JUMP(&more_cmds_prgm, ref_jumpl, l);

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
