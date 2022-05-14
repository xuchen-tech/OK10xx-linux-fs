/* Copyright (c) 2014, ENEA Software AB
 * Copyright (c) 2014, Nokia
 * All rights reserved.
 *
 * SPDX-License-Identifier:	BSD-3-Clause
 */

#include "ofp.h"
#include "suite_framework.h"

static void *suite_thread1(void *arg);
static void *suite_thread2(void *arg);

int fd_thread1 = -1;
int fd_thread2 = -1;
int core_id = -1;

int config_suite_framework(uint16_t linux_core_id)
{
	core_id = linux_core_id;

	return 0;
}

int init_suite(init_function init_func)
{
	fd_thread1 = -1;
	fd_thread2 = -1;

	if (init_func)
		return init_func(&fd_thread1, &fd_thread2);
	else
		return 0;
}

void run_suite(odp_instance_t instance,
	run_function run_func1, run_function run_func2)
{
	odph_linux_pthread_t sock_pthread1;
	odph_linux_pthread_t sock_pthread2;
	odp_cpumask_t sock_cpumask;
	odph_linux_thr_params_t thr_params;

	odp_cpumask_zero(&sock_cpumask);
	odp_cpumask_set(&sock_cpumask, core_id);

	thr_params.start = suite_thread1;
	thr_params.arg = run_func1;
	thr_params.thr_type = ODP_THREAD_CONTROL;
	thr_params.instance = instance;
	odph_linux_pthread_create(&sock_pthread1,
			&sock_cpumask,
			&thr_params);

	thr_params.start = suite_thread2;
	thr_params.arg = run_func2;
	thr_params.thr_type = ODP_THREAD_CONTROL;
	thr_params.instance = instance;
	odph_linux_pthread_create(&sock_pthread2,
			&sock_cpumask,
			&thr_params);

	odph_linux_pthread_join(&sock_pthread1, 1);
	odph_linux_pthread_join(&sock_pthread2, 1);
}

void end_suite(void)
{
	if (fd_thread1 != -1) {
		if (ofp_close(fd_thread1) == -1)
			OFP_ERR("Faild to close socket 1 (errno = %d)\n",
				ofp_errno);
		fd_thread1 = -1;
	}

	if (fd_thread2 != -1) {
		if (ofp_close(fd_thread2) == -1)
			OFP_ERR("Faild to close socket 1 (errno = %d)\n",
				ofp_errno);
		fd_thread2 = -1;
	}
}

static void *suite_thread1(void *arg)
{
	run_function run_func = (run_function)arg;

	if (ofp_init_local()) {
		OFP_ERR("Error: OFP local init failed.\n");
		return NULL;
	}

	(void)run_func(fd_thread1);

	return NULL;
}

static void *suite_thread2(void *arg)
{
	run_function run_func = (run_function)arg;

	if (ofp_init_local()) {
		OFP_ERR("Error: OFP local init failed.\n");
		return NULL;
	}

	(void)run_func(fd_thread2);

	return NULL;
}
