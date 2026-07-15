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
