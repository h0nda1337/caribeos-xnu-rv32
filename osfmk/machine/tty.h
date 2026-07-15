#ifndef _MACHINE_TTY_H_
#define _MACHINE_TTY_H_

#if (defined(__riscv) && (__riscv_xlen == 32)) || defined(__RISCV32__)
#include "riscv32/tty.h"
#else
#error architecture not supported
#endif

#endif /* _MACHINE_TTY_H_ */
