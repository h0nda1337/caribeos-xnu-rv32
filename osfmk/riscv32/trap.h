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
#ifndef _RISCV32_TRAP_H_
#define _RISCV32_TRAP_H_

#include <mach/kern_return.h>
#include <mach/thread_status.h>
#include <kern/ast.h>
#include <stdint.h>

#define T_INST_MISALIGNED	0
#define T_INST_ACCESS		1
#define T_ILLEGAL_INST		2
#define T_BREAKPOINT		3
#define T_LOAD_MISALIGNED	4
#define T_LOAD_ACCESS		5
#define T_STORE_MISALIGNED	6
#define T_STORE_ACCESS		7
#define T_ECALL_U		8
#define T_ECALL_S		9
#define T_INST_PAGE_FAULT	12
#define T_LOAD_PAGE_FAULT	13
#define T_STORE_PAGE_FAULT	15
#define T_PREEMPT		255

#define TRAP_NAMES "instruction address misaligned", "instruction access fault", \
	"illegal instruction", "breakpoint", "load address misaligned", \
	"load access fault", "store address misaligned", "store access fault", \
	"environment call from U-mode", "environment call from S-mode"

typedef kern_return_t (*perfASTCallback)(ast_t reasons, ast_t *myast);
typedef kern_return_t (*perfCallback)(int trapno, void *regs,
    uintptr_t *lo_spp, int user_mode);

extern volatile perfASTCallback perfASTHook;
extern volatile perfCallback perfTrapHook;
extern volatile perfCallback perfIntHook;

extern void riscv32_trap_vector(void);
extern void riscv32_return_to_user(riscv_saved_state_t *state);
extern void riscv32_trap_init(void);
extern void riscv32_trap_dispatch(riscv_saved_state_t *state);
extern void riscv32_kernel_trap(riscv_saved_state_t *state);
extern void riscv32_user_trap(riscv_saved_state_t *state);
extern void riscv32_interrupt(riscv_saved_state_t *state);

#endif /* _RISCV32_TRAP_H_ */
