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
#ifndef _MACH_RISCV32_VM_PARAM_H_
#define _MACH_RISCV32_VM_PARAM_H_

#define BYTE_SIZE		8

#define RISCV32_PGBYTES		4096
#define RISCV32_PGSHIFT		12

#define PAGE_SIZE		RISCV32_PGBYTES
#define PAGE_SHIFT		RISCV32_PGSHIFT
#define PAGE_MASK		(PAGE_SIZE - 1)
#define KERNEL_STACK_SIZE	(PAGE_SIZE * 4)

#define riscv32_btop(x)		((x) >> RISCV32_PGSHIFT)
#define machine_btop(x)		riscv32_btop(x)
#define riscv32_ptob(x)		((x) << RISCV32_PGSHIFT)
#define machine_ptob(x)		riscv32_ptob(x)

#define riscv32_round_page(x)	(((x) + RISCV32_PGBYTES - 1) & ~(RISCV32_PGBYTES - 1))
#define riscv32_trunc_page(x)	((x) & ~(RISCV32_PGBYTES - 1))

#define MACH_VM_MIN_ADDRESS	((mach_vm_offset_t)0)
#define MACH_VM_MAX_ADDRESS	((mach_vm_offset_t)0xffffffffULL)

#define VM_MIN_ADDRESS		((vm_offset_t)0)
#define VM_MAX_ADDRESS		((vm_offset_t)0x80000000U)
#define VM_USRSTACK32		((vm_offset_t)0x80000000U)
#define VM_MAP_MIN_ADDRESS	MACH_VM_MIN_ADDRESS
#define VM_MAP_MAX_ADDRESS	MACH_VM_MAX_ADDRESS
#define VM32_MIN_ADDRESS	((vm32_offset_t)0)
#define VM32_MAX_ADDRESS	((vm32_offset_t)VM_MAX_ADDRESS)

#ifdef KERNEL_PRIVATE

#define KB			(1024ULL)
#define MB			(1024 * KB)
#define GB			(1024 * MB)

#define KERNEL_IMAGE_TO_PHYS(x)	(x)
#define VM_MIN_KERNEL_ADDRESS		((vm_offset_t)0x80200000U)
#define VM_MIN_KERNEL_AND_KEXT_ADDRESS	VM_MIN_KERNEL_ADDRESS
#define VM_MAX_KERNEL_ADDRESS		((vm_offset_t)0xffffffffU)
#define VM_KERNEL_IS_KEXT(x)		(0)

#endif /* KERNEL_PRIVATE */

#if defined(KERNEL_PRIVATE) || defined(MACH_KERNEL_PRIVATE)
#define KALLOC_MINSIZE		16
#define KALLOC_LOG2_MINALIGN	4
#endif

#define VM32_SUPPORT	1

#endif /* _MACH_RISCV32_VM_PARAM_H_ */
