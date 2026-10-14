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
#ifndef _RISCV32_LINUX_SYSCALL_H_
#define _RISCV32_LINUX_SYSCALL_H_

#include <mach/boolean.h>
#include <mach/thread_status.h>
#include <stdint.h>

struct riscv32_linux_process;
struct riscv32_linux_exit_resources;

typedef struct riscv32_linux_ofd_stats {
	uint32_t capacity;
	uint32_t in_use;
	uint32_t references;
	uint32_t high_watermark;
	uint32_t total_allocations;
	uint32_t total_frees;
	uint32_t total_retains;
	uint32_t total_releases;
} riscv32_linux_ofd_stats_t;

typedef struct riscv32_linux_pipe_stats {
	uint32_t capacity;
	uint32_t in_use;
	uint32_t high_watermark;
	uint32_t total_allocations;
	uint32_t total_releases;
	uint32_t bytes_read;
	uint32_t bytes_written;
	uint32_t read_blocks;
	uint32_t read_wakeups;
	uint32_t write_blocks;
	uint32_t write_wakeups;
} riscv32_linux_pipe_stats_t;

boolean_t riscv32_linux_syscall_dispatch(riscv_saved_state_t *state,
    uint32_t syscall_number);
boolean_t riscv32_linux_native_heap_dispatch(riscv_saved_state_t *state);
void riscv32_linux_syscall_teardown_process(
    struct riscv32_linux_process *process,
    struct riscv32_linux_exit_resources *resources);
boolean_t riscv32_linux_syscall_inherit_fork_state(
    struct riscv32_linux_process *parent,
    struct riscv32_linux_process *child);
void riscv32_linux_syscall_get_ofd_stats(
    riscv32_linux_ofd_stats_t *stats);
void riscv32_linux_syscall_get_pipe_stats(
    riscv32_linux_pipe_stats_t *stats);
uint32_t riscv32_linux_syscall_close_cloexec_current(void);
void riscv32_linux_terminate_current(uint32_t status, uint32_t signal)
    __attribute__((noreturn));

#endif /* _RISCV32_LINUX_SYSCALL_H_ */
