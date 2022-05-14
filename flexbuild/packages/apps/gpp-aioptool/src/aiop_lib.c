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
 * @file	aiop_lib.c
 *
 * @brief	AIOP Tool Library
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
#include <aiop_tool_dummy.h>
#include <aiop_lib.h>

/*MC header files*/                                                            
#include <fsl_dpaiop.h>                                                         
#include <fsl_dpaiop_cmd.h>
#include <fsl_mc_sys.h>
#include <fsl_mc_cmd.h>

/* VFIO headers */
#include <fsl_vfio.h>

/* ========================================================================
 * MACROs and defines
 * ======================================================================== */

/* @def AIOPT_RUN_CORES_ALL
 * @brief Mask for defining cores on which AIOP Tile has to be kicked
 */
#define AIOPT_RUN_CORES_ALL		0xFFFF

/*=========================================================================
 * Internal Functions
 *=========================================================================*/

/*
 * @brief
 * Dump information about the AIOPT Object. It includes information about MCP
 * and DPAIOP Objects.
 *
 * @param [in] obj aiopt_obj_t type object to dump
 * @return void
 */
static inline void
print_aiopt_obj(aiopt_obj_t *obj)
{
	if (obj) {
		AIOPT_LIB_INFO("AIOP Object Information.\n");
		AIOPT_LIB_INFO("MC Object Information: (%s)\n",
				obj->devices[MCP_TYPE].name);
		AIOPT_LIB_INFO("  MC HW ID = %d\n",
				obj->devices[MCP_TYPE].id);
		AIOPT_LIB_INFO("  MC FD (Invalid) = %d\n",
				obj->devices[MCP_TYPE].fd);
		AIOPT_LIB_INFO("  MC VirtualAddress = 0x%lX\n",
				obj->mcp_addr64);
		AIOPT_LIB_INFO("AIOP Object Information: (%s)\n",
				obj->devices[AIOP_TYPE].name);
		AIOPT_LIB_INFO("  AIOP HW ID = %d\n",
				obj->devices[AIOP_TYPE].id);
		AIOPT_LIB_INFO("  AIOP FD (Invalid) = %d\n",
				obj->devices[AIOP_TYPE].fd);
		AIOPT_LIB_INFO("  AIOP Device Info: Num Regions = %d, "
				"Num IRQ = %d.\n",
				obj->devices[AIOP_TYPE].di.num_regions,
				obj->devices[AIOP_TYPE].di.num_irqs);
	}
}

/*
 * @brief
 * Obtain AIOP object ID
 *
 * @param [in] obj aiopt_obj_t type object
 * @return (int) AIOP ID
 */
static inline int
aiopt_get_aiop_id(aiopt_obj_t *obj)
{
	if (!obj) {
		AIOPT_DEV("Incorrect API usage.\n");
		return 0;
	}
	return obj->devices[AIOP_TYPE].id;
}

/*
 * @brief
 * Obtain MC API's token obtained after dpaiop_open call
 *
 * @param [in] obj aiopt_obj_t type object
 * @return (int) Token value in integer
 */
static inline int
aiopt_get_aiop_token(aiopt_obj_t *obj)
{
	if (!obj) {
		AIOPT_DEV("Incorrect API usage.\n");
		return 0;
	}
	return obj->devices[AIOP_TYPE].token;
}

/*
 * @brief Obtain reference to MC API's token obtained after dpaiop_open call
 *
 * @param [in] obj aiopt_obj_t type object
 * @return (unsigned short int *) Pointer to Token embedded in obj
 */
static inline unsigned short int *
aiopt_get_aiop_token_byref(aiopt_obj_t *obj)
{
	if (!obj) {
		AIOPT_DEV("Incorrect API usage.\n");
		return NULL;
	}
	return &(obj->devices[AIOP_TYPE].token);
}

/*
 * @brief
 * Cleanup of the aiopt_obj_t instance by releasing all allocated space to
 * members.
 *
 * @param [in] obj aiopt_obj_t type object for cleanup
 *
 * @return void
 */
static void
cleanup_aiopt_obj(aiopt_obj_t *obj) {
	int i = 0;
	dpobj_type_t *dp = NULL;

	for (i = 0; i < MAX_DPOBJ_DEVICES; i++) {
		dp = &obj->devices[i];
		if (dp->name) {
			free(dp->name);
			dp->name = NULL;
		}
	}
}

/*
 * @brief
 * Allocating and initializing MC portal through VFIO APIs
 *
 * @param [in] obj aiopt_obj_t type handle for AIOPT
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
static int
setup_mc_portal(aiopt_obj_t *obj)
{
	fsl_vfio_t vfio_handle;

	if (!obj) {
		AIOPT_DEV("Incorrect API Usage.\n");
		return AIOPT_FAILURE;
	}

	vfio_handle = obj->vfio_handle;
	/* Allocate memory for MC Portal list */
	obj->mcp_addr64 = fsl_vfio_map_mcp_obj(vfio_handle,
						obj->devices[MCP_TYPE].name);
	if (obj->mcp_addr64 == (int64_t) MAP_FAILED) {
		AIOPT_DEV("Unable to map MCP address. (%d)\n", errno);
		return AIOPT_FAILURE;
	}

	return AIOPT_SUCCESS;
}

