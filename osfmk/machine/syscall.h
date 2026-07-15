#ifndef _MACHINE_SYSCALL_H_
#define _MACHINE_SYSCALL_H_

#if (defined(__riscv) && (__riscv_xlen == 32)) || defined(__RISCV32__)
#include "riscv32/syscall.h"
#else
#error architecture not supported
#endif

#endif /* _MACHINE_SYSCALL_H_ */
