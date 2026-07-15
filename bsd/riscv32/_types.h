/*
 * SPDX-License-Identifier: APSL-2.0
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * https://opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 * CARIBEOS MODIFICATION NOTICE
 * This file was created as part of the experimental CaribeOS RV32IMACSU
 * port of Apple XNU 2050.48.11 on 2026-07-14.
 */

#line 1
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
