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
 * Modified 2026-10-13: native port ECALL, without broadening Linux worker syscalls.
 * Modified 2026-10-14: opt-in bounded allocator VM channel, separate from Linux worker syscalls.
 */

#line 1
#include <machine/syscall.h>
#include <machine/linux_syscall.h>
#include <machine/native_thread.h>
#include <machine/native_sync.h>
#include <machine/native_ipc.h>
#include <kern/thread.h>

#define RISCV32_ECALL_SIZE	4U
#define RISCV32_ENOSYS		38U

static void
riscv32_syscall_advance_pc(riscv_saved_state_t *state)
{
	state->exception.sepc += RISCV32_ECALL_SIZE;
	state->thread.pc = state->exception.sepc;
}

static void
riscv32_syscall_return_errno(riscv_saved_state_t *state, uint32_t error)
{
	state->thread.x[RISCV_X_A0] = (uint32_t)(0U - error);
	state->thread.x[RISCV_X_A1] = error;
}

boolean_t
riscv32_syscall_dispatch(riscv_saved_state_t *state, int user_mode)
{
	uint32_t raw_number;
	uint32_t syscall_class;
	uint32_t syscall_number;

	if (!user_mode || state == (riscv_saved_state_t *)0) {
		return FALSE;
	}

	raw_number = state->thread.x[RISCV_X_A7];
	syscall_class = (raw_number & RISCV32_SYSCALL_CLASS_MASK) >>
	    RISCV32_SYSCALL_CLASS_SHIFT;
	syscall_number = raw_number & RISCV32_SYSCALL_NUMBER_MASK;
	(void)syscall_number;

	riscv32_syscall_advance_pc(state);
	if (raw_number == CARIBE_THREAD_VM_ECALL) return riscv32_linux_native_heap_dispatch(state);
	if (raw_number == CARIBE_THREAD_ECALL || raw_number == CARIBE_THREAD_SYNC_ECALL ||
	    raw_number == CARIBE_THREAD_IPC_ECALL) {
		uint32_t user_arg = state->thread.x[RISCV_X_A0];
		uint32_t error;
		if (raw_number == CARIBE_THREAD_ECALL) error = riscv32_native_thread_ioctl(CARIBE_THREAD_CALL, user_arg);
		else if (raw_number == CARIBE_THREAD_SYNC_ECALL) error = riscv32_native_sync_ioctl(CARIBE_SYNC_CALL, user_arg);
		else error = riscv32_native_ipc_ioctl(CARIBE_IPC_CALL, user_arg);
		if (error) riscv32_syscall_return_errno(state, error);
		else state->thread.x[RISCV_X_A0] = state->thread.x[RISCV_X_A1] = 0;
		return TRUE;
	}
	/* Raw threads must not alias the main thread's Linux signal/FD/VM state.
	   Their libc/pthread and GNU TLS contract is a separate later gate. */
	if (riscv32_native_thread_current()) {
		if ((syscall_class == RISCV32_SYSCALL_CLASS_NONE || syscall_class == RISCV32_SYSCALL_CLASS_LINUX) &&
		    (syscall_number == 98U || syscall_number == 422U) &&
		    riscv32_native_futex_dispatch(state, syscall_number == 422U)) return TRUE;
		if ((syscall_class == RISCV32_SYSCALL_CLASS_NONE || syscall_class == RISCV32_SYSCALL_CLASS_LINUX) &&
		    syscall_number == 93U) riscv32_native_thread_exit(state->thread.x[RISCV_X_A0]);
		riscv32_syscall_return_errno(state, 95U);
		return TRUE;
	}

	switch (syscall_class) {
	case RISCV32_SYSCALL_CLASS_NONE:
	case RISCV32_SYSCALL_CLASS_LINUX:
		return riscv32_linux_syscall_dispatch(state, syscall_number);
	case RISCV32_SYSCALL_CLASS_MACH:
	case RISCV32_SYSCALL_CLASS_UNIX:
	case RISCV32_SYSCALL_CLASS_MDEP:
	case RISCV32_SYSCALL_CLASS_DIAG:
	case RISCV32_SYSCALL_CLASS_IPC:
	default:
		riscv32_syscall_return_errno(state, RISCV32_ENOSYS);
		return TRUE;
	}
}
