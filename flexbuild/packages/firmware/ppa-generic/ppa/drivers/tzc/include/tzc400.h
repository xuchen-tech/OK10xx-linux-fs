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

#ifndef __TZC400_H__
#define __TZC400_H__

#include "types.h"
#include "io.h"

#define TZC400_REGION_SIZE        0x1000

#define TZC400_BUILD_CONFIG_OFF        0x000
#define TZC400_ACTION_OFF        0x004
#define TZC400_GATE_KEEPER_OFF        0x008
#define TZC400_SPECULATION_CTRL_OFF    0x00c
#define TZC400_INT_STATUS        0x010
#define TZC400_INT_CLEAR        0x014

#define TZC400_FAIL_ADDRESS_LOW_OFF    0x020
#define TZC400_FAIL_ADDRESS_HIGH_OFF    0x024
#define TZC400_FAIL_CONTROL_OFF        0x028
#define TZC400_FAIL_ID            0x02c

#define TZC400_REGION_BASE_LOW_OFF    0x100
#define TZC400_REGION_BASE_HIGH_OFF    0x104
#define TZC400_REGION_TOP_LOW_OFF    0x108
#define TZC400_REGION_TOP_HIGH_OFF    0x10c
#define TZC400_REGION_ATTRIBUTES_OFF    0x110
#define TZC400_REGION_ID_ACCESS_OFF    0x114
#define TZC400_REGION_NUM_OFF(region)  (0x20 * region)

 //ID Registers
#define TZC400_PID0_OFF        0xfe0
#define TZC400_PID1_OFF        0xfe4
#define TZC400_PID2_OFF        0xfe8
#define TZC400_PID3_OFF        0xfec
#define TZC400_PID4_OFF        0xfd0
#define TZC400_PID5_OFF        0xfd4
#define TZC400_PID6_OFF        0xfd8
#define TZC400_PID7_OFF        0xfdc
#define TZC400_CID0_OFF        0xff0
#define TZC400_CID1_OFF        0xff4
#define TZC400_CID2_OFF        0xff8
#define TZC400_CID3_OFF        0xffc

#define TZC400_BUILD_CONFIG_NF_SHIFT    24
#define TZC400_BUILD_CONFIG_NF_MASK    0x3
#define TZC400_BUILD_CONFIG_AW_SHIFT    8
#define TZC400_BUILD_CONFIG_AW_MASK    0x3f
#define TZC400_BUILD_CONFIG_NR_SHIFT    0
#define TZC400_BUILD_CONFIG_NR_MASK    0x1f

 // Fetch the GateKeeper count from BUILD Config as it is SoC specfic.
 // Creating defines upto GateKeeper = 4.
#define TZC400_GATE_KEEPER_OS_SHIFT    16
#define TZC400_GATE_KEEPER_OS_MASK    0xf
#define TZC400_GATE_KEEPER_OR_SHIFT    0
#define TZC400_GATE_KEEPER_OR_MASK    0xf
#define TZC400_GATE_KEEPER_FILTER_MASK    0x1

 // Sppeculation is enabled by default. */
#define TZC400_SPECULATION_CTRL_WRITE_DISABLE    (1 << 1)
#define TZC400_SPECULATION_CTRL_READ_DISABLE    (1 << 0)

 // Considering no of filter=4
#define TZC400_INT_STATUS_OVERLAP_SHIFT    16
#define TZC400_INT_STATUS_OVERLAP_MASK        0xf
#define TZC400_INT_STATUS_OVERRUN_SHIFT    8
#define TZC400_INT_STATUS_OVERRUN_MASK        0xf
#define TZC400_INT_STATUS_STATUS_SHIFT        0
#define TZC400_INT_STATUS_STATUS_MASK        0xf

#define TZC400_INT_CLEAR_CLEAR_SHIFT        0
#define TZC400_INT_CLEAR_CLEAR_MASK        0xf

