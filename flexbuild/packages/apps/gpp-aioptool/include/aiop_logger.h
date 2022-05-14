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

#ifndef AIOPT_LOGGER_H
#define AIOPT_LOGGER_H

#include <stdio.h>
#include <errno.h>

unsigned short int _debug_flag;
unsigned short int _verbose_flag;

#define LOG(...)		printf(__VA_ARGS__)

#define AIOPT_INFO(format,...)	do {					\
					if (!_verbose_flag)		\
						break;			\
					LOG("INFO: "			\
					format,				\
					##__VA_ARGS__);			\
				} while(0);

#define AIOPT_PRINT(format,...)	do {					\
					LOG(format,			\
					##__VA_ARGS__);			\
				} while(0);

#define AIOPT_ERR(format,...)	do {\
					LOG("ERROR: (L%d): "		\
					format, __LINE__,		\
					##__VA_ARGS__);			\
				} while(0);

#define AIOPT_DEBUG(format,...)	do {\
					if (!_debug_flag)		\
						break;			\
					LOG("DEBUG: (%s)(L%d): "	\
					format, __func__, __LINE__,	\
					##__VA_ARGS__);			\
				} while(0);

#define AIOPT_DEV(format,...)	do {\
					if (!_debug_flag)		\
						break;			\
					LOG("DEBUG_DEV: (%s)(L%d): "	\
					format, __func__, __LINE__,	\
					##__VA_ARGS__);			\
				} while(0);

#define AIOPT_LIB_INFO(format,...)	do {\
					if (!_verbose_flag)		\
						break;			\
					LOG("INFO: "			\
					format,				\
					##__VA_ARGS__);			\
				} while(0);

void init_aiopt_logger(int d, int v);

#endif /* AIOPT_LOGGER_H */
