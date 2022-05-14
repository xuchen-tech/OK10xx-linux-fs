/*
 * Copyright (c) 2015 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the above-listed copyright holders nor the
 *     names of any contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*!
 * @file	aiop_tool.c
 *
 * @brief	AIOP Tool
 *
 */

/* Generic includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>

/* AIOP Tool Specific includes */
#include <aiop_cmd.h>
#include <aiop_lib.h>
#include <aiop_tool.h>
#include <aiop_logger.h>
#include <aiop_tool_dummy.h>

/* Flib and VFIO Headers */
#include <fsl_vfio.h>

/* ===========================================================================
 * Externs
 * ===========================================================================
 */
extern struct global_args gvars;
extern char *sub_cmd_name;

/* ===========================================================================
 * Function Declarations
 * ===========================================================================
 */
int perform_aiop_load(aiopt_handle_t handle, aiopt_conf_t *conf);
int perform_aiop_reset(aiopt_handle_t handle, aiopt_conf_t *conf);
int perform_aiop_get_status(aiopt_handle_t handle, aiopt_conf_t *conf);
int perform_aiop_gettod(aiopt_handle_t handle, aiopt_conf_t *conf);
int perform_aiop_settod(aiopt_handle_t handle, aiopt_conf_t *conf);
/* XXX Add more operations, as required, and update the aiopt_ops */

/* ===========================================================================
 * Globals
 * ===========================================================================
 */

/* 
 * @brief
 * Callback table for operations.
 * For each command line sub-command (cmd_table) supported, corresponding
 * operation method is added in this table.
 *
 * @see cmd_table
 *
 */
#ifndef AIOP_CMDSYS_UNIT_TEST
aiopt_ops_t aiopt_ops[MAX_SUB_COMMANDS] = {
	{"help", NULL},
	{"load", perform_aiop_load},
	{"reset", perform_aiop_reset},
	{"status", perform_aiop_get_status},
	{"gettod", perform_aiop_gettod},
	{"settod", perform_aiop_settod},
	{NULL, NULL} /* Add entries above this */
};
#else
aiopt_ops_t aiopt_ops[MAX_SUB_COMMANDS] = {
	{"help", NULL},
	{"load", dummy_perform_aiop_load},
	{"reset", dummy_perform_aiop_reset},
	{"status", dummy_perform_aiop_get_status},
	{"gettod", dummy_perform_aiop_gettod},
	{"settod", dummy_perform_aiop_settod},
	{NULL, NULL} /* Add entries above this */
};

#endif

/* ===========================================================================
 * Helpers Operations
 * ===========================================================================
 */
/*
 * @brief:
 * Fill configuration structure from global vars updated by command handling
 * sub-system.
 * For every addition to the globalvars, corresponding configuration
 * item should be added here. Other than that, tool specific, non
 * command line configuration items too can be added.
 *
 * @param [in] h aiopt_conf_t type reference to configuration structure
 * @return void
 */
static void
create_conf_inst(aiopt_conf_t *h)
{
	/* Only to restrict the usage of globals to this function */
	h->command = sub_cmd_name;
	h->container = gvars.container_name;
	h->image_file = gvars.image_file;
	h->args_file = gvars.args_file[0]?gvars.args_file:NULL;
	h->tpc = gvars.tpc;
	h->tpc_flag = gvars.tpc_flag;
	h->reset_flag = gvars.reset_flag;
	h->debug_flag = gvars.debug_flag;
	h->verbose_flag = gvars.verbose_flag;
	h->tod = gvars.tod_val;
}

/*
 * @brief:
 * Internal operation for obtaining operation handler based on user provided
 * sub-command.
 *
 * @param [in] cmd_name Name of the command as found by command handler
 *                      sub-system
 * @return aiopt_op Callback function pointer
 */
static aiopt_op
get_ops_handler(char *cmd_name)
{
	int i;
	int found = FALSE;

	if (!cmd_name) {
		AIOPT_DEV("Incorrect usage of function\n");
		return NULL;
	}

	AIOPT_DEV("Searching for (%s) Operation Handler.\n", cmd_name);

	for (i = 0; i < MAX_SUB_COMMANDS &&
			aiopt_ops[i].command != NULL; i++) {
		if (!strcmp(cmd_name, aiopt_ops[i].command)) {
			found = TRUE;
			break;
		}
	}

	if (found) {
		AIOPT_DEV("Handler for Ops (%s) found.\n", cmd_name);
		return aiopt_ops[i].aiopt_op;
	}

	AIOPT_DEBUG("Unable to find operations; Is it defined?\n");
	return NULL;
}

/* ===========================================================================
 * AIOPT Operations Definitions
 * ===========================================================================
 */

/*
 * @brief
 * Wrapper over aiopt_load library call
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [in] conf aiopt_conf_t type object filled with configuration info
 *
 * @return return value from aiopt_load
 */
int
perform_aiop_load(aiopt_handle_t handle, aiopt_conf_t *conf)
{
	int ret;
	AIOPT_DEV("Entering\n");

	ret = aiopt_load(handle, conf->image_file, conf->args_file,
			 conf->reset_flag,
			 conf->tpc_flag ? conf->tpc : DEFAULT_THREAD_PER_CORE);
	if (ret == AIOPT_SUCCESS) {
		AIOPT_PRINT("AIOP Image (%s) with args (%s) loaded successfully.\n",
			conf->image_file, conf->args_file);
	} else {
		AIOPT_PRINT("AIOP Image (%s) with args (%s) loading failed. (err=%d)\n",
			conf->image_file, conf->args_file, ret);
	}

	select(1, NULL, NULL, NULL, NULL);
	AIOPT_DEV("Exiting (%d)\n", ret);
	return ret;
}

