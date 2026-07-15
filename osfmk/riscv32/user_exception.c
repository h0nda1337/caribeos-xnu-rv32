#include <kern/thread.h>
#include <machine/trap.h>
#include <machine/user_exception.h>

static riscv32_user_exception_t riscv32_boot_user_exception;

static riscv32_user_exception_t *
riscv32_user_exception_slot(void)
{
	thread_t thread = current_thread();

	if (thread != THREAD_NULL) {
		return &thread->machine.last_user_exception;
	}
	return &riscv32_boot_user_exception;
}

static uint32_t
riscv32_user_exception_signal(uint32_t trapno)
{
	switch (trapno) {
	case T_ILLEGAL_INST:
		return RISCV32_LINUX_SIGILL;
	case T_BREAKPOINT:
		return RISCV32_LINUX_SIGTRAP;
	case T_INST_MISALIGNED:
	case T_LOAD_MISALIGNED:
	case T_STORE_MISALIGNED:
	case T_INST_ACCESS:
	case T_LOAD_ACCESS:
	case T_STORE_ACCESS:
		return RISCV32_LINUX_SIGBUS;
	case T_INST_PAGE_FAULT:
	case T_LOAD_PAGE_FAULT:
	case T_STORE_PAGE_FAULT:
	default:
		return RISCV32_LINUX_SIGSEGV;
	}
}

const riscv32_user_exception_t *
riscv32_user_exception_record(const riscv_saved_state_t *state)
{
	riscv32_user_exception_t *exception = riscv32_user_exception_slot();

	exception->count++;
	if (state == (const riscv_saved_state_t *)0) {
		exception->trapno = 0;
		exception->scause = 0;
		exception->stval = 0;
		exception->sepc = 0;
		exception->linux_signal = RISCV32_LINUX_SIGSEGV;
		exception->signal_pending = 1U;
		return exception;
	}

	exception->trapno = state->trapno;
	exception->scause = state->exception.scause;
	exception->stval = state->exception.stval;
	exception->sepc = state->exception.sepc;
	exception->linux_signal = riscv32_user_exception_signal(state->trapno);
	exception->signal_pending = 1U;
	return exception;
}

const riscv32_user_exception_t *
riscv32_user_exception_latest(void)
{
	return riscv32_user_exception_slot();
}

uint32_t
riscv32_user_exception_count(void)
{
	return riscv32_user_exception_slot()->count;
}

uint32_t
riscv32_user_exception_pending_signal(void)
{
	const riscv32_user_exception_t *exception = riscv32_user_exception_slot();

	if (!exception->signal_pending) {
		return 0;
	}
	return exception->linux_signal;
}

void
riscv32_user_exception_clear_pending(void)
{
	riscv32_user_exception_slot()->signal_pending = 0;
}
