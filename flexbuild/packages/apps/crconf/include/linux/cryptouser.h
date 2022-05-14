/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Crypto user configuration API.
 *
 * Copyright (C) 2011 secunet Security Networks AG
 * Copyright (C) 2011 Steffen Klassert <steffen.klassert@secunet.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/types.h>
#include <libnetlink.h>

#define CRYPTO_MAX_ALG_NAME 64
#define NLMSG_BUF_SIZE 4096

/*  
 * Algorithm masks and types. 
 */ 
#define CRYPTO_ALG_TYPE_MASK		0x0000000f
#define CRYPTO_ALG_TYPE_CIPHER		0x00000001
#define CRYPTO_ALG_TYPE_COMPRESS	0x00000002
#define CRYPTO_ALG_TYPE_AEAD		0x00000003
#define CRYPTO_ALG_TYPE_BLKCIPHER	0x00000004
#define CRYPTO_ALG_TYPE_ABLKCIPHER	0x00000005
#define CRYPTO_ALG_TYPE_SKCIPHER	0x00000005
#define CRYPTO_ALG_TYPE_GIVCIPHER	0x00000006
#define CRYPTO_ALG_TYPE_KPP		0x00000008
#define CRYPTO_ALG_TYPE_ACOMPRESS	0x0000000a
#define CRYPTO_ALG_TYPE_SCOMPRESS	0x0000000b
#define CRYPTO_ALG_TYPE_RNG		0x0000000c
#define CRYPTO_ALG_TYPE_AKCIPHER	0x0000000d
#define CRYPTO_ALG_TYPE_DIGEST		0x0000000e
#define CRYPTO_ALG_TYPE_HASH		0x0000000e
#define CRYPTO_ALG_TYPE_SHASH		0x0000000e
#define CRYPTO_ALG_TYPE_AHASH		0x0000000f

#define CRYPTO_ALG_TYPE_HASH_MASK	0x0000000e
#define CRYPTO_ALG_TYPE_AHASH_MASK	0x0000000e
#define CRYPTO_ALG_TYPE_BLKCIPHER_MASK	0x0000000c
#define CRYPTO_ALG_TYPE_ACOMPRESS_MASK	0x0000000e

#define CRYPTO_ALG_LARVAL		0x00000010
#define CRYPTO_ALG_DEAD			0x00000020
#define CRYPTO_ALG_DYING		0x00000040
#define CRYPTO_ALG_ASYNC		0x00000080

/*
 * Set this bit if and only if the algorithm requires another algorithm of
 * the same type to handle corner cases.
 */
#define CRYPTO_ALG_NEED_FALLBACK	0x00000100

/*
 * This bit is set for symmetric key ciphers that have already been wrapped
 * with a generic IV generator to prevent them from being wrapped again.
 */
#define CRYPTO_ALG_GENIV		0x00000200

/*
 * Set if the algorithm has passed automated run-time testing.  Note that
 * if there is no run-time testing for a given algorithm it is considered
 * to have passed.
 */

#define CRYPTO_ALG_TESTED		0x00000400

/*
 * Set if the algorithm is an instance that is build from telplates.
 */
#define CRYPTO_ALG_INSTANCE		0x00000800

/* Netlink configuration messages.  */
enum {
	CRYPTO_MSG_BASE = 0x10,
	CRYPTO_MSG_NEWALG = 0x10,
	CRYPTO_MSG_DELALG,
	CRYPTO_MSG_UPDATEALG,
	CRYPTO_MSG_GETALG,
	CRYPTO_MSG_DELRNG,
	__CRYPTO_MSG_MAX
};
#define CRYPTO_MSG_MAX (__CRYPTO_MSG_MAX - 1)
#define CRYPTO_NR_MSGTYPES (CRYPTO_MSG_MAX + 1 - CRYPTO_MSG_BASE)

#define CRYPTO_MAX_NAME 64

/* Netlink message attributes.  */
enum crypto_attr_type_t {
	CRYPTOCFGA_UNSPEC,
	CRYPTOCFGA_PRIORITY_VAL,	/* __u32 */
	CRYPTOCFGA_REPORT_LARVAL,	/* struct crypto_report_larval */
	CRYPTOCFGA_REPORT_HASH,		/* struct crypto_report_hash */
	CRYPTOCFGA_REPORT_BLKCIPHER,	/* struct crypto_report_blkcipher */
	CRYPTOCFGA_REPORT_AEAD,		/* struct crypto_report_aead */
	CRYPTOCFGA_REPORT_COMPRESS,	/* struct crypto_report_comp */
	CRYPTOCFGA_REPORT_RNG,		/* struct crypto_report_rng */
	CRYPTOCFGA_REPORT_CIPHER,	/* struct crypto_report_cipher */
	CRYPTOCFGA_REPORT_AKCIPHER,	/* struct crypto_report_akcipher */
	CRYPTOCFGA_REPORT_KPP,		/* struct crypto_report_kpp */
	CRYPTOCFGA_REPORT_ACOMP,	/* struct crypto_report_acomp */
	__CRYPTOCFGA_MAX

#define CRYPTOCFGA_MAX (__CRYPTOCFGA_MAX - 1)
};

struct crypto_user_alg {
	char cru_name[CRYPTO_MAX_NAME];
	char cru_driver_name[CRYPTO_MAX_NAME];
	char cru_module_name[CRYPTO_MAX_NAME];
	__u32 cru_type;
	__u32 cru_mask;
	__u32 cru_refcnt;
	__u32 cru_flags;
};

struct crypto_report_larval {
	char type[CRYPTO_MAX_NAME];
};

struct crypto_report_hash {
	char type[CRYPTO_MAX_NAME];
	unsigned int blocksize;
	unsigned int digestsize;
};

struct crypto_report_cipher {
	char type[CRYPTO_MAX_NAME];
	unsigned int blocksize;
	unsigned int min_keysize;
	unsigned int max_keysize;
};

struct crypto_report_blkcipher {
	char type[CRYPTO_MAX_NAME];
	char geniv[CRYPTO_MAX_NAME];
	unsigned int blocksize;
	unsigned int min_keysize;
	unsigned int max_keysize;
	unsigned int ivsize;
};

struct crypto_report_aead {
	char type[CRYPTO_MAX_NAME];
	char geniv[CRYPTO_MAX_NAME];
	unsigned int blocksize;
	unsigned int maxauthsize;
	unsigned int ivsize;
};

struct crypto_report_comp {
	char type[CRYPTO_MAX_NAME];
};

struct crypto_report_rng {
	char type[CRYPTO_MAX_NAME];
	unsigned int seedsize;
};

struct crypto_report_akcipher {
	char type[CRYPTO_MAX_NAME];
};

struct crypto_report_kpp {
	char type[CRYPTO_MAX_NAME];
};

struct crypto_report_acomp {
	char type[CRYPTO_MAX_NAME];
};

#define CRYPTO_REPORT_MAXSIZE (sizeof(struct crypto_user_alg) + \
			       sizeof(struct crypto_report_blkcipher))

#define CR_RTA(x)  ((struct rtattr*)(((char*)(x)) + NLMSG_ALIGN(sizeof(struct crypto_user_alg))))

