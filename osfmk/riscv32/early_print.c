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
 * 2026-10-12: serialize console writes with an IRQ-safe RV32A lock.
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

static volatile uint32_t early_console_lock;

static uint32_t
early_console_acquire(void)
{
	uint32_t status;
	/* No cpu_data dependency: this console also runs before CPU bootstrap. */
	__asm__ volatile("csrrc %0, sstatus, %1" : "=r"(status) : "r"(2U) : "memory");
	while (__sync_lock_test_and_set(&early_console_lock, 1U) != 0U) {
		__asm__ volatile("nop");
	}
	__asm__ volatile("fence iorw, iorw" ::: "memory");
	return status;
}

static void
early_console_release(uint32_t status)
{
	__asm__ volatile("fence iorw, iorw" ::: "memory");
	__sync_lock_release(&early_console_lock);
	if ((status & 2U) != 0U) {
		__asm__ volatile("csrs sstatus, %0" :: "r"(2U) : "memory");
	}
}

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

static void
early_putc_unlocked(char c)
{
	const riscv32_bootinfo_t *bootinfo;

	if (c == '\n') {
		early_putc_unlocked('\r');
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
riscv32_early_write(const char *bytes, uint32_t count)
{
	uint32_t status, i;
	if (bytes == (const char *)0 || count == 0U) return;
	status = early_console_acquire();
	for (i = 0U; i < count; i++) early_putc_unlocked(bytes[i]);
	early_console_release(status);
}

void
riscv32_early_putc(char c)
{
	riscv32_early_write(&c, 1U);
}

void
riscv32_early_puts(const char *str)
{
	uint32_t count = 0U;
	if (str == (const char *)0) return;
	while (str[count] != '\0') count++;
	riscv32_early_write(str, count);
}

void
riscv32_early_puthex32(uint32_t value)
{
	static const char digits[] = "0123456789abcdef";
	char buf[8];
	int i;

	for (i = 7; i >= 0; i--) {
		buf[7 - i] = digits[(value >> (i * 4)) & 0xfU];
	}
	riscv32_early_write(buf, sizeof(buf));
}

void
riscv32_early_putdec(uint32_t value)
{
	char buf[10];
	int i = 0;
	uint32_t status;

	if (value == 0) {
		riscv32_early_putc('0');
		return;
	}
	while (value != 0 && i < (int)sizeof(buf)) {
		buf[i++] = (char)('0' + value % 10U);
		value /= 10U;
	}
	status = early_console_acquire();
	while (i > 0) early_putc_unlocked(buf[--i]);
	early_console_release(status);
}
