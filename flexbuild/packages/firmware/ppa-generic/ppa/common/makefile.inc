
SHELL := /bin/bash
#-----------------------------------------------------------------------------
# 
# Copyright (C) 2015-2017 Freescale Semiconductor, Inc. All rights reserved.
# Copyright 2017-2018 NXP Semiconductors
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Author Rod Dorris <rod.dorris@nxp.com>
# 
#-----------------------------------------------------------------------------
#
# Makefile - build AArch64 PPA
#
#-----------------------------------------------------------------------------

# select the debug/release build -
ifeq ($(dbg), on)
    DBG_BLD=1
    DBG_FLAGS=-Wl,-g -Wa,-g
else
    dbg=off
    DBG_BLD=0
    DBG_FLAGS=
endif

# select if a dead-loop should be inserted at the start,
#  to allow a debugger to attach before proceeding
ifeq ($(halt), on)
    DBG_HALT=1
else
    DBG_HALT=0
endif

# select the location of the stacks and data region
ifeq ($(data_in_ddr), on)
    DATA_DDR=1
else
    DATA_DDR=0
endif

# select if uart driver is built
ifeq ($(uart), on)
    UART_BLD=1
else
    UART_BLD=0
endif

# select if ddr/i2c driver(s) are built
ifeq ($(ddr), on)
    DDR_BLD=1
    I2C_BLD=1
    TIMER_BLD=1
ifeq ($(sd), on)
    SD_BLD=1
    ifeq  ($(test), sd)
	TEST_SD=1
    endif
else
    SD_BLD=0
endif
else
    ddr=off
    sd = off
    DDR_BLD=0
    I2C_BLD=0
    TIMER_BLD=0
    SD_BLD=0
endif

# -----------------------------------------------------------------------------

# include the platform specifics
ifneq ($(PLAT_PATH),)
    include $(PLAT_PATH)/plat.mk
endif

# -----------------------------------------------------------------------------

MONITOR_SRC = $(PRE_PATH)../monitor
ARMV8_SRC   = $(PRE_PATH)../armv8
PSCI_SRC    = $(PRE_PATH)../psci
CMN_SRC     = $(PRE_PATH)../common
CMM_SRC     = $(PRE_PATH)../cmm
SPD_SRC     = $(PRE_PATH)../spd
DRVR_SRC    = $(PRE_PATH)../drivers
SOC_SRC     = $(PRE_PATH).
TEST_SRC    = $(PRE_PATH)../test
TOOL_SRC    = $(PRE_PATH)../tools
SRC_DIR     = build/src
OBJ_DIR     = build/obj

BMAP   = -Map $(OBJ_DIR)/bootmain.64.map
BENTRY = --entry=0 -Ttext=0x0
MMAP   = -Map $(OBJ_DIR)/monitor.map

ifneq ($(TEXTBASE),)
MSCRIPT += -Ttext $(TEXTBASE)
else
TEXTBASE = 0
endif

MSCRIPT += --script $(OBJ_DIR)/ppa.ld -pie -Bsymbolic

CROSS_COMPILE	?= $(ARMV8_TOOLS_DIR)/$(ARMV8_TOOLS_PREFIX)

CC64		= $(CROSS_COMPILE)gcc
LD64		= $(CROSS_COMPILE)ld
OBJ64		= $(CROSS_COMPILE)objcopy
MKIMAGE		= mkimage
NM		    = $(CROSS_COMPILE)nm

MAKE        = @make --no-print-directory

GENITS  = $(CMN_SRC)/gen_its

GIT_VERSION    := $(shell git describe --dirty --always 2>/dev/null)
CFLAGS		    = -fno-builtin -ggdb -mstrict-align -DVERSION=\"$(GIT_VERSION)\"
CFLAGS64		= $(CFLAGS) -Wa,-march=armv8-a -Wa,-EL -c -fno-stack-protector
LDFLAGS64		= --oformat=elf64-littleaarch64
ASFLAGS64		= -Wa,-march=armv8-a -Wa,-EL -x assembler-with-cpp -c
OPTFLAGS		= -dumpmachine

# -----------------------------------------------------------------------------

