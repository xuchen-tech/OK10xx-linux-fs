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
 * @file	aiop_lib.h
 *
 * @brief	AIOP Library interfacing with MC APIOP APIs
 *
 */

#ifndef AIOPT_LIB_H
#define AIOPT_LIB_H

/* Flib and VFIO Headers */
#include <fsl_vfio.h>

/* ======================================================================
 * Macros and Static Declarations
 * ======================================================================*/

/** @def SYSFS_IOMMU_PATH_VSTR
 * @brief IOMMU Directory Path in sysfs
 */
#define SYSFS_IOMMU_PATH_VSTR	"/sys/kernel/iommu_groups/%d/devices"

/** @def MAX_DPOBJ_DEVICES
 * @brief Number of devices which would be stored in the dpobj_type structure
 */
#define MAX_DPOBJ_DEVICES	2 /**< Only dpmcp and dpaiop, single 
					instance of each are supported >*/

/** @def MAX_AIOP_IMAGE_FILE_SZ
 * @breif Maximum size of an AIOP Image
 *
 */
#define MAX_AIOP_IMAGE_FILE_SZ	(8 * 1024 * 1024) /**< 8 MB max size of AIOP
							image file >*/

/** @def MAX_AIOP_ARGS_FILE_SZ
 * @breif Maximum size of an AIOP Arguments
 *
 */
#define MAX_AIOP_ARGS_FILE_SZ	(512) /**< 512 Bytes >*/


/** @def AIOPT_INVALID_HANDLE
 * @brief Invalid AIOPT Handle
 */
#define AIOPT_INVALID_HANDLE	NULL


/** @def AIOPT_ALIGNED_PAGE_SZ
 * @brief Alignment to default page size
 */
#define AIOPT_ALIGNED_PAGE_SZ	4096

/* ======================================================================
 * Structures Declarations
 * ======================================================================*/

/*
 * @brief Type of MC Devices supported by AIOP Tool
 * At present only dpmcp and dpaiop are supported (single instance of each)
 */ 
enum dpobj_type_list {
	MCP_TYPE,
	AIOP_TYPE,
	MAX_DPOBJ_LIMIT
};

typedef enum dpobj_type_list dpobj_type_list_t;

/*
 * @brief Definition of MC Object being used by AIOP Tool
 */
struct dpobj_type {
	char *name; 			/**< Name of the device >*/
	unsigned short int token;	/**< Unique token for context XXX >*/
	int id;				/**< Hardware ID of the device >*/
	int fd;				/**< fd of the device in sysfs >*/
	struct vfio_device_info di;	/**< device_info structure >*/
}; /* TODO Structure alignment has not been considered */

typedef struct dpobj_type dpobj_type_t;

/*
 * @brief Container for all internally used objects for AIOP lib
 * This would be exposed by aiopt_handle_t
 */
struct aiopt_obj {
	fsl_vfio_t	vfio_handle;
	union {
		void		*mcp_addr;
		int64_t		mcp_addr64;
	};
	dpobj_type_t devices[MAX_DPOBJ_DEVICES];
};

typedef struct aiopt_obj aiopt_obj_t;

/*
 * @brief Opaque handle exposed by AIOP lib
 */
typedef void *aiopt_handle_t;

/*
 * @brief Structure to hold AIOP Status which is fetched from multiple MC APIs
 */
struct aiopt_status {
	int id;
	int major_v;
	int minor_v;
	int sl_major_v;
	int sl_minor_v;
	int sl_revision;
	int state;
};

typedef struct aiopt_status aiopt_status_t;

/* ======================================================================
 * Externally available Function Declarations
 * ======================================================================*/

/* Initialization and deinitalization routines */
aiopt_handle_t aiopt_init(const char *container_name);
int aiopt_deinit(aiopt_handle_t obj);

/* Command handlers */
/*
 * @brief
 * AIOPT load call for loading an AIOP Image on a dpaiop object belonging to
 * provided (or default) container.
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [in] ifile AIOP Image file name, with path
 * @param [in] reset Flag to state if reset is to be done before load operation
 * @param [in] tpc threads per AIOP core configuration
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int aiopt_load(aiopt_handle_t handle, const char *ifile,
	       const char *afile, short int reset, unsigned short int tpc);

/*
 * @brief
 * AIOPT Status call. Returns information about State of AIOP Tile, Version
 * information and Service Layer Version information
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [out] s aiopt_status_t instance object which would be filled in
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int aiopt_status(aiopt_handle_t handle, aiopt_status_t *s);

/*
 * @brief
 * AIOPT Reset call for performing Reset of a AIOP Tile.
 * XXX This is not supported on rev1 hardware.
 *
 * @param [in] handle aiopt_handle_t type valid object
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int aiopt_reset(aiopt_handle_t handle);

/*
 * @brief
 * Convert MC State to String
 *
 * @param [in] state State as returned by dpaiop_get_state
 * @return const string explaining state
 */
const char *aiopt_get_state_str(int state);

/*
 * @brief
 * AIOPT Get Time of Day
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [out] tod Time of day as a unsigned long value
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int aiopt_gettod(aiopt_handle_t, uint64_t *tod);

/*
 * @brief
 * AIOPT Set Time of Day
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [in] tod Time of day to set as an unsigned long value
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int aiopt_settod(aiopt_handle_t, uint64_t tod);

#endif /* AIOPT_LIB_H */
