#ifndef _BSD_RISCV32__TYPES_H_
#define _BSD_RISCV32__TYPES_H_

#ifdef __GNUC__
typedef __signed char		__int8_t;
#else
typedef char			__int8_t;
#endif
typedef unsigned char		__uint8_t;
typedef short			__int16_t;
typedef unsigned short		__uint16_t;
typedef int			__int32_t;
typedef unsigned int		__uint32_t;
typedef long long		__int64_t;
typedef unsigned long long	__uint64_t;

typedef long			__darwin_intptr_t;
typedef unsigned int		__darwin_natural_t;
typedef int			__darwin_ct_rune_t;

typedef union {
	char		__mbstate8[128];
	long long	_mbstateL;
} __mbstate_t;

typedef __mbstate_t		__darwin_mbstate_t;

#if defined(__GNUC__) && defined(__PTRDIFF_TYPE__)
typedef __PTRDIFF_TYPE__	__darwin_ptrdiff_t;
#else
typedef int			__darwin_ptrdiff_t;
#endif

#if defined(__GNUC__) && defined(__SIZE_TYPE__)
typedef __SIZE_TYPE__		__darwin_size_t;
#else
typedef unsigned long		__darwin_size_t;
#endif

#if (__GNUC__ > 2)
typedef __builtin_va_list	__darwin_va_list;
#else
typedef void *			__darwin_va_list;
#endif

#if defined(__GNUC__) && defined(__WCHAR_TYPE__)
typedef __WCHAR_TYPE__		__darwin_wchar_t;
#else
typedef __darwin_ct_rune_t	__darwin_wchar_t;
#endif

typedef __darwin_wchar_t	__darwin_rune_t;

#if defined(__GNUC__) && defined(__WINT_TYPE__)
typedef __WINT_TYPE__		__darwin_wint_t;
#else
typedef __darwin_ct_rune_t	__darwin_wint_t;
#endif

typedef unsigned long		__darwin_clock_t;
typedef __uint32_t		__darwin_socklen_t;
typedef long			__darwin_ssize_t;
typedef long			__darwin_time_t;

#endif /* _BSD_RISCV32__TYPES_H_ */
