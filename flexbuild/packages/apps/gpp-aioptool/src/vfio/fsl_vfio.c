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
 * @file	fsl_vfio.c
 *
 * @brief	VFIO Library for VFIO Mapping and DMA
 *
 */

#include <fsl_vfio.h>
#include <aiop_logger.h>


/* ------------------- */
/* TODO: Remove/Replace this with appropriate logging APIs */
/* Some Logging Macros */
#define LOG(...)		printf(__VA_ARGS__)

#define DEBUG_FLAG		0
#define INFO_FLAG		0

#ifdef DEBUG_FLAG
#define DEBUG			AIOPT_DEBUG
#else
#define DEBUG(...)		/*-< Blank Operation >-*/
#endif

#ifdef INFO_FLAG
#define	INFO			AIOPT_INFO
#else
#define INFO(...)		/*-< Blank Operation >-*/
#endif

#define	ERROR			AIOPT_DEBUG
/* ------------------- */

/***** Global Variables ********/
/*
 * Structure Declarations
 */
struct vfio_group {
	int fd; /* /dev/vfio/"groupid" */
	int groupid;
	int used;
	struct vfio_container *container;
};

struct vfio_container {
	int fd; /* /dev/vfio/vfio */
	int used;
	int index; /* index in group list */
	struct vfio_group *group_list[VFIO_MAX_GRP];
};

/***** Global Variables ********/
/* Number of VFIO containers & groups with in */
struct vfio_group vfio_groups[VFIO_MAX_GRP];
struct vfio_container vfio_containers[VFIO_MAX_CONTAINERS];
int container_device_fd;
uint32_t *msi_intr_vaddr;

