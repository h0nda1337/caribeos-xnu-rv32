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
#include <machine/bootinfo.h>
#include <riscv32/early_print.h>
#include <riscv32/sbi.h>
#include <stdint.h>

#define RISCV32_UART_DEFAULT	0x10000000U
#define UART_THR		0x00U
#define UART_LSR		0x05U
#define UART_LSR_THRE		0x20U

static uint32_t
early_uart_base(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();

	if (bootinfo != (const riscv32_bootinfo_t *)0 &&
	    bootinfo->uart_base != 0) {
		return bootinfo->uart_base;
	}
	return RISCV32_UART_DEFAULT;
}

static uint8_t
early_uart_read(uint32_t offset)
{
	return *(volatile uint8_t *)(uintptr_t)(early_uart_base() + offset);
}

static void
early_uart_write(uint32_t offset, uint8_t value)
{
	*(volatile uint8_t *)(uintptr_t)(early_uart_base() + offset) = value;
}

void
riscv32_early_putc(char c)
{
	const riscv32_bootinfo_t *bootinfo;

	if (c == '\n') {
		riscv32_early_putc('\r');
	}
	bootinfo = riscv32_bootinfo();
	if (bootinfo != (const riscv32_bootinfo_t *)0 &&
	    bootinfo->uart_base == 0 &&
	    (bootinfo->sbi_extensions & RISCV32_SBI_EXTBIT_DBCN) != 0) {
		(void)riscv32_sbi_debug_console_write_byte((uint8_t)c);
		return;
	}
	while ((early_uart_read(UART_LSR) & UART_LSR_THRE) == 0) {
	}
	early_uart_write(UART_THR, (uint8_t)c);
}

void
riscv32_early_puts(const char *str)
{
	while (str != (const char *)0 && *str != '\0') {
		riscv32_early_putc(*str++);
	}
}

void
riscv32_early_puthex32(uint32_t value)
{
	static const char digits[] = "0123456789abcdef";
	int i;

	for (i = 7; i >= 0; i--) {
		riscv32_early_putc(digits[(value >> (i * 4)) & 0xfU]);
	}
}

void
riscv32_early_putdec(uint32_t value)
{
	char buf[10];
	int i = 0;

	if (value == 0) {
		riscv32_early_putc('0');
		return;
	}
	while (value != 0 && i < (int)sizeof(buf)) {
		buf[i++] = (char)('0' + value % 10U);
		value /= 10U;
	}
	while (i > 0) {
		riscv32_early_putc(buf[--i]);
	}
}