PLATFLAGS = $(DBG_FLAGS)                   \
	-DDEBUG_BUILD=$(DBG_BLD)               \
	-DDEBUG_HALT=$(DBG_HALT)               \
    -DDDR_INIT=$(DDR_BLD)                  \
	-DSIMULATOR_BUILD=$(SIM_BUILD)         \
	-DPSCI_TEST=$(TEST_PSCI)               \
	-DCPU_MAX_COUNT=$(NUMBER_OF_CORES)     \
	-DCLUSTER_COUNT=$(NUMBER_OF_CLUSTERS)  \
	-DCPU_PER_CLUSTER=$(CORES_PER_CLUSTER) \
	-DSYMMETRICAL_CLUSTERS=$(CLUSTER_SYMMETRY) \
    -DCORE=$(CORE_TYPE_A)             \
	-DL3_CACHE=$(SYSTEM_L3)           \
	-DCNFG_DDR=$(DDR_BLD)             \
	-DCNFG_TIMER=$(TIMER_BLD)             \
	-DCNFG_I2C=$(I2C_BLD)             \
	-DCNFG_UART=$(UART_BLD)           \
    -DCNFG_SD=$(SD_BLD) 		      \
    -DCNFG_SD_TEST=$(TEST_SD)         \
	-DDATA_LOC=$(DATA_DDR)            \
    -DLSCH=$(CHASSIS)

# -----------------------------------------------------------------------------

# set the path to the ddr-specific sources
ifeq ($(PLAT_PATH), platform-sim)
    ifeq ($(plat), rdb)
        DDR_PATH=platform-rdb
    else
        ifeq ($(plat), qds)
            DDR_PATH=platform-qds
        else
            ifeq ($(DDR), on)
                DDR_PATH=
                $(error -> simulated ddr platform (PLAT) not specified!)
            else
                DDR_PATH=
            endif
        endif
    endif
else
    DDR_PATH=$(PLAT_PATH)
endif

# -----------------------------------------------------------------------------

 # select the chassis file ---
ifeq ($(CHASSIS), 3)
    CHASSIS_FILE=lsch3
else
ifeq ($(CHASSIS), 2)
    CHASSIS_FILE=lsch2
else
    $(error -> Chassis type not set!)
endif
endif

# -----------------------------------------------------------------------------

LINK_ASM   =ppa.ld.s
COMMON_ASM =utils.s runtime_data.s
COMMON_C   =ppa_main.c lib.c alloc.c
COMMON_HDRS=common.h types.h errno.h io.h  lib.h i2c.h uart.h runtime_data.h\
			$(CHASSIS_FILE).h lnkr.cmd

TEST_C =$(TEST_FILE_C)
# -----------------------------------------------------------------------------

ifeq ($(DDR_BLD), 1)
  ifeq ($(DDRCNTLR), NXP)
    DDR_CNTLR_PATH=$(DRVR_SRC)/ddr/nxp-ddr

    ifeq ($(DDRPHY), NXP)
      DDR_PHY_PATH=$(DRVR_SRC)/ddr/nxp-phy
      include $(DDR_PHY_PATH)/ddrphy.mk
    else
      ifeq ($(DDRPHY), SNPS)
        DDR_PHY_PATH=$(DRVR_SRC)/ddr/snps-phy
        include $(DDR_PHY_PATH)/ddrphy.mk
      else
        $(error -> DDR PHY unknown or not specified!)
      endif
    endif
  else
    ifeq ($(DDRCNTLR), MMDC)
      DDR_CNTLR_PATH=$(DRVR_SRC)/ddr/fsl-mmdc
    else
      $(error -> DDR CNTLR unknown or not specified!)
    endif
  endif

I2C_PATH=$(DRVR_SRC)/i2c
TIMER_PATH=$(DRVR_SRC)/timer

COMMON_C += crc32.c

 # get the driver makefiles
include $(DDR_CNTLR_PATH)/ddrcntlr.mk
include $(I2C_PATH)/i2c.mk
include $(TIMER_PATH)/timer.mk

$(DDR_C): $(DDR_H) $(HDRS_SOC) src
	@cp -r "$(DDR_PATH)/$@" "$(SRC_DIR)/$@"

$(DDR_H): src
	@cp -r "$(DDR_PATH)/$@" "$(SRC_DIR)/$@"

$(DIMM_C): $(DDR_H) $(HDRS_SOC) src
	@cp -r "$(DDR_CNTLR_PATH)/$@" "$(SRC_DIR)/$@"

