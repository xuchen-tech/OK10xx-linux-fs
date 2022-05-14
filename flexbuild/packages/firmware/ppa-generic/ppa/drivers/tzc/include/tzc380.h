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
// Author Pankaj Gupta  <pankaj.gupta@nxp.com>
// 
//-----------------------------------------------------------------------------

#ifndef __TZC380_H__
#define __TZC380_H__

#include "types.h"
#include "io.h"

#define TZC380_REGION_SIZE        0x1000

#define TZC380_BUILD_CONFIG_OFF        0x000
#define TZC380_ACTION_OFF        0x004
#define TZC380_LOCKDOWN_RANGE_OFF    0x008
#define TZC380_LOCKDOWN_SELECT_OFF    0x00C
#define TZC380_INT_STATUS        0x010
#define TZC380_INT_CLEAR        0x014

#define TZC380_FAIL_ADDRESS_LOW_OFF    0x020
#define TZC380_FAIL_ADDRESS_HIGH_OFF    0x024
#define TZC380_FAIL_CONTROL_OFF    0x028
#define TZC380_FAIL_ID            0x02c

#define TZC380_SPECULATION_CTRL_OFF    0x030
#define TZC380_SECURITY_INV_EN_OFF    0x034

#define TZC380_REGION_SETUP_LOW_OFF(n)    (0x100 + n * 0x10)
#define TZC380_REGION_SETUP_HIGH_OFF(n) (0x104 + n * 0x10)
#define TZC380_REGION_ATTRIBUTES_OFF(n) (0x108 + n * 0x10)

#define  TZC380_ACTION_RV_SHIFT        0
#define  TZC380_ACTION_RV_MASK        0x3
#define  TZC380_ACTION_RV_LOWOK        0x0
#define  TZC380_ACTION_RV_LOWERR    0x1
#define  TZC380_ACTION_RV_HIGHOK    0x2
#define  TZC380_ACTION_RV_HIGHERR    0x3

 // TZC380 ID Register
#define TZC380_PID0_OFF        0xfe0
#define TZC380_PID1_OFF        0xfe4
#define TZC380_PID2_OFF        0xfe8
#define TZC380_PID3_OFF        0xfec
#define TZC380_PID4_OFF        0xfd0
#define TZC380_CID0_OFF        0xff0
#define TZC380_CID1_OFF        0xff4
#define TZC380_CID2_OFF        0xff8
#define TZC380_CID3_OFF        0xffc

#define TZC380_BUILD_CONFIG_AW_SHIFT    8
#define TZC380_BUILD_CONFIG_AW_MASK    0x3f
#define TZC380_BUILD_CONFIG_NR_SHIFT    0
#define TZC380_BUILD_CONFIG_NR_MASK    0xf

 //By Default TZC380 Speculation Enabled
#define TZC380_SPECULATION_CTRL_WRITE_DISABLE    BIT(1)
#define TZC380_SPECULATION_CTRL_READ_DISABLE    BIT(0)

#define TZC380_INT_STATUS_OVERRUN_SHIFT        1
#define TZC380_INT_STATUS_OVERRUN_MASK        0x1
#define TZC380_INT_STATUS_STATUS_SHIFT        0
#define TZC380_INT_STATUS_STATUS_MASK        0x1

#define TZC380_INT_CLEAR_CLEAR_SHIFT        0
#define TZC380_INT_CLEAR_CLEAR_MASK        0x1

#define TZC380_COMPONENT_ID    0xb105f00d
#define TZC380_PERIPH_ID_LOW    0x001bb380
#define TZC380_PERIPH_ID_HIGH    0x00000004

 //
 // Type of actions on an access violation:
 //  -- Memory requested is zeroed
 //  -- Raise Interrupt(INT) or/and Error Exception(ERR) -> sync external Data Abort
 //    to inform the system
 //
 //    -TZC380_ACTION_NONE-     No Interrupt   , No Exception
 //    -TZC380_ACTION_ERR -     No Interrupt   , Raise exception -> sync external Data Abort
 //    -TZC380_ACTION_INT     - Raise Interrupt, No exceptio
 //    -TZC380_ACTION_ERR_INT - Raise Interrupt, Raise Exception
enum tzc380_action {
    TZC380_ACTION_NONE = 0,
    TZC380_ACTION_ERR = 1,
    TZC380_ACTION_INT = 2,
    TZC380_ACTION_ERR_INT = (TZC380_ACTION_ERR | TZC380_ACTION_INT)
};


#define TZC380_SP_NS_W        BIT(0)
#define TZC380_SP_NS_R        BIT(1)
#define TZC380_SP_S_W        BIT(2)
#define TZC380_SP_S_R        BIT(3)

#define TZC380_ATTR_SP_SHIFT    28
#define TZC380_ATTR_SP_ALL        ((TZC380_SP_S_W | TZC380_SP_S_R | TZC380_SP_NS_W | \
                TZC380_SP_NS_R) << TZC380_ATTR_SP_SHIFT)
