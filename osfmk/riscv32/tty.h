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
#ifndef _RISCV32_TTY_H_
#define _RISCV32_TTY_H_

#include <mach/boolean.h>
#include <machine/linux_process.h>
#include <stdint.h>

#define RISCV32_TTY_TERMIOS_SIZE 36U

typedef struct riscv32_tty_stats {
	uint32_t rx_interrupts;
	uint32_t rx_bytes;
	uint32_t input_lines;
	uint32_t input_overruns;
	uint32_t echo_bytes;
	uint32_t echo_drops;
	uint32_t read_calls;
	uint32_t read_blocks;
	uint32_t read_wakeups;
	uint32_t read_eagain;
	uint32_t read_eintr;
	uint32_t vtime_timeouts;
	uint32_t foreground_changes;
	uint32_t isig_chars;
	uint32_t isig_deliveries;
} riscv32_tty_stats_t;

void riscv32_tty_init(void);
uint32_t riscv32_tty_read_user(riscv32_linux_process_t *process,
    uint32_t user_buffer, uint32_t count, boolean_t nonblocking,
    uint32_t *copied_out);
uint32_t riscv32_tty_wait_readable(riscv32_linux_process_t *process,
    boolean_t has_deadline, uint64_t deadline, boolean_t *timed_out_out);
uint32_t riscv32_tty_readable_bytes(void);
boolean_t riscv32_tty_poll_readable(void);
void riscv32_tty_get_termios(uint8_t termios[RISCV32_TTY_TERMIOS_SIZE]);
void riscv32_tty_set_termios(
    const uint8_t termios[RISCV32_TTY_TERMIOS_SIZE], boolean_t flush_input);
uint32_t riscv32_tty_get_foreground_pgrp(uint32_t *sid_out);
boolean_t riscv32_tty_set_foreground_pgrp(uint32_t pgid, uint32_t sid);
void riscv32_tty_get_stats(riscv32_tty_stats_t *stats);

#endif /* _RISCV32_TTY_H_ */