DRIVER_C = $(DDR_CNTLR_C) $(DDR_PHY_C) $(I2C_C) $(TIMER_C) $(DDR_C) $(DIMM_C)
DRIVER_H = $(DDR_CNTLR_H) $(DDR_PHY_H) $(I2C_H) $(UART_H) $(TIMER_H) $(DDR_H)

ifeq ($(SD_BLD), 1)

SD_PATH=$(DRVR_SRC)/sd
include $(SD_PATH)/sd.mk

DRIVER_C += $(SD_C)
DRIVER_H += $(SD_H)
endif

endif

ifeq ($(UART_BLD), 1)
  UART_PATH=$(DRVR_SRC)/uart
  include $(UART_PATH)/uart.mk

  DRIVER_C += $(UART_C)
  DRIVER_H += $(UART_H)
endif

PRNG_PATH=$(DRVR_SRC)/hash_drbg
SEC_PATH=$(DRVR_SRC)/fsl_sec
TZC_PATH=$(DRVR_SRC)/tzc
SFP_PATH=$(DRVR_SRC)/sfp
PSCI_PATH=$(PSCI_SRC)
MNTR_PATH=$(MONITOR_SRC)
ARMV8_PATH=$(ARMV8_SRC)
SPD_PATH=$(SPD_SRC)
CMM_PATH=$(CMM_SRC)

# get the subsystem makefiles
include $(PRNG_PATH)/hash_drbg.mk
include $(SEC_PATH)/sec.mk
include $(TZC_PATH)/tzc.mk
include $(SFP_PATH)/sfp.mk
include $(PSCI_PATH)/psci.mk
include $(MNTR_PATH)/mntr.mk
include $(ARMV8_PATH)/armv8.mk
include $(SPD_PATH)/spd.mk
include $(CMM_PATH)/cmm.mk

# -----------------------------------------------------------------------------

help:	help_out
help_out:
	@echo
	@echo 'make rdb|rdb-fit|sim|generic [options]'
	@echo
	@echo 'options:'
	@echo '  test=smp_boot|hotplug|off_abort|suspend'
	@echo '  dbg'
	@echo

# -----------------------------------------------------------------------------

TARGETS    =monitor.bin bootmain.64.bin
PRE_TARG   =monitor.elf

BUILD_ASM +=$(SRC_SOC) $(SRC_ARMV8) $(SRC_MNTR) $(SRC_PSCI) $(PLAT_ASM) $(COMMON_ASM) \
			$(TEST_ASM)
BUILD_C   +=$(HASH_DRBG_C) $(SEC_C) $(CSRC_SOC) $(SRC_PLAT) $(COMMON_C) $(DRIVER_C) $(DDR_C) $(TEST_C) $(TZC_C)
HDRS_ASM  +=$(HDRS_ARMV8) $(HDRS_PLAT)  $(HDRS_MNTR) $(DRIVER_H) $(DDR_H) $(HDRS_PSCI) \
			$(HASH_DRBG_HDRS) $(SEC_HDRS) $(HDRS_SOC) $(TZC_HDRS)
OBJECTS    =obj $(BUILD_ASM:.s=.o) $(BUILD_C:.c=.o) ppa.ld

# -----------------------------------------------------------------------------

# generic build targets:

#.NOTPARALLEL:

clean:
	@rm -f -r build

cleanout:
	@rm -f build/obj/*.*
	@rm -f build/src/*.*

# -----------------------------------------------------------------------------

# test the relocate-rela tool when GCC is 6.2
ifeq ($(tst-rela), on)
  GCC_VERSION_GE_62:=false
else
  GCC_VERSION_GE_62:=$(shell test `$(CC64) -dumpversion | sed 's/^\([0-9]\)\.\([0-9]\).*/\1\2/'` -ge 62 && echo true || echo false)
endif

# relocate-rela is available in tools folder
ifeq ($(GCC_VERSION_GE_62),false)
DO_STATIC_RELA_RELOC = \
        start=$$($(NM) $(1) | grep __rel_dyn_start | cut -f 1 -d ' '); \
        end=$$($(NM) $(1) | grep __rel_dyn_end | cut -f 1 -d ' '); \
        $(TOOL_SRC)/relocate-rela $(2) $(3) $$start $$end
else
DO_STATIC_RELA_RELOC =
endif

# -----------------------------------------------------------------------------