#define TZC380_ATTR_SP_S_RW    ((TZC380_SP_S_W | TZC380_SP_S_R) << \
                 TZC380_ATTR_SP_SHIFT)
#define TZC380_ATTR_SP_NS_RW    ((TZC380_SP_NS_W | TZC380_SP_NS_R) << \
                TZC380_ATTR_SP_SHIFT)

#define TZC380_REGION_SIZE_32K    0xe
#define TZC380_REGION_SIZE_64K    0xf
#define TZC380_REGION_SIZE_128K    0x10
#define TZC380_REGION_SIZE_256K    0x11
#define TZC380_REGION_SIZE_512K    0x12
#define TZC380_REGION_SIZE_1M    0x13
#define TZC380_REGION_SIZE_2M    0x14
#define TZC380_REGION_SIZE_4M    0x15
#define TZC380_REGION_SIZE_8M    0x16
#define TZC380_REGION_SIZE_16M    0x17
#define TZC380_REGION_SIZE_32M    0x18
#define TZC380_REGION_SIZE_64M    0x19
#define TZC380_REGION_SIZE_128M    0x1a
#define TZC380_REGION_SIZE_256M    0x1b
#define TZC380_REGION_SIZE_512M    0x1c
#define TZC380_REGION_SIZE_1G    0x1d
#define TZC380_REGION_SIZE_2G    0x1e
#define TZC380_REGION_SIZE_4G    0x1f
#define TZC380_REGION_SIZE_8G    0x20
#define TZC380_REGION_SIZE_16G    0x21
#define TZC380_REGION_SIZE_32G    0x22
#define TZC380_REGION_SIZE_64G    0x23
#define TZC380_REGION_SIZE_128G    0x24
#define TZC380_REGION_SIZE_256G    0x25
#define TZC380_REGION_SIZE_512G    0x26
#define TZC380_REGION_SIZE_1T    0x27
#define TZC380_REGION_SIZE_2T    0x28
#define TZC380_REGION_SIZE_4T    0x29
#define TZC380_REGION_SIZE_8T    0x2a
#define TZC380_REGION_SIZE_16T    0x2b
#define TZC380_REGION_SIZE_32T    0x2c
#define TZC380_REGION_SIZE_64T    0x2d
#define TZC380_REGION_SIZE_128T    0x2e
#define TZC380_REGION_SIZE_256T    0x2f
#define TZC380_REGION_SIZE_512T    0x30
#define TZC380_REGION_SIZE_1P    0x31
#define TZC380_REGION_SIZE_2P    0x32
#define TZC380_REGION_SIZE_4P    0x33
#define TZC380_REGION_SIZE_8P    0x34
#define TZC380_REGION_SIZE_16P    0x35
#define TZC380_REGION_SIZE_32P    0x36
#define TZC380_REGION_SIZE_64P    0x37
#define TZC380_REGION_SIZE_128P    0x38
#define TZC380_REGION_SIZE_256P    0x39
#define TZC380_REGION_SIZE_512P    0x3a
#define TZC380_REGION_SIZE_1E    0x3b
#define TZC380_REGION_SIZE_2E    0x3c
#define TZC380_REGION_SIZE_4E    0x3d
#define TZC380_REGION_SIZE_8E    0x3e
#define TZC380_REGION_SIZE_16E    0x3f

#define TZC380_REGION_SIZE_SHIFT        0x1
#define TZC380_REGION_SIZE_MASK    GENMASK_32    (6, 1)
#define TZC380_ATTR_REGION_SIZE(s)        ((s) << TZC380_REGION_SIZE_SHIFT)

#define TZC380_ATTR_REGION_EN_SHIFT        0x0
#define TZC380_ATTR_REGION_EN_MASK        0x1

#define TZC380_ATTR_REGION_ENABLE        0x1
#define TZC380_ATTR_REGION_DISABLE        0x0

typedef enum tzc380_region_attributes {
    TZC380_REGION_S_NONE = 0,
    TZC380_REGION_NS_RD = 1,
    TZC380_REGION_NS_WR = 2,
    TZC380_REGION_NS_RDWR = (TZC380_REGION_NS_RD | TZC380_REGION_NS_WR),
    TZC380_REGION_S_RD = 4,
    TZC380_REGION_S_WR = 8,
    TZC380_REGION_S_RDWR = (TZC380_REGION_S_RD | TZC380_REGION_S_WR),
} tzc380_region_attributes_t;

 // Structure to configure TZC Regions' boundaries and attributes.
struct tzc380_reg {
	tzc380_region_attributes_t sec_attr;
	uint32_t size;
	uint32_t enabled;
	uint32_t subreg_disable_mask;
	phys_addr_t start_addr;
};

void tzc380_init(phys_addr_t base);
void tzc380_configure_region(uint8_t region, phys_addr_t region_base, size_t size);
void tzc380_set_action(enum tzc380_action action);
void enable_mux_tzasc(void);

#endif // __TZC380_H__
