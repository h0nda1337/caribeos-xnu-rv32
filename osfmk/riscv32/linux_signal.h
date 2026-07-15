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
#ifndef _RISCV32_LINUX_SIGNAL_H_
#define _RISCV32_LINUX_SIGNAL_H_

#include <mach/boolean.h>
#include <mach/thread_status.h>
#include <machine/linux_process.h>

typedef struct riscv32_linux_signal_stats {
	uint32_t frames_delivered;
	uint32_t frames_returned;
	uint32_t default_ignored;
	uint32_t frame_failures;
	uint32_t signals_queued;
	uint32_t signals_discarded;
	uint32_t target_wakeups;
} riscv32_linux_signal_stats_t;

boolean_t riscv32_linux_signal_handle_user_exception(
    riscv_saved_state_t *state);
boolean_t riscv32_linux_signal_deliver_pending(riscv_saved_state_t *state);
boolean_t riscv32_linux_signal_rt_sigreturn(riscv_saved_state_t *state);
boolean_t riscv32_linux_signal_queue(riscv32_linux_process_t *process,
    uint32_t signal, uint32_t sender_pid, uint32_t sender_uid,
    uint32_t code, uint32_t status);
boolean_t riscv32_linux_signal_has_deliverable(
    riscv32_linux_process_t *process);
void riscv32_linux_signal_get_stats(riscv32_linux_signal_stats_t *stats);

#endif /* _RISCV32_LINUX_SIGNAL_H_ */
