/*
 * crconf, linux crypto layer configuration.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <linux/cryptouser.h>
#include <linux/netlink.h>
#include <libnetlink.h>

static void usage(void) __attribute__((noreturn));

static void usage(void)
{
	fprintf(stderr,
			"Usage: crconf add { ALG | DRIVER } TYPE [ PRIORITY ]\n"
			"       crconf del DRIVER TYPE\n"
			"       crconf update DRIVER TYPE [ PRIORITY ]\n"
			"       crconf show { DRIVER TYPE | all }\n"
			"       crconf help\n"
			"ALG := alg <alg-name>\n"
			"DRIVER := driver <driver-name>\n"
			"TYPE := type ALGO-TYPE\n"
			"PRIORITY := priority <number>\n"
			"ALGO-TYPE := { 1 | 2 | 3 | 4 | 5 | 6 | 8 | 9 | 10 | 12 | 15 }\n"
			"               1 == alg type cipher\n"
			"               2 == alg type compress\n"
			"               3 == alg type aead\n"
			"               4 == alg type blkcipher\n"
			"               5 == alg type ablkcipher\n"
			"               6 == alg type givcipher\n"
			"               8 == alg type digest\n"
			"               8 == alg type hash\n"
			"               9 == alg type shash\n"
			"              10 == alg type ahash\n"
			"              12 == alg type rng\n"
			"              15 == alg type pcompress\n");
	exit(-1);
}

static int crconf_help(int argc, char **argv)
{
	usage();
}

/* These helper functions are borrowed from utils.c as it comes with iproute2. */
static int matches(const char *cmd, const char *pattern)
{
	int len = strlen(cmd);
	if (len > strlen(pattern))
		return -1;

	return memcmp(pattern, cmd, len);
}

static int __get_u32(__u32 *val, const char *arg, int base)
{
	unsigned long res;
	char *ptr;

	if (!arg || !*arg)
		return -1;
	res = strtoul(arg, &ptr, base);
	if (!ptr || ptr == arg || *ptr || res > 0xFFFFFFFFUL)
		return -1;
	*val = res;
	return 0;
}

static void invarg(const char *arg)
{
	fprintf(stderr, "Error: invalid argument \"%s\"\n", arg);
	usage();
}

static void get_u32(int *pargc, char ***pargv, __u32 *val, const char *what)
{
	(*pargc)--;
	(*pargv)++;
	if (!*pargc) {
		fprintf(stderr, "Error: \"%s\" requires an argument\n", what);
		exit(-1);
	}
	if (__get_u32(val, **pargv, 0)) {
		fprintf(stderr, "Error: \"%s\" argument \"%s\" is wrong\n",
			what, **pargv);
		exit(-1);
	}
}

static void copy_name(char *dst, char *src, size_t maxlen)
{
	if (strlen(src) >= maxlen) {
		fprintf(stderr, "Algorithm/Driver name '%s' too long, max "
			"length %zu!\n", src, maxlen - 1);
		exit(-1);
	}
	strncpy(dst, src, strlen(src));
}

int crconf_update_driver(int argc, char **argv)
{
	struct rtnl_handle rth;
	struct {
		struct nlmsghdr n;
		struct crypto_user_alg cru;
		char buf[1024];
	} req;

	if (argc < 1)
		exit(1);

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.cru));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = CRYPTO_MSG_UPDATEALG;

	copy_name(req.cru.cru_driver_name, argv[0],
		  sizeof(req.cru.cru_driver_name));
	argc--;
	argv++;

	while (argc > 0) {
		if (strcmp(*argv, "type") == 0) {
			get_u32(&argc, &argv, &req.cru.cru_type, "type");
			req.cru.cru_mask = CRYPTO_ALG_TYPE_MASK;
		} else if (strcmp(*argv, "priority") == 0) {
			__u32 prio;
			get_u32(&argc, &argv, &prio, "priority");
			addattr32(&req.n, sizeof(req), CRYPTOCFGA_PRIORITY_VAL, prio);
		} else
			invarg(*argv);

		argc--;
		argv++;
	}

	if (rtnl_open_byproto(&rth, 0, NETLINK_CRYPTO) < 0)
		exit(1);

	if (rtnl_talk(&rth, &req.n, NULL) < 0)
		exit(2);

	rtnl_close(&rth);

	return 0;

}

int crconf_update(int argc, char **argv)
{
	if (matches(*argv, "driver") == 0)
		return crconf_update_driver(argc-1, argv+1);

	exit(1);
}

static int crconf_del_alg(int argc, char **argv)
{
	fprintf(stderr, "'del alg' is not implementd, try 'del driver' instead.\n");
	exit(-1);
}

