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
 * @file	aiop_cmd.c
 *
 * @brief	AIOP Tool Command Line Handling layer
 *
 */

/* Generic includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>

/* AIOP Tool Specific includes */
#include <aiop_cmd.h>
#include <aiop_tool.h>
#include <aiop_logger.h>

/* For unit Testing of Command Line Handling */
#include <aiop_tool_dummy.h>

/* ===========================================================================
 * Function Declarations (XXX Can be moved to header)
 * ===========================================================================
 */

int help_cmd_hndlr(int argc, char **argv);
int load_cmd_hndlr(int argc, char **argv);
int reset_cmd_hndlr(int argc, char **argv);
int status_cmd_hndlr(int argc, char **argv);
int gettod_cmd_hndlr(int argc, char **argv);
int settod_cmd_hndlr(int argc, char **argv);
static inline void usage(const char *tool_name, const char *error_str);

/* ===========================================================================
 * Globals
 * ===========================================================================
 */
struct command cmd_table[MAX_SUB_COMMANDS] = {
	{"help", help_cmd_hndlr},
	{"load", load_cmd_hndlr},
	{"reset", reset_cmd_hndlr},
	{"status", status_cmd_hndlr},
	{"gettod", gettod_cmd_hndlr},
	{"settod", settod_cmd_hndlr},
	{NULL, NULL}
};

/* Structure for Command Line Arguments into Global state */
struct global_args gvars;

/* Global Sub-command function handle */
int (*sub_cmd_hndlr)(int argc, char **argv) = NULL;
char *sub_cmd_name = NULL;

/* ===========================================================================
 * Helper Functions
 * ===========================================================================
 */

/*
 * @brief
 * Dump the values of variables stored in global command arguments structure
 *
 * @param void
 * @return void
 */
void
dump_cmdline_args(void)
{
	/* Mapped to CONTAINER_??? Macros in aiop_cmd.h */
	const char *container_from[] = {
		"Default Value",
		"Environment Variable",
		"User"
	};

	AIOPT_INFO(" Using: \n"
		"    Container Name: %s\n"
		"    Image File: %s\n"
		"    Args File: %s\n"
		"    Time of Day: %lu\n"
		"    Threads per core: %u\n"
		"    Reset Flag: %s\n"
		"    Debug: %s\n",
		gvars.container_name ? gvars.container_name : NULL,
		gvars.image_file ? gvars.image_file : NULL,
		gvars.args_file,
		gvars.tod_val,
		gvars.tpc_flag ? gvars.tpc : DEFAULT_THREAD_PER_CORE,
		gvars.reset_flag ? "Yes" : "No",
		gvars.debug_flag ? "Yes" : "No");
	if (gvars.container_name_flag > 0 &&
			gvars.container_name_flag < sizeof(container_from))
		AIOPT_INFO("Container name has been derived from %s.\n",
			container_from[gvars.container_name_flag - 1]);
}

/*
 * @brief
 * Check if option char string (getopt) has a specific character
 * This function is used to identify if a particular sub-command supports a
 * specific option character
 *
 * @param [in] str constant option string
 * @param [in] c character to check for
 * @return AIOPT_SUCCESS if a match, else AIOPT_FAILURE. In case of incorrect
 *         usage (invalid/NULL arguments), AIOPT_FAILURE is returned
 */
static int
check_if_valid_arg(const char *str, char c)
{
	int i = 0;
	int len;
	int ret = AIOPT_FAILURE;

	if (!str)
		return ret;

	len = strlen(str);
	if (!len)
		return ret;

	for (; i < len; i++) {
		if (str[i] == c) {
			ret = AIOPT_SUCCESS;
			break;
		}
	}

	return ret;
}

/*
 * @brief
 * Helper to extract name of the container from Argument list
 *
 * @param [in] arg string passed by user against -g option
 * @return void
 */
