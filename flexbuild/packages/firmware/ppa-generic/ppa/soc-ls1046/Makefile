#------------------------------------------------------------------------------
# 
# Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
# Copyright 2017 NXP Semiconductors
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
#------------------------------------------------------------------------------
#
# Define the following environment variables (and make sure they point to your
# gcc ARM toolchain):
#
# ARMV8_TOOLS_DIR=/c/utils/linaro_gcc/gcc-linaro-aarch64-none-elf-4.8-2014.01_win32/bin
# ARMV8_TOOLS_PREFIX=aarch64-none-elf-
# FILE_NAMES_DIR=/tmp
# export ARMV8_TOOLS_DIR
# export ARMV8_TOOLS_PREFIX
# export FILE_NAMES_DIR
#
# Put the tools dir on your path:
#
# PATH=$ARMV8_TOOLS_DIR:$PATH
#
# -----------------------------------------------------------------------------

 # check for supported platforms/boards here - these are the valid build targets
ifeq ($(PLAT_BUILD), 1)
     # for a platform-level build, the file is local
    PLAT_PATH=.
else
ifeq ($(MAKECMDGOALS), clean)
else

ifeq ($(MAKECMDGOALS), sim)
    PLAT_PATH=platform-sim
else
ifeq ($(MAKECMDGOALS), sim_out)
    PLAT_PATH=platform-sim
else
ifeq ($(MAKECMDGOALS), sim_bin)
    PLAT_PATH=platform-sim
else

ifeq ($(MAKECMDGOALS), rdb)
    PLAT_PATH=platform-rdb
else
ifeq ($(MAKECMDGOALS), rdb_out)
    PLAT_PATH=platform-rdb
else
ifeq ($(MAKECMDGOALS), rdb_bin)
    PLAT_PATH=platform-rdb
else
ifeq ($(MAKECMDGOALS), rdb-fit)
    PLAT_PATH=platform-rdb
else
ifeq ($(MAKECMDGOALS), rdb_fit_out)
    PLAT_PATH=platform-rdb
else
ifeq ($(MAKECMDGOALS), rdb_fit_bin)
    PLAT_PATH=platform-rdb
else

ifeq ($(MAKECMDGOALS), qds)
    PLAT_PATH=platform-qds
else
ifeq ($(MAKECMDGOALS), qds_out)
    PLAT_PATH=platform-qds
else
ifeq ($(MAKECMDGOALS), qds_bin)
    PLAT_PATH=platform-qds
else
ifeq ($(MAKECMDGOALS), qds-fit)
    PLAT_PATH=platform-qds
else
ifeq ($(MAKECMDGOALS), qds_fit_out)
    PLAT_PATH=platform-qds
else
ifeq ($(MAKECMDGOALS), qds_fit_bin)
    PLAT_PATH=platform-qds
else

  $(error -> Platform '$(MAKECMDGOALS)' not supported!)
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif

# -----------------------------------------------------------------------------

 # include the SoC architecture
ifeq ($(personality), base)
include $(PRE_PATH)soc.def
else
include $(PRE_PATH)soc$(personality).def
endif

# -----------------------------------------------------------------------------

 # include the gic architecture file
include ../armv8/gic.mk

# -----------------------------------------------------------------------------

 # include the interconnect architecture file
include ../armv8/inter.mk

# -----------------------------------------------------------------------------

# include the test infrastructure
TEST_SRC= $(PRE_PATH)../test
include $(TEST_SRC)/test.mk

# -----------------------------------------------------------------------------

# add soc-specific asm source and headers here
SRC_SOC    =soc.s
HDRS_SOC   =soc.h soc.mac

# add soc-specific C source and headers here
CSRC_SOC   =errata.c
CHDRS_SOC  =soc_tzasc.h

DIMM_C = dimm.c

CMMN_SRC= $(PRE_PATH)../common
include $(CMMN_SRC)/makefile.inc

# -----------------------------------------------------------------------------