static int crconf_del_driver(int argc, char **argv)
{
	struct rtnl_handle rth;
	struct {
		struct nlmsghdr n;
		struct crypto_user_alg cru;
		char buf[1024];
	} req;

	if (argc < 1)
		exit(1);

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.cru));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = CRYPTO_MSG_DELALG;

	copy_name(req.cru.cru_driver_name, argv[0],
		  sizeof(req.cru.cru_driver_name));
	argc--;
	argv++;

	while (argc > 0) {
		if (strcmp(*argv, "type") == 0) {
			get_u32(&argc, &argv, &req.cru.cru_type, "type");
			req.cru.cru_mask = CRYPTO_ALG_TYPE_MASK;
		} else {
			invarg(*argv);
		}

		argc--;
		argv++;
	}

	if (rtnl_open_byproto(&rth, 0, NETLINK_CRYPTO) < 0)
		exit(1);

	if (rtnl_talk(&rth, &req.n, NULL) < 0)
		exit(2);

	rtnl_close(&rth);

	return 0;

}

static int crconf_del(int argc, char **argv)
{
	if (argc == 1)
		usage();

	if (matches(*argv, "driver") == 0)
		return crconf_del_driver(argc-1, argv+1);
	else if (matches(*argv, "alg") == 0)
		return crconf_del_alg(argc-1, argv+1);

	usage();
}

static int crconf_add_alg(int argc, char **argv)
{
	struct rtnl_handle rth;
	struct {
		struct nlmsghdr n;
		struct crypto_user_alg cru;
		char buf[1024];
	} req;
	if (argc < 1)
		exit(1);
	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.cru));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = CRYPTO_MSG_NEWALG;

	copy_name(req.cru.cru_name, argv[0], sizeof(req.cru.cru_name));
	argc--;
	argv++;

	while (argc > 0) {
		if (strcmp(*argv, "type") == 0) {
			get_u32(&argc, &argv, &req.cru.cru_type, "type");
			req.cru.cru_mask = CRYPTO_ALG_TYPE_MASK;
		} else {
			invarg(*argv);
		}

		argc--;
		argv++;
	}

	if (rtnl_open_byproto(&rth, 0, NETLINK_CRYPTO) < 0)
		exit(1);

	if (rtnl_talk(&rth, &req.n, NULL) < 0)
		exit(2);

	rtnl_close(&rth);

	return 0;

}

static int crconf_add_driver(int argc, char **argv)
{
	struct rtnl_handle rth;
	struct {
		struct nlmsghdr n;
		struct crypto_user_alg cru;
		char buf[1024];
	} req;

	if (argc < 1)
		exit(1);

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.cru));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = CRYPTO_MSG_NEWALG;

	copy_name(req.cru.cru_driver_name, argv[0],
		 sizeof(req.cru.cru_driver_name));
	argc--;
	argv++;

	while (argc > 0) {
		if (strcmp(*argv, "type") == 0) {
			get_u32(&argc, &argv, &req.cru.cru_type, "type");
			req.cru.cru_mask = CRYPTO_ALG_TYPE_MASK;
		} else if (strcmp(*argv, "priority") == 0) {
			__u32 prio;
			get_u32(&argc, &argv, &prio, "priority");
			addattr32(&req.n, sizeof(req), CRYPTOCFGA_PRIORITY_VAL, prio);
		} else
			invarg(*argv);

		argc--;
		argv++;
	}

	if (rtnl_open_byproto(&rth, 0, NETLINK_CRYPTO) < 0)
		exit(1);

	if (rtnl_talk(&rth, &req.n, NULL) < 0)
		exit(2);

	rtnl_close(&rth);

	return 0;

}

static int crconf_add(int argc, char **argv)
{
	if (argc == 1)
		usage();

	if (matches(*argv, "driver") == 0)
		return crconf_add_driver(argc-1, argv+1);
	else if (matches(*argv, "alg") == 0)
		return crconf_add_alg(argc-1, argv+1);

	usage();
}

static void crypto_alg_print_base(struct crypto_user_alg *ualg,  FILE *fp)
{
	fprintf(fp, "name        : %s\n", ualg->cru_name);
	fprintf(fp, "driver_name : %s\n", ualg->cru_driver_name);
	fprintf(fp, "module_name : %s\n", ualg->cru_module_name);
	fprintf(fp, "refcnt      : %d\n", ualg->cru_refcnt);
	fprintf(fp, "selftest    : %s\n",
		(ualg->cru_flags & CRYPTO_ALG_TESTED) ? "passed" : "unknown");
	fprintf(fp, "async       : %s\n",
		(ualg->cru_flags & CRYPTO_ALG_ASYNC) ? "yes" : "no");
	fprintf(fp, "flags       : 0x%x\n", ualg->cru_flags);
}

