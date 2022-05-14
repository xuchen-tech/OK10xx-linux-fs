//-----------------------------------------------------------------------------
// 
// Copyright (c) 2015, 2016 Freescale Semiconductor, Inc.
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
// Author Rod Dorris <rod.dorris@nxp.com>
// 
//-----------------------------------------------------------------------------

#ifndef _SOC_H
#define	_SOC_H

//-----------------------------------------------------------------------------

#define CPUECTLR_EL1  S3_1_C15_C2_1

//-----------------------------------------------------------------------------

.equ MAX_SOC_CORES,     0x4
.equ DISABLE_RETRY_CNT, 800
.equ CCI400_PEND_CNT,   800

 // base addresses
.equ  RESET_BASE_ADDR,       0x01E60000
.equ  DCFG_BASE_ADDR,        0x01E00000
.equ  TIMER_BASE_ADDR,       0x023E0000
.equ  PMU_BASE_ADDR,         0x01E30000
.equ  SEC_REGFILE_BASE_ADDR, 0x01E88000
.equ  CCI_400_BASE_ADDR,     0x04090000

 // register offsets -------------

 // dcfg block registers
.equ COREDISABLEDSR_OFFSET, 0x990
.equ SCRATCHRW7_OFFSET,     0x218
.equ SVR_OFFSET,            0x0A4
.equ GENCR1_OFFSET,         0x620

 // reset block registers
.equ BOOTLOCPTRL_OFFSET,    0x400
.equ BOOTLOCPTRH_OFFSET,    0x404
.equ BRCORENBR_OFFSET,      0x090
.equ BRR_OFFSET,            0x060
.equ RCWSR9_OFFSET,         0x120

 // pmu register offsets
.equ CLTBENR_OFFSET,        0x18A0

 // timer register offsets
.equ CNTCR_OFFSET,          0x0
.equ CNTCV_OFFSET,          0x8
.equ CNTFID0_OFFSET,        0x020

 // secure register file offsets
.equ TZSECURECR_OFFSET,         0x0
.equ TZGDCR_OFFSET,             0x80
.equ TZCLKOVRDR_OFFSET,         0x100
.equ CORE_HOLD_OFFSET,          0x140

 // device base addresses
.equ DEV_PCIEX1_BASE,      0x2000000000
.equ DEV_MEMCMPLX_BASE_01, 0x0080000000
.equ DEV_MEMCMPLX_BASE_02, 0x8080000000
.equ DEV_OCRAM_BASE,       0x0018000000
.equ DEV_IFC_BASE,         0x0030000000
.equ DEV_SERNOR_BASE,      0x0020000000

 // cci-400 register offsets
.equ SNOOP_CNTRL_SLV0,     0x1000
.equ SNOOP_CNTRL_SLV1,     0x2000
.equ SNOOP_CNTRL_SLV2,     0x3000
.equ SNOOP_CNTRL_SLV3,     0x4000
.equ SNOOP_CNTRL_SLV4,     0x5000
.equ SNOOP_STATUS,         0x000C

 // cci-400 register bit masks
.equ SNOOP_CNTRL_SNP_EN,   0x1         // enable snooping on this interface
.equ SNOOP_CNTRL_DVM_EN,   0x2         // enable dvm messaging on this interface
.equ SNOOP_CNTRL_SNP_SUPP, 0x40000000  // interface supports snooping
.equ SNOOP_CNTRL_DVM_SUPP, 0x80000000  // interface supports dvm messaging
.equ STATUS_PENDING,       0x1

//-----------------------------------------------------------------------------

.equ  SCTLR_EL3_RES1,         0x30C50830       
.equ  SCTLR_EL2_RES1,         0x30C50830       
.equ  SCTLR_EL1_RES1,         0x30D00800       
.equ  SCTLR_SA_MASK,          0x00000008
.equ  SCTLR_I_MASK,           0x00001000
.equ  SCTLR_C_MASK,           0x00000004
.equ  SCTLR_A_MASK,           0x00000002
.equ  SCTLR_M_MASK,           0x00000001

.equ  CPUECTLR_SMPEN_EN,      0x40
.equ  CPUECTLR_TIMER_8TICKS,  0x2

.equ  SCR_EL3_RES1,           0x30
.equ  SCR_EL3_NS_MASK,        0x1
.equ  SCR_EL3_HCE_MASK,       0x100
.equ  SCR_EL3_RW_MASK,        0x400

.equ  HCR_EL2_RW_AARCH64,     0x80000000

.equ  CPTR_EL2_RES1,          0x33FF

.equ  DAIF_SET_MASK,          0x3C0

.equ  OSDLR_EL1_DLK_MASK,     0x1
.equ  OSDLR_EL1_DLK_LOCK,     0x1

//-----------------------------------------------------------------------------

 // this macro reads MPIDR_EL1 and extracts the lo-order 16 bits
 // the parameters to this macro MUST be 64-bit registers
.macro Get_MPIDR_EL1 $p1, $p2

     // read mp affinity reg (MPIDR_EL1)
    mrs \$p1, MPIDR_EL1
     // extract the affinity 0 & 1 fields - bits [15:0]
    mov   \$p2, xzr
    bfxil \$p2, \$p1, #0, #16
.endm

//-----------------------------------------------------------------------------

#endif // _SOC_H
