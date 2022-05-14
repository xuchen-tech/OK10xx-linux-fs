/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTA_RTA_H__
#define __RTA_RTA_H__

#include "rta/sec_run_time_asm.h"
#include "rta/fifo_load_store_cmd.h"
#include "rta/header_cmd.h"
#include "rta/jump_cmd.h"
#include "rta/key_cmd.h"
#include "rta/load_cmd.h"
#include "rta/math_cmd.h"
#include "rta/move_cmd.h"
#include "rta/nfifo_cmd.h"
#include "rta/operation_cmd.h"
#include "rta/protocol_cmd.h"
#include "rta/seq_in_out_ptr_cmd.h"
#include "rta/signature_cmd.h"
#include "rta/store_cmd.h"

/**
 * @file rta.h
 * @brief RTA API definition.
 */

/**
 * @defgroup rta_api_group RTA API
 * Contains RTA API details.
 *
 * @defgroup program_group Descriptor Buffer Management Routines
 * @ingroup rta_api_group
 * Contains details of RTA descriptor buffer management and SEC Era
 * management routines.
 *
 * @defgroup cmd_group SEC Commands Routines
 * @ingroup rta_api_group
 * Contains details of RTA wrapper routines over SEC engine commands.
 *
 * @defgroup refcode_group Self Referential Code Management Routines
 * @ingroup rta_api_group
 * Contains details of RTA self referential code routines.
 */

/**
 * @mainpage
 *
 * RTA (Runtime Assembler) Library is an easy and flexible runtime method for
 * writing SEC descriptors. It implements a thin abstraction layer above
 * SEC commands set; the resulting code is compact and similar to a
 * descriptor sequence.
 *
 * RTA library improves comprehension of the SEC code, adds flexibility for
 * writing complex descriptors and keeps the code lightweight. Should be used
 * by whom needs to encode descriptors at runtime, with comprehensible flow
 * control in descriptor.
 *
 * The \subpage userman "User Manual" page contains more details about RTA.
 *
 * \image html rta_arch.svg "RTA Integration Overview"
 *
 * In the SDK package, an example of RTA usage in user space is included.
 * RTA is used in user space by DPDK (Data Plane Development Kit).
 * The inclusion of RTA in kernel space in the diagram above is only
 * demonstrative.
 */

/** @addtogroup program_group
 *  @{
 */

/**
 * @def PROGRAM_CNTXT_INIT
 * @details Must be called before any descriptor run-time assembly call type
 *          field carry info i.e. whether descriptor is shared or job
 *          descriptor.
 * @param[out] program pointer to struct program
 * @param[in] buffer input buffer where the descriptor will be placed
 *            (@c uint32_t *)
 * @param[in] offset offset in input buffer from where the data will be
 *            written (@c unsigned)
 */
#define PROGRAM_CNTXT_INIT(program, buffer, offset) \
	rta_program_cntxt_init(program, buffer, offset)

/**
 * @def PROGRAM_FINALIZE
 * @details Must be called to mark completion of RTA call.
 * @param[out] program pointer to struct program
 * @return total size of the descriptor in words or negative number on error.
 */
#define PROGRAM_FINALIZE(program) rta_program_finalize(program)

/**
 * @def PROGRAM_SET_36BIT_ADDR
 * @details Must be called to set pointer size to 36 bits.
 * @param[out] program pointer to struct program
 * @return current size of the descriptor in words (@c unsigned).
 */
#define PROGRAM_SET_36BIT_ADDR(program) rta_program_set_36bit_addr(program)

/**
 * @def PROGRAM_SET_BSWAP
 * @details Must be called to enable byte swapping.
 * @param[out] program pointer to struct program
 * @return current size of the descriptor in words (@c unsigned).
 * @note Byte swapping on a 4-byte boundary will be performed at the end - when
 * calling PROGRAM_FINALIZE().
 */
#define PROGRAM_SET_BSWAP(program) rta_program_set_bswap(program)

/**
 * @def WORD
 * @details Nust be called to insert in descriptor buffer a 32bit value.
 * @param[out] program pointer to struct program
 * @param[in] val input value to be written in descriptor buffer (@c uint32_t)
 * @return the descriptor buffer offset where this command is inserted
 *         (@c unsigned).
 */
#define WORD(program, val) rta_word(program, val)

/**
 * @def DWORD
 * @details Must be called to insert in descriptor buffer a 64bit value.
 * @param[out] program pointer to struct program
 * @param[in] val input value to be written in descriptor buffer (@c uint64_t)
 * @return the descriptor buffer offset where this command is inserted
 *         (@c unsigned).
 */
#define DWORD(program, val) rta_dword(program, val)

/**
 * @def COPY_DATA
 * @details Must be called to insert in descriptor buffer data larger than
 *          64 bits.
 * @param[out] program pointer to struct program
 * @param[in] data input data to be written in descriptor buffer (@c uint8_t *)
 * @param[in] len length of input data (@c unsigned)
 * @return the descriptor buffer offset where this command is inserted
 *         (@c unsigned).
 */
#define COPY_DATA(program, data, len) rta_copy_data(program, (data), (len))

/**
 * @def DESC_LEN
 * @details Determines job / shared descriptor buffer length (in words).
 * @param[in] buffer descriptor buffer (@c uint32_t *)
 * @return descriptor buffer length in words (@c unsigned).
 */
#define DESC_LEN(buffer) rta_desc_len(buffer)

