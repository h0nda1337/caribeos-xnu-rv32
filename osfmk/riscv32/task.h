#ifndef _RISCV32_TASK_H_
#define _RISCV32_TASK_H_

#include <stdint.h>

struct user_ldt;

/*
 * task.c in this XNU vintage still has a few historical i386 field names in
 * generic code.  Keep the names while the RISC-V task path is separated.
 */
#define MACHINE_TASK						\
	struct user_ldt *i386_ldt;				\
	void *task_debug;					\
	uint64_t uexc_range_start;				\
	uint64_t uexc_range_size;				\
	uint64_t uexc_handler;

#endif /* _RISCV32_TASK_H_ */