monitor.bin: monitor.elf ppa.its
monitor.elf: $(BUILD_C) $(BUILD_ASM) $(HDRS_ASM) $(COMMON_HDRS) $(OBJECTS)
bootmain.64.bin: bootmain.64.elf
bootmain.64.elf: $(BUILD_C) $(BUILD_ASM) $(HDRS_ASM) $(COMMON_HDRS) $(OBJECTS)

$(TARGETS):
	$(OBJ64) -v -O binary $(OBJ_DIR)/$< $(OBJ_DIR)/$@
	$(call DO_STATIC_RELA_RELOC,$(OBJ_DIR)/$<,$(OBJ_DIR)/$@,$(TEXTBASE))

$(PRE_TARG): $(HDRS_ASM) $(COMMON_HDRS)
	$(LD64) $(MMAP) $(MSCRIPT) $(LDFLAGS64) $(wildcard $(OBJ_DIR)/*.o) -o $(OBJ_DIR)/$@

ppa.itb: ppa.its monitor.bin
	$(MKIMAGE) -f $(SRC_DIR)/ppa.its $(OBJ_DIR)/$@

bootmain.64.elf: $(HDRS_ASM) $(OBJECTS)
	$(LD64) $(BMAP) $(MSCRIPT) $(LDFLAGS64) $(wildcard $(OBJ_DIR)/*.o) -o $(OBJ_DIR)/$@

ppa.ld: ppa.ld.s lnkr.cmd src obj
	$(CC64) $(ASFLAGS64) $(PLATFLAGS) -P -E $(SRC_DIR)/$< -o $(OBJ_DIR)/$@ 

# -----------------------------------------------------------------------------

.s.o: $(BUILD_ASM) $(HDRS_ASM)
	$(CC64) $(ASFLAGS64) $(PLATFLAGS) $(SRC_DIR)/$< -o $(OBJ_DIR)/$@

.c.o: $(BUILD_C)
	$(CC64) $(CFLAGS64) $(PLATFLAGS) $(SRC_DIR)/$< -o $(OBJ_DIR)/$@

ppa.its: $(GENITS) src
	$(GENITS) $(SRC_DIR)/ppa.its spd=$(spd) fuse=$(fuse)

$(SRC_SOC): $(HDRS_SOC) $(COMMON_HDRS) $(LINK_ASM) src
	cp -r "$(SOC_SRC)/$@" "$(SRC_DIR)/$@"

$(HDRS_SOC): src
	cp -r "$(SOC_SRC)/$@" "$(SRC_DIR)/$@"

$(LINK_ASM): $(HDRS_SOC) $(COMMON_HDRS) src
	cp -r "$(SOC_SRC)/$@" "$(SRC_DIR)/$@"

$(CSRC_SOC): $(CHDRS_SOC) $(HDRS_SOC) src
	cp -r "$(SOC_SRC)/$@" "$(SRC_DIR)/$@"

$(CHDRS_SOC): src
	cp -r "$(SOC_SRC)/$@" "$(SRC_DIR)/$@"

$(SRC_PLAT): $(HDRS_PLAT) src
	cp -r "$(PLAT_PATH)/$@" "$(SRC_DIR)/$@"

$(HDRS_PLAT): src
	cp -r "$(PLAT_PATH)/$@" "$(SRC_DIR)/$@"

$(PLAT_ASM): src
	cp -r "$(PLAT_PATH)/$@" "$(SRC_DIR)/$@"

$(COMMON_ASM): $(HDRS_ARMV8) $(HDRS_PSCI) src
	cp -r "$(CMN_SRC)/$@" "$(SRC_DIR)/$@"

$(COMMON_C): $(COMMON_HDRS) src
	cp -r "$(CMN_SRC)/$@" "$(SRC_DIR)/$@"

$(COMMON_HDRS): src
	cp -r "$(CMN_SRC)/$@" "$(SRC_DIR)/$@"

$(TEST_ASM): $(HDRS_SOC) src
	cp -r "$(TEST_SRC)/$@" "$(SRC_DIR)/$@"

$(TEST_C): $(HDRS_SOC) src
	cp -r "$(TEST_SRC)/$@" "$(SRC_DIR)/$@"

# -----------------------------------------------------------------------------

src: Makefile build
	@mkdir -p $(SRC_DIR)

obj: Makefile build
	@mkdir -p $(OBJ_DIR)

build: Makefile
	@mkdir -p build

# -----------------------------------------------------------------------------

