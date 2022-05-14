//---------------------------------------------------------------------------
// 
// Copyright (c) 2016, Freescale Semiconductor, Inc. All rights reserved.
// Copyright 2017-2018 NXP Semiconductor
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
//---------------------------------------------------------------------------

#ifndef _AARCH64_H
#define	_AARCH64_H

#define  SPSR_MODE_MASK  0x1F
#define  SPSR_FOR_EL2H   0x3C9
#define  SPSR_FOR_EL1H   0x3C5
#define  SPSR_DAIF_MASK  0x3C0
#define  SPSR_MODE_EL2H  0x009
#define  SPSR_EL_MASK    0xC
#define  SPSR_EL0        0x0
#define  SPSR_EL1        0x4
#define  SPSR_EL2        0x8
#define  SPSR_EL3        0xC
#define  SPSR_EL3_M4     0x10
#define  SPSR_EL3_M4_32  0x10

#define  SPSR32_E_MASK          0x200
#define  SPSR32_E_BE            0x200
#define  SPSR32_DEFAULT         0x1DA
#define  SPSR32_EL2_LE          0x1DA
#define  SPSR32_EL2_BE          0x3DA
#define  SPSR32_EL1_LE          0x1D3
#define  SPSR32_EL1_BE          0x3D3
#define  SPSR32_MODE_HYP        0xA
#define  SPSR32_MODE_SUPV       0x3
#define  SPSR32_MODE_MASK       0xF

#define  SCTLR_EE_WXN_SA_A_MASK 0x0208000A
#define  SCTLR_EE_WXN_MASK      0x02080000
#define  SCTLR_EE_MASK          0x02000000
#define  SCTLR_EE_BE            0x02000000
#define  SCTLR_WXN_MASK         0x00080000
#define  SCTLR_I_C_M_MASK       0x00001005
#define  SCTLR_I_MASK           0x00001000
#define  SCTLR_SA_A_MASK        0x0000000A
#define  SCTLR_C_M_MASK         0x00000005
#define  SCTLR_SA_C_A_M_MASK    0x0000000F
#define  SCTLR_C_A_M_MASK       0x00000007
#define  SCTLR_SA_MASK          0x00000008
#define  SCTLR_C_MASK           0x00000004
#define  SCTLR_A_MASK           0x00000002
#define  SCTLR_M_MASK           0x00000001
#define  SCTLR_EL3_RES1         0x30C50830       
#define  SCTLR_EL2_RES1         0x30C50830       
#define  SCTLR_EL1_RES1         0x30D00800 
#define  SCTLR_EE_BIT           SCTLR_EE_MASK

#define  CPTR_EL2_RES1_MASK     0x33FF

 // DAIF Attributes
#define DAIF_FIQ_BIT        (1 << 6)
#define DAIF_IRQ_BIT        (1 << 7)
#define DAIF_ABT_BIT        (1 << 8)
#define DAIF_DBG_BIT        (1 << 9)

#define  DAIF_SET_MASK          0x3C0

#define  CPUECTLR_DISABLE_TWALK_PREFETCH  0x4000000000
#define  CPUECTLR_INS_PREFETCH_MASK       0x1800000000
#define  CPUECTLR_DAT_PREFETCH_MASK       0x0300000000
#define  CPUECTLR_SMPEN_MASK              0x40
#define  CPUECTLR_SMPEN_EN                0x40
#define  CPUECTLR_RET_MASK                0x7
#define  CPUECTLR_RET_SET                 0x2
#define  CPUECTLR_TIMER_MASK              0x7
#define  CPUECTLR_TIMER_8TICKS            0x2

#define  CPUACTLR_FRC_INORDER_MASK 0x400000
#define  CPUACTLR_FRC_INORDER_EN   0x400000
#define  CPUACTLR_ENDCCASCI_EN     0x100000000000
 // A72 CPUACTLR defines
#define  CPUACTLR_DIS_LS_HW_PRE    0x100000000000000
#define  CPUACTLR_DIS_SBP_MASK     0x400000000
#define  CPUACTLR_DIS_BTB_MASK     0x8
#define  CPUACTLR_DIS_INP_MASK     0x10
#define  CPUACTLR_DIS_LD_PASS_ST   0x80000000000000
 // A53 CPUACTLR defines
#define  CPUACTLR_L1PCTL_MASK      0x0000E000
#define  CPUACTLR_L1PCTL_EN_5      0xA000

#define  OSDLR_EL1_DLK_MASK     0x1
#define  OSDLR_EL1_DLK_LOCK     0x1

#define  HCR_EL2_RW_AARCH64     0x80000000