/*
 * @brief
 * Generic method for filling information into aiopt_obj_t type object from the
 * VFIO directory entry.
 *
 * @param [in] obj_p Object Name string embedded in object
 * @param [in] obj_id Object ID embedded in object
 * @param [in] dir_name constant string describing directory name
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
static int
fill_obj_info(aiopt_obj_t *obj, dpobj_type_list_t type, const char *dir_name)
{
	int ret;
	char *p = NULL, *t = NULL;
	char *dir_c = NULL;
	unsigned int id;
	dpobj_type_t *device = NULL;

	if (!obj || !dir_name || (type >= MAX_DPOBJ_DEVICES)) {
		AIOPT_DEV("Internal error; Incorrect API usage.\n");
		return AIOPT_FAILURE;
	}

	/* Creating a copy of dir_name as it might be modified by strtok */
	dir_c = calloc(1, strlen(dir_name) + 1);
	if (!dir_c) {
		AIOPT_DEBUG("Unable to allocate internal memory.\n");
		goto err_cleanup;
	}
	strcpy(dir_c, dir_name);

	p = calloc(1, strlen(dir_name) + 1);
	if (!p) {
		AIOPT_DEBUG("Unable to allocate memory.\n")
		goto err_cleanup;
	}
	strcpy(p, dir_name);
	t = strtok(dir_c, ".");
	t = strtok(NULL, ".");
	sscanf(t, "%d", &id);

	device = &(obj->devices[type]);

	device->name = p;
	device->id = id;

	/* For AIOP, open and obtain device information. This is
	 * not required for dpmcp object.
	 */
	if (type == AIOP_TYPE) {
		memset(&(device->di), 0, sizeof(struct vfio_device_info));
		device->di.argsz = sizeof(struct vfio_device_info);

		/* getting the device fd*/
		device->fd = fsl_vfio_get_dev_fd(obj->vfio_handle,
							device->name);
		if (device->fd < 0) {
			AIOPT_DEBUG("Unable to obtain device FD from VFIO (%s)"
				"; fd from group (%d)\n",
				device->name,
				fsl_vfio_get_group_fd(obj->vfio_handle));
			goto err_cleanup;
		}

		/* Get Device inofrmation */
		ret = fsl_vfio_get_device_info(obj->vfio_handle, device->name,
						&(device->di));
		if (ret != 0) {
			AIOPT_DEBUG("Unable to fetch device info "
					"(VFIO_DEVICE_FSL_MC_GET_INFO).\n");
			goto err_cleanup;
		}
	}

	if (dir_c)
		free(dir_c);

	return AIOPT_SUCCESS;

err_cleanup:
	if (dir_c)
		free(dir_c);
	if (p) {
		free(p);
		device->name = NULL;
	}

	return AIOPT_FAILURE;
}

/*
 * @brief
 * Wrapper around internal fill_obj_info method for MCP information. It is
 * expected that the caller has set dir_name parameter to the correct dpmcp
 * object in the sysfs directory structure.
 *
 * @param [in] obj aiopt_obj_t type object to fill information into
 * @param [in] dir_name constant string describing directory name
 *
 * @return value as returned by fill_obj_info
 */
static inline int
fill_mcp_obj_info(aiopt_obj_t *obj, const char *dir_name)
{
	int ret;

	ret = fill_obj_info(obj, MCP_TYPE, dir_name);
	AIOPT_DEV("fill_obj_info returns (%d).\n", ret);

	return ret;
}

/*
 * @brief
 * Calls internal fill_obj_info method for filling AIOP information. It is
 * expected that the caller has set dir_name parameter to the correct dpaiop
 * object in the sysfs directory structure.
 * Unlike fill_mcp_obj_info, this method also fills in device_info structure
 * and device_fd information.
 *
 * @param [in] obj aiopt_obj_t type object to fill information into
 * @param [in] dir_name constant string describing directory name
 *
 * @return value as returned by fill_obj_info
 */
static inline int
fill_aiop_obj_info(aiopt_obj_t *obj, const char *dir_name)
{
	int ret;

	ret = fill_obj_info(obj, AIOP_TYPE, dir_name);
	AIOPT_DEV("fill_obj_info returns (%d).\n", ret);

	return ret;
}

/*
 * @brief
 * Initialize the AIOP Device by calling the MC operations for dpaiop_open.
 * Takes as input a completely filled aiopt_obj_t type object, including info
 * for FD, HW ID and MC Portal Address.
 *
 * @param [IN] obj aiopt_obj_t type object containing MCP/AIOP Device info
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
static int
init_aiop(aiopt_obj_t *obj)
{
	int ret, ret_2;
	int *hw_id;
	unsigned short int *token;
	struct fsl_mc_io *dpaiop = NULL;
	struct dpaiop_sl_version sl_version = {0};

	AIOPT_DEV("Entering.\n");

	if (!obj) {
		AIOPT_DEBUG("Incorrect API usage.\n");
		ret = AIOPT_FAILURE;
		goto err;
	}

	/* Open the nadk device via MC and save the handle for further use */
	dpaiop = (struct fsl_mc_io *)calloc(1, sizeof(struct fsl_mc_io));
	if (!dpaiop) {
		AIOPT_DEBUG("Unable to allocate memory for dpaiop obj.\n");
		ret = AIOPT_FAILURE;
		goto err;
	}

	dpaiop->regs = obj->mcp_addr;
	hw_id = &(obj->devices[AIOP_TYPE].id);
	token = &(obj->devices[AIOP_TYPE].token);

	/* Opening AIOP device */
	ret = dpaiop_open(dpaiop, CMD_PRI_LOW, *hw_id, token);
	if (ret != 0) {
		AIOPT_DEBUG("Opening device failed with err code: %d", ret);
		ret = AIOPT_FAILURE;
		goto err;
	}

	/* Get the device version */
	ret = dpaiop_get_sl_version(dpaiop, CMD_PRI_LOW, *token, &sl_version);
	if (ret != 0) {
		AIOPT_DEV("Unable to get AIOP Version information: %d.\n",
			ret);
		/* This is not considered an error */
		AIOPT_DEV("Attributes: id=%d, v.major=-NA-, v.minor=-NA-.\n",
			*hw_id);
	} else {
		AIOPT_DEV("Attributes: id=%d, v.major=%d, v.minor=%d.\n",
			*hw_id, sl_version.major, sl_version.minor);
	}
	AIOPT_LIB_INFO("Successfully initialized the AIOP device.\n");

	/* Close the device */
	ret_2 = dpaiop_close(dpaiop, CMD_PRI_LOW, *token);
	if (ret_2 != 0)
		AIOPT_DEBUG("Closing AIOP dev failed (%d).\n", ret);

	/* token is invalid hereafter */
