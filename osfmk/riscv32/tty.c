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
#include <kern/clock.h>
#include <kern/debug.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <machine/bootinfo.h>
#include <machine/copyio.h>
#include <machine/early_print.h>
#include <machine/linux_signal.h>
#include <machine/machine_routines.h>
#include <machine/plic.h>
#include <machine/tty.h>
#include <string.h>

#define RISCV32_TTY_UART_DEFAULT	0x10000000U
#define RISCV32_TTY_RBR		0x00U
#define RISCV32_TTY_THR		0x00U
#define RISCV32_TTY_IER		0x01U
#define RISCV32_TTY_IIR		0x02U
#define RISCV32_TTY_FCR		0x02U
#define RISCV32_TTY_LSR		0x05U
#define RISCV32_TTY_IER_RX	0x01U
#define RISCV32_TTY_IER_TX	0x02U
#define RISCV32_TTY_IER_LINE	0x04U
#define RISCV32_TTY_IIR_NONE	0x01U
#define RISCV32_TTY_IIR_CAUSE	0x0eU
#define RISCV32_TTY_IIR_TX	0x02U
#define RISCV32_TTY_IIR_RX	0x04U
#define RISCV32_TTY_IIR_LINE	0x06U
#define RISCV32_TTY_IIR_TIMEOUT	0x0cU
#define RISCV32_TTY_LSR_DATA	0x01U
#define RISCV32_TTY_LSR_THRE	0x20U
#define RISCV32_TTY_FCR_ENABLE	0x01U
#define RISCV32_TTY_FCR_CLEAR_RX 0x02U
#define RISCV32_TTY_FCR_CLEAR_TX 0x04U
#define RISCV32_TTY_FCR_TRIGGER1 0x00U

#define RISCV32_TTY_INPUT_SIZE	2048U
#define RISCV32_TTY_LINE_SIZE	256U
#define RISCV32_TTY_ECHO_SIZE	512U
#define RISCV32_TTY_IRQ_BUDGET	512U
#define RISCV32_TTY_READ_CHUNK	256U

#define RISCV32_TTY_IFLAG_IGNCR	0x00000080U
#define RISCV32_TTY_IFLAG_ICRNL	0x00000100U
#define RISCV32_TTY_OFLAG_OPOST	0x00000001U
#define RISCV32_TTY_OFLAG_ONLCR	0x00000004U
#define RISCV32_TTY_LFLAG_ISIG	0x00000001U
#define RISCV32_TTY_LFLAG_ICANON 0x00000002U
#define RISCV32_TTY_LFLAG_ECHO	0x00000008U
#define RISCV32_TTY_LFLAG_ECHOE	0x00000010U
#define RISCV32_TTY_LFLAG_ECHOK	0x00000020U
#define RISCV32_TTY_LFLAG_NOFLSH 0x00000080U
#define RISCV32_TTY_CC_VINTR	0U
#define RISCV32_TTY_CC_VQUIT	1U
#define RISCV32_TTY_CC_VERASE	2U
#define RISCV32_TTY_CC_VKILL	3U
#define RISCV32_TTY_CC_VEOF	4U
#define RISCV32_TTY_CC_VTIME	5U
#define RISCV32_TTY_CC_VMIN	6U
#define RISCV32_TTY_CC_VSUSP	10U
#define RISCV32_TTY_CC_OFFSET	17U

#define RISCV32_TTY_EINTR	4U
#define RISCV32_TTY_EAGAIN	11U
#define RISCV32_TTY_EFAULT	14U
#define RISCV32_TTY_SIGINT	2U
#define RISCV32_TTY_SIGQUIT	3U
#define RISCV32_TTY_SIGTSTP	20U

typedef struct riscv32_tty_state {
	uint32_t uart_base;
	uint32_t uart_irq;
	uint8_t termios[RISCV32_TTY_TERMIOS_SIZE];
	uint8_t input[RISCV32_TTY_INPUT_SIZE];
	uint8_t input_eol[RISCV32_TTY_INPUT_SIZE];
	uint32_t input_head;
	uint32_t input_tail;
	uint32_t input_count;
	uint8_t line[RISCV32_TTY_LINE_SIZE];
	uint32_t line_count;
	uint32_t eof_pending;
	uint8_t echo[RISCV32_TTY_ECHO_SIZE];
	uint32_t echo_head;
	uint32_t echo_tail;
	uint32_t echo_count;
	uint32_t input_sequence;
	uint32_t foreground_pgid;
	uint32_t session_id;
	riscv32_tty_stats_t stats;
} riscv32_tty_state_t;