static inline void
container_name_from_args(const char *arg)
{
	int c_len;

	c_len = strlen(arg);
	if (c_len >= MAX_CONTAINER_NAME_LEN || c_len <= 0) {
		/* Exceeds max container name length */
		AIOPT_ERR("Container name length incorrect: "
			"(%d)(max:%d)\n", c_len, MAX_CONTAINER_NAME_LEN);
		/* Container name provided by User is ignored*/
		return;
	}

	strcpy(gvars.container_name, arg);
	gvars.container_name_flag = CONTAINER_USR;

	return;
}

/*
 * @brief
 * Helper to extract name of the container from Argument list
 *
 * @param [in] file string passed as argument by user, against -f option
 * @return AIOPT_SUCCESS if string can be extracted, else AIOPT_FAILURE.
 *         In case of incorrect usage (invalid/NULL arguments), AIOPT_FAILURE
 *         is returned.
 */
static inline int
image_file_from_args(const char *file)
{
	int ret;
	int file_len;
	struct stat file_s;

	if (!file) {
		/* Possible internal error; Caller has to assure non-NULL string
		 * is passed.*/
		AIOPT_DEV("Invalid API usage.\n");
		AIOPT_DEBUG("Internal error.\n");
		goto error_out;
	}

	file_len = strlen(file);
	if (file_len <= 0 || file_len > MAX_PATH_LEN) {
		AIOPT_ERR("Filename provided longer than allowed.\n");
		goto error_out;
	}

	AIOPT_DEV("File recieved from Args = %s\n", file);

	/* Validate that file is correct */
	ret = access(file, F_OK|R_OK);
	if (ret != 0) {
		AIOPT_ERR("Unable to access file path (err=%d)\n", errno);
		goto error_out;
	}

	/* Access to file is OK; Checking if a Regular file or not */
	memset(&file_s, 0, sizeof(file_s));
	ret = stat(file, &file_s);
	if (ret != 0) {
		AIOPT_ERR("Unable to stat file. (err=%d)\n", errno);
		goto error_out;
	}

	/* XXX; access and stat can be merged into stat call */
	ret = S_ISREG(file_s.st_mode);
	if (!ret) {
		AIOPT_ERR("Image file is not a regular file\n");
		goto error_out;
	}

	strcpy(gvars.image_file, file);
	gvars.image_file_flag = TRUE;

	return AIOPT_SUCCESS;

error_out:
	return AIOPT_FAILURE;
}

/*
 * @brief
 * Helper to extract the name of file containing AIOP command-line argument data
 * from argument list
 *
 * @param [in] args file string passed as argument by user, against -a option
 * @return AIOPT_SUCCESS if string can be extracted, else AIOPT_FAILURE.
 *         In case of incorrect usage (invalid/NULL arguments), AIOPT_FAILURE
 *         is returned.
 */
static inline int
args_file_from_args(const char *args_file)
{
	int ret;
	int file_len;
	struct stat file_s;

	if (!args_file) {
		/* Possible internal error; Caller has to assure non-NULL string
		 * is passed.*/
		AIOPT_DEV("Invalid API usage.\n");
		AIOPT_DEBUG("Internal error.\n");
		goto error_out;
	}

	file_len = strlen(args_file);
	if (file_len <= 0 || file_len > MAX_PATH_LEN) {
		AIOPT_ERR("Filename provided longer than allowed.\n");
		goto error_out;
	}

	AIOPT_DEV("File recieved from Args = %s\n", args_file);

	/* Validate that file is correct */
	ret = access(args_file, F_OK|R_OK);
	if (ret != 0) {
		AIOPT_ERR("Unable to access args file path (err=%d)\n", errno);
		goto error_out;
	}

	/* Access to file is OK; Checking if a regular file or not */
	memset(&file_s, 0, sizeof(file_s));
	ret = stat(args_file, &file_s);
	if (ret != 0) {
		AIOPT_ERR("Unable to stat args file. (err=%d)\n", errno);
		goto error_out;
	}

	/* XXX; access and stat can be merged into stat call */
	ret = S_ISREG(file_s.st_mode);
	if (!ret) {
		AIOPT_ERR("Args file is not a regular file\n");
		goto error_out;
	}

	strcpy(gvars.args_file, args_file);
	gvars.args_file_flag = TRUE;

	return AIOPT_SUCCESS;

error_out:
	return AIOPT_FAILURE;
}

