//-----------------------------------------------------------------------------
// 
// Copyright (C) 2015, 2016 Freescale Semiconductor, Inc. All rights reserved.
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

#ifndef _COMMON_H
#define _COMMON_H

 // Define the minimum required Stack. Linker File tries to allocate as much
 // as possible to stack

#define REQ_CORE_STACK_SIZE 0x1000

 // SP is aligned to 16bytes for armv8 aarch64
#define CORE_SP_ALIGNMENT	16

 // ROUNDDOWN e to b bytes boundary.
 // b must be power of two

#define ROUNDDOWN(e, b)  (e & ~(b - 1))

 // Checks e is aligned to b bytes boundary.
 // b must be power of 2

#define IS_ALIGNED(e, b) (!(e & (b - 1)))

 // PPA Size = 2MB
#define PPA_SIZE	(1 << 21)

#endif