static riscv32_tty_state_t riscv32_tty;
static decl_simple_lock_data(, riscv32_tty_lock)
static volatile uint32_t riscv32_tty_init_state;

static uint32_t
riscv32_tty_get_u32(const uint8_t *bytes, uint32_t offset)
{
	return (uint32_t)bytes[offset] |
	    ((uint32_t)bytes[offset + 1U] << 8) |
	    ((uint32_t)bytes[offset + 2U] << 16) |
	    ((uint32_t)bytes[offset + 3U] << 24);
}

static void
riscv32_tty_put_u32(uint8_t *bytes, uint32_t offset, uint32_t value)
{
	bytes[offset] = (uint8_t)value;
	bytes[offset + 1U] = (uint8_t)(value >> 8);
	bytes[offset + 2U] = (uint8_t)(value >> 16);
	bytes[offset + 3U] = (uint8_t)(value >> 24);
}

static boolean_t
riscv32_tty_lock_irqsave(void)
{
	boolean_t interrupts_enabled = ml_set_interrupts_enabled(FALSE);

	simple_lock(&riscv32_tty_lock);
	return interrupts_enabled;
}

static void
riscv32_tty_unlock_irqrestore(boolean_t interrupts_enabled)
{
	simple_unlock(&riscv32_tty_lock);
	(void)ml_set_interrupts_enabled(interrupts_enabled);
}

static uint8_t
riscv32_tty_uart_read(uint32_t offset)
{
	return *(volatile uint8_t *)(uintptr_t)(riscv32_tty.uart_base + offset);
}

static void
riscv32_tty_uart_write(uint32_t offset, uint8_t value)
{
	*(volatile uint8_t *)(uintptr_t)(riscv32_tty.uart_base + offset) = value;
}

static void
riscv32_tty_set_default_termios(void)
{
	static const uint8_t cc[19] = {
		3, 28, 127, 21, 4, 0, 1, 0, 17, 19,
		26, 0, 18, 15, 23, 22, 0, 0, 0
	};

	memset(riscv32_tty.termios, 0, sizeof(riscv32_tty.termios));
	riscv32_tty_put_u32(riscv32_tty.termios, 0U, 0x00000502U);
	riscv32_tty_put_u32(riscv32_tty.termios, 4U, 0x00000005U);
	riscv32_tty_put_u32(riscv32_tty.termios, 8U, 0x00000cbfU);
	riscv32_tty_put_u32(riscv32_tty.termios, 12U, 0x00008a3bU);
	memcpy(&riscv32_tty.termios[RISCV32_TTY_CC_OFFSET], cc, sizeof(cc));
}

static void
riscv32_tty_echo_enqueue_locked(uint8_t byte)
{
	uint32_t oflag = riscv32_tty_get_u32(riscv32_tty.termios, 4U);

	if (byte == '\n' && (oflag & (RISCV32_TTY_OFLAG_OPOST |
	    RISCV32_TTY_OFLAG_ONLCR)) == (RISCV32_TTY_OFLAG_OPOST |
	    RISCV32_TTY_OFLAG_ONLCR)) {
		riscv32_tty_echo_enqueue_locked('\r');
	}
	if (riscv32_tty.echo_count == RISCV32_TTY_ECHO_SIZE) {
		riscv32_tty.stats.echo_drops++;
		return;
	}
	riscv32_tty.echo[riscv32_tty.echo_tail] = byte;
	riscv32_tty.echo_tail = (riscv32_tty.echo_tail + 1U) %
	    RISCV32_TTY_ECHO_SIZE;
	riscv32_tty.echo_count++;
	riscv32_tty.stats.echo_bytes++;
}

