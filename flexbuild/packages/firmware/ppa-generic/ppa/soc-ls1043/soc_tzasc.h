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

#include "tzc380.h"

#define MAX_NUM_TZC_REGION        4

 // List of MAX_NUM_TZC_REGION TZC regions' boundaries and configurations. 
static const struct tzc380_reg tzc380_reg_list[] = {
    {
        TZC380_REGION_NS_RDWR,
        0x0,
        0, //donot try to set this bit as it reserved.
        0x0,
        0x0
    },
    {
        TZC380_REGION_S_RDWR,
        TZC380_REGION_SIZE_2M,
        TZC380_ATTR_REGION_ENABLE,
        0x0,
        0xFBE00000
    },
    {
        TZC380_REGION_S_RDWR,
        TZC380_REGION_SIZE_64M,
        TZC380_ATTR_REGION_ENABLE,
        0x80, //b 1000 0000, Disable region 7
        0xFC000000,
    },
    {
        TZC380_REGION_S_RDWR,
        TZC380_REGION_SIZE_8M,
        TZC380_ATTR_REGION_ENABLE,
        0xC0, //b 1100 0000, Disable region 7 & 8
        0xFF800000,
    },
    {}
};

#endif /* __SOC_TZASC_H__ */