err:
	if (dpaiop)
		free(dpaiop);

	AIOPT_DEV("Exiting (%d)\n", ret);
	return ret;
}

/*
 * @brief
 * Initialize the MCP and AIOP objects present in the container and tag them
 * into an internal data structure.
 *
 * @param [in] obj aiopt_obj_t type object to populate
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
static int
setup_aiopt_device(aiopt_obj_t *obj)
{
	int ret;
	unsigned char mcp_avail = FALSE, aiop_avail = FALSE;

	DIR *d;
	struct dirent *dir;
	char path[VFIO_PATH_MAX];

	if (!obj) {
		AIOPT_DEV("Incorrect API usage.\n");
		return AIOPT_FAILURE;
	}

	sprintf(path, SYSFS_IOMMU_PATH_VSTR,
			fsl_vfio_get_group_id(obj->vfio_handle));

	AIOPT_LIB_INFO("VFIO Devices path = %s\n", path);
	d = opendir(path);
	if (!d) {
		AIOPT_DEBUG("Unable to open VFIO directory: %s\n", path);
		return AIOPT_FAILURE;
	}

	/* Finding and extracting MCP and AIOP Objects.
	 * XXX only the first occurences of each are taken - if there are
	 * multiple instances of any (AIOP/MC), they are not looked for.
	 */
	while ((dir = readdir(d)) != NULL) {
		if (!(dir->d_type == DT_LNK))
			continue;
		if (!strncmp("dpmcp", dir->d_name, 5) && !mcp_avail) {
			ret = fill_mcp_obj_info(obj, dir->d_name);
			if (ret != AIOPT_SUCCESS) {
				goto err_cleanup;
			}
			/* Only the first instance of MC and AIOP is taken */
			mcp_avail = TRUE;
		}
		if (!strncmp("dpaiop", dir->d_name, 6) && !aiop_avail) {
			ret = fill_aiop_obj_info(obj, dir->d_name);
			if (ret != AIOPT_SUCCESS) {
				goto err_cleanup;
			}
			/* Only the first instance of MC and AIOP is taken */
			aiop_avail = TRUE;
		}
	}
	closedir(d);

	AIOPT_DEV("In Container, MCP=%s, AIOP=%s\n",
			mcp_avail ? "TRUE": "FALSE",
			aiop_avail ? "TRUE": "FALSE");
	if (!mcp_avail) {
		AIOPT_DEBUG("MCP Object not Found in container.\n");
		goto err_cleanup;
	}
	if (!aiop_avail) {
		AIOPT_DEBUG("AIOP Object not Found in container.\n");
		goto err_cleanup;
	}

	print_aiopt_obj(obj);

	ret = setup_mc_portal(obj);
	if (ret != AIOPT_SUCCESS) {
		AIOPT_DEBUG("Unable to open MC Portal.\n");
		goto err_cleanup;
	}

	ret = init_aiop(obj);
	if (ret != AIOPT_SUCCESS) {
		AIOPT_DEBUG("Unable to initialize the AIOP device.\n");
		goto err_cleanup;
	}

	return AIOPT_SUCCESS;

err_cleanup:
	cleanup_aiopt_obj(obj);
	return AIOPT_FAILURE;

}

/*
 * @brief
 * For a given AIOP Image file, verify existence, type and return FD after
 * opening it.
 *
 * @param [in] ifile AIOP ELF File name with path
 * @param [out] file_sz size of ELF file, if valid
 *
 * @return Value greater than 0 if file opened successfully or AIOPT_FAILURE
 */
static int
get_aiop_image_fd(const char *ifile, size_t *file_sz)
{
	int fd = -1;
	size_t filesize = 0;
	struct stat im_stat;

	AIOPT_DEV("Entering.\n");
	memset(&im_stat, 0, sizeof(im_stat));
	*file_sz = 0;

	/* Checking Validity of the file */
	if (!ifile || (stat(ifile, &im_stat) != 0)) {
		AIOPT_DEBUG("Unable to stat the file (%s).\n", ifile);
		return AIOPT_FAILURE;
	}

	/* get the file size */
	filesize = im_stat.st_size;
	if (filesize <= 0 || filesize > MAX_AIOP_IMAGE_FILE_SZ) {
		AIOPT_DEBUG("Incorrect file size. Give (%lu), Max Allowed (%d)"
				".\n", filesize, MAX_AIOP_IMAGE_FILE_SZ);
		return AIOPT_FAILURE;
	}

	/* Opening the file */
	fd = open(ifile, O_RDONLY);
	if (fd < 0) {
		AIOPT_DEBUG("Failed to open Image file (%s).\n", ifile);
		return AIOPT_FAILURE;
	}

	*file_sz = filesize;

	AIOPT_DEBUG("AIOP Image File opened Successfully (fd=%d).\n", fd);
	AIOPT_DEV("Exiting.\n");

	return fd;
}

