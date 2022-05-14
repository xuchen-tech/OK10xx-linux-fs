#
# Copyright (c) <2010-2017>, Intel Corp
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
# 1) Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2) Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# 3) Neither the name of Intel nor the names of its contributors may be
# used to endorse or promote products derived from this software without specific
# prior written permission.
#
# 4) The screens displayed by the application must contain the copyright notice as defined
# above and can not be removed without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Created 2010-2017 by Keith Wiles @ intel.com

ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# GUI is a work in progress
ifeq ($(GUI),true)
DIRS-y += lib gui app
else
DIRS-y += lib app
endif

DEPDIRS-app += lib gui

export GUI

.PHONY: docs

include $(RTE_SDK)/mk/rte.extsubdir.mk

realclean:
	@rm -fr app/app
	@rm -fr lib/common/lib
	@rm -fr lib/cli/lib
	@rm -fr lib/lua/src/lib
	@rm -fr app/$(RTE_TARGET)
	@rm -fr lib/common/$(RTE_TARGET)
	@rm -fr lib/cli/$(RTE_TARGET)
	@rm -fr lib/lua/src/$(RTE_TARGET)

docs:
	@make -C docs html

pdf:
	@make -C docs latexpdf

cleandocs:
	@make -C docs clean
