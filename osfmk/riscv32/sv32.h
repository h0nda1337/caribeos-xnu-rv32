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
#ifndef _RISCV32_SV32_H_
#define _RISCV32_SV32_H_

#include <stdint.h>

#define RISCV32_SATP_MODE_SV32	0x80000000U
#define RISCV32_SATP_PPN_MASK	0x003fffffU

static inline uint32_t
riscv32_sv32_satp(uint32_t root_ppn)
{
	return RISCV32_SATP_MODE_SV32 | (root_ppn & RISCV32_SATP_PPN_MASK);
}

static inline void
riscv32_write_satp(uint32_t satp)
{
#if defined(__riscv) && !defined(_WIN32)
	__asm__ volatile("csrw satp, %0" :: "r"(satp) : "memory");
#else
	(void)satp;
#endif
}

static inline uint32_t
riscv32_read_satp(void)
{
	uint32_t satp = 0;
#if defined(__riscv) && !defined(_WIN32)
	__asm__ volatile("csrr %0, satp" : "=r"(satp));
#endif
	return satp;
}

static inline void
riscv32_sfence_vma(void)
{
#if defined(__riscv) && !defined(_WIN32)
	__asm__ volatile("sfence.vma" ::: "memory");
#endif
}

#endif /* _RISCV32_SV32_H_ */
