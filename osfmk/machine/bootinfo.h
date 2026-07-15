#ifndef _MACHINE_BOOTINFO_H_
#define _MACHINE_BOOTINFO_H_

#if (defined(__riscv) && (__riscv_xlen == 32)) || defined(__RISCV32__)
#include "riscv32/bootinfo.h"
#else
#error architecture not supported
#endif

#endif /* _MACHINE_BOOTINFO_H_ */