/**
 * @def DESC_BYTES
 * @details Determines job / shared descriptor buffer length (in bytes).
 * @param[in] buffer descriptor buffer (@c uint32_t *)
 * @return descriptor buffer length in bytes (@c unsigned).
 */
#define DESC_BYTES(buffer) rta_desc_bytes(buffer)

/**
 * @brief SEC HW block revision.
 *
 * This *must not be confused with SEC version*:
 * - SEC HW block revision format is "v"
 * - SEC revision format is "x.y"
 */
extern enum rta_sec_era rta_sec_era;

/**
 * @brief Set SEC Era HW block revision for which the RTA library will generate
 *        generate the descriptors.
 * @warning Must be called *only once*, *before* using any other RTA API
 *          routine.
 * @warning *Not thread safe*.
 * @param[in] era SEC Era (<c>enum rta_sec_era</c>)
 * @return 0 if the ERA was set successfully, -1 otherwise (@c int)
 */
static inline int rta_set_sec_era(enum rta_sec_era era)
{
	if (era > MAX_SEC_ERA) {
		rta_sec_era = DEFAULT_SEC_ERA;
		pr_err("Unsupported SEC ERA. Defaulting to ERA %d\n",
		       DEFAULT_SEC_ERA + 1);
		return -1;
	}

	rta_sec_era = era;
	return 0;
}

/**
 * @brief Get SEC Era HW block revision for which the RTA library will generate
 *        the descriptors.
 * @return SEC Era (@c unsigned).
 */
static inline unsigned rta_get_sec_era(void)
{
	 return rta_sec_era;
}

/** @} */ /* end of program_group */

/** @addtogroup cmd_group
 *  @{
 */

