#ifndef _MACHINE_USER_EXCEPTION_H_
#define _MACHINE_USER_EXCEPTION_H_

#if (defined(__riscv) && (__riscv_xlen == 32)) || defined(__RISCV32__)
#include "riscv32/user_exception.h"
#else
#error architecture not supported
#endif

#endif /* _MACHINE_USER_EXCEPTION_H_ */
