#ifndef _MACHINE_HFSPLUS_H_
#define _MACHINE_HFSPLUS_H_

#if (defined(__riscv) && (__riscv_xlen == 32)) || defined(__RISCV32__)
#include "riscv32/hfsplus.h"
#else
#error architecture not supported
#endif

#endif /* _MACHINE_HFSPLUS_H_ */
