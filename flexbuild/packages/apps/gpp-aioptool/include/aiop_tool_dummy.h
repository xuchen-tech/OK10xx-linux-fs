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
 * @file	aiop_tool_dummy.h
 *
 * @brief	AIOP Tool Dummy operation headers for Unit Testing of Cmd line
 *
 */

#ifndef AIOP_TOOL_DUMMY_H
#define AIOP_TOOL_DUMMY_H

#include <aiop_tool.h>
#include <aiop_cmd.h>
#include <aiop_logger.h>

/** @def AIOP_CMDSYS_UNIT_TEST
 * @brief Macro for compiling-in dummy AIOP Tool operations
 * Once enabled during compile time, no VFIO or DPAIOP code is executed
 */
/* #define AIOP_CMDSYS_UNIT_TEST	1 */

/* ===========================================================================
 * AIOPT Operations Definitions
 * ===========================================================================
 */
int dummy_perform_aiop_load(fsl_vfio_t handle, aiopt_conf_t *conf);
int dummy_perform_aiop_reset(fsl_vfio_t handle, aiopt_conf_t *conf);
int dummy_perform_aiop_get_status(fsl_vfio_t handle, aiopt_conf_t *conf);
int dummy_perform_aiop_gettod(fsl_vfio_t handle, aiopt_conf_t *conf);
int dummy_perform_aiop_settod(fsl_vfio_t handle, aiopt_conf_t *conf);

#endif
