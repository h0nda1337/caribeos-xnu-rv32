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
#include <kern/thread.h>
#include <machine/trap.h>
#include <machine/user_exception.h>

static riscv32_user_exception_t riscv32_boot_user_exception;

static riscv32_user_exception_t *
riscv32_user_exception_slot(void)
{
	thread_t thread = current_thread();

	if (thread != THREAD_NULL) {
		return &thread->machine.last_user_exception;
	}
	return &riscv32_boot_user_exception;
}

static uint32_t
riscv32_user_exception_signal(uint32_t trapno)
{
	switch (trapno) {
	case T_ILLEGAL_INST:
		return RISCV32_LINUX_SIGILL;
	case T_BREAKPOINT:
		return RISCV32_LINUX_SIGTRAP;
	case T_INST_MISALIGNED:
	case T_LOAD_MISALIGNED:
	case T_STORE_MISALIGNED:
	case T_INST_ACCESS:
	case T_LOAD_ACCESS:
	case T_STORE_ACCESS:
		return RISCV32_LINUX_SIGBUS;
	case T_INST_PAGE_FAULT:
	case T_LOAD_PAGE_FAULT:
	case T_STORE_PAGE_FAULT:
	default:
		return RISCV32_LINUX_SIGSEGV;
	}
}

const riscv32_user_exception_t *
riscv32_user_exception_record(const riscv_saved_state_t *state)
{
	riscv32_user_exception_t *exception = riscv32_user_exception_slot();

	exception->count++;
	if (state == (const riscv_saved_state_t *)0) {
		exception->trapno = 0;
		exception->scause = 0;
		exception->stval = 0;
		exception->sepc = 0;
		exception->linux_signal = RISCV32_LINUX_SIGSEGV;
		exception->signal_pending = 1U;
		return exception;
	}

	exception->trapno = state->trapno;
	exception->scause = state->exception.scause;
	exception->stval = state->exception.stval;
	exception->sepc = state->exception.sepc;
	exception->linux_signal = riscv32_user_exception_signal(state->trapno);
	exception->signal_pending = 1U;
	return exception;
}

const riscv32_user_exception_t *
riscv32_user_exception_latest(void)
{
	return riscv32_user_exception_slot();
}

uint32_t
riscv32_user_exception_count(void)
{
	return riscv32_user_exception_slot()->count;
}

uint32_t
riscv32_user_exception_pending_signal(void)
{
	const riscv32_user_exception_t *exception = riscv32_user_exception_slot();

	if (!exception->signal_pending) {
		return 0;
	}
	return exception->linux_signal;
}

void
riscv32_user_exception_clear_pending(void)
{
	riscv32_user_exception_slot()->signal_pending = 0;
}
