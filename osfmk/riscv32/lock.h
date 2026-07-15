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
#ifdef KERNEL_PRIVATE

#ifndef _RISCV32_LOCK_H_
#define _RISCV32_LOCK_H_

#include <sys/appleapiopts.h>

#ifdef __APPLE_API_PRIVATE

#ifdef MACH_KERNEL_PRIVATE

#include <kern/macro_help.h>
#include <kern/assert.h>
#include <riscv32/locks.h>
#include <stdint.h>

typedef lck_rw_t lock_t;

#define bit_lock(bit,l)							\
	MACRO_BEGIN							\
		uint32_t __mask = (uint32_t)(1U << (bit));		\
		while (__sync_fetch_and_or((volatile uint32_t *)(l),	\
		    __mask) & __mask) {					\
		}							\
	MACRO_END

#define bit_unlock(bit,l)						\
	(void)__sync_fetch_and_and((volatile uint32_t *)(l),		\
	    (uint32_t)~(1U << (bit)))

#define i_bit_set(bit,l)						\
	(void)__sync_fetch_and_or((volatile uint32_t *)(l),		\
	    (uint32_t)(1U << (bit)))

#define i_bit_clear(bit,l)						\
	(void)__sync_fetch_and_and((volatile uint32_t *)(l),		\
	    (uint32_t)~(1U << (bit)))

static inline char
xchgb(volatile char *cp, char new_value)
{
	return __sync_lock_test_and_set(cp, new_value);
}

static inline void atomic_incl(volatile long *p, long delta)
{
	(void)__sync_fetch_and_add(p, delta);
}

static inline void atomic_decl(volatile long *p, long delta)
{
	(void)__sync_fetch_and_sub(p, delta);
}

static inline int atomic_decl_and_test(volatile long *p, long delta)
{
	return (__sync_sub_and_fetch(p, delta) == 0);
}

static inline long atomic_getl(const volatile long *p)
{
	return *p;
}

static inline void atomic_setl(volatile long *p, long value)
{
	*p = value;
}

#endif /* MACH_KERNEL_PRIVATE */

#endif /* __APPLE_API_PRIVATE */

#endif /* _RISCV32_LOCK_H_ */

#endif /* KERNEL_PRIVATE */
