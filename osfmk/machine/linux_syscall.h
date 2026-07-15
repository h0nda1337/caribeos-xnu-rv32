#ifndef _MACHINE_LINUX_SYSCALL_H_
#define _MACHINE_LINUX_SYSCALL_H_

#if (defined(__riscv) && (__riscv_xlen == 32)) || defined(__RISCV32__)
#include "riscv32/linux_syscall.h"
#else
#error architecture not supported
#endif

#endif /* _MACHINE_LINUX_SYSCALL_H_ */
