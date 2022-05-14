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
#include "plat.h"
#include "io.h"
#include "uart.h"

#if DDR_INIT
static void uart_setbaudrate(struct uart *ccsr_uart, int divisor)
{
    uart_out(&ccsr_uart->lcr, UART_LCR_DLAB | UART_LCR_INIT);
    uart_out(&ccsr_uart->dlb, divisor & 0xff);
    uart_out(&ccsr_uart->dmb, divisor >> 8);
    uart_out(&ccsr_uart->lcr, UART_LCR_INIT);
}
void print_version(void)
{
    puts("\nPPA git-" VERSION "\n");
}

void uart_init(void)
{
    struct uart *ccsr_uart = (void *)UART_BASE;

    /* Wait for transitter empty */
    while (!(uart_in(&ccsr_uart->lsr) & UART_LSR_TEMT_MASK))
        ;

    uart_out(&ccsr_uart->ier, UART_IER_INIT);
    uart_out(&ccsr_uart->mcr, UART_MCR_INIT);
    uart_out(&ccsr_uart->fcr, UART_FCR_INIT);
    uart_setbaudrate(ccsr_uart, UART_BAUD_DIV);

    print_version();
}
#endif

void putc(const char c)
{
    struct uart *ccsr_uart = (void *)UART_BASE;

    while(!(uart_in(&ccsr_uart->lsr) & UART_LSR_THRE_MASK))
        ;

    if (c == '\n')
        uart_out(&ccsr_uart->thr, '\r');
    uart_out(&ccsr_uart->thr, c);
}

void puts(const char *s)
{
    while(*s)
        putc(*s++);
}

#define PRINTF_BUF_LEN 65    /* max 64-bit data */
static void u64_to_asc(unsigned long long n, int base)
{
    char buf[PRINTF_BUF_LEN];
    char *ptr;
    unsigned long long r;

    if (base != 10 && base != 16)
        return;

    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        puts(buf);
        return;
    }

    /* process backward */
    ptr = buf + PRINTF_BUF_LEN - 1;
    *ptr = '\0';
    while (n) {
        r = n % base;
        *--ptr = ((base == 16 && r > 9) ? 'A' - 10 : '0') + r;
        n /= base;
    }
    puts(ptr);
}

void print_uint(unsigned long long n)
{
    u64_to_asc(n, 10);
}

void print_hex(unsigned long long n)
{
    u64_to_asc(n, 16);
}
