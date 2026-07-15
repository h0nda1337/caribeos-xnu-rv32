#ifndef _RISCV32_EARLY_PRINT_H_
#define _RISCV32_EARLY_PRINT_H_

#include <stdint.h>

void riscv32_early_putc(char c);
void riscv32_early_puts(const char *str);
void riscv32_early_puthex32(uint32_t value);
void riscv32_early_putdec(uint32_t value);

#endif /* _RISCV32_EARLY_PRINT_H_ */