static void crypto_print_priority(__u32 priority, FILE *fp)
{
	fprintf(fp, "priority    : %d\n", priority);
}

static void crypto_print_larval(struct crypto_report_larval *rl, FILE *fp)
{
	fprintf(fp, "type        : %s\n", rl->type);
}

static void crypto_print_hash(struct crypto_report_hash *rsh, FILE *fp)
{
	fprintf(fp, "type        : %s\n", rsh->type);
	fprintf(fp, "blocksize   : %d\n", rsh->blocksize);
	fprintf(fp, "digestsize  : %d\n", rsh->digestsize);
}

static void crypto_print_cipher(struct crypto_report_cipher *rcip, FILE *fp)
{
	fprintf(fp, "type        : %s\n", rcip->type);
	fprintf(fp, "blocksize   : %d\n", rcip->blocksize);
	fprintf(fp, "min_keysize : %d\n", rcip->min_keysize);
	fprintf(fp, "max_keysize : %d\n", rcip->max_keysize);
}

static void crypto_print_blkcipher(struct crypto_report_blkcipher *rblk, FILE *fp)
{
	fprintf(fp, "type        : %s\n", rblk->type);
	fprintf(fp, "blocksize   : %d\n", rblk->blocksize);
	fprintf(fp, "min_keysize : %d\n", rblk->min_keysize);
	fprintf(fp, "max_keysize : %d\n", rblk->max_keysize);
	fprintf(fp, "ivsize      : %d\n", rblk->ivsize);
	fprintf(fp, "geniv       : %s\n", rblk->geniv);
}

static void crypto_print_aead(struct crypto_report_aead *raead, FILE *fp)
{
	fprintf(fp, "type        : %s\n", raead->type);
	fprintf(fp, "blocksize   : %d\n", raead->blocksize);
	fprintf(fp, "ivsize      : %d\n", raead->ivsize);
	fprintf(fp, "maxauthsize : %d\n", raead->maxauthsize);
	fprintf(fp, "geniv       : %s\n", raead->geniv);
}

static void crypto_print_comp(struct crypto_report_comp *rcomp, FILE *fp)
{
	fprintf(fp, "type        : %s\n", rcomp->type);
}

static void crypto_print_rng(struct crypto_report_rng *rrng, FILE *fp)
{
	fprintf(fp, "type        : %s\n", rrng->type);
	fprintf(fp, "seedsize    : %d\n", rrng->seedsize);
}

static void crypto_alg_print_attr(struct rtattr *tb[], FILE *fp)
{
	if (tb[CRYPTOCFGA_PRIORITY_VAL]) {
		struct rtattr *rta = tb[CRYPTOCFGA_PRIORITY_VAL];
		__u32 priority = *(__u32 *) RTA_DATA(rta);
		crypto_print_priority(priority, fp);
	}

	if (tb[CRYPTOCFGA_REPORT_LARVAL]) {
		struct rtattr *rta = tb[CRYPTOCFGA_REPORT_LARVAL];
		struct crypto_report_larval *rl = (struct crypto_report_larval *) RTA_DATA(rta);
		crypto_print_larval(rl, fp);
	}

	if (tb[CRYPTOCFGA_REPORT_HASH]) {
		struct rtattr *rta = tb[CRYPTOCFGA_REPORT_HASH];
		struct crypto_report_hash *rsh = (struct crypto_report_hash *) RTA_DATA(rta);
		crypto_print_hash(rsh, fp);
	}

	if (tb[CRYPTOCFGA_REPORT_BLKCIPHER]) {
		struct rtattr *rta = tb[CRYPTOCFGA_REPORT_BLKCIPHER];
		struct crypto_report_blkcipher *rblk = (struct crypto_report_blkcipher *) RTA_DATA(rta);
		crypto_print_blkcipher(rblk, fp);
	}

	if (tb[CRYPTOCFGA_REPORT_AEAD]) {
		struct rtattr *rta = tb[CRYPTOCFGA_REPORT_AEAD];
		struct crypto_report_aead *raead = (struct crypto_report_aead *) RTA_DATA(rta);
		crypto_print_aead(raead, fp);
	}

	if (tb[CRYPTOCFGA_REPORT_RNG]) {
		struct rtattr *rta = tb[CRYPTOCFGA_REPORT_RNG];
		struct crypto_report_rng *rrng = (struct crypto_report_rng *) RTA_DATA(rta);
		crypto_print_rng(rrng, fp);
	}

	if (tb[CRYPTOCFGA_REPORT_CIPHER]) {
		struct rtattr *rta = tb[CRYPTOCFGA_REPORT_CIPHER];
		struct crypto_report_cipher *rcip = (struct crypto_report_cipher *) RTA_DATA(rta);
		crypto_print_cipher(rcip, fp);
	}

	if (tb[CRYPTOCFGA_REPORT_COMPRESS]) {
		struct rtattr *rta = tb[CRYPTOCFGA_REPORT_COMPRESS];
		struct crypto_report_comp *rcomp = (struct crypto_report_comp *) RTA_DATA(rta);
		crypto_print_comp(rcomp, fp);
	}
}