static int vfio_connect_container(struct vfio_group *vfio_group)
{
	struct vfio_container *container;
	int i, fd, ret;

	/* Try connecting to vfio container already created */
	for (i = 0; i < VFIO_MAX_CONTAINERS; i++) {
		container = &vfio_containers[i];
		if (!ioctl(vfio_group->fd, VFIO_GROUP_SET_CONTAINER,
				&container->fd)) {
			ERROR("Container pre-exists with FD[0x%x]"
					" for this group\n", container->fd);
			vfio_group->container = container;
			return VFIO_SUCCESS;
		}
	}

	/* Opens main vfio file descriptor which represents the "container" */
	fd = open("/dev/vfio/vfio", O_RDWR);
	if (fd < 0) {
		ERROR("vfio: error opening VFIO Container\n");
		return VFIO_FAILURE;
	}
	ret = ioctl(fd, VFIO_GET_API_VERSION);
	if (ret != VFIO_API_VERSION) {
		close(fd);
		return VFIO_FAILURE;
	}
	/* Check whether support for SMMU type IOMMU prresent or not */
	if (ioctl(fd, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) {
		/* Connect group to container */
		if (ioctl(vfio_group->fd, VFIO_GROUP_SET_CONTAINER, &fd)) {
			ERROR("VFIO_GROUP_SET_CONTAINER failed.\n");
			close(fd);
			return VFIO_FAILURE;
		}
		/* Initialize SMMU */
		if (ioctl(fd, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU)) {
			ERROR("VFIO_SET_IOMMU failed.\n");
			close(fd);
			return VFIO_FAILURE;
		}
		DEBUG("VFIO_TYPE1_IOMMU Supported\n");
	} else {
		ERROR("vfio error: No supported IOMMU\n");
		close(fd);
		return VFIO_FAILURE;
	}

	/* Configure the Container private data structure */
	container = NULL;
	for (i = 0; i < VFIO_MAX_CONTAINERS; i++) {
		if (vfio_containers[i].used)
			continue;
		DEBUG("Found unused container at index %d\n", i);
		container = &vfio_containers[i];
	}
	if (!container) {
		ERROR("vfio error: No Free Container Found\n");
		close(fd);
		return VFIO_FAILURE;
	}
	container->used = 1;
	container->fd = fd;
	container->group_list[container->index] = vfio_group;
	DEBUG("Assigning Group to index group_list[%d]\n", container->index);

	vfio_group->container = container;
	container->index++;
	return VFIO_SUCCESS;
}

static void vfio_disconnect_container(struct vfio_group *group)
{
	struct vfio_container *container = group->container;

	if (!container) {
		DEBUG("Invalid container.\n");
		return;
	}

	if (ioctl(group->fd, VFIO_GROUP_UNSET_CONTAINER, &container->fd))
		ERROR("UNSET Container API Failed with ERRNO = %d\n", errno);

	group->container = NULL;

	close(container->fd);
	container->fd = 0; /* In case container reused */
}

/* TODO - The below two API's are provided as a W.A.. as VFIO currently
   does not add the mapping of the interrupt region to SMMU. This should
   be removed once the support is added in the Kernel.
*/
static void vfio_unmap_irq_region(struct vfio_group *group)
{
	int ret;
	struct vfio_iommu_type1_dma_unmap unmap = {
		.argsz = sizeof(unmap),
		.flags = 0,
		.iova = 0x6030000,
		.size = 0x1000,
	};

	/* Attempting unmap only if msi_intr_vaddr has non zero value */
	if (msi_intr_vaddr) {
		ret = ioctl(group->container->fd, VFIO_IOMMU_UNMAP_DMA, &unmap);
		if (ret)
			ERROR("Error in vfio_dma_unmap (errno = %d)", errno);
	}

	msi_intr_vaddr = 0;
}

static int vfio_map_irq_region(struct vfio_group *group)
{
	int ret;
	unsigned long *vaddr = NULL;
	struct vfio_iommu_type1_dma_map map = {
		.argsz = sizeof(map),
		.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE,
		.vaddr = 0x6030000,
		.iova = 0x6030000,
		.size = 0x1000,
	};

	if (msi_intr_vaddr) {
		/* IRQ region already mapped; Preventing multiple calls */
		return VFIO_SUCCESS;
	}

	vaddr = (unsigned long *)mmap(NULL, 0x1000, PROT_WRITE |
		PROT_READ, MAP_SHARED, container_device_fd, 0x6030000);
	if (vaddr == MAP_FAILED) {
		ERROR("Error mapping GITS region (errno = %d)", errno);
		return -errno;
	}

	msi_intr_vaddr = (uint32_t *)((char *)(vaddr) + 64);
	map.vaddr = (unsigned long)vaddr;
	ret = ioctl(group->container->fd, VFIO_IOMMU_MAP_DMA, &map);
	if (ret == 0)
		return VFIO_SUCCESS;

	ERROR("vfio_map_irq_region fails (errno = %d)", errno);
	return -errno;
}

int32_t fsl_vfio_setup_dmamap(fsl_vfio_t handle, uint64_t addr, size_t len)
{
	int ret;
	struct vfio_group *group;
	struct vfio_iommu_type1_dma_map dma_map = {
		.argsz = sizeof(dma_map),
		.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE,
	};

	if (!handle) {
		ERROR("vfio: Incorrect handle passed\n");
		return VFIO_FAILURE;
	}
	group = (struct vfio_group *)handle;

	dma_map.vaddr = addr;
	dma_map.size = len;
	dma_map.iova = dma_map.vaddr;

	/* SET DMA MAP for IOMMU */
	DEBUG("vfio: -- Initial SHM Virtual ADDR %llX\n", dma_map.vaddr);
	DEBUG("vfio: -- DMA size 0x%llX\n", dma_map.size);

	ret = ioctl(group->container->fd, VFIO_IOMMU_MAP_DMA, &dma_map);
	if (ret) {
		ERROR("VFIO_IOMMU_MAP_DMA API Error %d.\n", errno);
		return VFIO_FAILURE;
	}
	DEBUG("vfio: >> dma_map.vaddr = 0x%llX\n", dma_map.vaddr);

	/* TODO - This is a W.A. as VFIO currently does not add the mapping of
	    the interrupt region to SMMU. This should be removed once the
	    support is added in the Kernel.
	 */
	vfio_map_irq_region(group);

	return VFIO_SUCCESS;
}

void fsl_vfio_destroy_dmamap(fsl_vfio_t handle, uint64_t addr, size_t len)
{
	int ret;
	struct vfio_group *group;
	struct vfio_iommu_type1_dma_unmap dma_unmap = {
		.argsz = sizeof(dma_unmap),
		.flags = 0,
	};

	if (!handle) {
		ERROR("vfio: Incorrect handle passed\n");
		return;
	}
	group = (struct vfio_group *)handle;

	dma_unmap.iova = addr;
	dma_unmap.size = len;

	DEBUG("vfio: -- DMA-UNMAP IOVA ADDR %llX\n", dma_unmap.iova);
	DEBUG("vfio: -- DMA-UNMAP size 0x%llX\n", dma_unmap.size);

	ret = ioctl(group->container->fd, VFIO_IOMMU_UNMAP_DMA, &dma_unmap);
	if (ret)
		ERROR("VFIO_IOMMU_UNMAP_DMA API Error %d.\n", errno);

	vfio_unmap_irq_region(group);
}

static int vfio_set_group(struct vfio_group *group, int groupid)
{
	char path[VFIO_PATH_MAX];
	struct vfio_group_status status = { .argsz = sizeof(status) };

	/* Open the VFIO file corresponding to the IOMMU group */
	snprintf(path, sizeof(path), "/dev/vfio/%d", groupid);

	group->fd = open(path, O_RDWR);
	if (group->fd < 0) {
		ERROR("vfio: error opening %s\n", path);
		return VFIO_FAILURE;
	}
	DEBUG("vfio: Open FD[0x%X] for IOMMU group = %s\n",
		group->fd, path);

	/* Test & Verify that group is VIABLE & AVAILABLE */
	if (ioctl(group->fd, VFIO_GROUP_GET_STATUS, &status)) {
		goto fail;
	}
	if (!(status.flags & VFIO_GROUP_FLAGS_VIABLE)) {
		goto fail;
	}
	/* Since Group is VIABLE, Store the groupid */
	group->groupid = groupid;

	/* Now connect this IOMMU group to given container */
	if (vfio_connect_container(group)) {
		goto fail;
	}
	group->used = 1;
	return VFIO_SUCCESS;
fail:
	close(group->fd);
	group->fd = 0;
	return VFIO_FAILURE;
}

static void vfio_put_group(struct vfio_group *group)
{
	vfio_disconnect_container(group);
	if (group->fd) {
		close(group->fd);
		group->fd = 0;
	}
	group->used = 0;
}

fsl_vfio_t fsl_vfio_setup(const char *vfio_container)
{
	char path[VFIO_PATH_MAX];
	char iommu_group_path[VFIO_PATH_MAX], *group_name;
	struct vfio_group *group = NULL;
	struct stat st;
	int groupid;
	int ret, len, i;

	/* Check whether LS-Container exists or not */
	sprintf(path, "/sys/bus/fsl-mc/devices/%s", vfio_container);
	DEBUG("\tcontainer device path = %s\n", path);
	if (stat(path, &st) < 0) {
		ERROR("vfio: LS-container device does not exists\n");
		goto fail;
	}

	/* DPRC container exists. NOw checkout the IOMMU Group */
	strncat(path, "/iommu_group", sizeof(path) - strlen(path) - 1);

	len = readlink(path, iommu_group_path, VFIO_PATH_MAX);
	if (len == -1) {
		ERROR("\tvfio: error no iommu_group for device\n");
		ERROR("\t%s: len = %d, errno = %d\n", path, len, errno);
		goto fail;
	}

	iommu_group_path[len] = 0;
	group_name = basename(iommu_group_path);
	DEBUG("vfio: IOMMU group_name = %s\n", group_name);
	if (sscanf(group_name, "%d", &groupid) != 1) {
		ERROR("vfio: error reading: %s\n", path);
		goto fail;
	}

	DEBUG("vfio: IOMMU group_id = %d\n", groupid);

	group = &vfio_groups[0];

	/* Check if group already exists */
	for (i = 0; i < VFIO_MAX_GRP && vfio_groups[i].used; i++) {
		group = &vfio_groups[i];
		if (group->groupid == groupid) {
			DEBUG("groupid already exists %d\n", groupid);
			return (fsl_vfio_t)group;
		}
	}

	if (i >= VFIO_MAX_GRP) {
		ERROR("vfio: No more unused group space in container\n");
		goto cleanup;
	}

	if (VFIO_SUCCESS != vfio_set_group(group, groupid)) {
		ERROR("group setup failure - %d\n", groupid);
		goto cleanup;
	}

	/* Get Device information */
	ret = ioctl(group->fd, VFIO_GROUP_GET_DEVICE_FD, vfio_container);
	if (ret < 0) {
		ERROR("\tvfio: error getting device %s fd from group %d\n",
			vfio_container, group->groupid);
		goto cleanup;
	}

	/* For use in map_irq_region */
	container_device_fd = ret;
	DEBUG("vfio: Container FD is [0x%X]n", container_device_fd);

	return (fsl_vfio_t)group;

cleanup:
	vfio_put_group(group);
fail:
	return FSL_VFIO_INVALID_HANDLE;
};

int fsl_vfio_destroy(fsl_vfio_t handle)
{
	struct vfio_group *group = NULL;

	if (FSL_VFIO_INVALID_HANDLE == handle) {
		ERROR("vfio: Incorrect handle passed.\n");
		return VFIO_FAILURE;
	}
	group = (struct vfio_group *)handle;

	vfio_put_group(group);

	return VFIO_SUCCESS;
}

int64_t fsl_vfio_map_mcp_obj(fsl_vfio_t handle, char *mcp_obj)
{
	int64_t v_addr = (int64_t)MAP_FAILED;
	int32_t ret, mcp_fd;

	struct vfio_group *group = NULL;
	struct vfio_device_info d_info = { .argsz = sizeof(d_info) };
	struct vfio_region_info reg_info = { .argsz = sizeof(reg_info) };

	DEBUG("\t MCP object = %s\n", mcp_obj);

	if (FSL_VFIO_INVALID_HANDLE == handle) {
		ERROR("vfio: Incorrect handle passed.\n");
		goto failure;
	}

	group = (struct vfio_group *)handle;

	/* getting the mcp object's fd*/
	mcp_fd = ioctl(group->fd, VFIO_GROUP_GET_DEVICE_FD, mcp_obj);
	if (mcp_fd < 0) {
		ERROR("\tvfio: error getting device %s fd from group %d\n",
				mcp_obj, group->fd);
		return v_addr;
	}

	/* getting device info*/
	ret = ioctl(mcp_fd, VFIO_DEVICE_GET_INFO, &d_info);
	if (ret < 0) {
		ERROR("\tvfio: error getting DEVICE_INFO\n");
		goto mcp_failure;
	}

	/* getting device region info*/
	ret = ioctl(mcp_fd, VFIO_DEVICE_GET_REGION_INFO, &reg_info);
	if (ret < 0) {
		ERROR("\tvfio: error getting REGION_INFO\n");
		goto mcp_failure;
	}

	DEBUG("region offset = %llx  , region size = %llx\n",
			reg_info.offset, reg_info.size);

	v_addr = (uint64_t)mmap(NULL, reg_info.size,
		PROT_WRITE | PROT_READ, MAP_SHARED,
		mcp_fd, reg_info.offset);

mcp_failure:
	close(mcp_fd);
failure:
	return v_addr;
}

int
fsl_vfio_get_group_id(fsl_vfio_t handle)
{
	struct vfio_group *group;

	if (FSL_VFIO_INVALID_HANDLE == handle) {
		ERROR("vfio: Incorrect handle passed.\n");
		return VFIO_FAILURE;
	}

	group = (struct vfio_group *)handle;
	return group->groupid;
}

int
fsl_vfio_get_group_fd(fsl_vfio_t handle)
{
	struct vfio_group *group;

	if (FSL_VFIO_INVALID_HANDLE == handle) {
		ERROR("vfio: Incorrect handle passed.\n");
		return VFIO_FAILURE;
	}

	group = (struct vfio_group *)handle;
	return group->fd;
}

int
fsl_vfio_get_dev_fd(fsl_vfio_t handle, char *dev_name)
{
	struct vfio_group *group;
	int dev_fd;

	if (!handle || !dev_name) {
		ERROR("vfio: Incorrect handle or dev_name.\n");
		return VFIO_FAILURE;
	}

	group = (struct vfio_group *)handle;

	dev_fd = ioctl(group->fd, VFIO_GROUP_GET_DEVICE_FD, dev_name);
	if (dev_fd < 0) {
		ERROR("vfio: IOCTL Failure (%d).\n", dev_fd);
		return VFIO_FAILURE;
	}

	return dev_fd;
}

int
fsl_vfio_get_device_info(fsl_vfio_t handle, char *dev_name,
				struct vfio_device_info *dev_info)
{
	int dev_fd;

	dev_fd = fsl_vfio_get_dev_fd(handle, dev_name);
	if (dev_fd <= 0)
		return VFIO_FAILURE;

	if (ioctl(dev_fd, VFIO_DEVICE_GET_INFO, dev_info)) {
		ERROR("vfio: VFIO_DEVICE_GET_INFO IOCTL Failed\n");
		return VFIO_FAILURE;
	}

	return VFIO_SUCCESS;
}