/*
 * @brief
 * Helper to extract thread per core value against argument -c
 *
 * @param tpc_value string containing thread per core value from cmd line
 * @return void
 * 	   If invalid argument is provided, thread per core is assumed as
 *	   default.
 */
static void inline
thread_per_core_from_args(const char *tpc_value)
{
	int tpc;

	if (tpc_value) {
		/* atoi would simply return the valid value from first set of
		 * valid characters which are integers. It would not report
		 * any error.
		 */
		tpc = atoi(tpc_value);
		if (tpc <= 0 || tpc > MAX_THREAD_PER_CORE)
			tpc = DEFAULT_THREAD_PER_CORE;
	}

	AIOPT_DEV("Setting thread per AIOP core to: %d\n", tpc);
	gvars.tpc = tpc;
	gvars.tpc_flag = TRUE;
}

/*
 * @brief
 * Helper to extract/update reset flag against argument -r passed by user
 *
 * @param void
 * @return void
 */
static void inline
reset_flag_from_args(void)
{
	/* This flag conveys that the application should attempt to perform
	 * RESET before performing LOAD
	 */
	gvars.reset_flag = TRUE;
}

/*
 * @brief
 * Helper to extract debug output toggle against argument -d
 * This would enable the verbose toggle also.
 *
 * @param void
 * @return void
 */
static void inline
debug_flag_from_args(void)
{
	/* This flag controls the screen output of Debug Information
	 * TODO: At present, only single state; In future, can be converted
	 * into a multiple level toggle (INFO/DEBUG/...)
	 */
	gvars.debug_flag = TRUE;
	gvars.verbose_flag = TRUE;
}

/*
 * @brief
 * Helper to extract info/verbose output toggle against argument -v
 *
 * @param void
 * @return void
 */
static void inline
verbose_flag_from_args(void)
{
	/* This flag controls the screen output of INFO (Verbose) */
	gvars.verbose_flag = TRUE;
}

/*
 * @brief
 * Helper to extract Time of day passed as argument to -t option
 *
 * @param [in] timestr time passed as argument by user, against -f option
 * @return AIOPT_SUCCESS if string can be extracted, else AIOPT_FAILURE.
 *         In case of incorrect usage (invalid/NULL arguments), AIOPT_FAILURE
 *         is returned.
 */
static int inline
timeofday_from_args(const char *timestr)
{
	char *err_str;
	uint64_t tod_val;

	if (!timestr) {
		AIOPT_ERR("Invalid time of day.\n");
		return AIOPT_FAILURE;
	}

	errno = 0; /* So as to check value post strotul call */
	tod_val = strtoul(timestr, &err_str, 10);

	if ((errno == ERANGE &&
		(tod_val == LONG_MAX || tod_val == LONG_MIN)) ||
		(errno != 0 && tod_val == 0) ||
		(*err_str != '\0')) {
		/* All are error cases; Either too long a value, or a NAN
		 * provided as time string
		 */
		AIOPT_ERR("Incorrect Time String: (%s)(err=%d)\n",
			timestr, errno);
		return AIOPT_FAILURE;
	}
	
	gvars.tod_val = tod_val;
	gvars.tod_flag = TRUE;

	return AIOPT_SUCCESS;
}

/*
 * @brief
 * Helper to extract container name if set as environment variable
 *
 * @param void
 * @return void
 *
 */
