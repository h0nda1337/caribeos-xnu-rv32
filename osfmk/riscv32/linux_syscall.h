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
