/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP
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

/**************************************************************************//**
@File		aiop_verification_cwapf.c

@Description	This file contains the AIOP CWAPF SW Verification Structures.

*//***************************************************************************/


#include "aiop_verification.h"

extern __TASK struct aiop_default_task_params default_task_params;

uint16_t  aiop_verification_cwapf(
		uint32_t data_addr)
{
	uint16_t str_size = STR_SIZE_ERR;
	uint32_t opcode;

	opcode  = *((uint32_t *) data_addr);

	switch (opcode) {
	/* CWAPF Init Context command */
	case CWAPF_CONTEXT_INIT_CMD_STR :
	{
		struct cwapf_init_command *str =
			(struct cwapf_init_command *)data_addr;

		cwapf_context_init(str->mtu, (uint8_t *)(str->cwapf_ctx_addr));
		str_size = sizeof(struct cwapf_init_command);
		break;
	}
	/* CWAPF Generate Fragment Command */
	case CWAPF_GENERATE_FRAG_CMD_STR:
	{
		struct cwapf_generate_frag_command *str =
			(struct cwapf_generate_frag_command *)data_addr;
		
		str->status = cwapf_generate_frag(
				(uint8_t *)(str->cwapf_ctx_addr));
		/* The fragment that was generated is now the default frame of 
		 * the task */
		*((int32_t *)(str->cwapf_status_addr)) = str->status;
		str->prc = *((struct presentation_context *) HWC_PRC_ADDRESS);
		str->pr = *((struct parse_result *) HWC_PARSE_RES_ADDRESS);
		str->default_task_params = default_task_params;

		str_size = sizeof(struct cwapf_generate_frag_command);
		break;
	}
	/* CWAPF Discard Remaining Frame Command */
	case CWAPF_DISCARD_REMAINING_FRAME_CMD_STR:
	{
		struct cwapf_discard_remainder_frame_command *str =
			(struct cwapf_discard_remainder_frame_command *)
				data_addr;
		
		str->status = cwapf_discard_frame_remainder(
				(uint8_t *)(str->cwapf_ctx_addr));
		*((int32_t *)(str->cwapf_status_addr)) = str->status;

		str_size = sizeof(struct cwapf_discard_remainder_frame_command);
		break;
	}
	default:
	{
		return STR_SIZE_ERR;
	}
	}

	return str_size;
}
