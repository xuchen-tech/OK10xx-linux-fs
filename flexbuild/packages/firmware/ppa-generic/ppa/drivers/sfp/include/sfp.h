//-----------------------------------------------------------------------------
//
// Copyright 2018 NXP Semiconductors
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
// Authors:
//  Sumit Garg <sumit.garg@nxp.com>
//
//-----------------------------------------------------------------------------

#ifndef SFP_H
#define SFP_H

 // Flag bit shifts
#define FLAG_POVDD_SHIFT     0
#define FLAG_SYSCFG_SHIFT    1
#define FLAG_SRKH_SHIFT      2
#define FLAG_MC_SHIFT        3
#define FLAG_DCV0_SHIFT      4
#define FLAG_DCV1_SHIFT      5
#define FLAG_DRV0_SHIFT      6
#define FLAG_DRV1_SHIFT      7
#define FLAG_OUID0_SHIFT     8
#define FLAG_OUID1_SHIFT     9
#define FLAG_OUID2_SHIFT     10
#define FLAG_OUID3_SHIFT     11
#define FLAG_OUID4_SHIFT     12
#define FLAG_DBG_LVL_SHIFT   13
#define FLAG_OTPMK_SHIFT     16
#define FLAG_OUID_MASK       0x1F
#define FLAG_DEBUG_MASK      0xF
#define FLAG_OTPMK_MASK      0xF

 // OTPMK flag values
#define PROG_OTPMK_MIN         0b0000
#define PROG_OTPMK_RANDOM      0b0001
#define PROG_OTPMK_USER        0b0010
#define PROG_OTPMK_RANDOM_MIN  0b0101
#define PROG_OTPMK_USER_MIN    0b0110
#define PROG_NO_OTPMK          0b1000

#define OTPMK_MIM_BITS_MASK    0xF0000000

 // System configuration bit shifts
#define SCB_WP_SHIFT        0
#define SCB_ITS_SHIFT        2
#define SCB_NSEC_SHIFT        4
#define SCB_ZD_SHIFT        5
#define SCB_K0_SHIFT        15
#define SCB_K1_SHIFT        14
#define SCB_K2_SHIFT        13
#define SCB_K3_SHIFT        12
#define SCB_K4_SHIFT        11
#define SCB_K5_SHIFT        10
#define SCB_K6_SHIFT        9
#define SCB_FR0_SHIFT        30
#define SCB_FR1_SHIFT        31

 // Fuse Header Structure
struct fuse_hdr_t {
    uint8_t barker[4];           // 0x00 Barker code
    uint32_t flags;              // 0x04 Script flags
    uint32_t povdd_gpio;         // 0x08 GPIO for POVDD
    uint32_t otpmk[8];           // 0x0C-0x2B OTPMK
    uint32_t srkh[8];            // 0x2C-0x4B SRKH
    uint32_t oem_uid[5];         // 0x4C-0x5F OEM unique id's
    uint32_t dcv[2];             // 0x60-0x67 Debug Challenge
    uint32_t drv[2];             // 0x68-0x6F Debug Response
    uint32_t ospr1;              // 0x70 OSPR1
    uint32_t sc;                 // 0x74 OSPR0 (System Configuration)
    uint32_t reserved[2];        // 0x78-0x7F Reserved
};

 // SFP Configuration Register Offsets
#define SFP_INGR_ADDR          (SFP_BASE_ADDR + 0x20)
#define SFP_SVHESR_ADDR        (SFP_BASE_ADDR + 0x24)
#define SFP_SFPCR_ADDR         (SFP_BASE_ADDR + 0x28)
#define SFP_VER_ADDR           (SFP_BASE_ADDR + 0x38)

 // SFP Hamming register masks for OTPMK and DRV
#define SFP_SVHESR_DRV_MASK    0x7F
#define SFP_SVHESR_OTPMK_MASK  0x7FC00

 // SFP commands
#define SFP_INGR_READFB_CMD    0x1
#define SFP_INGR_PROGFB_CMD    0x2
#define SFP_INGR_ERROR_MASK    0x100

 // DCFG register to dump error code
#define DCFG_SCRATCH4_OFFSET   0x20C

#define SFP_FUSE_REGS_ADDR     (SFP_BASE_ADDR + 0x200)

#ifdef CONFIG_SYS_FSL_SFP_VER_3_4
#define OSPR0_SC_MASK          0xC000FE35
#elif defined(CONFIG_SYS_FSL_SFP_VER_3_2)
#define OSPR0_SC_MASK          0x0000E035
#endif

#define OSPR1_MC_MASK          0xFFFF0000
#define OSPR1_DBG_LVL_MASK     0x00000007


 // SFP CCSR Register Map
struct sfp_ccsr_regs_t {
    uint32_t ospr;               // 0x200 OSPR0
    uint32_t ospr1;              // 0x204 OSPR1
    uint32_t dcv[2];             // 0x208 Debug Challenge Value
    uint32_t drv[2];             // 0x210 Debug Response Value
    uint32_t fswpr;              // 0x218 FSL Section Write Protect
    uint32_t fsl_uid[2];         // 0x21c FSL UID 0
    uint32_t isbcr;              // 0x224 ISBC Configuration
    uint32_t fsspr[3];           // 0x228 FSL Scratch Pad
    uint32_t otpmk[8];           // 0x234 OTPMK
    uint32_t srk_hash[8];        // 0x254 Super Root Key Hash
    uint32_t oem_uid[5];         // 0x274 OEM UID 0
};

 // This function does fuse provisioning
int provision_fuses(unsigned long long fuse_scr_addr);

#endif // SFP_H
