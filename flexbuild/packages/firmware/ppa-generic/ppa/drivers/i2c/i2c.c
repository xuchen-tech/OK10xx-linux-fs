/*

 * All rights reserved.
 *

 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of NXP Semiconductors nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY NXP Semiconductors "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NXP Semiconductors BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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

#include "lib.h"
#include "i2c.h"
#include "io.h"
#include "timer.h"

void i2c_init(void)
{
	struct ls_i2c *ccsr_i2c = (void *)CONFIG_SYS_I2C_BASE;

	/* Presume workaround for erratum a009203 applied */
	i2c_out(&ccsr_i2c->cr, I2C_CR_DIS);
	i2c_out(&ccsr_i2c->fd, I2C_FD_CONSERV);
	i2c_out(&ccsr_i2c->sr, I2C_SR_RST);
	i2c_out(&ccsr_i2c->cr, I2C_CR_EN);
}

static int wait_for_state(struct ls_i2c *ccsr_i2c,
			  unsigned char state, unsigned char mask)
{
	unsigned char sr;
	unsigned long start_time = get_timer(0);
	unsigned long timer;

	do {
		sr = i2c_in(&ccsr_i2c->sr);
		if (sr & I2C_SR_AL) {
			i2c_out(&ccsr_i2c->sr, sr);
			debug("I2C arbitration lost\n");
			return -ERSTART;
		}
		if ((sr & mask) == state) {
			return sr;
		}
		timer = get_timer(start_time);
		if (timer > I2C_TIMEOUT)
			break;
		mdelay(1);
	} while (1);
	debug("I2C: Timeout waiting for state ");
	dbgprint_hex(state);
	debug(", sr=0x");
	dbgprint_hex(sr);
	debug("\n");

	return -ETIME;
}

static int tx_byte(struct ls_i2c *ccsr_i2c, unsigned char c)
{
	int ret;

	i2c_out(&ccsr_i2c->sr, I2C_SR_IF);
	i2c_out(&ccsr_i2c->dr, c);
	ret = wait_for_state(ccsr_i2c, I2C_SR_IF, I2C_SR_IF);
	if (ret < 0) {
		debug_int("tx_byte error ", __LINE__);
		return ret;
	}
	if (ret & I2C_SR_RX_NAK) {
		debug_int("tx_byte nodev ", __LINE__);
		return -ENODEV;
	}

	return 0;
}

static void gen_stop(struct ls_i2c *ccsr_i2c)
{
	unsigned char cr;

	cr = i2c_in(&ccsr_i2c->cr);
	cr &= ~(I2C_CR_MA | I2C_CR_TX);
	i2c_out(&ccsr_i2c->cr, cr);
	if (wait_for_state(ccsr_i2c, I2C_SR_IDLE, I2C_SR_BB) < 0)
		debug("I2C: generating stop failed\n");
}

static int i2c_write_addr(struct ls_i2c *ccsr_i2c, unsigned char chip,
			int addr, int alen)
{
	int ret;
	unsigned char cr;

	if (i2c_in(&ccsr_i2c->ad) == (chip << 1)) {
		debug("I2C: slave address same as self\n");
		return -ENODEV;
	}
	i2c_out(&ccsr_i2c->sr, I2C_SR_IF);
	ret = wait_for_state(ccsr_i2c, I2C_SR_IDLE, I2C_SR_BB);
	if (ret < 0)
		return ret;

	cr = i2c_in(&ccsr_i2c->cr);
	cr |= I2C_CR_MA;
	i2c_out(&ccsr_i2c->cr, cr);
	ret = wait_for_state(ccsr_i2c, I2C_SR_BB, I2C_SR_BB);
	if (ret < 0)
		return ret;

	debug_int("before writing chip ", __LINE__);
	debug_int("chip number ", chip);
	cr |= I2C_CR_TX | I2C_CR_TX_NAK;
	i2c_out(&ccsr_i2c->cr, cr);
	ret = tx_byte(ccsr_i2c, chip << 1);
	if (ret < 0) {
		gen_stop(ccsr_i2c);
		return ret;
	}

	debug_int("before writing addr", __LINE__);
	while (alen--) {
		ret = tx_byte(ccsr_i2c, (addr >> (alen << 3)) & 0xff);
		if (ret < 0) {
			gen_stop(ccsr_i2c);
			return ret;
		}
	}

	return 0;
}

static int read_data(struct ls_i2c *ccsr_i2c, unsigned char chip,
		     unsigned char *buf, int len)
{
	int i;
	int ret;
	unsigned char cr;

	cr = i2c_in(&ccsr_i2c->cr);
	cr &= ~(I2C_CR_TX | I2C_CR_TX_NAK);
	if (len == 1)
		cr |= I2C_CR_TX_NAK;
	i2c_out(&ccsr_i2c->cr, cr);
	i2c_out(&ccsr_i2c->sr, I2C_SR_IF);
	i2c_in(&ccsr_i2c->dr);	/* dummy read */
	for (i = 0; i < len; i++) {
		ret = wait_for_state(ccsr_i2c, I2C_SR_IF, I2C_SR_IF);
		if (ret < 0) {
			gen_stop(ccsr_i2c);
			return ret;
		}
		if (i == (len - 1)) {
			gen_stop(ccsr_i2c);
		} else if (i == (len - 2)) {
			cr = i2c_in(&ccsr_i2c->cr);
			cr |= I2C_CR_TX_NAK;
			i2c_out(&ccsr_i2c->cr, cr);
		}
		i2c_out(&ccsr_i2c->sr, I2C_SR_IF);
		buf[i] = i2c_in(&ccsr_i2c->dr);
	}

	return 0;
}

static int write_data(struct ls_i2c *ccsr_i2c, unsigned char chip,
		      const unsigned char *buf, int len)
{
	int i;
	int ret = 0;

	for (i = 0; i < len; i++) {
		ret = tx_byte(ccsr_i2c, buf[i]);
		if (ret < 0)
			break;
	}
	gen_stop(ccsr_i2c);

	return ret;
}


int i2c_read(unsigned char chip, int addr, int alen, unsigned char *buf, int len)
{
	int ret;
	unsigned char cr;
	struct ls_i2c *ccsr_i2c = (void *)CONFIG_SYS_I2C_BASE;

	ret = i2c_write_addr(ccsr_i2c, chip, addr, alen);
	if (ret < 0) {
		gen_stop(ccsr_i2c);
		return ret;
	}

	cr = i2c_in(&ccsr_i2c->cr);
	cr |= I2C_CR_RSTA;
	i2c_out(&ccsr_i2c->cr, cr);

	ret = tx_byte(ccsr_i2c, (chip << 1) | 1);
	if (ret < 0) {
		gen_stop(ccsr_i2c);
		return ret;
	}

	return read_data(ccsr_i2c, chip, buf, len);
}

int i2c_write(unsigned char chip, int addr, int alen, const unsigned char *buf, int len)
{
	int ret;
	struct ls_i2c *ccsr_i2c = (void *)CONFIG_SYS_I2C_BASE;

	ret = i2c_write_addr(ccsr_i2c, chip, addr, alen);
	if (ret < 0)
		return ret;

	return write_data(ccsr_i2c, chip, buf, len);
}

int i2c_probe_chip(unsigned char chip)
{
	int ret;
	struct ls_i2c *ccsr_i2c = (void *)CONFIG_SYS_I2C_BASE;

	ret = i2c_write_addr(ccsr_i2c, chip, 0, 0);
	if (ret < 0) {
		debug_int("write addr failed", __LINE__);
		return ret;
	}

	gen_stop(ccsr_i2c);

	return 0;
}
