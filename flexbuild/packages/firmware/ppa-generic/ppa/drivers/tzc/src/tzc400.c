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

#include "tzc400.h"
#include "io.h"
#include "lib.h"
#include "soc_tzasc.h"

struct tzc400_instance {
    phys_addr_t base;
    uint8_t addr_width;
    uint8_t num_filters;
    uint8_t num_regions;
};

static struct tzc400_instance tzc400;


static uint32_t tzc400_read_build_config(phys_addr_t base)
{
    return in_le32(base + TZC400_BUILD_CONFIG_OFF);
}

static uint32_t tzc400_read_gate_keeper(phys_addr_t base)
{
    return in_le32(base + TZC400_GATE_KEEPER_OFF);
}

static void tzc400_write_gate_keeper(phys_addr_t base, uint32_t val)
{
    out_le32(base + TZC400_GATE_KEEPER_OFF, val);
}

static void tzc400_write_action(phys_addr_t base, enum tzc400_action action)
{
    out_le32(base + TZC400_ACTION_OFF, action);
}

static void tzc400_write_region_base_low(phys_addr_t base, uint32_t region,
                      uint32_t val)
{
    out_le32(base + TZC400_REGION_BASE_LOW_OFF +
        TZC400_REGION_NUM_OFF(region), val);
}

static void tzc400_write_region_base_high(phys_addr_t base, uint32_t region,
                       uint32_t val)
{
    out_le32(base + TZC400_REGION_BASE_HIGH_OFF +
        TZC400_REGION_NUM_OFF(region), val);
}

static void tzc400_write_region_top_low(phys_addr_t base, uint32_t region,
                     uint32_t val)
{
    out_le32(base + TZC400_REGION_TOP_LOW_OFF +
        TZC400_REGION_NUM_OFF(region), val);
}

static void tzc400_write_region_top_high(phys_addr_t base, uint32_t region,
                      uint32_t val)
{
    out_le32(base + TZC400_REGION_TOP_HIGH_OFF +
        TZC400_REGION_NUM_OFF(region), val);
}

static void tzc400_write_region_attributes(phys_addr_t base, uint32_t region,
                    uint32_t val)
{
    out_le32(base + TZC400_REGION_ATTRIBUTES_OFF +
        TZC400_REGION_NUM_OFF(region), val);
}

static void tzc400_write_region_id_access(phys_addr_t base, uint32_t region,
                       uint32_t val)
{
    out_le32(base + TZC400_REGION_ID_ACCESS_OFF +
        TZC400_REGION_NUM_OFF(region), val);
}

static uint32_t tzc400_read_component_id(phys_addr_t base)
{
    uint32_t id;

#define LSHIFT_U32(val, lshift)    ((uint32_t)(val) << (lshift))

    id = in8(base + TZC400_CID0_OFF);
    id |= LSHIFT_U32(in8(base + TZC400_CID1_OFF), 8);
    id |= LSHIFT_U32(in8(base + TZC400_CID2_OFF), 16);
    id |= LSHIFT_U32(in8(base + TZC400_CID3_OFF), 24);

    return id;
}

static uint32_t tzc400_get_gate_keeper(phys_addr_t base, uint8_t filter)
{
    uint32_t tmp;

    tmp = (tzc400_read_gate_keeper(base) >> TZC400_GATE_KEEPER_OS_SHIFT) &
        TZC400_GATE_KEEPER_OS_MASK;

    return (tmp >> filter) & TZC400_GATE_KEEPER_FILTER_MASK;
}

static void tzc400_set_gate_keeper(phys_addr_t base, uint8_t filter, uint32_t val)
{
    uint32_t tmp;

     // Current State:  Upper half.
     // Requested State: Lower half
    tmp = (tzc400_read_gate_keeper(base) >> TZC400_GATE_KEEPER_OS_SHIFT) &
        TZC400_GATE_KEEPER_OS_MASK;

    (val)? (tmp |=  (1 << filter)) : (tmp &= ~(1 << filter));

    tzc400_write_gate_keeper(base, (tmp & TZC400_GATE_KEEPER_OR_MASK) <<
                  TZC400_GATE_KEEPER_OR_SHIFT);

     // Need to wait till changes reflected in the status of TZC400.
    while (((tzc400_read_gate_keeper(base) >> TZC400_GATE_KEEPER_OS_SHIFT) &
        TZC400_GATE_KEEPER_OS_MASK) != tmp);
}


void tzc400_init(phys_addr_t base)
{
    uint32_t tzc400_id, tzc400_build;

    assert(base);
    tzc400.base = base;

     // Fetching component id and comparing it as TZC-400 TRM.
     // Value must be equal to "0xB105F00D".
    tzc400_id = tzc400_read_component_id(tzc400.base);

    if (tzc400_id != TZC400_COMPONENT_ID) {
        panic("TZC400 : Wrong device ID.\n");
    }

    tzc400_build = tzc400_read_build_config(tzc400.base);
    tzc400.num_filters = ((tzc400_build >> TZC400_BUILD_CONFIG_NF_SHIFT) &
               TZC400_BUILD_CONFIG_NF_MASK) + 1;
    tzc400.addr_width  = ((tzc400_build >> TZC400_BUILD_CONFIG_AW_SHIFT) &
               TZC400_BUILD_CONFIG_AW_MASK) + 1;
    tzc400.num_regions = ((tzc400_build >> TZC400_BUILD_CONFIG_NR_SHIFT) &
               TZC400_BUILD_CONFIG_NR_MASK) + 1;
}

 // tzc400_configure_region: Used to program the regions in TZC400.
 // Multiple filters can be associated with a region (bit0 = filter0). 
