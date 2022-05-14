/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __DESC_COMMON_H__
#define __DESC_COMMON_H__

#include "flib/rta.h"

/**
 * @file common.h
 * @brief Shared Descriptor Constructors - shared structures
 *        Data structures shared between algorithm, protocol implementations.
 */

/**
 * @defgroup descriptor_lib_group RTA Descriptors Library
 * @{
 */
/** @} end of descriptor_lib_group */

/**
 * @defgroup typedefs_group Auxiliary Data Structures
 * @ingroup descriptor_lib_group
 * @{
 */

/**
 * @struct alginfo common.h
 * @details Container for algorithm details
 */
struct alginfo {
	uint32_t algtype; /**< algorithm selector; for valid values, see
			       documentation of the functions where it is
			       used */
	uint32_t keylen; /**< length of the provided algorithm key, in bytes */
	uint64_t key; /**< address where algorithm key resides; virtual address
			   if key_type is RTA_DATA_IMM, physical (bus) address
			   if key_type is RTA_DATA_PTR or RTA_DATA_IMM_DMA */
	uint32_t key_enc_flags; /**< key encryption flags; see encrypt_flags
				     parameter of KEY command for valid
				     values */
	enum rta_data_type key_type; /**< enum rta_data_type */
	uint16_t algmode; /**< algorithm mode selector; for valid values, see
			       documentation of the functions where it is
			       used */
};

#define INLINE_KEY(alginfo)	inline_flags(alginfo->key_type)

/**
 * @details Provide indications on which data items can be inlined
 *          and which shall be referenced in a shared descriptor.
 * @param[in] sd_base_len Shared descriptor base length - bytes consumed by the
 *            commands, excluding the data items to be inlined (or corresponding
 *            pointer if an item is not inlined). Each cnstr_* function that
 *            generates descriptors should have a define mentioning
 *            corresponding length.
 * @param[in] jd_len Maximum length of the job descriptor(s) that will be used
 *            together with the shared descriptor
 * @param[in] data_len Array of lengths of the data items trying to be inlined
 * @param[out] inl_mask 32bit mask with bit x = 1 if data item x can be inlined,
 *             0 otherwise
 * @param[in] count Number of data items (size of @p data_len array);
 *            must be <= 32.
 * @return 0 if data can be inlined / referenced, negative value if not. If 0,
 *         check @p inl_mask for details.
 */
static inline int rta_inline_query(unsigned sd_base_len, unsigned jd_len,
				   unsigned *data_len, uint32_t *inl_mask,
				   unsigned count)
{
	int rem_bytes = (int)(CAAM_DESC_BYTES_MAX - sd_base_len - jd_len);
	unsigned i;

	*inl_mask = 0;
	for (i = 0; (i < count) && (rem_bytes > 0); i++) {
		if (rem_bytes - (int)(data_len[i] +
			(count - i - 1) * CAAM_PTR_SZ) >= 0) {
			rem_bytes -= data_len[i];
			*inl_mask |= (1 << i);
		} else {
			rem_bytes -= CAAM_PTR_SZ;
		}
	}

	return (rem_bytes >= 0) ? 0 : -1;
}

/**
 * @struct protcmd common.h
 * @details Container for Protocol Operation Command fields
 */
struct protcmd {
	uint32_t optype; /**< command type */
	uint32_t protid; /**< protocol Identifier */
	uint16_t protinfo; /**< protocol Information */
};

/** @} end of typedefs_group */

#endif /* __DESC_COMMON_H__ */