/*
 * @brief
 * For a given AIOP Args file, verify existence, type and return FD after
 * opening it.
 *
 * @param [in]  afile AIOP args file name with path
 * @param [out] file_sz size of ELF file, if valid
 *
 * @return Value greater than 0 if file opened successfully or AIOPT_FAILURE
 */
static int
get_aiop_args_fd(const char *afile, size_t *file_sz)
{
	int fd = -1;
	size_t filesize = 0;
	struct stat im_stat;

	AIOPT_DEV("Entering.\n");
	memset(&im_stat, 0, sizeof(im_stat));
	*file_sz = 0;

	/* Checking Validity of the file */
	if (!afile || (stat(afile, &im_stat) != 0)) {
		AIOPT_DEBUG("Unable to stat the file (%s).\n", afile);
		return AIOPT_FAILURE;
	}

	/* get the file size */
	filesize = im_stat.st_size;
	if (filesize <= 0 || filesize > MAX_AIOP_ARGS_FILE_SZ) {
		AIOPT_DEBUG("Incorrect file size. Give (%lu), Max Allowed (%d)"
				".\n", filesize, MAX_AIOP_ARGS_FILE_SZ);
		return AIOPT_FAILURE;
	}

	/* Opening the file */
	fd = open(afile, O_RDONLY);
	if (fd < 0) {
		AIOPT_DEBUG("Failed to open AIOP arguments file (%s).\n", afile);
		return AIOPT_FAILURE;
	}

	*file_sz = filesize;

	AIOPT_DEBUG("AIOP Args File opened Successfully (fd=%d).\n", fd);
	AIOPT_DEV("Exiting.\n");

	return fd;
}
/*
 * @brief
 * Internal operation interfacing with flib/mc APIs for dpaiop_load/dpaiop_run
 * This operation should _not_ be called directly - it is wrapped around by
 * aiopt_load()
 *
 * @param [in] obj Pointer to valid aiopt_obj_t object
 * @param [in] addr dma-mapped address on which dpaiop_load is to be issued
 * @param [in] filesize Size of data in addr
 * @param [in] reset flag to state if dpaiop_reset() has to be called before
 *             dpaiop_load is called
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE if loading fails.
 */
static int
perform_dpaiop_load(aiopt_obj_t *obj, void *addr, size_t filesize,
			void *args_addr, size_t args_filesize,
			short int reset, unsigned short int tpc)
{
	int ret, result;
	unsigned short int *dpaiop_token;

	struct fsl_mc_io *dpaiop;
	struct dpaiop_load_cfg load_cfg = {0};
	struct dpaiop_run_cfg run_cfg = {0};

	AIOPT_DEV("Entering.\n");

	/* We have the device in dev, extracting mc_portal from it */
	dpaiop = (struct fsl_mc_io *)calloc(1, sizeof(struct fsl_mc_io));
	if (!dpaiop) {
		AIOPT_DEBUG("Unable to allocate internal memory.\n");
		return AIOPT_FAILURE;
	}

	dpaiop->regs = obj->mcp_addr;
	dpaiop_token = aiopt_get_aiop_token_byref(obj);

	/* Opening the AIOP device */
	ret = dpaiop_open(dpaiop, CMD_PRI_LOW, aiopt_get_aiop_id(obj),
				dpaiop_token);
	if (ret != 0) {
		AIOPT_DEBUG("Unable to open dpaiop (MC API err=%d).\n", ret);
		free(dpaiop);
		return AIOPT_FAILURE;
	}
	AIOPT_DEBUG("Opened AIOP device. (Token=%d)\n", *dpaiop_token);

	/* Now that the device dpaiop is open, load the image on it */
	load_cfg.img_iova = (uint64_t)addr;
	load_cfg.img_size = filesize;
	load_cfg.options = 0;
	load_cfg.tpc = tpc;

	if (reset) {
		/* Performing Reset before load */
		AIOPT_DEV("Calling dpaiop_reset before dpaiop_load.\n");
		/* TODO Warning to users that dpaiop_run is only for rev2 */
		ret = dpaiop_reset(dpaiop, 0, *dpaiop_token);
		if (ret) {
			AIOPT_DEBUG("Unable to perform reset of AIOP tile."
				"(err=%d).\n", ret);
			/* Error is ignored */
			AIOPT_LIB_INFO("AIOP Reset may not be supported on some"
					" hardware. Please check.\n");
			/* cleanup and return AIOPT_FAILURE */
		} else {
			AIOPT_LIB_INFO("AIOP Tile Reset done. (err=%d)\n", ret);
		}
	}
	AIOPT_DEBUG("dpaiop_load call: iova=%p, size=%u\n",
			(void *)load_cfg.img_iova, load_cfg.img_size);

	/* MC API for performing AIOP Load */
	ret = dpaiop_load(dpaiop, 0, *dpaiop_token, &load_cfg);
	if (ret) {
		/* dpaiop load failed */
		AIOPT_DEBUG("MC API dpaiop_load failed. (err=%d)\n", ret);
	} else {
		AIOPT_LIB_INFO("MC API dpaiop_load successful. (err=%d)\n",
				ret);

		/* Preparing arguments for run */
		run_cfg.cores_mask = AIOPT_RUN_CORES_ALL;
		run_cfg.options = 0;
		run_cfg.args_iova = (uint64_t)args_addr;
		run_cfg.args_size = args_filesize;
	
		/* Calling dpaiop_run */
		ret = dpaiop_run(dpaiop, 0, *dpaiop_token, &run_cfg);
		if (ret != 0) {
			AIOPT_DEBUG("MC API dpaiop_run failed. (err=%d)\n",
					ret);
		} else {
			AIOPT_LIB_INFO("MC API dpaiop_run result: (%d).\n",
					ret);
		}
		/* Irrespective of error or success, we have to cleanup */
	}

	/* Closing the dpaiop_device, after releasing memory allocated for
	 * elf
	 */
	result = dpaiop_close(dpaiop, 0, *dpaiop_token);
	AIOPT_DEBUG("MC API dpaiop_close performed. (err=%d)\n", result);
	if (result != 0) {
		AIOPT_DEBUG("MC API dpaiop_close unsuccessful. (err=%d)\n",
				result);
	}
	/* Even if result from dpaiop_close was error, still returning positive
	 * to caller.
	 */

	/* Releasing memory for dpaiop object */
	if (dpaiop)
		free(dpaiop);

	/* Following doesn't return FAILURE if close failed. */
	if (ret != 0)
		return AIOPT_FAILURE;

	return AIOPT_SUCCESS;
}

