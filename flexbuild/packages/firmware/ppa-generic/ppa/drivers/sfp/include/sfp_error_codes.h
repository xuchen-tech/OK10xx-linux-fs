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

#ifndef SFP_ERROR_CODES_H
#define SFP_ERROR_CODES_H

 // Error codes
#define ERROR_FUSE_BARKER           0x1
#define ERROR_READFB_CMD            0x2
#define ERROR_PROGFB_CMD            0x3
#define ERROR_SRKH_ALREADY_BLOWN    0x4
#define ERROR_SRKH_WRITE            0x5
#define ERROR_OEMUID_ALREADY_BLOWN  0x6
#define ERROR_OEMUID_WRITE          0x7
#define ERROR_DCV_ALREADY_BLOWN     0x8
#define ERROR_DCV_WRITE             0x9
#define ERROR_DRV_ALREADY_BLOWN     0xa
#define ERROR_DRV_HAMMING_ERROR     0xb
#define ERROR_OTPMK_ALREADY_BLOWN   0xc
#define ERROR_OTPMK_HAMMING_ERROR   0xd
#define ERROR_OTPMK_USER_MIN        0xe
#define ERROR_OSPR1_ALREADY_BLOWN   0xf
#define ERROR_OSPR1_WRITE           0x10
#define ERROR_SC_ALREADY_BLOWN      0x11
#define ERROR_SC_WRITE              0x12

#endif // SFP_ERROR_CODES_H