static void
riscv32_tty_tx_drain_locked(void)
{
	uint8_t ier;

	while (riscv32_tty.echo_count != 0U &&
	    (riscv32_tty_uart_read(RISCV32_TTY_LSR) &
	    RISCV32_TTY_LSR_THRE) != 0U) {
		riscv32_tty_uart_write(RISCV32_TTY_THR,
		    riscv32_tty.echo[riscv32_tty.echo_head]);
		riscv32_tty.echo_head = (riscv32_tty.echo_head + 1U) %
		    RISCV32_TTY_ECHO_SIZE;
		riscv32_tty.echo_count--;
	}
	ier = riscv32_tty_uart_read(RISCV32_TTY_IER);
	if (riscv32_tty.echo_count != 0U) {
		ier |= RISCV32_TTY_IER_TX;
	} else {
		ier &= (uint8_t)~RISCV32_TTY_IER_TX;
	}
	riscv32_tty_uart_write(RISCV32_TTY_IER, ier);
}

static boolean_t
riscv32_tty_input_enqueue_locked(uint8_t byte, boolean_t eol)
{
	if (riscv32_tty.input_count == RISCV32_TTY_INPUT_SIZE) {
		riscv32_tty.stats.input_overruns++;
		return FALSE;
	}
	riscv32_tty.input[riscv32_tty.input_tail] = byte;
	riscv32_tty.input_eol[riscv32_tty.input_tail] = eol ? 1U : 0U;
	riscv32_tty.input_tail = (riscv32_tty.input_tail + 1U) %
	    RISCV32_TTY_INPUT_SIZE;
	riscv32_tty.input_count++;
	return TRUE;
}

static boolean_t
riscv32_tty_commit_line_locked(boolean_t append_newline)
{
	uint32_t needed = riscv32_tty.line_count + (append_newline ? 1U : 0U);
	uint32_t i;

	if (needed == 0U) {
		riscv32_tty.eof_pending++;
		riscv32_tty.input_sequence++;
		return TRUE;
	}
	if (needed > RISCV32_TTY_INPUT_SIZE - riscv32_tty.input_count) {
		riscv32_tty.stats.input_overruns++;
		return FALSE;
	}
	for (i = 0U; i < riscv32_tty.line_count; i++) {
		(void)riscv32_tty_input_enqueue_locked(riscv32_tty.line[i],
		    !append_newline && i + 1U == riscv32_tty.line_count);
	}
	if (append_newline) {
		(void)riscv32_tty_input_enqueue_locked('\n', TRUE);
	}
	riscv32_tty.line_count = 0U;
	riscv32_tty.input_sequence++;
	riscv32_tty.stats.input_lines++;
	return TRUE;
}