/*
 * @brief
 * Wrapper over aiopt_reset library call
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [in] conf aiopt_conf_t type object filled with configuration info
 *
 * @return return value from aiopt_reset
 */
int
perform_aiop_reset(aiopt_handle_t handle, aiopt_conf_t *conf)
{
	int ret;
	AIOPT_DEV("Entering\n");

	ret = aiopt_reset(handle);
	if (ret == AIOPT_SUCCESS) {
		AIOPT_PRINT("AIOP Tile Reset Successful.\n");
	} else {
		AIOPT_PRINT("AIOPT Tile Reset Failed. (err=%d)\n", ret);
	}

	AIOPT_DEV("Exiting\n");
	return ret;
}

/*
 * @brief
 * Wrapper over aiopt_status library call
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [in] conf aiopt_conf_t type object filled with configuration info
 *
 * @return return value from aiopt_status
 */
int
perform_aiop_get_status(aiopt_handle_t handle, aiopt_conf_t *conf)
{
	int ret;
	aiopt_status_t status;

	AIOPT_DEV("Entering\n");

	ret = aiopt_status(handle, &status);

	if (ret == AIOPT_SUCCESS) {
		AIOPT_PRINT("AIOP Tile Status:\n");
		AIOPT_PRINT("\t Service Layer:- Major Version: %d,"
			" Minor Version: "
			"%d, Revision: %d\n",
			status.sl_minor_v, status.sl_major_v,
			status.sl_revision);
		AIOPT_PRINT("\t State: %s\n",
			aiopt_get_state_str(status.state));
		AIOPT_PRINT("\n");
	}

	AIOPT_DEV("Exiting\n");
	return ret;
}

/*
 * @brief
 * Wrapper over aiopt_gettod library call for Getting Time of Day
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [in] conf aiopt_conf_t type object filled with configuration info
 *
 * @return return value from aiopt_gettod
 */
int
perform_aiop_gettod(aiopt_handle_t handle, aiopt_conf_t *conf)
{
	int ret;
	uint64_t time_of_day;

	AIOPT_DEV("Entering\n");

	ret = aiopt_gettod(handle, &time_of_day);
	if (ret == AIOPT_SUCCESS) {
		AIOPT_PRINT("Time of day: %lu\n", time_of_day);
	} else {
		AIOPT_PRINT("Get time of day unsuccessful. (err=%d)\n", ret);
	}

	AIOPT_DEV("Exiting\n");
	return ret;
}

/*
 * @brief
 * Wrapper over aiopt_settod library call for Setting Time of Day
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [in] conf aiopt_conf_t type object filled with configuration info
 *
 * @return return value from aiopt_settod
 */
int
perform_aiop_settod(aiopt_handle_t handle, aiopt_conf_t *conf)
{
	int ret;

	AIOPT_DEV("Entering\n");

	ret = aiopt_settod(handle, conf->tod);
	if (ret == AIOPT_SUCCESS) {
		AIOPT_DEBUG("Time of day set to: %lu\n", conf->tod);
	} else {
		AIOPT_DEBUG("Set time of day unsuccessful. (err=%d)\n", ret);
	}

	AIOPT_DEV("Exiting\n");
	return ret;
}
/* ===========================================================================
 * Function Definitions
 * ===========================================================================
 */

int main(int argc, char **argv)
{
	int ret = 0, ret_2 = 0;
	aiopt_conf_t	conf = {0};
	aiopt_op	op = NULL;

	aiopt_handle_t aiopt_handle = AIOPT_INVALID_HANDLE;

	/* TODO Signal Handling is missing */

	/* Parsing command line arguments and creating global structure */
	ret = parse_command_line_args(argc, argv);
	if (ret != AIOPT_SUCCESS) {
		return 0;
	}

	/* Create Conf structure */
	create_conf_inst(&conf);

	/* Enable logger */
	init_aiopt_logger(conf.debug_flag, conf.verbose_flag);

	/* Dumping Command Line Arguments, if verbose is enabled */
	dump_cmdline_args();

	/* Fetch Operation Handler */
	op = get_ops_handler(conf.command);
	if (!op) {
		AIOPT_ERR("Incorrect or unhandled command.\n");
		return AIOPT_FAILURE;
	}
	
#ifndef AIOP_CMDSYS_UNIT_TEST /* If not command line sub-sys unit testing */

	/* Initialize the AIOP library and obtain handle */
	aiopt_handle = aiopt_init(conf.container);
	if (AIOPT_INVALID_HANDLE == aiopt_handle) {
		AIOPT_ERR("Unable to open Container (%s)\n", conf.container);
		AIOPT_DEV("Handle cannot be opened/allocated.\n");
		return AIOPT_FAILURE;
	}

	/* Hereafter, this handle would be used by all callers to perform 
	 * necessary operation
	 */
	AIOPT_DEV("Obtained AIOP handle (%p).\n", aiopt_handle);
	AIOPT_DEBUG("AIOP sub-system initialized.\n");

	/* Handle Sub-command */
	ret = op(aiopt_handle, &conf);
	if (ret != AIOPT_SUCCESS) {
		AIOPT_ERR("AIOP Sub-command %s failed\n", conf.command);
	}

	/* Deinitializing VFIO Container */
	ret_2 = aiopt_deinit(aiopt_handle);
	if (ret_2 != AIOPT_SUCCESS) {
		AIOPT_ERR("Cleanup/Teardown Failure.\n");
	}

	if (ret == AIOPT_SUCCESS && ret_2 != AIOPT_SUCCESS)
		ret = ret_2;
#endif

	return ret;
}
