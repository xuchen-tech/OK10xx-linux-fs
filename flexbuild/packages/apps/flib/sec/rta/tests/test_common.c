/*
 * Copyright 2008-2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdio.h>
#include "test_common.h"

/* Test vectors from dxlt/ are in big endian format */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
const int need_bswap = 0;
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
const int need_bswap = 1;
#else
#error "No endianness set in environment!"
#endif

void print_prog(uint32_t *buff, int size)
{
	int i;
	uint8_t *it;

	if (size < 0) {
		printf("Invalid descriptor size (%d)\n", size);
		return;
	}

	for (i = 0; i < size; i++) {
		it = (uint8_t *)&buff[i];
		printf("%02x", *it);
		printf("%02x", *(it + 1));
		printf("%02x", *(it + 2));
		printf("%02x\n", *(it + 3));
	}
}