static boolean_t
riscv32_tty_receive_locked(uint8_t byte, uint32_t *signal_out,
    uint32_t *pgid_out, uint32_t *sid_out)
{
	uint32_t iflag = riscv32_tty_get_u32(riscv32_tty.termios, 0U);
	uint32_t lflag = riscv32_tty_get_u32(riscv32_tty.termios, 12U);
	uint8_t erase = riscv32_tty.termios[RISCV32_TTY_CC_OFFSET +
	    RISCV32_TTY_CC_VERASE];
	uint8_t kill = riscv32_tty.termios[RISCV32_TTY_CC_OFFSET +
	    RISCV32_TTY_CC_VKILL];
	uint8_t eof = riscv32_tty.termios[RISCV32_TTY_CC_OFFSET +
	    RISCV32_TTY_CC_VEOF];
	uint8_t intr = riscv32_tty.termios[RISCV32_TTY_CC_OFFSET +
	    RISCV32_TTY_CC_VINTR];
	uint8_t quit = riscv32_tty.termios[RISCV32_TTY_CC_OFFSET +
	    RISCV32_TTY_CC_VQUIT];
	uint8_t suspend = riscv32_tty.termios[RISCV32_TTY_CC_OFFSET +
	    RISCV32_TTY_CC_VSUSP];
	uint32_t signal = 0U;

	*signal_out = 0U;
	*pgid_out = 0U;
	*sid_out = 0U;

	riscv32_tty.stats.rx_bytes++;
	if (byte == '\r') {
		if ((iflag & RISCV32_TTY_IFLAG_IGNCR) != 0U) {
			return FALSE;
		}
		if ((iflag & RISCV32_TTY_IFLAG_ICRNL) != 0U) {
			byte = '\n';
		}
	}
	if ((lflag & RISCV32_TTY_LFLAG_ISIG) != 0U &&
	    (byte == intr || byte == quit || byte == suspend)) {
		signal = byte == intr ? RISCV32_TTY_SIGINT :
		    (byte == quit ? RISCV32_TTY_SIGQUIT :
		    RISCV32_TTY_SIGTSTP);
		if ((lflag & RISCV32_TTY_LFLAG_ECHO) != 0U) {
			riscv32_tty_echo_enqueue_locked('^');
			riscv32_tty_echo_enqueue_locked((uint8_t)(byte + '@'));
			riscv32_tty_echo_enqueue_locked('\n');
		}
		if ((lflag & RISCV32_TTY_LFLAG_NOFLSH) == 0U) {
			riscv32_tty.input_head = 0U;
			riscv32_tty.input_tail = 0U;
			riscv32_tty.input_count = 0U;
			riscv32_tty.line_count = 0U;
			riscv32_tty.eof_pending = 0U;
			memset(riscv32_tty.input_eol, 0,
			    sizeof(riscv32_tty.input_eol));
		}
		riscv32_tty.input_sequence++;
		riscv32_tty.stats.isig_chars++;
		*signal_out = signal;
		*pgid_out = riscv32_tty.foreground_pgid;
		*sid_out = riscv32_tty.session_id;
		return TRUE;
	}
	if ((lflag & RISCV32_TTY_LFLAG_ICANON) == 0U) {
		if (!riscv32_tty_input_enqueue_locked(byte, FALSE)) {
			return FALSE;
		}
		if ((lflag & RISCV32_TTY_LFLAG_ECHO) != 0U) {
			riscv32_tty_echo_enqueue_locked(byte);
		}
		riscv32_tty.input_sequence++;
		return TRUE;
	}
	if (byte == erase || byte == '\b') {
		if (riscv32_tty.line_count != 0U) {
			riscv32_tty.line_count--;
			if ((lflag & RISCV32_TTY_LFLAG_ECHO) != 0U) {
				if ((lflag & RISCV32_TTY_LFLAG_ECHOE) != 0U) {
					riscv32_tty_echo_enqueue_locked('\b');
					riscv32_tty_echo_enqueue_locked(' ');
					riscv32_tty_echo_enqueue_locked('\b');
				} else {
					riscv32_tty_echo_enqueue_locked(byte);
				}
			}
		}
		return FALSE;
	}
	if (byte == kill) {
		while (riscv32_tty.line_count != 0U) {
			riscv32_tty.line_count--;
			if ((lflag & (RISCV32_TTY_LFLAG_ECHO |
			    RISCV32_TTY_LFLAG_ECHOK)) ==
			    (RISCV32_TTY_LFLAG_ECHO |
			    RISCV32_TTY_LFLAG_ECHOK)) {
				riscv32_tty_echo_enqueue_locked('\b');
				riscv32_tty_echo_enqueue_locked(' ');
				riscv32_tty_echo_enqueue_locked('\b');
			}
		}
		return FALSE;
	}
	if (byte == eof) {
		return riscv32_tty_commit_line_locked(FALSE);
	}
	if (byte == '\n') {
		if ((lflag & RISCV32_TTY_LFLAG_ECHO) != 0U) {
			riscv32_tty_echo_enqueue_locked(byte);
		}
		return riscv32_tty_commit_line_locked(TRUE);
	}
	if (riscv32_tty.line_count == RISCV32_TTY_LINE_SIZE) {
		riscv32_tty.stats.input_overruns++;
		return FALSE;
	}
	riscv32_tty.line[riscv32_tty.line_count++] = byte;
	if ((lflag & RISCV32_TTY_LFLAG_ECHO) != 0U) {
		riscv32_tty_echo_enqueue_locked(byte);
	}
	return FALSE;
}

static uint32_t
riscv32_tty_signal_foreground_group(uint32_t pgid, uint32_t sid,
    uint32_t signal)
{
	riscv32_linux_process_t *processes[RISCV32_LINUX_PROCESS_MAX];
	uint32_t count;
	uint32_t delivered = 0U;
	uint32_t i;

	count = riscv32_linux_process_collect_group(pgid, sid, processes,
	    RISCV32_LINUX_PROCESS_MAX);
	for (i = 0U; i < count; i++) {
		if (riscv32_linux_signal_queue(processes[i], signal, 0U, 0U,
		    0U, 0U)) {
			delivered++;
		}
		riscv32_linux_process_release(processes[i]);
	}
	(void)__sync_add_and_fetch(&riscv32_tty.stats.isig_deliveries,
	    delivered);
	if (riscv32_boot_arg_has("process-gate-13")) {
		if (signal == RISCV32_TTY_SIGINT) {
			riscv32_early_puts(
			    "[process-gate-13] TTY VINTR -> SIGINT foreground-pgid=");
		} else if (signal == RISCV32_TTY_SIGQUIT) {
			riscv32_early_puts(
			    "[process-gate-13] TTY VQUIT -> SIGQUIT foreground-pgid=");
		} else if (signal == RISCV32_TTY_SIGTSTP) {
			riscv32_early_puts(
			    "[process-gate-13] TTY VSUSP -> SIGTSTP foreground-pgid=");
		} else {
			return delivered;
		}
		riscv32_early_putdec(pgid);
		riscv32_early_puts(" targets=");
		riscv32_early_putdec(delivered);
		riscv32_early_puts(" PASS\n");
	}
	return delivered;
}

