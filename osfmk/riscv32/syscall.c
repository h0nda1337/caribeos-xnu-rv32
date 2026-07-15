#include <machine/syscall.h>
#include <machine/linux_syscall.h>

#define RISCV32_ECALL_SIZE	4U
#define RISCV32_ENOSYS		38U

static void
riscv32_syscall_advance_pc(riscv_saved_state_t *state)
{
	state->exception.sepc += RISCV32_ECALL_SIZE;
	state->thread.pc = state->exception.sepc;
}

static void
riscv32_syscall_return_errno(riscv_saved_state_t *state, uint32_t error)
{
	state->thread.x[RISCV_X_A0] = (uint32_t)(0U - error);
	state->thread.x[RISCV_X_A1] = error;
}

boolean_t
riscv32_syscall_dispatch(riscv_saved_state_t *state, int user_mode)
{
	uint32_t raw_number;
	uint32_t syscall_class;
	uint32_t syscall_number;

	if (!user_mode || state == (riscv_saved_state_t *)0) {
		return FALSE;
	}

	raw_number = state->thread.x[RISCV_X_A7];
	syscall_class = (raw_number & RISCV32_SYSCALL_CLASS_MASK) >>
	    RISCV32_SYSCALL_CLASS_SHIFT;
	syscall_number = raw_number & RISCV32_SYSCALL_NUMBER_MASK;
	(void)syscall_number;

	riscv32_syscall_advance_pc(state);

	switch (syscall_class) {
	case RISCV32_SYSCALL_CLASS_NONE:
	case RISCV32_SYSCALL_CLASS_LINUX:
		return riscv32_linux_syscall_dispatch(state, syscall_number);
	case RISCV32_SYSCALL_CLASS_MACH:
	case RISCV32_SYSCALL_CLASS_UNIX:
	case RISCV32_SYSCALL_CLASS_MDEP:
	case RISCV32_SYSCALL_CLASS_DIAG:
	case RISCV32_SYSCALL_CLASS_IPC:
	default:
		riscv32_syscall_return_errno(state, RISCV32_ENOSYS);
		return TRUE;
	}
}