/**
 * @def SHR_HDR
 * @details Configures Shared Descriptor @b HEADER command.
 * @param[in,out] program pointer to struct program
 * @param[in] share descriptor share state (<c>enum rta_share_type</c>)
 * @param[in] start_idx index in descriptor buffer where the execution of the
 *            shared descriptor should start (@c unsigned).
 * @param[in] flags operational flags:@n @li <em>RIF, DNR, CIF, SC, PD</em>
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define SHR_HDR(program, share, start_idx, flags) \
	rta_shr_header(program, share, start_idx, flags)

/**
 * @def JOB_HDR
 * @details Configures JOB Descriptor @b HEADER command.
 * @param[in,out] program pointer to struct program
 * @param[in] share descriptor share state (<c>enum rta_share_type</c>)
 * @param[in] start_idx index in descriptor buffer where the execution of the
 *            job descriptor should start (@c unsigned). In case SHR bit is
 *            present in flags, this will be the shared descriptor length.
 * @param[in] share_desc pointer to shared descriptor, in case @em SHR bit is
 *            set (@c uint64_t)
 * @param[in] flags operational flags:@n @li <em>RSMS, DNR, TD, MTD, REO,
 *            SHR</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define JOB_HDR(program, share, start_idx, share_desc, flags) \
	rta_job_header(program, share, start_idx, share_desc, flags, 0)

/**
 * @def JOB_HDR_EXT
 * @details Configures JOB Descriptor @b HEADER command.
 * @param[in,out] program pointer to struct program
 * @param[in] share descriptor share state (<c>enum rta_share_type</c>)
 * @param[in] start_idx index in descriptor buffer where the execution of the
 *            job descriptor should start (@c unsigned). In case SHR bit is
 *            present in flags, this will be the shared descriptor length.
 * @param[in] share_desc pointer to shared descriptor, in case @em SHR bit is
 *            set (@c uint64_t)
 * @param[in] flags operational flags:@n @li <em>RSMS, DNR, TD, MTD, REO,
 *            SHR</em>.
 * @param[in] ext_flags extended header flags: @n @li <em> DSV (DECO Select
 *            Valid), DECO Id (limited by DSEL_MASK)</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define JOB_HDR_EXT(program, share, start_idx, share_desc, flags, ext_flags) \
	rta_job_header(program, share, start_idx, share_desc, flags | EXT, \
		       ext_flags)

/**
 * @def MOVE
 * @details Configures @b MOVE and @b MOVE_LEN commands.
 * @param[in,out] program pointer to struct program
 * @param[in] src internal source of data that will be moved:@n @li
 *            <em>CONTEXT1, CONTEXT2, OFIFO, DESCBUF, MATH0-MATH3, IFIFOABD,
 *            IFIFOAB1, IFIFOAB2, AB1, AB2, ABD</em>.
 * @param[in] src_offset offset in source data (@c uint16_t)
 * @param[in] dst internal destination of data that will be moved:@n @li
 *            <em>CONTEXT1, CONTEXT2, OFIFO, DESCBUF, MATH0-MATH3, IFIFOAB1,
 *            IFIFOAB2, IFIFO, PKA, KEY1, KEY2, ALTSOURCE</em>.
 * @param[in] dst_offset offset in destination data (@c uint16_t)
 * @param[in] length size of data to be moved:@n @li for @b MOVE must be
 *            specified as immediate value and IMMED flag must be set; @li for
 *            @b MOVE_LEN must be specified using @e MATH0-MATH3.
 * @param[in] opt operational flags:@n @li <em>WAITCOMP, FLUSH1, FLUSH2, LAST1,
 *            LAST2, SIZE_WORD, SIZE_BYTE, SIZE_DWORD, IMMED (not valid for
 *            MOVE_LEN)</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define MOVE(program, src, src_offset, dst, dst_offset, length, opt) \
	rta_move(program, __MOVE, src, src_offset, dst, dst_offset, length, opt)

/**
 * @def MOVEB
 * @details Configures @b MOVEB command.
 * Identical with @b MOVE command if byte swapping not enabled; else, when
 * src/dst is descriptor buffer or MATH registers, data type is byte array when
 * MOVE data type is 4-byte array and vice versa.
 * @param[in,out] program pointer to struct program
 * @param[in] src internal source of data that will be moved:@n @li
 *            <em>CONTEXT1, CONTEXT2, OFIFO, DESCBUF, MATH0-MATH3, IFIFOABD,
 *            IFIFOAB1, IFIFOAB2, AB1, AB2, ABD</em>.
 * @param[in] src_offset offset in source data (@c uint16_t)
 * @param[in] dst internal destination of data that will be moved:@n @li
 *            <em>CONTEXT1, CONTEXT2, OFIFO, DESCBUF, MATH0-MATH3, IFIFOAB1,
 *            IFIFOAB2, IFIFO, PKA, KEY1, KEY2, ALTSOURCE</em>.
 * @param[in] dst_offset offset in destination data (@c uint16_t)
 * @param[in] length  size of data to be moved:@n @li for @b MOVE must be
 *            specified as immediate value and IMMED flag must be set; @li for
 *            @b MOVE_LEN must be specified using @e MATH0-MATH3.
 * @param[in] opt operational flags:@n @li <em>WAITCOMP, FLUSH1, FLUSH2, LAST1,
 *            LAST2, SIZE_WORD, SIZE_BYTE, SIZE_DWORD, IMMED (not valid for
 *            MOVE_LEN)</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define MOVEB(program, src, src_offset, dst, dst_offset, length, opt) \
	rta_move(program, __MOVEB, src, src_offset, dst, dst_offset, length, \
		 opt)

/**
 * @def MOVEDW
 * @details Configures @b MOVEDW command.
 * Identical with @b MOVE command, with the following differences:@n @li data
 * type is 8-byte array; @li word swapping is performed when SEC is programmed
 * in little endian mode.
 * @param[in,out] program pointer to struct program
 * @param[in] src internal source of data that will be moved:@n @li
 *            <em>CONTEXT1, CONTEXT2, OFIFO, DESCBUF, MATH0-MATH3, IFIFOABD,
 *            IFIFOAB1, IFIFOAB2, AB1, AB2, ABD</em>.
 * @param[in] src_offset offset in source data (@c uint16_t)
 * @param[in] dst internal destination of data that will be moved:@n @li
 *            <em>CONTEXT1, CONTEXT2, OFIFO, DESCBUF, MATH0-MATH3, IFIFOAB1,
 *            IFIFOAB2, IFIFO, PKA, KEY1, KEY2, ALTSOURCE</em>.
 * @param[in] dst_offset offset in destination data (@c uint16_t)
 * @param[in] length size of data to be moved:@n @li for @b MOVE must be
 *            specified as immediate value and IMMED flag must be set; @li for
 *            @b MOVE_LEN must be specified using @e MATH0-MATH3.
 * @param[in] opt operational flags:@n @li <em>WAITCOMP, FLUSH1, FLUSH2, LAST1,
 *            LAST2, SIZE_WORD, SIZE_BYTE, SIZE_DWORD, IMMED (not valid for
 *            MOVE_LEN)</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define MOVEDW(program, src, src_offset, dst, dst_offset, length, opt) \
	rta_move(program, __MOVEDW, src, src_offset, dst, dst_offset, length, \
		 opt)

/**
 * @def FIFOLOAD
 * @details Configures @b FIFOLOAD command to load message data, PKHA data, IV,
 *          ICV, AAD and bit length message data into Input Data FIFO.
 * @param[in,out] program pointer to struct program
 * @param[in] data input data type to store:@n @li <em>PKHA registers, IFIFO,
 *            MSG1, MSG2, MSGOUTSNOOP, MSGINSNOOP, IV1, IV2, AAD1, ICV1, ICV2,
 *            BIT_DATA, SKIP</em>.
 * @param[in] src pointer or actual data in case of immediate load; IMMED, COPY
 *            and DCOPY flags indicate action taken (inline imm data, inline
 *            ptr, inline from ptr).
 * @param[in] length number of bytes to load (@c uint32_t)
 * @param[in] flags operational flags:@n @li <em>SGF, IMMED, EXT, CLASS1,
 *            CLASS2, BOTH, FLUSH1, LAST1, LAST2, COPY, DCOPY</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define FIFOLOAD(program, data, src, length, flags) \
	rta_fifo_load(program, data, src, length, flags)

/**
 * @def SEQFIFOLOAD
 * @details Configures <b>SEQ FIFOLOAD</b> command to load message data, PKHA
 *          data, IV, ICV, AAD and bit length message data into Input Data FIFO.
 * @param[in,out] program pointer to struct program
 * @param[in] data input data type to store:@n @li <em>PKHA registers, IFIFO,
 *            MSG1, MSG2, MSGOUTSNOOP, MSGINSNOOP, IV1, IV2, AAD1, ICV1, ICV2,
 *            BIT_DATA, SKIP</em>.
 * @param[in] length number of bytes to load; can be set to 0 for SEQ command
 *            w/ @e VLF set (@c uint32_t).
 * @param[in] flags operational flags:@n @li <em>VLF, CLASS1, CLASS2, BOTH,
 *            FLUSH1, LAST1, LAST2, AIDF</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define SEQFIFOLOAD(program, data, length, flags) \
	rta_fifo_load(program, data, NONE, length, flags|SEQ)

/**
 * @def FIFOSTORE
 * @details Configures @b FIFOSTORE command, to move data from Output Data FIFO
 *          to external memory via DMA.
 * @param[in,out] program pointer to struct program
 * @param[in] data output data type to store:@n @li <em>PKHA registers, IFIFO,
 *            OFIFO, RNG, RNGOFIFO, AFHA_SBOX, MDHA_SPLIT_KEY, MSG, KEY1, KEY2,
 *            SKIP</em>.
 * @param[in] encrypt_flags store data encryption mode:@n @li <em>EKT, TK</em>
 * @param[in] dst pointer to store location (@c uint64_t).
 * @param[in] length number of bytes to load (@c uint32_t).
 * @param[in] flags operational flags:@ @li <em>SGF, CONT, EXT, CLASS1, CLASS2,
 *            BOTH</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define FIFOSTORE(program, data, encrypt_flags, dst, length, flags) \
	rta_fifo_store(program, data, encrypt_flags, dst, length, flags)

/**
 * @def SEQFIFOSTORE
 * @details Configures <b>SEQ FIFOSTORE</b> command, to move data from Output
 *          Data FIFO to external memory via DMA.
 * @param[in,out] program pointer to struct program
 * @param[in] data  output data type to store:@n @li <em>PKHA registers, IFIFO,
 *            OFIFO, RNG, RNGOFIFO, AFHA_SBOX, MDHA_SPLIT_KEY, MSG, KEY1, KEY2,
 *            METADATA, SKIP</em>.
 * @param[in] encrypt_flags store data encryption mode:@n @li <em>EKT, TK</em>
 * @param[in] length  number of bytes to load; can be set to 0 for SEQ command
 *            w/ @e VLF set (@c uint32_t).
 * @param[in] flags  operational flags:@n @li <em>VLF, CONT, EXT, CLASS1,
 *            CLASS2, BOTH</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define SEQFIFOSTORE(program, data, encrypt_flags, length, flags) \
	rta_fifo_store(program, data, encrypt_flags, 0, length, flags|SEQ)

/**
 * @def KEY
 * @details Configures @b KEY and <b>SEQ KEY</b> commands.
 * @param[in,out] program pointer to struct program
 * @param[in] key_dst key store location:@n @li <em>KEY1, KEY2, PKE, AFHA_SBOX,
 *            MDHA_SPLIT_KEY</em>.
 * @param[in] encrypt_flags key encryption mode:@n @li <em>ENC, EKT, TK, NWB,
 *            PTS</em>.
 * @param[in] src pointer or actual data in case of immediate load
 *            (@c uint64_t); IMMED, COPY and DCOPY flags indicate action taken
 *            (inline imm data, inline ptr, inline from ptr).
 * @param[in] length number of bytes to load; can be set to 0 for SEQ command
 *            w/ @e VLF set (@c uint32_t).
 * @param[in] flags operational flags:@n @li for KEY: <em>SGF, IMMED, COPY,
 *            DCOPY</em>; @li for SEQKEY: <em>SEQ, VLF, AIDF</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define KEY(program, key_dst, encrypt_flags, src, length, flags) \
	rta_key(program, key_dst, encrypt_flags, src, length, flags)

/**
 * @def SEQINPTR
 * @details Configures <b>SEQ IN PTR</b> command.
 * @param[in,out] program pointer to struct program
 * @param[in] src starting address for Input Sequence (@c uint64_t)
 * @param[in] length number of bytes in (or to be added to) Input Sequence
 *            (@c uint32_t).
 * @param[in] flags operational flags:@n @li <em>RBS, INL, SGF, PRE, EXT, RTO,
 *            RJD, SOP</em> (when @e PRE, @e RTO or @e SOP are set, @e src
 *            parameter must be 0).
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define SEQINPTR(program, src, length, flags) \
	rta_seq_in_ptr(program, src, length, flags)

/**
 * @def SEQOUTPTR
 * @details Configures <b>SEQ OUT PTR</b> command.
 * @param[in,out] program pointer to struct program
 * @param[in] dst starting address for Output Sequence (@c uint64_t).
 * @param[in] length number of bytes in (or to be added to) Output Sequence
 *            (@c uint32_t).
 * @param[in] flags operational flags:@n @li <em>SGF, PRE, EXT, RTO, RST, EWS
 *            </em> (when @e PRE or @e RTO are set, @e dst parameter must be 0).
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define SEQOUTPTR(program, dst, length, flags) \
	rta_seq_out_ptr(program, dst, length, flags)

/**
 * @def ALG_OPERATION
 * @details Configures <b>ALGORITHM OPERATION</b> command.
 * @param[in,out] program pointer to struct program
 * @param[in] cipher_alg algorithm to be used
 * @param[in] aai Additional Algorithm Information; contains mode information
 *            that is associated with the algorithm (check desc.h for specific
 *            values).
 * @param[in] algo_state algorithm state; defines the state of the algorithm
 *            that is being executed (check desc.h file for specific values).
 * @param[in] icv_check ICV checking; selects whether the algorithm should check
 *            calculated ICV with known ICV:@n @li <em>ICV_CHECK_ENABLE,
 *            ICV_CHECK_DISABLE</em>.
 * @param[in] enc selects between encryption and decryption:@n @li<em>DIR_ENC,
 *            DIR_DEC</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define ALG_OPERATION(program, cipher_alg, aai, algo_state, icv_check, enc) \
	rta_operation(program, cipher_alg, aai, algo_state, icv_check, enc)

/**
 * @def PROTOCOL
 * @details Configures <b>PROTOCOL OPERATION</b> command.
 * @param[in,out] program pointer to struct program
 * @param[in] optype operation type:@n @li <em>OP_TYPE_UNI_PROTOCOL /
 *            OP_TYPE_DECAP_PROTOCOL / OP_TYPE_ENCAP_PROTOCOL</em>.
 * @param[in] protid protocol identifier value (check desc.h file for specific
 *            values).
 * @param[in] protoinfo protocol dependent value (check desc.h file for specific
 *            values).
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define PROTOCOL(program, optype, protid, protoinfo) \
	rta_proto_operation(program, optype, protid, protoinfo)

/**
 * @def DKP_PROTOCOL
 * @details Configures <b>DKP (Derived Key Protocol) PROTOCOL</b> command.
 * @param[in,out] program pointer to struct program
 * @param[in] protid protocol identifier value - one of the following:
 *            <em>OP_PCLID_DKP_{MD5 | SHA1 | SHA224 | SHA256 | SHA384 |
 *            SHA512}</em>.
 * @param[in] key_src How the initial ("negotiated") key is provided to the DKP
 *            protocol. Valid values - one of <em>OP_PCL_DKP_SRC_{IMM, SEQ, PTR,
 *            SGF}</em>. Not all (key_src,key_dst) combinations are allowed.
 * @param[in] key_dst How the derived ("split") key is returned by the DKP
 *            protocol. Valid values - one of <em>OP_PCL_DKP_DST_{IMM, SEQ, PTR,
 *            SGF}</em>. Not all (key_src,key_dst) combinations are allowed.
 * @param[in] keylen length of the initial key, in bytes (@c uint16_t).
 * @param[in] key address where algorithm key resides;@n @li virtual address if
 *            key_type is RTA_DATA_IMM; @li physical (bus) address if key_type
 *            is RTA_DATA_PTR or RTA_DATA_IMM_DMA.
 * @param[in] key_type enum rta_data_type
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define DKP_PROTOCOL(program, protid, key_src, key_dst, keylen, key, key_type) \
	rta_dkp_proto(program, protid, key_src, key_dst, keylen, key, key_type)

/**
 * @def PKHA_OPERATION
 * @details Configures <b>PKHA OPERATION</b> command.
 * @param[in,out] program pointer to struct program
 * @param[in] op_pkha PKHA operation; indicates the modular arithmetic function
 *            to execute (check desc.h file for specific values).
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define PKHA_OPERATION(program, op_pkha)   rta_pkha_operation(program, op_pkha)

/**
 * @def JUMP
 * @details Configures @b JUMP command.
 * @param[in,out] program pointer to struct program
 * @param[in] addr local offset for local jumps or address pointer for non-local
 *            jumps; @b IMM or @b PTR macros must be used to indicate type.
 * @param[in] jump_type type of action taken by jump (enum rta_jump_type)
 * @param[in] test_type defines how jump conditions are evaluated
 *            (enum rta_jump_cond)
 * @param[in] cond jump conditions:@n @li operational flags <em>DONE1, DONE2,
 *            BOTH</em>; @li various sharing and wait conditions (@e JSL = 1):
 *            <em> NIFP, NIP, NOP, NCP, CALM, SELF, SHARED, JQP</em>; @li Math
 *            and PKHA status conditions (@e JSL = 0): <em>Z, N, NV, C, PK0,
 *            PK1, PKP</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define JUMP(program, addr, jump_type, test_type, cond) \
	rta_jump(program, addr, jump_type, test_type, cond, NONE)

/**
 * @def JUMP_INC
 * @details Configures @b JUMP_INC command.
 * @param[in,out] program pointer to struct program
 * @param[in] addr local offset; @b IMM or @b PTR macros must be used to
 *            indicate type
 * @param[in] test_type defines how jump conditions are evaluated
 *            (enum rta_jump_cond)
 * @param[in] cond jump conditions:@n @li Math status conditions (@e JSL = 0):
 *            <em>Z, N, NV, C</em>.
 * @param[in] src_dst register to increment / decrement:@n @li <em>MATH0-MATH3,
 *            DPOVRD, SEQINSZ, SEQOUTSZ, VSEQINSZ, VSEQOUTSZ</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define JUMP_INC(program, addr, test_type, cond, src_dst) \
	rta_jump(program, addr, LOCAL_JUMP_INC, test_type, cond, src_dst)

/**
 * @def JUMP_DEC
 * @details Configures @b JUMP_DEC command.
 * @param[in,out] program pointer to struct program
 * @param[in] addr local offset; @b IMM or @b PTR macros must be used to
 *            indicate type
 * @param[in] test_type defines how jump conditions are evaluated
 *            (enum rta_jump_cond)
 * @param[in] cond jump conditions:@n @li Math status conditions (@e JSL = 0):
 *            <em>Z, N, NV, C</em>.
 * @param[in] src_dst register to increment / decrement:@n @li <em>MATH0-MATH3,
 *            DPOVRD, SEQINSZ, SEQOUTSZ, VSEQINSZ, VSEQOUTSZ</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define JUMP_DEC(program, addr, test_type, cond, src_dst) \
	rta_jump(program, addr, LOCAL_JUMP_DEC, test_type, cond, src_dst)

/**
 * @def LOAD
 * @details Configures @b LOAD command to load data registers from descriptor or
 *          from a memory location.
 * @param[in,out] program pointer to struct program
 * @param[in] addr immediate value or pointer to the data to be loaded; IMMED,
 *            COPY and DCOPY flags indicate action taken (inline imm data,
 *            inline ptr, inline from ptr).
 * @param[in] dst destination register (@c uint64_t).
 * @param[in] offset start point to write data in destination register
 *            (@c uint32_t).
 * @param[in] length number of bytes to load (@c uint32_t).
 * @param[in] flags  operational flags:@n @li <em>VLF, IMMED, COPY, DCOPY</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define LOAD(program, addr, dst, offset, length, flags) \
	rta_load(program, addr, dst, offset, length, flags)

/**
 * @def SEQLOAD
 * @details Configures <b>SEQ LOAD</b> command to load data registers from
 *          descriptor or from a memory location.
 * @param[in,out] program pointer to struct program
 * @param[in] dst destination register (@c uint64_t).
 * @param[in] offset start point to write data in destination register
 *            (@c uint32_t).
 * @param[in] length number of bytes to load (@c uint32_t).
 * @param[in] flags operational flags: @e SGF.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define SEQLOAD(program, dst, offset, length, flags) \
	rta_load(program, NONE, dst, offset, length, flags|SEQ)

/**
 * @def STORE
 * @details Configures @b STORE command to read data from registers and write
 *          them to a memory location.
 * @param[in,out] program pointer to struct program
 * @param[in] src immediate value or source register for data to be stored:@n
 *            @li <em>KEY1SZ, KEY2SZ, DJQDA, MODE1, MODE2, DJQCTRL, DATA1SZ,
 *            DATA2SZ, DSTAT, ICV1SZ, ICV2SZ, DPID, CCTRL, ICTRL, CLRW, CSTAT,
 *            MATH0-MATH3, PKHA registers, CONTEXT1, CONTEXT2, DESCBUF,
 *            JOBDESCBUF, SHAREDESCBUF</em>. In case of immediate value, IMMED,
 *            COPY and DCOPY flags indicate action taken (inline imm data,
 *            inline ptr, inline from ptr).
 * @param[in] offset start point for reading from source register (@c uint16_t).
 * @param[in] dst pointer to store location (@c uint64_t).
 * @param[in] length number of bytes to store (@c uint32_t).
 * @param[in] flags operational flags:@n @li <em>VLF, IMMED, COPY, DCOPY</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define STORE(program, src, offset, dst, length, flags) \
	rta_store(program, src, offset, dst, length, flags)

/**
 * @def SEQSTORE
 * @details Configures <b>SEQ STORE</b> command to read data from registers and
 *          write them to a memory location.
 * @param[in,out] program pointer to struct program
 * @param[in] src immediate value or source register for data to be stored:@n
 *            @li <em>KEY1SZ, KEY2SZ, DJQDA, MODE1, MODE2, DJQCTRL, DATA1SZ,
 *            DATA2SZ, DSTAT, ICV1SZ, ICV2SZ, DPID, CCTRL, ICTRL, CLRW, CSTAT,
 *            MATH0-MATH3, PKHA registers, CONTEXT1, CONTEXT2, DESCBUF,
 *            JOBDESCBUF, SHAREDESCBUF</em>. In case of immediate value, IMMED,
 *            COPY and DCOPY flags indicate action taken (inline imm data,
 *            inline ptr, inline from ptr).
 * @param[in] offset start point for reading from source register (@c uint16_t).
 * @param[in] length number of bytes to store (@c uint32_t).
 * @param[in] flags operational flags:@n @li <em>SGF, IMMED, COPY, DCOPY</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define SEQSTORE(program, src, offset, length, flags) \
	rta_store(program, src, offset, NONE, length, flags|SEQ)

/**
 * @def MATHB
 * @details Configures @b MATHB command to perform binary operations.
 * @param[in,out] program pointer to struct program
 * @param[in] operand1 first operand:@n @li <em>MATH0-MATH3, DPOVRD, SEQINSZ,
 *            SEQOUTSZ, VSEQINSZ, VSEQOUTSZ, ZERO, ONE, NONE, Immediate value
 *            </em>. @n @b IMMED must be used to indicate immediate value.
 * @param[in] operator function to be performed:@n @li <em>ADD, ADDC, SUB, SUBB,
 *            OR, AND, XOR, LSHIFT, RSHIFT, SHLD</em>.
 * @param[in] operand2 second operand:@n @li <em>MATH0-MATH3, DPOVRD, VSEQINSZ,
 *            VSEQOUTSZ, ABD, OFIFO, JOBSRC, ZERO, ONE, Immediate value</em>. @n
 *            @b IMMED2 must be used to indicate immediate value.
 * @param[in] result destination for the result:@n @li <em>MATH0-MATH3, DPOVRD,
 *            SEQINSZ, SEQOUTSZ, NONE, VSEQINSZ, VSEQOUTSZ</em>.
 * @param[in] length length in bytes of the operation and the immediate value,
 *            if there is one (@c int).
 * @param[in] opt operational flags: @e IFB, NFU, STL, SWP, IMMED, IMMED2.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define MATHB(program, operand1, operator, operand2, result, length, opt) \
	rta_math(program, operand1, MATH_FUN_##operator, operand2, result, \
		 length, opt)

/**
 * @def MATHI
 * @details Configures @b MATHI command to perform binary operations.
 * @param[in,out] program pointer to struct program
 * @param[in] operand @n @li if !SSEL: <em>MATH0-MATH3, DPOVRD, SEQINSZ,
 *            SEQOUTSZ, VSEQINSZ, VSEQOUTSZ, ZERO, ONE</em>.
 *            @li if SSEL: <em>MATH0-MATH3, DPOVRD, VSEQINSZ, VSEQOUTSZ, ABD,
 *            OFIFO, JOBSRC, ZERO, ONE</em>.
 * @param[in] operator function to be performed:@n @li <em>ADD, ADDC, SUB, SUBB,
 *            OR, AND, XOR, LSHIFT, RSHIFT, FBYT (for !SSEL only)</em>.
 * @param[in] imm Immediate value (@c uint8_t).@n @b IMMED must be used to
 *            indicate immediate value.
 * @param[in] result destination for the result:@n @li <em>MATH0-MATH3, DPOVRD,
 *            SEQINSZ, SEQOUTSZ, NONE, VSEQINSZ, VSEQOUTSZ</em>.
 * @param[in] length length in bytes of the operation and the immediate value,
 *            if there is one (@c int). @p imm is left-extended with zeros if
 *            needed.
 * @param[in] opt operational flags:@n @li <em>NFU, SSEL, SWP, IMMED</em>.
 *            @li If !SSEL, @p operand \< @p operator \> @p imm -> @p result
 *            @li If SSEL, @p imm \< @p operator \> @p operand -> @p result
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define MATHI(program, operand, operator, imm, result, length, opt) \
	rta_mathi(program, operand, MATH_FUN_##operator, imm, result, length, \
		  opt)

/**
 * @def MATHU
 * @details Configures @b MATHU command to perform unary operations.
 * @param[in,out] program pointer to struct program
 * @param[in] operand1 operand:@n @li <em>MATH0-MATH3, DPOVRD, SEQINSZ,
 *            SEQOUTSZ, VSEQINSZ, VSEQOUTSZ, ZERO, ONE, NONE, Immediate value
 *            </em>.@n @b IMMED must be used to indicate immediate value.
 * @param[in] operator function to be performed:@n @li <em>ZBYT, BSWAP</em>.
 * @param[in] result destination for the result:@n @li <em>MATH0-MATH3, DPOVRD,
 *            SEQINSZ, SEQOUTSZ, NONE, VSEQINSZ, VSEQOUTSZ</em>.
 * @param[in] length length in bytes of the operation and the immediate value,
 *            if there is one (@c int).
 * @param[in] opt operational flags:@n @li <em>NFU, STL, SWP, IMMED</em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define MATHU(program, operand1, operator, result, length, opt) \
	rta_math(program, operand1, MATH_FUN_##operator, NONE, result, length, \
		 opt)

/**
 * @def SIGNATURE
 * @details Configures @b SIGNATURE command.
 * @param[in,out] program pointer to struct program
 * @param[in] sign_type signature type:@n @li <em>SIGN_TYPE_FINAL,
 *            SIGN_TYPE_FINAL_RESTORE, SIGN_TYPE_FINAL_NONZERO, SIGN_TYPE_IMM_2,
 *            SIGN_TYPE_IMM_3, SIGN_TYPE_IMM_4</em>.
 *            @n After @b SIGNATURE command, @b DWORD or @b WORD must be used to
 *            insert signature in descriptor buffer.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define SIGNATURE(program, sign_type)   rta_signature(program, sign_type)

/**
 * @def NFIFOADD
 * @details Configures @b NFIFO command, a shortcut of RTA Load command to write
 *          to iNfo FIFO.
 * @param[in,out] program pointer to struct program
 * @param[in] src source for the input data in Alignment Block:@n @li <em>IFIFO,
 *            OFIFO, PAD, MSGOUTSNOOP, ALTSOURCE, OFIFO_SYNC, MSGOUTSNOOP_ALT
 *            </em>.
 * @param[in] data type of data that is going through the Input Data FIFO:@n @li
 *            <em>MSG, MSG1, MSG2, IV1, IV2, ICV1, ICV2, SAD1, AAD1, AAD2,
 *            AFHA_SBOX, SKIP, PKHA registers, AB1, AB2, ABD</em>.
 * @param[in] length length of the data copied in FIFO registers (@c uint32_t).
 * @param[in] flags select options between:@n @li operational flags: <em>LAST1,
 *            LAST2, FLUSH1, FLUSH2, OC, BP</em>.
 *            @li when PAD is selected as source: <em>BM, PR, PS</em>.
 *            @li padding type: <em>PAD_ZERO, PAD_NONZERO, PAD_INCREMENT,
 *            PAD_RANDOM, PAD_ZERO_N1, PAD_NONZERO_0, PAD_N1, PAD_NONZERO_N
 *            </em>.
 * @return @li On success, descriptor buffer offset where this command is
 *         inserted.
 *         @li On error, a negative error code; first error program counter will
 *         point to offset in descriptor buffer where the instruction should
 *         have been written.
 */
