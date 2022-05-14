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
 * @file	aiop_cmd.h
 *
 * @brief	AIOP Tool Command Line Handling Header
 *
 */

#ifndef AIOPT_CMD_H
#define AIOPT_CMD_H

/* ===========================================================================
 * MACROS/Constants
 * ===========================================================================
 */

/** @def MAX_SUB_COMMANDS
 * @brief Max limit for number of supported sub-commands (load, reset ...)
 */
#define MAX_SUB_COMMANDS	10

/** @def MAX_CMD_STR_LEN
 * @brief MAX Length of a sub-command name
 */
#define MAX_CMD_STR_LEN		10

/** @def DEFAULT_DPRC_NAME
 * @brief Default containiner
 * Default container name which would be used in case nothing provided by
 * user or none set on environment variable.
 */
#define DEFAULT_DPRC_NAME	"dprc.5"

/** @def CONTAINER_ENV_VAR
 * @brief Name of environment key-value pair for DPRC
 */
#define CONTAINER_ENV_VAR	"DPRC"

/** @def MAX_CONTAINER_NAME_LEN
 * @brief Maximum size of container name string
 */
#define MAX_CONTAINER_NAME_LEN	10

/** @def MAX_PATH_LEN
 * @brief Maximum length for AIOP Image file, with path; Ideally FILENAME_MAX
 */
#define MAX_PATH_LEN		256 /**< Max file path length >*/

/** @def MAX_THREAD_PER_CORE
 * Maximum number of threads per core for AIOP.
 * This is extracted from MC flib value (dpaiop_load_cfg).
 * Valid values are 1, 2, 4, 8, 16.
 */
#define MAX_THREAD_PER_CORE	16

/** @def DEFAULT_THREAD_PER_CORE
 * Default value of threads per AIOP core.
 */
#define DEFAULT_THREAD_PER_CORE	0

/* @def CONTAINER_ENV
 * @brief Value to identify if container being used is default, env or user
 * defined.
 */
#define CONTAINER_DEF	1	/**< Set to True = 1 >*/
#define CONTAINER_ENV	2	/**< Container as picked from Env variable >*/
#define CONTAINER_USR	3	/**< Container as provided by user >*/

/* ===========================================================================
 * Structures
 * ===========================================================================
 */

/*
 * @brief Sub-Command definition
 *
 * Each sub-command consists of a name and its handler. Multiple instances of
 * this structure are created for representing sub-commands.
 */
struct command {
	char *cmd_name; /**< Name of the sub-command: MAX_CMD_STR_LEN >*/
	int (*hndlr)(int argc, char **argv); /**< Handler for sub-commands >*/
	int flag;	/**< Current unused >*/
};

/* TODO: Rather than having separate flags for all, we can have a single int
 * and mulitple bit-mask Macros
 */
/*
 * @brief
 * Global structure which holds all the configuration parameters for AIOP Tool
 */
struct global_args {
	/* Name (with path) of the Image file.
	 * MAX_PATH_LEN is small enough for static allocation.
	 * XXX: If FILENAME_MAX is used, would require dynamic allocation.
 	*/
	short int image_file_flag;
	char image_file[MAX_PATH_LEN]; /* TODO Make it dynamic allocation */

	/* Name (with path) of the AIOP arguments file.
	 * MAX_PATH_LEN is small enough for static allocation.
 	*/
	short int args_file_flag;
	char args_file[MAX_PATH_LEN]; /* TODO Make it dynamic allocation */

	/* Threads per AIOP Core (tpc) to deploy */
	short int tpc_flag;
	unsigned short int tpc;

	/* Name of the container containing dpaiop object. This would be updated
	 * either through user provided value (-g argument), or environment 
	 * variable (DPRC) or default value specified in DEFAULT_DPRC_NAME
	 */
	short int container_name_flag;
	char container_name[MAX_CONTAINER_NAME_LEN]; /* TODO Dynamic alloc */

	/* Flag specifying if reset operations should be performed or not */
	short int reset_flag;

	/* Time of day */
	short int tod_flag;
	uint64_t tod_val;

	/* Obtaining Status
	 * XXX Doesn't serve much purpose; Only for maintaining symmetry of
	 * argument parsing handler code style
	 */
	short int status_flag;

	/* Debugging enabled or disabled */
	short int debug_flag;

	/* Verbose (INFO) enabled or disabled */
	short int verbose_flag;

};

/*
 * @brief Handle command line arguments for AIOP Tool
 *
 * @param [in] argc count of arguments passed by user, including bin name
 * @param [in] argv all sub-strings passed as argument by user
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 *
 */
int parse_command_line_args(int argc, char **argv);
void dump_cmdline_args(void);

#endif /* AIOPT_CMD_H */
