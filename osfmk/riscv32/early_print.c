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