#define NFIFOADD(program, src, data, length, flags) \
	rta_nfifo_load(program, src, data, length, flags)

/** @} */ /* end of cmd_group */

/** @addtogroup refcode_group
 *  @{
 */

/**
 * @def REFERENCE
 * @details Initialize a variable used for storing an index inside a descriptor
 *          buffer.
 * @param[out] ref reference to a descriptor buffer's index where an update is
 *             required with a value that will be known latter in the program
 *             flow.
 */
#define REFERENCE(ref)    int ref = -1

/**
 * @def LABEL
 * @details Initialize a variable used for storing an index inside a descriptor
 *          buffer.
 * @param[out] label label stores the value with what should be updated the
 *             REFERENCE line in the descriptor buffer.
 */
#define LABEL(label)      unsigned label = 0

/**
 * @def SET_LABEL
 * @details Set a LABEL value.
 * @param[in,out] program pointer to struct program
 * @param[in] label value that will be inserted in a line previously written in
 *            the descriptor buffer.
 */
#define SET_LABEL(program, label)  label = rta_set_label(program)

/**
 * @def PATCH_JUMP
 * @details Auxiliary command to resolve self referential code.
 * @param[in,out] program buffer to be updated (struct program *)
 * @param[in] line position in descriptor buffer where the update will be done;
 *            this value is previously retained in program flow using a
 *            reference near the sequence to be modified.
 * @param[in] new_ref updated value that will be inserted in descriptor buffer
 *            at the specified line; this value is previously obtained using
 *            @b SET_LABEL macro near the line that will be used as reference
 *            (@c unsigned). For @b JUMP command, the value represents the
 *            offset field (in words).
 * @return 0 in case of success, a negative error code if it fails
 */
