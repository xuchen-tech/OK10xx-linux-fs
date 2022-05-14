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


#include "lib.h"
#include "tzc380.h"
#include "io.h"
#include "soc_tzasc.h"

struct tzc380_instance {
    phys_addr_t base;
    uint8_t addr_width;
    uint8_t num_regions;
};

static struct tzc380_instance tzc380;

static uint32_t tzc380_read_build_config(phys_addr_t base)
{
    return in_le32(base + TZC380_BUILD_CONFIG_OFF);
}

static void tzc380_write_action(phys_addr_t base, enum tzc380_action action)
{
    out_le32(base + TZC380_ACTION_OFF, action);
}

static void tzc380_write_region_base_low(phys_addr_t base, uint32_t region,
                      uint32_t val)
{
    out_le32(base + TZC380_REGION_SETUP_LOW_OFF(region), val);
}

static void tzc380_write_region_base_high(phys_addr_t base, uint32_t region,
                       uint32_t val)
{
    out_le32(base + TZC380_REGION_SETUP_HIGH_OFF(region), val);
}

static void tzc380_write_region_attributes(phys_addr_t base, uint32_t region,
                    uint32_t val)
{
    out_le32(base + TZC380_REGION_ATTRIBUTES_OFF(region), val);
}

void tzc380_init(phys_addr_t base)
{
    uint32_t tzc380_build;

    assert(base);
    tzc380.base = base;

    /* Save values we will use later. */
    tzc380_build = tzc380_read_build_config(tzc380.base);
    tzc380.addr_width  = ((tzc380_build >> TZC380_BUILD_CONFIG_AW_SHIFT) &
               TZC380_BUILD_CONFIG_AW_MASK) + 1;
    tzc380.num_regions = ((tzc380_build >> TZC380_BUILD_CONFIG_NR_SHIFT) &
               TZC380_BUILD_CONFIG_NR_MASK) + 1;
}


 // tzc380_configure_region: Used to program the regions in TZC380.
void tzc380_configure_region(uint8_t region, phys_addr_t region_base, uint32_t attr)
{
    assert(tzc380.base);

    assert(region < tzc380.num_regions);

    debug_hex("low addr=%d\n", PHYS_ADDR_LO(region_base));
    tzc380_write_region_base_low(tzc380.base, region, PHYS_ADDR_LO(region_base));
    tzc380_write_region_base_high(tzc380.base, region, PHYS_ADDR_HI(region_base));
    debug_hex("attr=%d\n", attr);
    tzc380_write_region_attributes(tzc380.base, region, attr);
}

void tzc380_set_action(enum tzc380_action action)
{
    assert(tzc380.base);

     // No Handler defined either for Error or Interrupt.
    tzc380_write_action(tzc380.base, action);
}

#ifdef TZASC_BYPASS_MUX_DISABLE
void enable_mux_tzasc(void)
{
    phys_addr_t ccs_sec_accs_reg;

     // Setting the TERMINATE BARRIER for TX of CCI.
    out_le32(CCI_400_BASE_ADDR, (unsigned int)CCI_TERMINATE_BARRIER_TX);

        __asm__ volatile("dsb sy\n"
                         "isb\n");
        
    ccs_sec_accs_reg = CONFIG_SYS_FSL_CSU_ADDR;
    ccs_sec_accs_reg += CSU_SEC_ACCESS_REG_OFFSET;
    out_le32(ccs_sec_accs_reg, (unsigned int)TZASC_BYPASS_MUX_DISABLE);
        
    __asm__ volatile("dsb sy\n"
                         "isb\n");
        
     //   __asm__ volatile("b .\n");
}
#endif

 // ARM TZC400 setup based on the as per platform defined:
 //  -- Region ID.
 //  -- Region Boundaries (start_addr)
 //  -- Region Attributes:
 //   - Allowed Security Access Types (sec_attr)
 //   - Sub-Region Mask for 8 equal sub-regions.
 //   - Size of the Region
 //   - Enable Region.
 //  in the tzc400_reg_list defined in soc-lsxxxx/platform-xxx/plat.h
void arm_tzc380_setup(void)
{

    int reg_id  = 0;
    tzc380_init(PLAT_ARM_TZC_BASE);
    uint32_t size, enabled, subreg_disable_mask;
    phys_addr_t start_addr;
    tzc380_set_action(TZC380_ACTION_NONE);

    for (reg_id = 0; reg_id < MAX_NUM_TZC_REGION; reg_id++)
    {
         tzc380_configure_region(
                      reg_id,
                      tzc380_reg_list[reg_id].start_addr, 
                      ((tzc380_reg_list[reg_id].sec_attr & 0xF) <<28) 
                       | ((tzc380_reg_list[reg_id].subreg_disable_mask & 0xFF) << 8)
                       | ((tzc380_reg_list[reg_id].size & 0x3F) << 1)
                       | (tzc380_reg_list[reg_id].enabled & 0x1));
    }

    // Setting the Error Exception for now when access violation happened.
    // Later Error Interrupts will be handled.
    tzc380_set_action(TZC380_ACTION_ERR);

#ifdef TZASC_BYPASS_MUX_DISABLE
    enable_mux_tzasc();
#endif
}