static void
get_container_from_env(void)
{
	char *c_name = NULL;
	int len;

	/* Default container name is hardcoded in application */
	strcpy(gvars.container_name, DEFAULT_DPRC_NAME);
	/* This is dummy, only for future use */
	gvars.container_name_flag = CONTAINER_DEF;


	c_name = getenv(CONTAINER_ENV_VAR);

	if (!c_name) {
		/* No environment variable has been set by user */
		AIOPT_DEBUG("Environment variable %s not set.\n",
				CONTAINER_ENV_VAR);
		AIOPT_DEBUG("Using internally defined container (%s) until"
				" explicitly provided by user.\n",
				DEFAULT_DPRC_NAME);
		return;
	}

	/* Else, if env variable is available, copy-in */
	len = strlen(c_name);
	if (len >= MAX_CONTAINER_NAME_LEN) {
		AIOPT_DEBUG("Len of env variable larger than expected\n");
		return;
	}

	/* If user has provided any container name in env variable, it
	 * overrides the default. This can be further overriden using value
	 * passed as argument to a sub-command
	 */
	strcpy(gvars.container_name, c_name);
	gvars.container_name_flag = CONTAINER_ENV;

	AIOPT_DEBUG("Container Name found set in env: %s\n",
			gvars.container_name);
	return;
}

/*
 * @brief
 * A generic command handler. For all command handlers, the options are parsed
 * by this. Caller simply verifies whether its necessary args were provided
 * or not. This allows the sub-command to have a modular/pluggable architecture
 * helpful for expanding in future
 *
 * @param [in] argc Count of Command line arguments, as available in main
 * @param [in] argv Array of command line arguments, as available in main
 * @param [in] valid_args string of option characters which are not expected
 *             by caller.
 *
 * @return AIOPT_SUCCESS if parsing was successful, else AIOPT_FAILURE
 */
static int
generic_cmd_hndlr(int argc, char **argv, char *valid_args)
{
	int ret = AIOPT_SUCCESS;
	int opt;
	char *opt_str = "+g:f:a:t:rdvc:";

	static struct option longopts[] = {
		{"container", required_argument, NULL, 'g'},
		{"file", required_argument, NULL, 'f'},
		{"args-file", required_argument, NULL, 'a'},
		{"reset", no_argument, NULL, 'r'},
		{"timeofday", required_argument, NULL, 't'},
		{"debug", no_argument, NULL, 'd'},
		{"verbose", no_argument, NULL, 'v'},
		{"threadpercore", required_argument, NULL, 'c'},
		{NULL, 0, NULL, 0}
	};

	opterr = 0;

	while (1) {
		opt = getopt_long(argc, argv, opt_str,
				  longopts, NULL);

		if (opt == -1)
			break;	/* No more options */

		switch (opt) {
		case 'g':
			ret = check_if_valid_arg(valid_args,'g');
			if (ret != AIOPT_SUCCESS) {
				AIOPT_ERR("Invalid arg (%c) provided\n", 'g');
				break;
			}

			AIOPT_DEV("Provided with 'g' -%s-\n", optarg);
			container_name_from_args(optarg);
			AIOPT_DEV("Using container=%s\n",
					gvars.container_name);
			break;
		case 'f':
			ret = check_if_valid_arg(valid_args,'f');
			if (ret != AIOPT_SUCCESS) {
				AIOPT_ERR("Invalid arg (%c) provided\n", 'f');
				break;
			}

			AIOPT_DEV("Provided with 'f' -%s-\n", optarg);
			ret = image_file_from_args(optarg);
			if (ret != AIOPT_SUCCESS) {
				AIOPT_ERR("Unable to validate image file.\n");
			}
			break;
		case 'a':
			ret = check_if_valid_arg(valid_args,'a');
			if (ret != AIOPT_SUCCESS) {
				AIOPT_ERR("Invalid arg (%c) provided\n", 'a');
				break;
			}

			AIOPT_DEV("Provided with 'a' -%s-\n", optarg);
			ret = args_file_from_args(optarg);
			if (ret != AIOPT_SUCCESS) {
				AIOPT_ERR("Unable to validate args file.\n");
			}
			break;
		case 'r':
			ret = check_if_valid_arg(valid_args,'r');
			if (ret != AIOPT_SUCCESS) {
				AIOPT_ERR("Invalid arg (%c) provided\n", 'r');
				break;
			}

			AIOPT_DEV("Provided with 'r' -%s-\n", optarg);
			reset_flag_from_args();
			break;
		case 't':
			ret = check_if_valid_arg(valid_args,'t');
			if (ret != AIOPT_SUCCESS) {
				AIOPT_ERR("Invalid arg (%c) provided\n", 't');
				break;
			}

			AIOPT_DEV("Provided with 't' -%s-\n", optarg);
			timeofday_from_args(optarg);
			break;
		case 'd':
			ret = check_if_valid_arg(valid_args,'d');
			if (ret != AIOPT_SUCCESS) {
				AIOPT_ERR("Invalid arg (%c) provided\n", 'd');
				break;
			}

			AIOPT_DEV("Provided with 'd' -%s-\n", optarg);
			debug_flag_from_args();
			break;
		case 'v':
			ret = check_if_valid_arg(valid_args,'v');
			if (ret != AIOPT_SUCCESS) {
				AIOPT_ERR("Invalid arg (%c) provided\n", 'v');
				break;
			}

			AIOPT_DEV("Provided with 'd' -%s-\n", optarg);
			verbose_flag_from_args();
			break;
		case 'c':
			ret = check_if_valid_arg(valid_args,'c');
			if (ret != AIOPT_SUCCESS) {
				AIOPT_ERR("Invalid arg (%c) provided\n", 'c');
				break;
			}

			AIOPT_DEV("Provided with 'd' -%s-\n", optarg);
			thread_per_core_from_args(optarg);
			break;
		case '?':
		default:
			AIOPT_ERR("Incorrect or Incomplete args.\n");
			ret = AIOPT_FAILURE;
			break;
		}

		if (ret != AIOPT_SUCCESS) {
			break;
		}
	}

	if (ret != AIOPT_SUCCESS) {
		return AIOPT_FAILURE;
	}
	
	optind = 1;		/* reset 'extern optind' from the getopt lib */

	return AIOPT_SUCCESS;
}