#define PATCH_JUMP(program, line, new_ref) rta_patch_jmp(program, line, new_ref)

/**
 * @def PATCH_MOVE
 * @details Auxiliary command to resolve self referential code.
 * @param[in,out] program buffer to be updated (struct program *)
 * @param[in] line position in descriptor buffer where the update will be done;
 *            this value is previously retained in program flow using a
 *            reference near the sequence to be modified.
 * @param[in] new_ref updated value that will be inserted in descriptor buffer
 *            at the specified line; this value is previously obtained using
 *            @b SET_LABEL macro near the line that will be used as reference
 *            (@c unsigned). For @b MOVE command, the value represents the
 *            offset field (in words).
 * @return 0 in case of success, a negative error code if it fails
 */
#define PATCH_MOVE(program, line, new_ref) \
	rta_patch_move(program, line, new_ref)

/**
 * @def PATCH_LOAD
 * @details Auxiliary command to resolve self referential code.
 * @param[in,out] program buffer to be updated (struct program *)
 * @param[in] line position in descriptor buffer where the update will be done;
 *            this value is previously retained in program flow using a
 *            reference near the sequence to be modified.
 * @param[in] new_ref updated value that will be inserted in descriptor buffer
 *            at the specified line; this value is previously obtained using
 *            @b SET_LABEL macro near the line that will be used as reference
 *            (@c unsigned). For @b LOAD command, the value represents the
 *            offset field (in words).
 * @return 0 in case of success, a negative error code if it fails
 */
