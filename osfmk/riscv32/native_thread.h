/*
 * SPDX-License-Identifier: APSL-2.0
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * Please obtain a copy of the License at
 * https://opensource.apple.com/apsl/ and read it before using this file.
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 * CARIBEOS MODIFICATION NOTICE
 * Created by h0nda1337, 2026-10-13: progressive shared-task user threads.
 */
#ifndef CARIBE_NATIVE_THREAD_H
#define CARIBE_NATIVE_THREAD_H
#include <stdint.h>
#define CARIBE_THREAD_ABI 0x00010000U
#define CARIBE_THREAD_CALL 0xc0805401U
#define CARIBE_THREAD_ECALL 0x05000001U
#define CARIBE_THREAD_SYNC_ECALL 0x05000002U
#define CARIBE_THREAD_IPC_ECALL 0x05000003U
#define CARIBE_THREAD_CREATE 1U
#define CARIBE_THREAD_START 2U
#define CARIBE_THREAD_JOIN 3U
#define CARIBE_THREAD_CANCEL 4U
#define CARIBE_THREAD_INFO 5U
#define CARIBE_THREAD_STATS 6U
#define CARIBE_THREAD_SELF 7U
#define CARIBE_THREAD_EXIT 8U
#define CARIBE_THREAD_YIELD 9U
#define CARIBE_THREAD_CLOCK 10U
#define CARIBE_THREAD_SLEEP 11U
/* CLOCK output only: reserved[0:1] contain monotonic nanoseconds, low/high.
   All reserved words must still be zero on input for every operation. */
#define CARIBE_THREAD_TIME_LOW 0U
#define CARIBE_THREAD_TIME_HIGH 1U
/* STATS output only: actual native sleep thread_block calls. */
#define CARIBE_THREAD_SLEEP_WAITS 2U
#define CARIBE_THREAD_MAX 16U
#define CARIBE_THREAD_SUSPENDED 2U
#define CARIBE_THREAD_RUNNING 3U
#define CARIBE_THREAD_EXITED 4U
#define CARIBE_THREAD_CANCELLED 125U
#define CARIBE_THREAD_FAULTED 14U
struct caribe_thread_request {
    uint32_t abi, operation, id, entry, argument, stack_base, stack_size, tp;
    uint32_t status, state, owner, cpu, timeout_us, live, created, reaped;
    uint32_t entered, exited, cancelled, faults, waits, preemptions, reserved[10];
};
typedef char caribe_thread_abi_size[(sizeof(struct caribe_thread_request) == 128) ? 1 : -1];
#ifdef KERNEL
#include <mach/boolean.h>
#include <mach/thread_status.h>
boolean_t riscv32_native_futex_dispatch(riscv_saved_state_t *state, boolean_t time64);
struct pmap;
uint32_t riscv32_native_futex_owner_waiters(struct pmap *pmap);
int riscv32_native_thread_ioctl(uint32_t request, uint32_t user_arg);
boolean_t riscv32_native_thread_current(void);
boolean_t riscv32_native_thread_cpu_allowed(void);
uint32_t riscv32_native_thread_identity(void);
boolean_t riscv32_native_thread_owner_busy(uint32_t pid);
void riscv32_native_thread_bootstrap(void) __attribute__((noreturn));
void riscv32_native_thread_exit(uint32_t status) __attribute__((noreturn));
void riscv32_native_thread_fault(void) __attribute__((noreturn));
void riscv32_native_thread_user_boundary(void);
void riscv32_native_thread_release_owner(uint32_t pid);
#endif
#endif
