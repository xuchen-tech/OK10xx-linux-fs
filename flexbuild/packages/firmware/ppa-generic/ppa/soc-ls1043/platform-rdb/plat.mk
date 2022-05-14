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
# rdb platform specific definitions
#
# supported targets:
#   rdb     - binary image
#   rdb_fit - fit image
#
# -----------------------------------------------------------------------------
#
# builds a binary image for the rdb board
rdb: 
	$(MAKE) SIM_BUILD=0 rdb_out
	$(MAKE) SIM_BUILD=0 rdb_bin
rdb_out:
	@echo 'build: image=bin \ $(GIC_FILE) \ $(INTER_FILE) \ ddr $(ddr) \ debug $(dbg) \ test "$(test)"'
	@echo
rdb_bin: monitor.bin

# builds a fit image for the rdb board
rdb-fit: 
	$(MAKE) SIM_BUILD=0 rdb_fit_out
	$(MAKE) SIM_BUILD=0 rdb_fit_bin
rdb_fit_out:
	@echo 'build: image=fit \ $(GIC_FILE) \ $(INTER_FILE) \ ddr $(ddr) \ debug $(dbg) \ test "$(test)"'
	@echo
rdb_fit_bin: ppa.itb

# -----------------------------------------------------------------------------

# add platform-specific asm here
PLAT_ASM =

# add platform-specific C source and headers here
SRC_PLAT   =
HDRS_PLAT  =policy.h plat.h

# add platform-test-specific asm files here
TEST_ASM =$(TEST_FILE)

ifeq ($(DDR_BLD), 1)
   # add ddr-specific source and headers here
  DDR_C =ddr_init.c
  DDR_H =
else
  DDR_C =
  DDR_H =
endif

# -----------------------------------------------------------------------------

TEXTBASE=0x60100000