static int crypto_alg_print(const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg)
{
	FILE *fp = (FILE*)arg;
	struct rtattr * tb[CRYPTOCFGA_MAX+1];
	struct rtattr * rta;
	struct crypto_user_alg *ualg = NULL;
	int len = n->nlmsg_len;

	if (n->nlmsg_type == CRYPTO_MSG_GETALG) {
		ualg = NLMSG_DATA(n);
		len -= NLMSG_SPACE(sizeof(*ualg));
	}

	if (len < 0) {
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	rta = CR_RTA(ualg);

	parse_rtattr(tb, CRYPTOCFGA_MAX, rta, len);

	crypto_alg_print_base(ualg, fp);

	crypto_alg_print_attr(tb, fp);

	fprintf(fp, "\n");

	fflush(fp);

	return 0;
}

static int crconf_show_all(int argc, char **argv)
{
	struct rtnl_handle rth;

	if (rtnl_open_byproto(&rth, 0, NETLINK_CRYPTO) < 0)
		exit(1);

	if (rtnl_wilddump_request(&rth, AF_UNSPEC, CRYPTO_MSG_GETALG) < 0)
		exit(1);

	if (rtnl_dump_filter(&rth, crypto_alg_print, stdout) < 0)
		exit(1);

	rtnl_close(&rth);

	return 0;

}

static int crconf_show_driver(int argc, char **argv)
{
	struct rtnl_handle rth;
	struct nlmsghdr *res_n = NULL;
	struct {
		struct nlmsghdr n;
		struct crypto_user_alg cru;
	} req;

	if (argc == 0) {
		fprintf(stderr, "Need to specify a driver!");
		exit(-1);
	}

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.cru));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = CRYPTO_MSG_GETALG;

	copy_name(req.cru.cru_driver_name, argv[0],
		  sizeof(req.cru.cru_driver_name));

	argc--;
	argv++;

	while (argc > 0) {
		if (strcmp(*argv, "type") == 0) {
			get_u32(&argc, &argv, &req.cru.cru_type, "type");
			req.cru.cru_mask = CRYPTO_ALG_TYPE_MASK;
		} else {
			invarg(*argv);
		}

		argc--;
		argv++;
	}

	if (rtnl_open_byproto(&rth, 0, NETLINK_CRYPTO) < 0)
		exit(1);

	if (rtnl_talk(&rth, &req.n, &res_n) < 0)
		exit(2);

	if (crypto_alg_print(NULL, res_n, (void*)stdout) < 0)
		exit(1);

	free(res_n);
	rtnl_close(&rth);

	return 0;

}

static int crconf_show(int argc, char **argv)
{
	if (matches(*argv, "driver") == 0)
		return crconf_show_driver(argc-1, argv+1);
	else if (matches(*argv, "all") == 0)
		return crconf_show_all(argc-1, argv+1);

	usage();
}


static const struct cmd {
	const char *cmd;
	int (*func)(int argc, char **argv);
} cmds[] = {
	{ "add", 	crconf_add },
	{ "del",	crconf_del },
	{ "update",	crconf_update },
	{ "show",	crconf_show },
	{ "help",	crconf_help },
	{ 0 }
};

static int crconf_cmd(const char *argv0, int argc, char **argv)
{
	const struct cmd *c;

	if (argc == 1)
		usage();

	for (c = cmds; c->cmd; ++c) {
		if (matches(argv0, c->cmd) == 0) {
			return c->func(argc-1, argv+1);
		}
	}

	fprintf(stderr, "Object \"%s\" is unknown, try \"crconf help\".\n", argv0);
	return -1;
}

int main(int argc, char **argv)
{
	while (argc > 1) {
		char *opt = argv[1];
		if (opt[0] != '-')
			break;
		if (opt[1] == '-')
			opt++;
		if (matches(opt, "-help") == 0) {
			usage();
		} else {
			fprintf(stderr, "Option \"%s\" is unknown, try \"crconf -help\".\n", opt);
			exit(-1);
		}
		argc--;
		argv++;
	}

	if (argc > 1)
		return crconf_cmd(argv[1], argc-1, argv+1);

	usage();
}
