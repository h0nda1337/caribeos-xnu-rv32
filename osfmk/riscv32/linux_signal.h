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