/*
 * @brief
 * Usage Manual dump
 *
 * @param [in] tool_name Name of binary
 * @param [in] error_str Any pre-pending string to the Usage information
 * @return void
 */
static inline void
usage(const char *tool_name, const char *error_str)
{
	int len = 0;
	char *bin_name = NULL;

	if (!tool_name)
		return;

	len = strlen(tool_name);
	bin_name = malloc(sizeof(char) * (len + 1));
	if (!bin_name) {
		AIOPT_DEBUG("Unable to allocate internal memory.\n");
		/* Possible incorrect usage of this function */
		AIOPT_DEV("Incorrect usage of the function; Passed Null\n");
	} else {
		strcpy(bin_name, tool_name);
	}

	if (error_str != NULL) {
		/* In case the caller provides any custom string, print that
		 * before the usage message
		 */
		printf("%s\n", error_str);
	}

	/* TODO: Long arguments are missing from this */
	printf("\n");
	printf("%s [sub-command] [arguments]\n",
		bin_name ? basename(bin_name) : "NULL");
	printf("Version: %s.%s, Compatible MC Version: %s\n\n",
		AIOPT_MAJ_VER, AIOPT_MIN_VER, COMPAT_MC_VER);
	printf("Following are valid [sub-commands]:\n");
	printf("  help:   Prints help content for the binary.\n");
	printf("  load:   Loading AIOP Image to AIOP Tile.\n");
	printf("  reset:  Resetting the AIOP Tile.\n");
	printf("  gettod: Fetch the Time of Day.\n");
	printf("  settod: Set the Time of Day.\n");
	printf("  status: Status of the AIOP Tile.\n");
	printf("Following are sub-command specific arguments\n");
	printf("  help: No Arguments\n");
	printf("  status:\n");
	printf("                         No mandatory arguments.\n");
	printf("\n");
	printf("  load:\n");
	printf("    -f <AIOP Image Path> Mandatory: Path of a valid AIOP \n");
	printf("                         image file.\n");
	printf("                         Also: --file\n");
	printf("    -a <AIOP Args Path>  Optional: Path of a valid file \n");
	printf("                         containing AIOP command-line argument data\n");
	printf("                         Also: --args-file\n");
	printf("    -r                   Optional: Reset AIOP tile before\n");
	printf("                         performing load. If not provided,\n");
	printf("                         reset would not be done\n");
	printf("                         Also: --reset\n");
	printf("    -c                   Optional: Threads per AIOP core to execute\n");
	printf("                         Also: --threadpercore\n");
	printf("  reset:\n");
	printf("                         No mandatory arguments.\n");
	printf("  gettod:\n");
	printf("                         No mandatory arguments.\n");
	printf("  settod:\n");
	printf("    -t <Time since Epoch>\n");
	printf("                         Mandatory: Time, in milliseconds\n");
	printf("                         provided as string\n");
	printf("                         Also: --timeofday\n");
	printf("\n");
	printf("Arguments valid for all sub-commands:\n");
	printf("    -g <Container name>  Optional: Name of the container\n");
	printf("                         containing the dpaiop object.\n");
	printf("                         Also: --container\n");
	printf("    -v                   Optional: Enable verbose output.\n");
	printf("                         Also: --verbose\n");
	printf("    -d                   Optional: Enable debug output.\n");
	printf("                         This would also enable -v.\n");
	printf("                         Also: --debug\n");
	printf("\n");
	printf("Container Name can be:\n");
	printf("    1. Provided along with sub-command using '-g' option.\n");
	printf("    2. If not provided by '-g' as mentioned in (1), by \n");
	printf("       setting Environment variable 'DPRC'.\n");
	printf("    3. Default (%s) if not provided by (1) and (2).\n",
		DEFAULT_DPRC_NAME);
	printf("\n\n");

	if (bin_name)
		free(bin_name);
}

