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
#ifndef _RISCV32_USER_EXCEPTION_H_
#define _RISCV32_USER_EXCEPTION_H_

#include <mach/thread_status.h>
#include <stdint.h>

#define RISCV32_LINUX_SIGILL	4U
#define RISCV32_LINUX_SIGTRAP	5U
#define RISCV32_LINUX_SIGBUS	7U
#define RISCV32_LINUX_SIGSEGV	11U

typedef struct riscv32_user_exception {
	uint32_t count;
	uint32_t trapno;
	uint32_t scause;
	uint32_t stval;
	uint32_t sepc;
	uint32_t linux_signal;
	uint32_t signal_pending;
} riscv32_user_exception_t;

const riscv32_user_exception_t *riscv32_user_exception_record(
    const riscv_saved_state_t *state);
const riscv32_user_exception_t *riscv32_user_exception_latest(void);
uint32_t riscv32_user_exception_count(void);
uint32_t riscv32_user_exception_pending_signal(void);
void riscv32_user_exception_clear_pending(void);

#endif /* _RISCV32_USER_EXCEPTION_H_ */