/* ==========================================================================
 * Externally available API definitions
 * ==========================================================================*/

/*
 * @brief
 * Convert MC State to String
 *
 * @param [in] state State as returned by dpaiop_get_state
 * @return const string explaining state
 */
const char *
aiopt_get_state_str(int state)
{
	char *p = NULL;

	switch (state) {
	case DPAIOP_STATE_RESET_DONE:
		p = "DPAIOP_STATE_RESET_DONE";
		break;
	case DPAIOP_STATE_RESET_ONGOING:
		p = "DPAIOP_STATE_RESET_ONGOING";
		break;
	case DPAIOP_STATE_LOAD_DONE:
		p = "DPAIOP_STATE_LOAD_DONE";
		break;
	case DPAIOP_STATE_LOAD_ONGIONG:
		p = "DPAIOP_STATE_LOAD_ONGIONG";
		break;
	case DPAIOP_STATE_LOAD_ERROR:
		p = "DPAIOP_STATE_LOAD_ERROR";
		break;
	case DPAIOP_STATE_BOOT_ONGOING:
		p = "DPAIOP_STATE_BOOT_ONGOING";
		break;
	case DPAIOP_STATE_BOOT_ERROR:
		p = "DPAIOP_STATE_BOOT_ERROR";
		break;
	case DPAIOP_STATE_RUNNING:
		p = "DPAIOP_STATE_RUNNING";
		break;
	default:
		p = "Invalid MC State";
		break;
	}

	return (const char *)p;
}

/*
 * @brief
 * AIOPT Get Time of Day
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [out] tod Time of day as a unsigned long value
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int
aiopt_gettod(aiopt_handle_t handle, uint64_t *tod)
{
	int ret, result;
	aiopt_obj_t *obj = NULL;
	unsigned short int *dpaiop_token;

	struct fsl_mc_io *dpaiop = NULL;

	AIOPT_DEV("Entering.\n");

	if (!tod) {
		AIOPT_DEV("Incorrect API Usage. (arg==NULL).\n");
		return AIOPT_FAILURE;
	}

	obj = (aiopt_obj_t *)handle;

	/* We have the device in dev, extracting mc_portal from it */
	dpaiop = (struct fsl_mc_io *)calloc(1, sizeof(struct fsl_mc_io));
	if (!dpaiop) {
		AIOPT_DEBUG("Unable to allocate internal memory.\n");
		return AIOPT_FAILURE;
	}

	dpaiop->regs = obj->mcp_addr;
	dpaiop_token = aiopt_get_aiop_token_byref(obj);

	ret = dpaiop_open(dpaiop, CMD_PRI_LOW, aiopt_get_aiop_id(obj),
				dpaiop_token);
	if (ret != 0) {
		AIOPT_DEBUG("Unable to open dpaiop (MC API err=%d).\n", ret);
		free(dpaiop);
		return AIOPT_FAILURE;
	}
	AIOPT_DEBUG("Opened AIOP device. (Token=%d)\n", *dpaiop_token);

	ret = dpaiop_get_time_of_day(dpaiop, 0, *dpaiop_token, tod);
	if (ret) {
		AIOPT_DEBUG("Unable to fetch Time of Day. "
				"(err=%d)\n", ret);
	} else {
		AIOPT_LIB_INFO("Time of day from MC API:- (%lu)\n", *tod);
	}

	/* Closing the dpaiop_device */
	result = dpaiop_close(dpaiop, 0, *dpaiop_token);
	AIOPT_DEBUG("MC API dpaiop_close performed. (err=%d)\n", ret);
	if (ret != 0) {
		AIOPT_DEBUG("MC API dpaiop_close unsuccessful. (err=%d)\n",
				ret);
		/* In case of failure to close, error is returned, else error
		 * as reported by previous calls is returned.
		 */
		ret = result;
	}

	/* Releasing memory for dpaiop object */
	if (dpaiop)
		free(dpaiop);

	if (ret != 0)
		return AIOPT_FAILURE;

	return AIOPT_SUCCESS;

	
}