void tzc400_configure_region(uint32_t filters,
              uint8_t  region,
              phys_addr_t  region_base,
              phys_addr_t  region_top,
              enum tzc400_region_attributes sec_attr,
              uint32_t ns_device_access)
{
    assert(tzc400.base);

     // Range checks on filter/regions.
    assert(((filters >> tzc400.num_filters) == 0) &&
           (region < tzc400.num_regions));

#define UINT64_MAX   0xffffffffffffffffUL
     // Address Range Checking for the Start/End input addresses.
    assert(((region_top <= (UINT64_MAX >> (64 - tzc400.addr_width))) &&
        (region_base < region_top)));

     // region_base and (region_top + 1) must be 4KB aligned.
#define FOUR_KB_SIZE 4096
    assert(((region_base | (region_top + 1)) & (FOUR_KB_SIZE - 1)) == 0);

    assert(sec_attr <= TZC400_REGION_S_RDWR);

     // Start/End input address provided to the API 
     // are with acceptable memory boundry.
    tzc400_write_region_base_low(tzc400.base, region,
                  PHYS_ADDR_LO(region_base));
    tzc400_write_region_base_high(tzc400.base, region,
                   PHYS_ADDR_HI(region_base));

    tzc400_write_region_top_low(tzc400.base, region,
                PHYS_ADDR_LO(region_top));
    tzc400_write_region_top_high(tzc400.base, region,
                PHYS_ADDR_HI(region_top));

     // Setting Secure Attributes and assign the filter to a region.
    tzc400_write_region_attributes(tzc400.base, region,
        (sec_attr << TZC400_REG_ATTR_SEC_SHIFT) | filters);

     // Setting NSAID for allowing access to the region.
    tzc400_write_region_id_access(tzc400.base, region, ns_device_access);
}


void tzc400_set_action(enum tzc400_action action)
{
    assert(tzc400.base);

     // No Handler defined either for Error or Interrupt.
    tzc400_write_action(tzc400.base, action);
}


void tzc400_enable_filters(void)
{
    uint32_t filter;

    assert(tzc400.base);

    for (filter = 0; filter < tzc400.num_filters; filter++) {

        tzc400_get_gate_keeper(tzc400.base, filter)?
            panic("TZC : Filter Gatekeeper already enabled.\n")
                : tzc400_set_gate_keeper(tzc400.base, filter, 1);
    }
}


void tzc400_disable_filters(void)
{
    uint32_t filter;

    assert(tzc400.base);

     // Reset value of the GateKeepers are disabled.
    for (filter = 0; filter < tzc400.num_filters; filter++)
        tzc400_set_gate_keeper(tzc400.base, filter, 0);
}

void mark_ocram_secure()
{
    phys_addr_t ccs_sec_accs_reg;
    ccs_sec_accs_reg = CONFIG_SYS_FSL_CSU_ADDR;
    ccs_sec_accs_reg += CSU_SEC_LEVEL_REG_OFFSET;
    out_le32(ccs_sec_accs_reg, (unsigned int)OCRAM_SECURE_ACCESS_ENABLE);

    __asm__ volatile("dsb sy\n"
                       "isb\n");

}

#ifdef TZASC_BYPASS_MUX_DISABLE
void enable_mux_tzasc()
{
    phys_addr_t ccs_sec_accs_reg;

         // Setting the TERMINATE BARRIER for TX of CCI.
    out_le32(CCI_400_BASE_ADDR, (unsigned int)CCI_TERMINATE_BARRIER_TX);

#if (DATA_LOC != DATA_IN_DDR)
     // Mark OCRAM Secure
    mark_ocram_secure();
#endif
    __asm__ volatile("dsb sy\n"
                     "isb\n");
        
    ccs_sec_accs_reg = CONFIG_SYS_FSL_CSU_ADDR;
    ccs_sec_accs_reg += CSU_SEC_ACCESS_REG_OFFSET;
    out_le32(ccs_sec_accs_reg, (unsigned int)TZASC_BYPASS_MUX_DISABLE);
}
#endif

 // ARM TZC400 setup based on the as per platform defined:
 //  -- Filter Enable state.
 //  -- Region Boundaries (start_add & end_addr)
 //  -- Allowed Security Access Types (sec_attr)
 //  -- NSAID Permission (nsaid_permision).
 //  in the tzc400_reg_list defined in soc-lsxxxx/platform-xxx/plat.h
void arm_tzc400_setup(void)
{

    int index = 0;
    tzc400_init(PLAT_ARM_TZC_BASE);

     // Disable TZC400 filters.
    tzc400_disable_filters();

    for (index = 0; index < MAX_NUM_TZC_REGION; index++)
    {
        tzc400_configure_region(
                        tzc400_reg_list[index].reg_filter_en,
                        index,
                        tzc400_reg_list[index].start_addr,
                        tzc400_reg_list[index].end_addr,
                        tzc400_reg_list[index].sec_attr,
                        tzc400_reg_list[index].nsaid_permissions);
    }

     // Setting the Error Exception for now when access violation happened.
     // Later Error Interrupts will be handled.
    tzc400_set_action(TZC400_ACTION_ERR);

     //Enable the TZC400 filters.
    tzc400_enable_filters();


#ifdef TZASC_BYPASS_MUX_DISABLE
    enable_mux_tzasc();
#endif
}
