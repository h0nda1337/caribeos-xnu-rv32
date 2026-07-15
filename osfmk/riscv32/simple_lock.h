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

#ifndef _RISCV32_SIMPLE_LOCK_TYPES_H_
#define _RISCV32_SIMPLE_LOCK_TYPES_H_

#include <mach/boolean.h>
#include <kern/kern_types.h>
#include <stdint.h>

typedef struct hw_lock_data {
	volatile uint32_t	lock_data;
} hw_lock_data_t, *hw_lock_t;

typedef struct slock {
	hw_lock_data_t		interlock;
} usimple_lock_data_t, *usimple_lock_t;

#define USIMPLE_LOCK_NULL	((usimple_lock_t)0)

#if !defined(decl_simple_lock_data)
typedef usimple_lock_data_t	*simple_lock_t;
typedef usimple_lock_data_t	simple_lock_data_t;

#define decl_simple_lock_data(class,name) \
	class simple_lock_data_t name;
#endif

#endif /* _RISCV32_SIMPLE_LOCK_TYPES_H_ */

#endif /* KERNEL_PRIVATE */