/*
 * @brief
 * AIOPT Set Time of Day
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [in] tod Time of day to set as an unsigned long value
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int
aiopt_settod(aiopt_handle_t handle, uint64_t tod)
{
	int ret, result;
	aiopt_obj_t *obj = NULL;
	unsigned short int *dpaiop_token;

	struct fsl_mc_io *dpaiop = NULL;

	AIOPT_DEV("Entering.\n");

	obj = (aiopt_obj_t *)handle;

	/* We have the device in dev, extracting mc_portal from it */
	dpaiop = (struct fsl_mc_io *)calloc(1, sizeof(struct fsl_mc_io));
	if (!dpaiop) {
		AIOPT_DEBUG("Unable to allocate internal memory.\n");
		return AIOPT_FAILURE;
	}

	dpaiop->regs = obj->mcp_addr;
	dpaiop_token = aiopt_get_aiop_token_byref(obj);

	ret = dpaiop_open(dpaiop, CMD_PRI_LOW, aiopt_get_aiop_id(obj),
				dpaiop_token);
	if (ret != 0) {
		AIOPT_DEBUG("Unable to open dpaiop (MC API err=%d).\n", ret);
		free(dpaiop);
		return AIOPT_FAILURE;
	}
	AIOPT_LIB_INFO("Opened AIOP device. (Token=%d)\n", *dpaiop_token);
	AIOPT_DEV("Attempting to set Time of day to %lu.\n", tod);

	ret = dpaiop_set_time_of_day(dpaiop, 0, *dpaiop_token, tod);
	if (ret) {
		AIOPT_DEBUG("Unable to set Time of Day. "
				"(err=%d)\n", ret);
	} else {
		AIOPT_LIB_INFO("Setting time of day successful.\n");
	}

	/* Closing the dpaiop_device */
	result = dpaiop_close(dpaiop, 0, *dpaiop_token);
	AIOPT_DEV("MC API dpaiop_close performed. (err=%d)\n", ret);
	if (ret != 0) {
		AIOPT_DEBUG("MC API dpaiop_close unsuccessful. (err=%d)\n",
				ret);
		/* In case of failure to close, error is returned, else error
		 * as reported by previous calls is returned.
		 */
		ret = result;
	}

	/* Releasing memory for dpaiop object */
	if (dpaiop)
		free(dpaiop);

	if (ret != 0)
		return AIOPT_FAILURE;

	return AIOPT_SUCCESS;

	
}

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
int
aiopt_status(aiopt_handle_t handle, aiopt_status_t *s)
{
	int ret, result;
	unsigned int tile_state;
	aiopt_obj_t *obj = NULL;
	unsigned short int *dpaiop_token;
	struct dpaiop_sl_version dpaiop_slv = {0};

	struct fsl_mc_io *dpaiop = NULL;

	AIOPT_DEV("Entering.\n");

	if (!s) {
		AIOPT_DEBUG("Incorrect API Usage. (arg==NULL).\n");
		return AIOPT_FAILURE;
	}

	obj = (aiopt_obj_t *)handle;

	/* We have the device in dev, extracting mc_portal from it */
	dpaiop = (struct fsl_mc_io *)calloc(1, sizeof(struct fsl_mc_io));
	if (!dpaiop) {
		AIOPT_DEBUG("Unable to allocate internal memory.\n");
		return AIOPT_FAILURE;
	}

	dpaiop->regs = obj->mcp_addr;
	dpaiop_token = aiopt_get_aiop_token_byref(obj);

	ret = dpaiop_open(dpaiop, CMD_PRI_LOW, aiopt_get_aiop_id(obj),
				dpaiop_token);
	if (ret != 0) {
		AIOPT_DEBUG("Unable to open dpaiop (MC API err=%d).\n", ret);
		free(dpaiop);
		return AIOPT_FAILURE;
	}
	AIOPT_LIB_INFO("Opened AIOP device. (Token=%d)\n", *dpaiop_token);

	/* Getting the Service Layer Version information */
	ret = dpaiop_get_sl_version(dpaiop, 0, *dpaiop_token, &dpaiop_slv);
	if (ret) {
		AIOPT_DEBUG("Unable to fetch Service Layer Version. "
				"(err=%d)\n", ret);
		goto close_aiop;
	} else {
		AIOPT_DEBUG("AIOP SL Attributes: major=%d, minor=%d, rev=%d\n",
				dpaiop_slv.major, dpaiop_slv.minor,
				dpaiop_slv.revision);
		s->sl_major_v = dpaiop_slv.major;
		s->sl_minor_v = dpaiop_slv.minor;
		s->sl_revision = dpaiop_slv.revision; 
	}

	/* State of the AIOP Tile; Can be converted to string using the
	 * aiopt_get_state_str
	 */
	ret = dpaiop_get_state(dpaiop, 0, *dpaiop_token, &tile_state);
	if (ret) {
		AIOPT_DEBUG("Unable to fetch AIOP Tile state. (err=%d).\n",
				ret);
		goto close_aiop;
	} else {
		AIOPT_DEBUG("Obtained tile_state = %d\n", tile_state);
		s->state = tile_state;
	}

	AIOPT_LIB_INFO("State and Status information successfully obtained.\n");

close_aiop:
	/* Closing the dpaiop_device */
	result = dpaiop_close(dpaiop, 0, *dpaiop_token);
	AIOPT_DEBUG("MC API dpaiop_close performed. (err=%d)\n", ret);
	if (ret != 0) {
		AIOPT_DEBUG("MC API dpaiop_close unsuccessful. (err=%d)\n",
				ret);
		/* In case of failure to close, error is returned, else error
		 * as reported by previous calls is returned.
		 */
		ret = result;
	}

	/* Releasing memory for dpaiop object */
	if (dpaiop)
		free(dpaiop);

	if (ret != 0)
		return AIOPT_FAILURE;

	return AIOPT_SUCCESS;
}

