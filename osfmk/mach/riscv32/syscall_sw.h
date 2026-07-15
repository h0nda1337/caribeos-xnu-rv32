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
#ifdef PRIVATE

#ifndef _MACH_RISCV32_SYSCALL_SW_H_
#define _MACH_RISCV32_SYSCALL_SW_H_

#define SYSCALL_CLASS_SHIFT	24
#define SYSCALL_CLASS_MASK	(0xff << SYSCALL_CLASS_SHIFT)
#define SYSCALL_NUMBER_MASK	(~SYSCALL_CLASS_MASK)

#define SYSCALL_CLASS_NONE	0
#define SYSCALL_CLASS_MACH	1
#define SYSCALL_CLASS_UNIX	2
#define SYSCALL_CLASS_MDEP	3
#define SYSCALL_CLASS_DIAG	4
#define SYSCALL_CLASS_IPC	5
#define SYSCALL_CLASS_LINUX	6

#define SYSCALL_CONSTRUCT_MACH(syscall_number)	\
	((SYSCALL_CLASS_MACH << SYSCALL_CLASS_SHIFT) | \
	 (SYSCALL_NUMBER_MASK & (syscall_number)))
#define SYSCALL_CONSTRUCT_UNIX(syscall_number)	\
	((SYSCALL_CLASS_UNIX << SYSCALL_CLASS_SHIFT) | \
	 (SYSCALL_NUMBER_MASK & (syscall_number)))
#define SYSCALL_CONSTRUCT_LINUX(syscall_number)	\
	((SYSCALL_CLASS_LINUX << SYSCALL_CLASS_SHIFT) | \
	 (SYSCALL_NUMBER_MASK & (syscall_number)))

#ifndef KERNEL
#define UNIX_SYSCALL_TRAP	ecall
#define MACHDEP_SYSCALL_TRAP	ecall
#define kernel_trap(trap_name,trap_number,number_args)
#endif

#endif /* _MACH_RISCV32_SYSCALL_SW_H_ */

#endif /* PRIVATE */
