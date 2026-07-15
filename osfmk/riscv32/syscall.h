#ifndef _RISCV32_SYSCALL_H_
#define _RISCV32_SYSCALL_H_

#include <mach/boolean.h>
#include <mach/thread_status.h>
#include <stdint.h>

#define RISCV32_SYSCALL_CLASS_SHIFT	24
#define RISCV32_SYSCALL_CLASS_MASK	(0xffU << RISCV32_SYSCALL_CLASS_SHIFT)
#define RISCV32_SYSCALL_NUMBER_MASK	(~RISCV32_SYSCALL_CLASS_MASK)

#define RISCV32_SYSCALL_CLASS_NONE	0U
#define RISCV32_SYSCALL_CLASS_MACH	1U
#define RISCV32_SYSCALL_CLASS_UNIX	2U
#define RISCV32_SYSCALL_CLASS_MDEP	3U
#define RISCV32_SYSCALL_CLASS_DIAG	4U
#define RISCV32_SYSCALL_CLASS_IPC	5U
#define RISCV32_SYSCALL_CLASS_LINUX	6U

boolean_t riscv32_syscall_dispatch(riscv_saved_state_t *state,
    int user_mode);

#endif /* _RISCV32_SYSCALL_H_ */
