#ifndef _RISCV32_TRAP_H_
#define _RISCV32_TRAP_H_

#include <mach/kern_return.h>
#include <mach/thread_status.h>
#include <kern/ast.h>
#include <stdint.h>

#define T_INST_MISALIGNED	0
#define T_INST_ACCESS		1
#define T_ILLEGAL_INST		2
#define T_BREAKPOINT		3
#define T_LOAD_MISALIGNED	4
#define T_LOAD_ACCESS		5
#define T_STORE_MISALIGNED	6
#define T_STORE_ACCESS		7
#define T_ECALL_U		8
#define T_ECALL_S		9
#define T_INST_PAGE_FAULT	12
#define T_LOAD_PAGE_FAULT	13
#define T_STORE_PAGE_FAULT	15
#define T_PREEMPT		255

#define TRAP_NAMES "instruction address misaligned", "instruction access fault", \
	"illegal instruction", "breakpoint", "load address misaligned", \
	"load access fault", "store address misaligned", "store access fault", \
	"environment call from U-mode", "environment call from S-mode"

typedef kern_return_t (*perfASTCallback)(ast_t reasons, ast_t *myast);
typedef kern_return_t (*perfCallback)(int trapno, void *regs,
    uintptr_t *lo_spp, int user_mode);

extern volatile perfASTCallback perfASTHook;
extern volatile perfCallback perfTrapHook;
extern volatile perfCallback perfIntHook;

extern void riscv32_trap_vector(void);
extern void riscv32_return_to_user(riscv_saved_state_t *state);
extern void riscv32_trap_init(void);
extern void riscv32_trap_dispatch(riscv_saved_state_t *state);
extern void riscv32_kernel_trap(riscv_saved_state_t *state);
extern void riscv32_user_trap(riscv_saved_state_t *state);
extern void riscv32_interrupt(riscv_saved_state_t *state);

#endif /* _RISCV32_TRAP_H_ */
