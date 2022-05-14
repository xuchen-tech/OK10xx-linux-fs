//-----------------------------------------------------------------------------
// 
// Copyright (C) 2015, 2016 Freescale Semiconductor, Inc.
// Copyright 2017 NXP Semiconductors
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//-----------------------------------------------------------------------------

OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)

#if (SIMULATOR_BUILD)
ENTRY(_reset_vector_el3)
#else
ENTRY(_start_monitor_el3)
#endif

//-----------------------------------------------------------------------------

 // The code is position independent

SECTIONS
{
	.text . : {
	__PPA_PROG_START__ = .;
#if (SIMULATOR_BUILD)
		*bootmain.64.o(.text*)
#else
		*monitor.o(.text*)
#endif
		*(.text*)
		*(.rodata*)
	__PPA_PROG_END__ = .;
	}

	.data . : {
	__DATA_START__ = .;
		*(.data*)
	__DATA_END__ = .;
	}

	. = ALIGN(8);
	.rel_dyn_start :
	{
	__REL_DYN_START__ = .;
		*(.__rel_dyn_start)
	}

	.rela.dyn : {
		*(.rela*)
	}

	.rel_dyn_end :
	{
		*(.__rel_dyn_end)
	__REL_DYN_END__ = .;
	}

	_end = .;

	. = ALIGN(8);
	.bss : {
	__BSS_START__ = .;
		*(.bss*)
		*(COMMON)
	. = ALIGN(8);
	__BSS_END__ = .;
	}
}

//-----------------------------------------------------------------------------