/*
 * @brief
 * Method for checking if any -h/--help/-v/--verbose/? type characters are
 * provided by user for expecting help output
 *
 * @param [in] argc Count of arguments
 * @param [in] argv Array of argument strings
 * @return AIOPT_SUCCESS if help argument found, else AIOPT_FAILURE
 */
static inline int
check_for_help_cmd(int argc, char **argv)
{
	int i;

	/* Look for combinations of help or verison args in cmd line */
	for (i = 0;i < argc; i++) {
		if (!strcmp(argv[i], "-h") ||
			!strcmp(argv[i], "--help")	||
			!strcmp(argv[i], "-help")	||
			!strcmp(argv[i], "-version")	||
			!strcmp(argv[i], "--version")	||
			!strcmp(argv[i], "--?")		||
			!strcmp(argv[i], "-?")) {
			return AIOPT_SUCCESS;
		}
	}
	/* No help or version information requested by user */
	return AIOPT_FAILURE;
}

/*
 * @brief
 * Internal method for extracting the sub-command name from command line and
 * fetching its handler. Sets the sub-command handler in the global variable
 * sub_cmd_hndlr and name in global variable sub_cmd_name
 *
 * @param [in] cmd sub-command string
 * @return AIOPT_SUCCESS if handler was found, else AIOPT_FAILURE
 */
