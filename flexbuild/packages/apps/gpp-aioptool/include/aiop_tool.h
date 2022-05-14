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
 * @file	aiop_tool.h
 *
 * @brief	AIOP Tool Header file
 *
 */

#ifndef AIOPT_TOOL_H
#define AIOPT_TOOL_H

/* TODO This should be auto-extracted just like ODP is doing */
#define AIOPT_MAJ_VER		"2"
#define AIOPT_MIN_VER		"0"

#define COMPAT_MC_VER		"10.1.3"

/* General success/failure Macros */
#define AIOPT_SUCCESS	0	/**< Success of a Method/Function >*/
#define AIOPT_FAILURE	(-1)	/**< Failure of a Method/Function >*/
#define AIOPT_ENOMEM	(-ENOMEM) /**< NO Memory to allocate >*/

#define FALSE		0
#define TRUE		1

/* Error Codes */
#define AIOPT_INT_ERROR	AIOPT_FAILURE	/**< Internal Failure of tool > */

#include <fsl_vfio.h>

/* ===========================================================================
 * Global Structures
 * ===========================================================================
 */

/*
 * @brief AIOP Tool Configuration structure
 */
struct aiop_tool_conf {
	char		*command; /**< Sub-command provided on cmd line >*/
	char		*container; /**< Container Name string >*/
	char		*image_file; /**< AIOP Image file for Load command >*/
	char		*args_file; /**< AIOP Arguments file for Load command >*/
	unsigned short int reset_flag; /**< Reset option for Load command >*/
	unsigned short int debug_flag; /**< DEBUG Output, DEBUG/DEV >*/
	unsigned short int verbose_flag; /**< verbose Output, INFO >*/
	unsigned short int tpc; /**< threads per AIOP core >*/
	unsigned short int tpc_flag; /**< Enabled if tpc provided by user >*/
	uint64_t	tod; /**< Time of Day, for settod >*/
};

typedef struct aiop_tool_conf aiopt_conf_t;

/*
 * @brief AIOP Tool Sub-Command handling structure
 * For each sub-command supported by AIOP Tool, an instance of this added to
 * a global structure. Thereafter the command line handling logic would plug
 * this operation into main flow.
 *
 * ** UNUSED **
 */
struct aiop_tool_operations {
	char *command;
	int (*aiopt_op)(fsl_vfio_t handle, aiopt_conf_t *);
};

typedef int (*aiopt_op)(fsl_vfio_t handle, aiopt_conf_t *);
typedef struct aiop_tool_operations aiopt_ops_t;

#endif /* AIOPT_TOOL_H */
