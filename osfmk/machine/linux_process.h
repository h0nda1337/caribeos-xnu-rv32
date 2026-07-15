#ifndef _MACHINE_LINUX_PROCESS_H_
#define _MACHINE_LINUX_PROCESS_H_

#if (defined(__riscv) && (__riscv_xlen == 32)) || defined(__RISCV32__)
#include "riscv32/linux_process.h"
#else
#error architecture not supported
#endif

#endif /* _MACHINE_LINUX_PROCESS_H_ */