#define  ID_AA64PFR0_MASK_EL2    0xF00
#define  ID_AA64PFR0_EL2_64_ONLY 0x100
#define  ID_AA64PFR0_EL2_64OR32  0x200

#define  MPIDR_AFFINITY0_MASK   0x00FF
#define  MPIDR_AFFINITY0_OFFSET 0x0
#define  MPIDR_AFFINITY0_SIZE   0x8
#define  MPIDR_AFFINITY1_MASK   0xFF00
#define  MPIDR_AFFINITY1_OFFSET 0x8
#define  MPIDR_AFFINITY1_SIZE   0x8
#define  MPIDR_CORE_MASK        0x00FF
#define  MPIDR_CLUSTER_MASK     0xFF00

 // masks and constants for midr_el1
#define  MIDR_PARTNUM_MASK       0xFFF0
#define  MIDR_PARTNUM_START      4
#define  MIDR_PARTNUM_WIDTH      12
#define  MIDR_REVISION_START     0
#define  MIDR_REVISION_WIDTH     4
#define  MIDR_VARIANT_START      20
#define  MIDR_VARIANT_WIDTH      4
#define  MIDR_PARTNUM_A53        0xD03
#define  MIDR_PARTNUM_A57        0xD07
#define  MIDR_PARTNUM_A72        0xD08
#define  A53_DCACHE_RNPN_START   0x03

#define  SCR_NS_MASK            0x1
#define  SCR_FIQ_MASK           0x4
#define  SCR_IRQ_MASK           0x2
#define  SCR_HCE_MASK           0x100
#define  SCR_RW_MASK            0x400
#define  SCR_RW_AARCH64         0x400
#define  SCR_ST_MASK            0x800
#define  SCR_EL3_4_EL2_AARCH32  0x131
#define  SCR_EL3_4_EL1_AARCH32  0x031
#define  SCR_EL3_HCE_EN         0x100
#define  SCR_EL3_SIF_DIS        0x200
#define  SCR_EL3_FIQ_EN         0x4
#define  SCR_EL3_NS_MASK        0x1
#define  SCR_FIQ_BIT            SCR_FIQ_MASK
#define  SCR_IRQ_BIT            SCR_IRQ_MASK
#define  SCR_RW_BIT             SCR_RW_MASK
#define  SCR_NS_BIT             SCR_NS_MASK
#define  SCR_ST_BIT             SCR_ST_MASK
#define  SCR_HCE_BIT            SCR_HCE_MASK

#define  SPSEL_SP               0x1

#define  CLEAN_DCACHE           0x0
#define  CLN_INV_DCACHE         0x1

#define  CNTP_CTL_EL0_EN        0x1
#define  CNTP_CTL_EL0_IMASK     0x2
#define  CNTP_CTL_EL0_ISTAT     0x4

 // these spsr mode bits are somewhat abstract
#define   AMODE_AARCH64_EL2       0x0
#define   AMODE_AARCH64_EL1       0x1
#define   AMODE_AARCH32_EL2       0x2
#define   AMODE_AARCH32_EL1       0x3
#define   AMODE_AARCH_MASK        0x2
#define   AMODE_AARCH_64          0x0
#define   AMODE_AARCH_32          0x2
#define   AMODE_EL_MASK           0x1
#define   AMODE_EL_2              0x0
#define   AMODE_EL_1              0x1

 // these spsr mode bits more closely resemble the documentation
#define MODE_SP_SHIFT     0x0
#define MODE_SP_MASK      0x1
#define MODE_SP_EL0       0x0
#define MODE_SP_ELX       0x1
#define MODE_EL_MASK      0x3
#define MODE_EL_SHIFT     0x2
#define MODE_EL1          0x1
#define MODE_EL2          0x2
#define MODE_RW_SHIFT     0x4
#define MODE_RW_MASK      0x1
#define MODE_RW_64        0x0
#define MODE_RW_32        0x1
#define MODE32_SHIFT       0
#define MODE32_MASK       0xf
#define MODE32_hyp        0xa

#define ACTLR_EL3_CPUACTLR  0x1
#define ACTLR_EL3_L2ACTLR   0x40
#define ACTLR_EL3_L2ECTLR   0x20

#define ACTLR_EL2_CPUACTLR  0x1
#define ACTLR_EL2_L2ACTLR   0x40
#define ACTLR_EL2_L2ECTLR   0x20

//-----------------------------------------------------------------------------

#define CPUECTLR_EL1  S3_1_C15_C2_1
#define CPUACTLR_EL1  S3_1_C15_C2_0

//-----------------------------------------------------------------------------


#endif // _AARCH64_H