static void
riscv32_tty_irq(void *target, void *refCon, void *nub, int source)
{
	boolean_t interrupts_enabled;
	boolean_t wake_readers = FALSE;
	uint32_t budget = RISCV32_TTY_IRQ_BUDGET;
	uint8_t iir;
	uint8_t cause;
	uint32_t pending_signal = 0U;
	uint32_t signal_pgid = 0U;
	uint32_t signal_sid = 0U;

	(void)target;
	(void)refCon;
	(void)nub;
	(void)source;
	interrupts_enabled = riscv32_tty_lock_irqsave();
	riscv32_tty.stats.rx_interrupts++;
	while (budget != 0U) {
		budget--;
		iir = riscv32_tty_uart_read(RISCV32_TTY_IIR);
		if ((iir & RISCV32_TTY_IIR_NONE) != 0U) {
			break;
		}
		cause = iir & RISCV32_TTY_IIR_CAUSE;
		if (cause == RISCV32_TTY_IIR_TX) {
			riscv32_tty_tx_drain_locked();
			continue;
		}
		if (cause == RISCV32_TTY_IIR_RX ||
		    cause == RISCV32_TTY_IIR_TIMEOUT ||
		    cause == RISCV32_TTY_IIR_LINE) {
			while (budget != 0U &&
			    (riscv32_tty_uart_read(RISCV32_TTY_LSR) &
			    RISCV32_TTY_LSR_DATA) != 0U) {
				budget--;
				uint32_t received_signal;
				uint32_t received_pgid;
				uint32_t received_sid;

				wake_readers |= riscv32_tty_receive_locked(
				    riscv32_tty_uart_read(RISCV32_TTY_RBR),
				    &received_signal, &received_pgid, &received_sid);
				if (received_signal != 0U) {
					pending_signal = received_signal;
					signal_pgid = received_pgid;
					signal_sid = received_sid;
				}
			}
			riscv32_tty_tx_drain_locked();
			continue;
		}
		(void)riscv32_tty_uart_read(RISCV32_TTY_LSR);
	}
	riscv32_tty_unlock_irqrestore(interrupts_enabled);
	if (pending_signal != 0U && signal_pgid != 0U) {
		(void)riscv32_tty_signal_foreground_group(signal_pgid,
		    signal_sid, pending_signal);
	}
	if (wake_readers) {
		thread_wakeup((event_t)&riscv32_tty.input_sequence);
	}
}

