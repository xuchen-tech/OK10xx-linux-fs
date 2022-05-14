//-----------------------------------------------------------------------------
// 
// Copyright (c) 2015, Freescale Semiconductor, Inc.
// Copyright 2017-2018 NXP Semiconductors
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

#ifndef _POLICY_H
#define	_POLICY_H

 // the following defines affect the PLATFORM SECURITY POLICY

 // set to '1' to suppress accessing the crypto accelerator
 // NOTE: THIS WILL LOWER THE SECURITY CAPABILITIES OF THE PLATFORM
#define SUPPRESS_SEC    1

 // set to '1' to suppress initializing the tzpc and the tzasc
 // NOTE: THIS WILL LOWER THE SECURITY CAPABILITIES OF THE PLATFORM
#define SUPPRESS_TZC    1

 // set this to 0x0 if EL2 is Aarch32
 // set this to 0x1 if EL2 is Aarch64
 // sets SCR_EL3.RW (bit[10])
#define  POLICY_EL2_WIDTH 0x1

 // set this to 0x0 if EL2 is little endian (LE)
 // set this to 0x1 if EL2 is big endian (BE)
 // sets SCTLR_EL2.EE
#define  POLICY_EL2_EE 0x0

 // set this to 0x0 if EL1 is Aarch32
 // set this to 0x1 if EL1 is Aarch64
 // sets HCR_EL2.RW (bit[31])
#define  POLICY_EL1_WIDTH 0x1

 // set this to 0x0 if EL1 is little endian (LE)
 // set this to 0x1 if EL1 is big endian (BE)
 // sets SCTLR_EL1.EE
#define  POLICY_EL1_EE 0x0

 // set this to 0x0 if secure instruction fetch from non-secure memory is allowed
 // set this to 0x1 if secure instruction fetch from non-secure memory is prohibited
 // sets SCR_EL3.SIF (bit[9])
#define  POLICY_SIF_NS 0x0

 // set this to 0x0 if FIQ interrupts are not reserved for EL3
 // set this to 0x1 if FIQ interrupts are reserved for EL3
 // sets SCR_EL3.FIQ (bit[2])
#define  POLICY_FIQ_EL3 0x1

 // set this to 0x0 if the platform is not using/responding to ECC errors
 // set this to 0x1 if ECC is being used (we have to do some init)
#define  POLICY_USING_ECC 0x0

//-----------------------------------------------------------------------------

#endif // _POLICY_H
