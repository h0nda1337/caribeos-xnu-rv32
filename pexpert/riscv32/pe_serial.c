/*
 * SPDX-License-Identifier: APSL-2.0
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * https://opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 * CARIBEOS MODIFICATION NOTICE
 * This file was created as part of the experimental CaribeOS RV32IMACSU
 * port of Apple XNU 2050.48.11 on 2026-07-14.
 */

#line 1
#include <mach/boolean.h>
#include <machine/bootinfo.h>
#include <stdint.h>

#define RISCV32_UART_DEFAULT	0x10000000U
#define UART_THR		0x00U
#define UART_RBR		0x00U
#define UART_LSR		0x05U
#define UART_LSR_DR		0x01U
#define UART_LSR_THRE		0x20U

static uint32_t
riscv32_uart_base(void)
{
	const riscv32_bootinfo_t *bi = riscv32_bootinfo();

	if (bi != (const riscv32_bootinfo_t *)0 && bi->uart_base != 0) {
		return bi->uart_base;
	}
	return RISCV32_UART_DEFAULT;
}

static uint8_t
uart_read(uint32_t offset)
{
	return *(volatile uint8_t *)(uintptr_t)(riscv32_uart_base() + offset);
}

static void
uart_write(uint32_t offset, uint8_t value)
{
	*(volatile uint8_t *)(uintptr_t)(riscv32_uart_base() + offset) = value;
}

int
serial_init(void)
{
	return 0;
}

void
serial_putc(char c)
{
	while ((uart_read(UART_LSR) & UART_LSR_THRE) == 0) {
	}
	uart_write(UART_THR, (uint8_t)c);
}

int
serial_getc(void)
{
	if ((uart_read(UART_LSR) & UART_LSR_DR) == 0) {
		return -1;
	}
	return uart_read(UART_RBR);
}

void
cninit(void)
{
	(void)serial_init();
}

void
cnputc(char c)
{
	if (c == '\n') {
		serial_putc('\r');
	}
	serial_putc(c);
}

void
cnputc_unbuffered(char c)
{
	cnputc(c);
}

void
cnputcusr(char c)
{
	cnputc(c);
}

int
cngetc(void)
{
	return serial_getc();
}

int
switch_to_serial_console(void)
{
	return 0;
}

void
switch_to_old_console(int old_console)
{
	(void)old_console;
}

boolean_t
console_is_serial(void)
{
	return TRUE;
}
