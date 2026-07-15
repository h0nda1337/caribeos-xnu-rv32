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
#ifndef _MACH_RISCV32_EXCEPTION_H_
#define _MACH_RISCV32_EXCEPTION_H_

#define EXC_TYPES_COUNT		12
#define EXCEPTION_CODE_MAX	2

#define EXC_RISCV_INST_MISALIGNED	0
#define EXC_RISCV_INST_ACCESS		1
#define EXC_RISCV_ILLEGAL_INST		2
#define EXC_RISCV_BREAKPOINT		3
#define EXC_RISCV_LOAD_MISALIGNED	4
#define EXC_RISCV_LOAD_ACCESS		5
#define EXC_RISCV_STORE_MISALIGNED	6
#define EXC_RISCV_STORE_ACCESS		7
#define EXC_RISCV_ECALL_U		8
#define EXC_RISCV_ECALL_S		9
#define EXC_RISCV_INST_PAGE_FAULT	12
#define EXC_RISCV_LOAD_PAGE_FAULT	13
#define EXC_RISCV_STORE_PAGE_FAULT	15

#define EXC_MASK_MACHINE	0

#endif /* _MACH_RISCV32_EXCEPTION_H_ */