static int
find_subcmd_hndlr(char *cmd)
{
	int cmd_idx = 0;
	int found = 0;
	struct command *ptr = NULL;

	/* Parse the commands to find the sub-command in it */
	AIOPT_DEV("Searching for cmd=%s\n", cmd);
	ptr = &cmd_table[cmd_idx];
	while (ptr->cmd_name && ptr->hndlr) {
		if (!strcmp(ptr->cmd_name, cmd)) {
			found = 1;
			break;
		}
		cmd_idx++;
		ptr = &cmd_table[cmd_idx];
	}

	if (found) {
		AIOPT_DEV("Found sub-command at idx=%d\n", cmd_idx);
		/* Sub-command handler is available */
		sub_cmd_hndlr = cmd_table[cmd_idx].hndlr;
	} else {
		return AIOPT_FAILURE;
	}

	AIOPT_DEBUG("Handling sub-command(id=%d): %s.\n", cmd_idx,
			cmd_table[cmd_idx].cmd_name);

	/* Updating global sub_cmd_name variable */
	sub_cmd_name = cmd;

	return AIOPT_SUCCESS;
}

/* ===========================================================================
 * Command Handling Functions
 * ===========================================================================
 */

/*
 * @brief
 * Help sub-command handler
 *
 * @param [in] argc Count of arguments
 * @param [in] argv Array of argument strings
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int
help_cmd_hndlr(int argc, char **argv)
{
	AIOPT_DEV("Entering help_cmd function\n");
	usage(argv[0], "Help:");
	return AIOPT_FAILURE;
}

/*
 * @brief
 * Load sub-command handler
 *
 * @param [in] argc Count of arguments
 * @param [in] argv Array of argument strings
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int
load_cmd_hndlr(int argc, char **argv)
{
	int ret = AIOPT_SUCCESS;
	char *valid_args = "gafrdvc";

	AIOPT_DEBUG("Load Cmd: argc=%d\n", argc);

	ret = generic_cmd_hndlr(argc - 1, argv + 1, valid_args);
	if (ret != AIOPT_SUCCESS) {
		usage(argv[0], "Incomplete or Incorrect Arguments.");
		return AIOPT_FAILURE;
	}
	
	if (!gvars.container_name_flag || !gvars.image_file_flag) {
		AIOPT_DEV("Container or Image file not provided.\n");
		usage(argv[0], "One or more Mandatory Arguments not provided");
		/* Parsing found issues; Abort */
		return AIOPT_FAILURE;
	}

	optind = 1;		/* reset 'extern optind' from the getopt lib */

	dump_cmdline_args();

	return AIOPT_SUCCESS;
}

/*
 * @brief
 * Reset sub-command handler
 *
 * @param [in] argc Count of arguments
 * @param [in] argv Array of argument strings
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int
reset_cmd_hndlr(int argc, char **argv)
{
	int ret = AIOPT_SUCCESS;
	char *valid_args = "gdv";
	
	ret = generic_cmd_hndlr(argc - 1, argv + 1, valid_args);
	if (ret != AIOPT_SUCCESS) {
		usage(argv[0], "Incomplete or Incorrect Arguments.");
		return AIOPT_FAILURE;
	}
	
	if (!gvars.container_name_flag) {
		AIOPT_DEV("Container name not provided.\n");
		usage(argv[0], "One or more Mandatory Arguments not provided");
		/* Parsing found issues; Abort */
		return AIOPT_FAILURE;
	}

	reset_flag_from_args();

	optind = 1;		/* reset 'extern optind' from the getopt lib */

	dump_cmdline_args();

	return AIOPT_SUCCESS;
}

/*
 * @brief
 * Get Time of Day sub-command handler
 *
 * @param [in] argc Count of arguments
 * @param [in] argv Array of argument strings
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int
gettod_cmd_hndlr(int argc, char **argv)
{
	int ret = AIOPT_SUCCESS;
	char *valid_args = "gdv";
	
	ret = generic_cmd_hndlr(argc - 1, argv + 1, valid_args);
	if (ret != AIOPT_SUCCESS) {
		usage(argv[0], "Incomplete or Incorrect Arguments.");
		return AIOPT_FAILURE;
	}
	
	if (!gvars.container_name_flag) {
		AIOPT_DEV("Container name not provided.\n");
		usage(argv[0], "One or more Mandatory Arguments not provided");
		/* Parsing found issues; Abort */
		return AIOPT_FAILURE;
	}

	optind = 1;		/* reset 'extern optind' from the getopt lib */

	dump_cmdline_args();

	return AIOPT_SUCCESS;
}