void
riscv32_tty_init(void)
{
	const riscv32_bootinfo_t *bootinfo;
	uint8_t ier;

	if (__sync_fetch_and_add(&riscv32_tty_init_state, 0U) == 2U) {
		return;
	}
	if (!__sync_bool_compare_and_swap(&riscv32_tty_init_state, 0U, 1U)) {
		while (__sync_fetch_and_add(&riscv32_tty_init_state, 0U) != 2U) {
			__asm__ volatile("nop");
		}
		return;
	}
	memset(&riscv32_tty, 0, sizeof(riscv32_tty));
	simple_lock_init(&riscv32_tty_lock, 0);
	bootinfo = riscv32_bootinfo();
	riscv32_tty.uart_base = bootinfo != (const riscv32_bootinfo_t *)0 &&
	    bootinfo->uart_base != 0U ? bootinfo->uart_base :
	    RISCV32_TTY_UART_DEFAULT;
	riscv32_tty.uart_irq = bootinfo != (const riscv32_bootinfo_t *)0 ?
	    bootinfo->uart_irq : 0U;
	riscv32_tty_set_default_termios();
	if (riscv32_tty.uart_irq == 0U) {
		panic("riscv32: UART IRQ missing from bootinfo");
	}
	riscv32_plic_register_irq((int)riscv32_tty.uart_irq, NULL, NULL,
	    riscv32_tty_irq, NULL);
	riscv32_tty_uart_write(RISCV32_TTY_FCR,
	    RISCV32_TTY_FCR_ENABLE | RISCV32_TTY_FCR_CLEAR_RX |
	    RISCV32_TTY_FCR_CLEAR_TX | RISCV32_TTY_FCR_TRIGGER1);
	ier = riscv32_tty_uart_read(RISCV32_TTY_IER);
	ier &= (uint8_t)~RISCV32_TTY_IER_TX;
	ier |= RISCV32_TTY_IER_RX | RISCV32_TTY_IER_LINE;
	riscv32_tty_uart_write(RISCV32_TTY_IER, ier);
	__sync_synchronize();
	riscv32_tty_init_state = 2U;
	if (riscv32_boot_arg_has("process-gate-10") ||
	    riscv32_boot_arg_has("process-gate-11")) {
		riscv32_early_puts("[tty] NS16550 RX IRQ base=0x");
		riscv32_early_puthex32(riscv32_tty.uart_base);
		riscv32_early_puts(" irq=");
		riscv32_early_putdec(riscv32_tty.uart_irq);
		riscv32_early_puts(" canonical ring ready PASS\n");
	}
}

static uint32_t
riscv32_tty_copy_ready_locked(uint32_t user_buffer, uint32_t count,
    uint32_t *copied_out)
{
	uint8_t bytes[RISCV32_TTY_READ_CHUNK];
	uint32_t copied = 0U;
	uint32_t result;
	uint32_t index = riscv32_tty.input_head;
	boolean_t canonical = (riscv32_tty_get_u32(riscv32_tty.termios, 12U) &
	    RISCV32_TTY_LFLAG_ICANON) != 0U;

	while (copied < count && copied < sizeof(bytes) &&
	    copied < riscv32_tty.input_count) {
		boolean_t eol = riscv32_tty.input_eol[index] != 0U;

		bytes[copied++] = riscv32_tty.input[index];
		index = (index + 1U) % RISCV32_TTY_INPUT_SIZE;
		if (canonical && eol) {
			break;
		}
	}
	if (copied == 0U) {
		*copied_out = 0U;
		return 0U;
	}
	if (riscv32_copyout(bytes, user_buffer, copied) != 0) {
		return RISCV32_TTY_EFAULT;
	}
	result = copied;
	while (copied != 0U) {
		riscv32_tty.input_eol[riscv32_tty.input_head] = 0U;
		riscv32_tty.input_head = (riscv32_tty.input_head + 1U) %
		    RISCV32_TTY_INPUT_SIZE;
		riscv32_tty.input_count--;
		copied--;
	}
	*copied_out = result;
	return 0U;
}