#define TZC400_FAIL_CONTROL_DIR_SHIFT        (1 << 24)
#define TZC400_FAIL_CONTROL_DIR_READ        0x0
#define TZC400_FAIL_CONTROL_DIR_WRITE        0x1
#define TZC400_FAIL_CONTROL_NS_SHIFT        (1 << 21)
#define TZC400_FAIL_CONTROL_NS_SECURE        0x0
#define TZC400_FAIL_CONTROL_NS_NONSECURE    0x1
#define TZC400_FAIL_CONTROL_PRIV_SHIFT        (1 << 20)
#define TZC400_FAIL_CONTROL_PRIV_PRIV        0x0
#define TZC400_FAIL_CONTROL_PRIV_UNPRIV    0x1


#define TZC400_REG_ATTR_SEC_SHIFT        30
#define TZC400_REG_ATTR_F_EN_SHIFT        0
#define TZC400_REG_ATTR_F_EN_MASK        0xf
#define TZC400_REG_ATTR_FILTER_BIT(x)        ((1 << x) << TZC400_REG_ATTR_F_EN_SHIFT)
#define TZC400_REG_ATTR_FILTER_BIT_ALL        (TZC400_REG_ATTR_F_EN_MASK << \
                                              TZC400_REG_ATTR_F_EN_SHIFT)

#define TZC400_REGION_ID_ACCESS_NSAID_WR_EN_SHIFT    16
#define TZC400_REGION_ID_ACCESS_NSAID_RD_EN_SHIFT    0
#define TZC400_REGION_ID_ACCESS_NSAID_ID_MASK        0xf


 // Region ID Access Permission Setting Macros on NSAID basis. */
#define TZC400_REGION_ACCESS_RD(reg_id)                    \
        ((1 << (reg_id & TZC400_REGION_ID_ACCESS_NSAID_ID_MASK)) <<    \
         TZC400_REGION_ID_ACCESS_NSAID_RD_EN_SHIFT)

#define TZC400_REGION_ACCESS_WR(reg_id)                    \
        ((1 << (reg_id & TZC400_REGION_ID_ACCESS_NSAID_ID_MASK)) <<    \
         TZC400_REGION_ID_ACCESS_NSAID_WR_EN_SHIFT)

#define TZC400_REGION_ACCESS_RDWR(reg_id)                    \
        (TZC400_REGION_ACCESS_RD(reg_id) | TZC400_REGION_ACCESS_WR(reg_id))

 //Bit 0 to 3 are mapped to filters.
#define TZC400_COMPONENT_ID    0xb105f00d

 //
 // Type of actions on an access violation:
 //  -- Memory requested is zeroed
 //  -- Raise Interrupt(INT) or/and Error Exception(ERR) -> sync external Data Abort
 //    to inform the system
 //
 //    -TZC400_ACTION_NONE-     No Interrupt   , No Exception
 //    -TZC400_ACTION_ERR -     No Interrupt   , Raise exception -> sync external Data Abort
 //    -TZC400_ACTION_INT     - Raise Interrupt, No exceptio
 //    -TZC400_ACTION_ERR_INT - Raise Interrupt, Raise Exception
enum tzc400_action {
    TZC400_ACTION_NONE = 0,
    TZC400_ACTION_ERR = 1,
    TZC400_ACTION_INT = 2,
    TZC400_ACTION_ERR_INT = (TZC400_ACTION_ERR | TZC400_ACTION_INT)
};

 // Types of secure access that can be allowed too a region.
typedef enum tzc400_region_attributes {
    TZC400_REGION_S_NONE = 0,
    TZC400_REGION_S_RD = 1,
    TZC400_REGION_S_WR = 2,
    TZC400_REGION_S_RDWR = (TZC400_REGION_S_RD | TZC400_REGION_S_WR)
} tzc400_region_attributes_t;


 // Structure to configure TZC Regions' boundaries and attributes.
struct tzc400_reg {
	uint8_t reg_filter_en;
	unsigned long long start_addr;
	unsigned long long end_addr;
	tzc400_region_attributes_t sec_attr;
	unsigned int nsaid_permissions;
};

void tzc400_init(phys_addr_t base);
void tzc400_configure_region(uint32_t filters, uint8_t region,
                             phys_addr_t region_base, phys_addr_t region_top,
                             enum tzc400_region_attributes sec_attr,
                             uint32_t ns_device_access);
void tzc400_enable_filters(void);
void tzc400_disable_filters(void);
void tzc400_set_action(enum tzc400_action action);


#endif /* __TZC400_H__ */
