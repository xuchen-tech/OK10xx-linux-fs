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
// Author York Sun <york.sun@nxp.com>
// 
//-----------------------------------------------------------------------------

#ifndef __I2C_H__
#define __I2C_H__

#define I2C_TIMEOUT	1000	/* ms */

#define I2C_FD_CONSERV	0x7e
#define I2C_CR_DIS	(1 << 7)
#define I2C_CR_EN	(0 << 7)
#define I2C_CR_MA	(1 << 5)
#define I2C_CR_TX	(1 << 4)
#define I2C_CR_TX_NAK	(1 << 3)
#define I2C_CR_RSTA	(1 << 2)
#define I2C_SR_BB	(1 << 5)
#define I2C_SR_IDLE	(0 << 5)
#define I2C_SR_AL	(1 << 4)
#define I2C_SR_IF	(1 << 1)
#define I2C_SR_RX_NAK	(1 << 0)
#define I2C_SR_RST	(I2C_SR_AL | I2C_SR_IF)

#define I2C_GLITCH_EN 0x8

struct ls_i2c {
	unsigned char ad;
	unsigned char fd;
	unsigned char cr;
	unsigned char sr;
	unsigned char dr;
	unsigned char ic;
	unsigned char dbg;
};

void i2c_init(void);
int i2c_read(unsigned char chip, int addr, int alen, unsigned char *buf, int len);
int i2c_write(unsigned char chip, int addr, int alen, const unsigned char *buf, int len);
int i2c_probe_chip(unsigned char chip);

#endif /* __I2C_H__ */