/*
 * @brief
 * AIOPT Reset call for performing Reset of a AIOP Tile
 * XXX This is not supported on rev1 hardware
 *
 * @param [in] handle aiopt_handle_t type valid object
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int
aiopt_reset(aiopt_handle_t handle)
{
	int ret, result;
	aiopt_obj_t *obj = NULL;
	unsigned short int *dpaiop_token;

	struct fsl_mc_io *dpaiop = NULL;

	AIOPT_DEV("Entering.\n");

	obj = (aiopt_obj_t *)handle;

	/* We have the device in dev, extracting mc_portal from it */
	dpaiop = (struct fsl_mc_io *)calloc(1, sizeof(struct fsl_mc_io));
	if (!dpaiop) {
		AIOPT_DEBUG("Unable to allocate internal memory.\n");
		return AIOPT_FAILURE;
	}

	dpaiop->regs = obj->mcp_addr;
	dpaiop_token = aiopt_get_aiop_token_byref(obj);

	ret = dpaiop_open(dpaiop, CMD_PRI_LOW, aiopt_get_aiop_id(obj),
				dpaiop_token);
	if (ret != 0) {
		AIOPT_DEBUG("Unable to open dpaiop (MC API err=%d).\n", ret);
		free(dpaiop);
		return AIOPT_FAILURE;
	}
	AIOPT_DEBUG("Opened AIOP device. (Token=%d)\n", *dpaiop_token);

	ret = dpaiop_reset(dpaiop, 0, *dpaiop_token);
	if (ret) {
		AIOPT_DEBUG("Unable to reset the AIOP tile. (err=%d)\n", ret);
	} else {
		AIOPT_LIB_INFO("AIOP Tile Reset successful.\n");
	}

	/* Closing the dpaiop_device */
	result = dpaiop_close(dpaiop, 0, *dpaiop_token);
	AIOPT_DEBUG("MC API dpaiop_close performed. (err=%d)\n", ret);
	if (result != 0) {
		AIOPT_DEBUG("MC API dpaiop_close unsuccessful. (err=%d)\n",
				result);
	}

	/* Releasing memory for dpaiop object */
	if (dpaiop)
		free(dpaiop);

	/* Following doesn't return FAILURE if close fails */
	if (ret != 0)
		return AIOPT_FAILURE;

	return AIOPT_SUCCESS;
}

