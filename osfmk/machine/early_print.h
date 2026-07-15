#ifndef _MACHINE_EARLY_PRINT_H_
#define _MACHINE_EARLY_PRINT_H_

#if (defined(__riscv) && (__riscv_xlen == 32)) || defined(__RISCV32__)
#include "riscv32/early_print.h"
#else
#error architecture not supported
#endif

#endif /* _MACHINE_EARLY_PRINT_H_ */