#define PATCH_LOAD(program, line, new_ref) \
	rta_patch_load(program, line, new_ref)

/**
 * @def PATCH_STORE
 * @details Auxiliary command to resolve self referential code.
 * @param[in,out] program buffer to be updated (struct program *)
 * @param[in] line position in descriptor buffer where the update will be done;
 *            this value is previously retained in program flow using a
 *            reference near the sequence to be modified.
 * @param[in] new_ref updated value that will be inserted in descriptor buffer
 *            at the specified line; this value is previously obtained using
 *            @b SET_LABEL macro near the line that will be used as reference
 *            (@c unsigned). For @b STORE command, the value represents the
 *            offset field (in words).
 * @return 0 in case of success, a negative error code if it fails
 */
#define PATCH_STORE(program, line, new_ref) \
	rta_patch_store(program, line, new_ref)

/**
 * @def PATCH_HDR
 * @details Auxiliary command to resolve self referential code.
 * @param[in,out] program buffer to be updated (struct program *)
 * @param[in] line position in descriptor buffer where the update will be done;
 *            this value is previously retained in program flow using a
 *            reference near the sequence to be modified.
 * @param[in] new_ref updated value that will be inserted in descriptor buffer
 *            at the specified line; this value is previously obtained using
 *            @b SET_LABEL macro near the line that will be used as reference
 *            (@c unsigned). For @b HEADER command, the value represents the
 *            start index field.
 * @return 0 in case of success, a negative error code if it fails
 */
#define PATCH_HDR(program, line, new_ref) \
	rta_patch_header(program, line, new_ref)

/**
 * @def PATCH_RAW
 * @details Auxiliary command to resolve self referential code.
 * @param[in,out] program buffer to be updated (struct program *)
 * @param[in] line position in descriptor buffer where the update will be done;
 *            this value is previously retained in program flow using a
 *            reference near the sequence to be modified.
 * @param[in] mask mask to be used for applying the new value (@c unsigned). The
 *            mask selects which bits from the provided @p new_val are taken
 *            into consideration when overwriting the existing value.
 * @param[in] new_val updated value that will be masked using the provided mask
 *            value and inserted in descriptor buffer at the specified line.
 * @return 0 in case of success, a negative error code if it fails
 */
#define PATCH_RAW(program, line, mask, new_val) \
	rta_patch_raw(program, line, mask, new_val)

/** @} */ /* end of refcode_group */

#endif /* __RTA_RTA_H__ */
