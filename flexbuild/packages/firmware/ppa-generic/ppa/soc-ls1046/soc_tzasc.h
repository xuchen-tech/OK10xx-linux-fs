//-----------------------------------------------------------------------------
// 
// Copyright (c) 2016, NXP Semiconductors
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
// Author Pankaj Gupta <pankaj.gupta@nxp.com>
// 
//-----------------------------------------------------------------------------

#ifndef __SOC_TZASC_H__
#define __SOC_TZASC_H__

#include "tzc400.h"

#define MAX_NUM_TZC_REGION    5

 // List of MAX_NUM_TZC_REGION TZC regions' boundaries and configurations. 
static const struct tzc400_reg tzc400_reg_list[] = {
    {1,  0x00000000,  0xFFFFFFFF, TZC400_REGION_S_NONE, 0x00000000},
    {1,  0xFBE00000,  0xFFDFFFFF, TZC400_REGION_S_RDWR, 0x00000000},
    {1,  0xFFE00000,  0xFFFFFFFF, TZC400_REGION_S_RDWR, 0xFFFFFFFF},
    {1,  0x80000000,  0xFBDFFFFF, TZC400_REGION_S_NONE, 0xFFFFFFFF},
    {1, 0x880000000, 0xFFFFFFFFF, TZC400_REGION_S_NONE, 0xFFFFFFFF},
    {}
};

#endif // __SOC_TZASC_H__