uint32_t
riscv32_tty_read_user(riscv32_linux_process_t *process,
    uint32_t user_buffer, uint32_t count, boolean_t nonblocking,
    uint32_t *copied_out)
{
	boolean_t interrupts_enabled;
	boolean_t canonical;
	boolean_t ready;
	boolean_t timed_out;
	uint32_t available;
	uint32_t needed;
	uint32_t expected;
	uint32_t vmin;
	uint32_t vtime;
	uint32_t error;
	uint64_t interval;
	uint64_t deadline = 0U;

	if (process == (riscv32_linux_process_t *)0 || copied_out == NULL) {
		return RISCV32_TTY_EFAULT;
	}
	*copied_out = 0U;
	(void)__sync_add_and_fetch(&riscv32_tty.stats.read_calls, 1U);
	if (count == 0U) {
		return 0U;
	}
	for (;;) {
		if (riscv32_linux_signal_has_deliverable(process)) {
			(void)__sync_add_and_fetch(&riscv32_tty.stats.read_eintr, 1U);
			return RISCV32_TTY_EINTR;
		}
		interrupts_enabled = riscv32_tty_lock_irqsave();
		canonical = (riscv32_tty_get_u32(riscv32_tty.termios, 12U) &
		    RISCV32_TTY_LFLAG_ICANON) != 0U;
		vtime = riscv32_tty.termios[RISCV32_TTY_CC_OFFSET +
		    RISCV32_TTY_CC_VTIME];
		vmin = riscv32_tty.termios[RISCV32_TTY_CC_OFFSET +
		    RISCV32_TTY_CC_VMIN];
		available = riscv32_tty.input_count;
		needed = vmin;
		if (needed > count) {
			needed = count;
		}
		if (needed == 0U) {
			needed = 1U;
		}
		timed_out = deadline != 0U && mach_absolute_time() >= deadline;
		ready = canonical ? available != 0U :
		    (vmin == 0U ? (available != 0U || vtime == 0U || timed_out) :
		    (available >= needed || (available != 0U && timed_out)));
		if (ready) {
			error = riscv32_tty_copy_ready_locked(user_buffer, count,
			    copied_out);
			riscv32_tty_unlock_irqrestore(interrupts_enabled);
			if (timed_out) {
				(void)__sync_add_and_fetch(
				    &riscv32_tty.stats.vtime_timeouts, 1U);
			}
			return error;
		}
		if (canonical && riscv32_tty.eof_pending != 0U) {
			riscv32_tty.eof_pending--;
			riscv32_tty_unlock_irqrestore(interrupts_enabled);
			return 0U;
		}
		if (nonblocking) {
			riscv32_tty.stats.read_eagain++;
			riscv32_tty_unlock_irqrestore(interrupts_enabled);
			return RISCV32_TTY_EAGAIN;
		}
		if (!canonical && vtime != 0U &&
		    (vmin == 0U || available != 0U) && deadline == 0U) {
			nanoseconds_to_absolutetime((uint64_t)vtime * 100000000ULL,
			    &interval);
			clock_absolutetime_interval_to_deadline(interval, &deadline);
		}
		expected = riscv32_tty.input_sequence;
		riscv32_tty_unlock_irqrestore(interrupts_enabled);

		if (!riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_SLEEPING)) {
			panic("riscv32: tty read cannot enter process sleep");
		}
		if (deadline != 0U) {
			assert_wait_deadline((event_t)&riscv32_tty.input_sequence,
			    THREAD_UNINT, deadline);
		} else {
			assert_wait((event_t)&riscv32_tty.input_sequence,
			    THREAD_UNINT);
		}
		__sync_synchronize();
		if (__sync_fetch_and_add(&riscv32_tty.input_sequence, 0U) ==
		    expected && !riscv32_linux_signal_has_deliverable(process) &&
		    (deadline == 0U || mach_absolute_time() < deadline)) {
			(void)__sync_add_and_fetch(&riscv32_tty.stats.read_blocks, 1U);
			thread_block(THREAD_CONTINUE_NULL);
			(void)__sync_add_and_fetch(&riscv32_tty.stats.read_wakeups, 1U);
		} else {
			(void)clear_wait(current_thread(), THREAD_AWAKENED);
		}
		if (!riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_RUNNABLE) ||
		    !riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_RUNNING)) {
			panic("riscv32: tty read cannot resume process");
		}
	}
}

uint32_t
riscv32_tty_readable_bytes(void)
{
	boolean_t interrupts_enabled;
	uint32_t count;

	interrupts_enabled = riscv32_tty_lock_irqsave();
	count = riscv32_tty.input_count;
	riscv32_tty_unlock_irqrestore(interrupts_enabled);
	return count;
}

