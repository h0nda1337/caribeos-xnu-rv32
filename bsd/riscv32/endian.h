#ifndef _BSD_RISCV32_ENDIAN_H_
#define _BSD_RISCV32_ENDIAN_H_

#include <sys/cdefs.h>

#define _QUAD_HIGHWORD	1
#define _QUAD_LOWWORD	0

#define __DARWIN_LITTLE_ENDIAN	1234
#define __DARWIN_BIG_ENDIAN	4321
#define __DARWIN_PDP_ENDIAN	3412

#define __DARWIN_BYTE_ORDER	__DARWIN_LITTLE_ENDIAN

#if defined(KERNEL) || (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))

#define LITTLE_ENDIAN	__DARWIN_LITTLE_ENDIAN
#define BIG_ENDIAN	__DARWIN_BIG_ENDIAN
#define PDP_ENDIAN	__DARWIN_PDP_ENDIAN
#define BYTE_ORDER	__DARWIN_BYTE_ORDER

#include <sys/_endian.h>

#endif

#endif /* _BSD_RISCV32_ENDIAN_H_ */
