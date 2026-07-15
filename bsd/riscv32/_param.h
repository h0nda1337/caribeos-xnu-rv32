#ifndef _BSD_RISCV32__PARAM_H_
#define _BSD_RISCV32__PARAM_H_

#include <riscv32/_types.h>

#define __DARWIN_ALIGNBYTES	(sizeof(__darwin_size_t) - 1)
#define __DARWIN_ALIGN(p)	((__darwin_size_t)((char *)(__darwin_size_t)(p) + __DARWIN_ALIGNBYTES) & ~__DARWIN_ALIGNBYTES)

#define __DARWIN_ALIGNBYTES32	(sizeof(__uint32_t) - 1)
#define __DARWIN_ALIGN32(p)	((__darwin_size_t)((char *)(__darwin_size_t)(p) + __DARWIN_ALIGNBYTES32) & ~__DARWIN_ALIGNBYTES32)

#endif /* _BSD_RISCV32__PARAM_H_ */