/*
 * @brief
 * AIOPT load call for loading an AIOP Image on a dpaiop object belonging to
 * provided (or default) container
 *
 * @param [in] handle aiopt_handle_t type valid object
 * @param [in] ifile AIOP Image file name, with path
 * @param [in] afile AIOP Commandline arguments file name, with path
 * @param [in] reset Flag to state if reset is to be done before load operation
 * @param [in] tpc for threads per AIOP core
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int
aiopt_load(aiopt_handle_t handle, const char *ifile,
	   const char *afile, short int reset,
	   unsigned short int tpc)
{
	int ret, fd;
	size_t filesize = 0;
	size_t aligned_size = 0;
	void *addr = NULL;
	aiopt_obj_t *obj = NULL;
	int args_fd;
	size_t args_filesize = 0;
	size_t args_aligned_size = 0;
	void *args_addr = NULL;

	/* Get the FD of the AIOP Image file after opening it. Failure to open
	 * is an error.
	 */
	fd = get_aiop_image_fd(ifile, &filesize);
	if (fd <= 0 ) { /* Including AIOPT_FAILURE */
		AIOPT_DEBUG("Unable to open AIOP Image File.\n");
		ret = AIOPT_FAILURE;
		goto err_out;
	}
	AIOPT_LIB_INFO("AIOP Image file opened: (fd=%d).\n", fd);

	if (afile) {
		args_fd = get_aiop_args_fd(afile, &args_filesize);
		if (args_fd <= 0 ) { /* Including AIOPT_FAILURE */
			AIOPT_DEBUG("Unable to open AIOP Arguments File.\n");
			ret = AIOPT_FAILURE;
			goto err_out;
		}
		AIOPT_LIB_INFO("AIOP Arguments file opened: (fd=%d).\n", args_fd);
	}

	/* Allocating memory in virtual space and dma-mapping it for loading
	 * AIOP Image on it
	 */
	if (filesize % AIOPT_ALIGNED_PAGE_SZ)
		aligned_size = ((filesize + AIOPT_ALIGNED_PAGE_SZ)/
				AIOPT_ALIGNED_PAGE_SZ) * AIOPT_ALIGNED_PAGE_SZ;
	else
		aligned_size = filesize;

	addr = mmap(NULL, aligned_size, PROT_READ|PROT_WRITE,
			MAP_PRIVATE|MAP_POPULATE, fd, 0);
	if (addr == MAP_FAILED) {
		AIOPT_DEBUG("Unable to mmap internal memory. (err=%d)\n",
				errno);
		ret = AIOPT_ENOMEM;
		goto err_out;
	}

	AIOPT_DEV("mmap-ing (%ld) bytes of aligned buffer for AIOP "
			"image. (addr=%p)\n", aligned_size, addr);

	if (afile) {
		/* Allocating memory in virtual space and dma-mapping it for loading
		 * AIOP arguments on it
		 */
		args_aligned_size =
			((args_filesize + AIOPT_ALIGNED_PAGE_SZ)/AIOPT_ALIGNED_PAGE_SZ) \
			* AIOPT_ALIGNED_PAGE_SZ;
		args_addr = mmap(NULL, args_aligned_size, PROT_READ|PROT_WRITE,
				MAP_PRIVATE|MAP_POPULATE, args_fd, 0);
		if (args_addr == MAP_FAILED) {
			AIOPT_DEBUG("Unable to mmap internal memory. (err=%d)\n",
					errno);
			ret = AIOPT_ENOMEM;
			goto err_out;
		}
	
		AIOPT_DEV("mmap-ing (%ld) bytes of aligned buffer for AIOP "
			"arguments. (addr=%p)\n", args_aligned_size, args_addr);
	}

	/* After memory allocation, dma-mapping the memory through VFIO API */
	obj = (aiopt_obj_t *)handle;
	ret = fsl_vfio_setup_dmamap(obj->vfio_handle, (uint64_t)addr,
					aligned_size);
	if (ret != VFIO_SUCCESS) {
		AIOPT_DEBUG("Unable to perform DMA Mapping. (err=%d)\n", ret);
		goto err_out;
	}
	AIOPT_LIB_INFO("DMA Map of allocated memory (%p) successful.\n", addr);

	if (afile) {
		/* After memory allocation for args, dma-mapping the memory through VFIO API */
		obj = (aiopt_obj_t *)handle;
		ret = fsl_vfio_setup_dmamap(obj->vfio_handle, (uint64_t)args_addr,
						args_aligned_size);
		if (ret != VFIO_SUCCESS) {
			AIOPT_DEBUG("Unable to perform DMA Mapping for args. (err=%d)\n", ret);
			goto err_cleanup_elf;
		}
		AIOPT_LIB_INFO("DMA Map of allocated memory (%p) successful for args.\n", args_addr);
	}

	ret = perform_dpaiop_load(obj, addr, filesize, args_addr,
				  args_filesize, reset, tpc);
	if (ret != AIOPT_SUCCESS) {
		AIOPT_DEBUG("Error in performing aiop load.\n");
		/* Fall through to err_cleanup */
		goto err_cleanup_args;
	}

err_cleanup_args:
	if (afile && args_addr && args_aligned_size > 0)
		fsl_vfio_destroy_dmamap(obj->vfio_handle,
					(uint64_t)args_addr,
					args_aligned_size);

err_cleanup_elf:
	if (addr && aligned_size > 0)
		fsl_vfio_destroy_dmamap(obj->vfio_handle,
					(uint64_t)addr,
					aligned_size);

err_out:
	if (args_addr)
		munmap(args_addr, args_filesize);
	if (args_fd > 0)
		close(args_fd);
	if (addr)
		munmap(addr, filesize);
	if (fd > 0)
		close(fd);
	return ret;

}


/*
 * @brief
 * Deinitialize the AIOP Object
 *
 * @param [in] obj aiopt_handle_t type valid object
 *
 * @return AIOPT_SUCCESS or AIOPT_FAILURE
 */
int
aiopt_deinit(aiopt_handle_t obj)
{
	int ret = AIOPT_SUCCESS;

	AIOPT_DEV("Entering.\n");
	if (obj) {
		cleanup_aiopt_obj((aiopt_obj_t *)obj);
	}

	AIOPT_DEV("Exiting (%d)\n", ret);
	return ret;
}

/*
 * @brief:
 * Initialize the AIOP Library instance. Create and return an aiopt_handle type
 * object to caller. For all subsequent operations, this object is required.
 *
 * @param [IN] container_name Name of the container with dpaiop
 *
 * @return aiopt_handle_t type object or NULL in case of failure
 */
aiopt_handle_t
aiopt_init(const char *container_name)
{
	int ret;
	aiopt_obj_t *obj = NULL;

	obj = calloc(1, sizeof(aiopt_obj_t));
	if (!obj) {
		AIOPT_DEBUG("Unable to allocate memory for AIOP Obj\n");
		return AIOPT_INVALID_HANDLE;
	}

	/* Initializing handle on the VFIO context for the container */
	obj->vfio_handle = fsl_vfio_setup(container_name);
	if (FSL_VFIO_INVALID_HANDLE == obj->vfio_handle) {
		AIOPT_DEBUG("Unable to open VFIO. (Invalid handle).\n");
		free(obj);
		return AIOPT_INVALID_HANDLE;
	}

	/* Fetch Devices: AIOP and MC; And if these are not present, return
	 * error
	 */
	ret = setup_aiopt_device(obj);
	if (ret != AIOPT_SUCCESS) {
		/* Unable to initialize */
		AIOPT_DEBUG("Initialization of AIOP failed.\n");
		free(obj);
		return AIOPT_INVALID_HANDLE;
	}

	return (aiopt_handle_t)obj;
}

