#ifndef _MACHINE_UMODE_SMOKE_H_
#define _MACHINE_UMODE_SMOKE_H_

#if (defined(__riscv) && (__riscv_xlen == 32)) || defined(__RISCV32__)
#include "riscv32/umode_smoke.h"
#else
#error architecture not supported
#endif

#endif /* _MACHINE_UMODE_SMOKE_H_ */
