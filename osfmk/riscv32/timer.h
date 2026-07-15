#ifndef _RISCV32_TIMER_H_
#define _RISCV32_TIMER_H_

#include <platforms.h>
#include <mach/riscv32/thread_status.h>
#include <stdint.h>

uint64_t riscv32_timer_read(void);
void riscv32_timer_enable(void);
void rtclock_intr(riscv_saved_state_t *state);

#endif /* _RISCV32_TIMER_H_ */