/*
 * @brief
 * Set Time of Day sub-command handler
 *
 * @param [in] argc Count of arguments
 * @param [in] argv Array of argument strings
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int
settod_cmd_hndlr(int argc, char **argv)
{
	int ret = AIOPT_SUCCESS;
	char *valid_args = "gtdv";
	
	ret = generic_cmd_hndlr(argc - 1, argv + 1, valid_args);
	if (ret != AIOPT_SUCCESS) {
		usage(argv[0], "Incomplete or Incorrect Arguments.");
		return AIOPT_FAILURE;
	}
	
	if (!gvars.container_name_flag ||
		!gvars.tod_flag) {
		AIOPT_DEV("Container name or TimeofDay not provided.\n");
		usage(argv[0], "One or more Mandatory Arguments not provided");
		/* Parsing found issues; Abort */
		return AIOPT_FAILURE;
	}

	optind = 1;		/* reset 'extern optind' from the getopt lib */

	dump_cmdline_args();

	return AIOPT_SUCCESS;
}

/*
 * @brief
 * Status sub-command handler
 *
 * @param [in] argc Count of arguments
 * @param [in] argv Array of argument strings
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int
status_cmd_hndlr(int argc, char **argv)
{
	int ret = AIOPT_SUCCESS;
	char *valid_args = "gdv";
	
	ret = generic_cmd_hndlr(argc - 1, argv + 1, valid_args);
	if (ret != AIOPT_SUCCESS) {
		usage(argv[0], "Incomplete or Incorrect Arguments.");
		return AIOPT_FAILURE;
	}
	
	if (!gvars.container_name_flag) {
		AIOPT_DEV("Container name not provided.\n");
		usage(argv[0], "One or more Mandatory Arguments not provided");
		/* Parsing found issues; Abort */
		return AIOPT_FAILURE;
	}

	optind = 1;		/* reset 'extern optind' from the getopt lib */

	/* TODO: Code for Status Printing and -v/--version handling can be same */
	gvars.status_flag = 1;

	dump_cmdline_args();

	return AIOPT_SUCCESS;
}


/* ===========================================================================
 * Functions Definitions
 * Exposed to external compilations units
 * ===========================================================================
 */
/*
 * @brief
 * Externally exposed operation for parsing the command line arguments provided
 * by user.
 *
 * @param [in] argc Count of arguments passed by user
 * @param [in] argv Array of arguments
 * @return AIOPT_SUCCESS or AIOPT_FAILURE if parsing failed.
 *
 */
int
parse_command_line_args(int argc, char **argv)
{
	int ret = 0;

	/* Validate if indeed any arguments have been passed by caller */
	if (argc <= 1) {
		/* Show the help */
		usage(argv[0], "No sub-command provided.");
		return AIOPT_FAILURE;
	}

	/* Check if '-h' or '--help' has been passed */
	ret = check_for_help_cmd((argc - 1), (argv + 1));
	if (!ret) {
		usage(argv[0], NULL);
		return AIOPT_FAILURE;
	}

	/* Extracting sub-command handler from arguments */
	ret = find_subcmd_hndlr(argv[1]);
	if (ret != AIOPT_SUCCESS || !sub_cmd_hndlr) {
		usage(argv[0], "No valid sub-command provided.");
		return AIOPT_FAILURE;
	}

	/* If the container name has been set in Environment, updating it in
	 * global variable. This would be overwritten with user provided value
	 */
	get_container_from_env();

	/* Calling the sub-command handler and parsing remaining arguments */
	ret = sub_cmd_hndlr(argc, argv);
	if (ret != AIOPT_SUCCESS) {
		return AIOPT_FAILURE;
	}

	return AIOPT_SUCCESS;
}