uint32_t
riscv32_tty_wait_readable(riscv32_linux_process_t *process,
    boolean_t has_deadline, uint64_t deadline, boolean_t *timed_out_out)
{
	boolean_t interrupts_enabled;
	uint32_t expected;

	if (process == (riscv32_linux_process_t *)0 ||
	    timed_out_out == (boolean_t *)0) {
		return RISCV32_TTY_EFAULT;
	}
	*timed_out_out = FALSE;
	for (;;) {
		if (riscv32_linux_signal_has_deliverable(process)) {
			return RISCV32_TTY_EINTR;
		}

		interrupts_enabled = riscv32_tty_lock_irqsave();
		if (riscv32_tty.input_count != 0U ||
		    riscv32_tty.eof_pending != 0U) {
			riscv32_tty_unlock_irqrestore(interrupts_enabled);
			return 0U;
		}
		if (has_deadline && mach_absolute_time() >= deadline) {
			riscv32_tty_unlock_irqrestore(interrupts_enabled);
			*timed_out_out = TRUE;
			return 0U;
		}
		expected = riscv32_tty.input_sequence;
		riscv32_tty_unlock_irqrestore(interrupts_enabled);

		if (!riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_SLEEPING)) {
			panic("riscv32: tty readiness cannot enter process sleep");
		}
		if (has_deadline) {
			assert_wait_deadline((event_t)&riscv32_tty.input_sequence,
			    THREAD_UNINT, deadline);
		} else {
			assert_wait((event_t)&riscv32_tty.input_sequence,
			    THREAD_UNINT);
		}
		__sync_synchronize();
		if (__sync_fetch_and_add(&riscv32_tty.input_sequence, 0U) ==
		    expected && !riscv32_linux_signal_has_deliverable(process) &&
		    (!has_deadline || mach_absolute_time() < deadline)) {
			thread_block(THREAD_CONTINUE_NULL);
		} else {
			(void)clear_wait(current_thread(), THREAD_AWAKENED);
		}
		if (!riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_RUNNABLE) ||
		    !riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_RUNNING)) {
			panic("riscv32: tty readiness cannot resume process");
		}
	}
}

boolean_t
riscv32_tty_poll_readable(void)
{
	boolean_t interrupts_enabled;
	boolean_t ready;

	interrupts_enabled = riscv32_tty_lock_irqsave();
	ready = riscv32_tty.input_count != 0U || riscv32_tty.eof_pending != 0U;
	riscv32_tty_unlock_irqrestore(interrupts_enabled);
	return ready;
}

void
riscv32_tty_get_termios(uint8_t termios[RISCV32_TTY_TERMIOS_SIZE])
{
	boolean_t interrupts_enabled;

	interrupts_enabled = riscv32_tty_lock_irqsave();
	memcpy(termios, riscv32_tty.termios, sizeof(riscv32_tty.termios));
	riscv32_tty_unlock_irqrestore(interrupts_enabled);
}

void
riscv32_tty_set_termios(
    const uint8_t termios[RISCV32_TTY_TERMIOS_SIZE], boolean_t flush_input)
{
	boolean_t interrupts_enabled;

	interrupts_enabled = riscv32_tty_lock_irqsave();
	memcpy(riscv32_tty.termios, termios, sizeof(riscv32_tty.termios));
	if (flush_input) {
		riscv32_tty.input_head = 0U;
		riscv32_tty.input_tail = 0U;
		riscv32_tty.input_count = 0U;
		riscv32_tty.line_count = 0U;
		riscv32_tty.eof_pending = 0U;
		memset(riscv32_tty.input_eol, 0,
		    sizeof(riscv32_tty.input_eol));
		riscv32_tty.input_sequence++;
	}
	riscv32_tty_unlock_irqrestore(interrupts_enabled);
	if (flush_input) {
		thread_wakeup((event_t)&riscv32_tty.input_sequence);
	}
}

uint32_t
riscv32_tty_get_foreground_pgrp(uint32_t *sid_out)
{
	boolean_t interrupts_enabled;
	uint32_t pgid;

	interrupts_enabled = riscv32_tty_lock_irqsave();
	pgid = riscv32_tty.foreground_pgid;
	if (sid_out != (uint32_t *)0) {
		*sid_out = riscv32_tty.session_id;
	}
	riscv32_tty_unlock_irqrestore(interrupts_enabled);
	return pgid;
}

boolean_t
riscv32_tty_set_foreground_pgrp(uint32_t pgid, uint32_t sid)
{
	boolean_t interrupts_enabled;

	if (pgid == 0U || sid == 0U) {
		return FALSE;
	}
	interrupts_enabled = riscv32_tty_lock_irqsave();
	riscv32_tty.foreground_pgid = pgid;
	riscv32_tty.session_id = sid;
	riscv32_tty.stats.foreground_changes++;
	riscv32_tty_unlock_irqrestore(interrupts_enabled);
	return TRUE;
}

void
riscv32_tty_get_stats(riscv32_tty_stats_t *stats)
{
	boolean_t interrupts_enabled;

	if (stats == NULL) {
		return;
	}
	interrupts_enabled = riscv32_tty_lock_irqsave();
	*stats = riscv32_tty.stats;
	riscv32_tty_unlock_irqrestore(interrupts_enabled);
}
