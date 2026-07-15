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
