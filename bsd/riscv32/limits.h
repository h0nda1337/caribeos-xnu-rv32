#ifndef _BSD_RISCV32_LIMITS_H_
#define _BSD_RISCV32_LIMITS_H_

#include <sys/cdefs.h>
#include <riscv32/_limits.h>

#define CHAR_BIT	8
#define MB_LEN_MAX	6

#if !defined(_ANSI_SOURCE) && (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
#define CLK_TCK		__DARWIN_CLK_TCK
#endif

#define SCHAR_MAX	127
#define SCHAR_MIN	(-128)
#define UCHAR_MAX	255
#define CHAR_MAX	127
#define CHAR_MIN	(-128)

#define USHRT_MAX	65535
#define SHRT_MAX	32767
#define SHRT_MIN	(-32768)

#define UINT_MAX	0xffffffff
#define INT_MAX		2147483647
#define INT_MIN		(-2147483647 - 1)

#define ULONG_MAX	0xffffffffUL
#define LONG_MAX	2147483647L
#define LONG_MIN	(-2147483647L - 1)

#define ULLONG_MAX	0xffffffffffffffffULL
#define LLONG_MAX	0x7fffffffffffffffLL
#define LLONG_MIN	(-0x7fffffffffffffffLL - 1)

#if !defined(_ANSI_SOURCE)
#define LONG_BIT	32
#define SSIZE_MAX	LONG_MAX
#define WORD_BIT	32

#if (!defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)) || defined(_DARWIN_C_SOURCE)
#define SIZE_T_MAX	ULONG_MAX
#define UQUAD_MAX	ULLONG_MAX
#define QUAD_MAX	LLONG_MAX
#define QUAD_MIN	LLONG_MIN
#endif
#endif

#endif /* _BSD_RISCV32_LIMITS_H_ */
