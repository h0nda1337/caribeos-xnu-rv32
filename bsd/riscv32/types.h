#ifndef _BSD_RISCV32_TYPES_H_
#define _BSD_RISCV32_TYPES_H_

#ifndef __ASSEMBLER__
#include <riscv32/_types.h>
#include <sys/cdefs.h>

#ifndef _INT8_T
#define _INT8_T
typedef __signed char		int8_t;
#endif
typedef unsigned char		u_int8_t;

#ifndef _INT16_T
#define _INT16_T
typedef short			int16_t;
#endif
typedef unsigned short		u_int16_t;

#ifndef _INT32_T
#define _INT32_T
typedef int			int32_t;
#endif
typedef unsigned int		u_int32_t;

#ifndef _INT64_T
#define _INT64_T
typedef long long		int64_t;
#endif
typedef unsigned long long	u_int64_t;

typedef int32_t			register_t;

#ifndef _INTPTR_T
#define _INTPTR_T
typedef __darwin_intptr_t	intptr_t;
#endif

#ifndef _UINTPTR_T
#define _UINTPTR_T
typedef unsigned long		uintptr_t;
#endif

#if !defined(_ANSI_SOURCE) && (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
typedef u_int64_t		user_addr_t;
typedef u_int64_t		user_size_t;
typedef int64_t			user_ssize_t;
typedef int64_t			user_long_t;
typedef u_int64_t		user_ulong_t;
typedef int64_t			user_time_t;
typedef int64_t			user_off_t;
#define USER_ADDR_NULL	((user_addr_t)0)
#define CAST_USER_ADDR_T(a_ptr) ((user_addr_t)((uintptr_t)(a_ptr)))

#ifdef KERNEL
typedef __uint64_t		user64_addr_t __attribute__((aligned(8)));
typedef __uint64_t		user64_size_t __attribute__((aligned(8)));
typedef __int64_t		user64_ssize_t __attribute__((aligned(8)));
typedef __int64_t		user64_long_t __attribute__((aligned(8)));
typedef __uint64_t		user64_ulong_t __attribute__((aligned(8)));
typedef __int64_t		user64_time_t __attribute__((aligned(8)));
typedef __int64_t		user64_off_t __attribute__((aligned(8)));

typedef __uint32_t		user32_addr_t;
typedef __uint32_t		user32_size_t;
typedef __int32_t		user32_ssize_t;
typedef __int32_t		user32_long_t;
typedef __uint32_t		user32_ulong_t;
typedef __int32_t		user32_time_t;
typedef __int64_t		user32_off_t __attribute__((aligned(4)));
#endif /* KERNEL */

#endif /* !_ANSI_SOURCE && (!_POSIX_C_SOURCE || _DARWIN_C_SOURCE) */

typedef u_int64_t		syscall_arg_t;

#ifndef __offsetof
#define __offsetof(type, field) ((size_t)(&((type *)0)->field))
#endif

#endif /* __ASSEMBLER__ */

#endif /* _BSD_RISCV32_TYPES_H_ */
