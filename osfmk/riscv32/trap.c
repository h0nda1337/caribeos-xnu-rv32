#include <kern/ast.h>
#include <kern/debug.h>
#include <kern/thread.h>
#include <machine/copyio.h>
#include <machine/cpu_data.h>
#include <machine/early_print.h>
#include <machine/linux_process.h>
#include <machine/linux_signal.h>
#include <machine/machine_cpu.h>
#include <machine/machine_routines.h>
#include <machine/plic.h>
#include <machine/syscall.h>
#include <machine/timer.h>
#include <machine/trap.h>
#include <machine/user_exception.h>

#define RISCV32_SCAUSE_INTERRUPT	0x80000000U
#define RISCV32_SCAUSE_CODE_MASK	0x7fffffffU
#define RISCV32_SSTATUS_SPP		0x00000100U
#define RISCV32_SSTATUS_SPIE		0x00000020U
#define RISCV32_SIP_SSIP		0x00000002U
#define RISCV32_IRQ_SUPERVISOR_SOFTWARE	1U
#define RISCV32_IRQ_SUPERVISOR_TIMER	5U
#define RISCV32_IRQ_SUPERVISOR_EXTERNAL	9U

volatile perfCallback perfTrapHook;
volatile perfCallback perfIntHook;

static inline void
riscv32_write_stvec(uint32_t stvec)
{
	__asm__ volatile("csrw stvec, %0" :: "r"(stvec) : "memory");
}

static inline void
riscv32_write_sscratch(uint32_t sscratch)
{
	__asm__ volatile("csrw sscratch, %0" :: "r"(sscratch) : "memory");
}

static inline void
riscv32_clear_supervisor_software_interrupt(void)
{
	__asm__ volatile("csrc sip, %0" :: "r"(RISCV32_SIP_SSIP) : "memory");
}

void
riscv32_trap_init(void)
{
	riscv32_write_sscratch((uint32_t)(uintptr_t)current_cpu_datap());
	riscv32_write_stvec((uint32_t)(uintptr_t)riscv32_trap_vector);
}

void
riscv32_trap_dispatch(riscv_saved_state_t *state)
{
	uint32_t scause = state->exception.scause;
	uint32_t code = scause & RISCV32_SCAUSE_CODE_MASK;
	uintptr_t lo_spp = 0;
	int user_mode = (state->csr.sstatus & RISCV32_SSTATUS_SPP) == 0;
	riscv32_linux_process_t *process = (riscv32_linux_process_t *)0;

	state->trapno = code;
	if (user_mode) {
		process = riscv32_linux_process_current();
		if (!riscv32_linux_process_current_cpu_allowed(process)) {
			panic("riscv32: userspace trap violated CPU affinity cpu=%d pid=%u affinity=%u",
			    get_cpu_number(), process != (riscv32_linux_process_t *)0 ?
			    process->pid : 0U,
			    process != (riscv32_linux_process_t *)0 ?
			    process->affinity_cpu : UINT32_MAX);
		}
	}

	if ((scause & RISCV32_SCAUSE_INTERRUPT) != 0) {
		if (perfIntHook != 0 &&
		    perfIntHook((int)code, state, &lo_spp, user_mode) == KERN_SUCCESS) {
			return;
		}
		riscv32_interrupt(state);
		return;
	}

	if (perfTrapHook != 0 &&
	    perfTrapHook((int)code, state, &lo_spp, user_mode) == KERN_SUCCESS) {
		return;
	}

	if ((code == T_ECALL_U || code == T_ECALL_S) &&
	    riscv32_syscall_dispatch(state, user_mode)) {
		if (user_mode) {
			(void)riscv32_linux_signal_deliver_pending(state);
		}
		return;
	}

	if (user_mode) {
		riscv32_user_trap(state);
	} else {
		riscv32_kernel_trap(state);
	}
}

void
riscv32_kernel_trap(riscv_saved_state_t *state)
{
	thread_t thread = current_thread();
	riscv32_linux_process_t *process;

	if (thread != THREAD_NULL &&
	    (thread->machine.specFlags & CopyIOActive) != 0 &&
	    thread->machine.copyio_recover != 0 &&
	    (state->trapno == T_LOAD_ACCESS ||
	    state->trapno == T_STORE_ACCESS ||
	    state->trapno == T_LOAD_PAGE_FAULT ||
	    state->trapno == T_STORE_PAGE_FAULT)) {
		thread->machine.copyio_error = RISCV32_COPYIO_EFAULT;
		thread->machine.copyio_fault_addr = state->exception.stval;
		thread->machine.copyio_fault_code = state->trapno;
		state->exception.sepc = (uint32_t)thread->machine.copyio_recover;
		state->thread.pc = state->exception.sepc;
		return;
	}

	process = riscv32_linux_process_current();
	riscv32_early_puts("[riscv32-trap] cpu=");
	riscv32_early_putdec((uint32_t)get_cpu_number());
	riscv32_early_puts(" thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)thread);
	riscv32_early_puts(" pid=");
	riscv32_early_putdec(process != (riscv32_linux_process_t *)0 ?
	    process->pid : 0U);
	riscv32_early_puts(" kernel-stack=0x");
	riscv32_early_puthex32(thread != THREAD_NULL ?
	    (uint32_t)thread->kernel_stack : 0U);
	riscv32_early_puts(" kstackptr=0x");
	riscv32_early_puthex32(thread != THREAD_NULL ?
	    (uint32_t)thread->machine.kstackptr : 0U);
	riscv32_early_puts(" continuation=0x");
	riscv32_early_puthex32(thread != THREAD_NULL ?
	    (uint32_t)(uintptr_t)thread->continuation : 0U);
	riscv32_early_puts("\n[riscv32-trap] ra=0x");
	riscv32_early_puthex32(state->thread.x[RISCV_X_RA]);
	riscv32_early_puts(" sp=0x");
	riscv32_early_puthex32(state->thread.x[RISCV_X_SP]);
	riscv32_early_puts(" s0=0x");
	riscv32_early_puthex32(state->thread.x[8]);
	riscv32_early_puts(" sstatus=0x");
	riscv32_early_puthex32(state->csr.sstatus);
	riscv32_early_puts(" satp=0x");
	riscv32_early_puthex32(state->csr.satp);
	riscv32_early_puts("\n");
	panic("riscv32: kernel trap scause=0x%x stval=0x%x sepc=0x%x",
	    state->exception.scause, state->exception.stval,
	    state->exception.sepc);
}

void
riscv32_user_trap(riscv_saved_state_t *state)
{
	const riscv32_user_exception_t *exception;

	exception = riscv32_user_exception_record(state);
	if (riscv32_linux_signal_handle_user_exception(state)) {
		return;
	}
	panic("riscv32: user trap scause=0x%x stval=0x%x sepc=0x%x signal=%u count=%u",
	    state->exception.scause, state->exception.stval,
	    state->exception.sepc, exception->linux_signal, exception->count);
}

void
riscv32_interrupt(riscv_saved_state_t *state)
{
	cpu_data_t *cpu_data = current_cpu_datap();

	(void)ml_set_interrupts_enabled(FALSE);
	cpu_data->cpu_int_state = state;
	cpu_data->cpu_interrupt_level++;
	if (state->trapno == RISCV32_IRQ_SUPERVISOR_TIMER) {
		(void)__sync_add_and_fetch(
		    &cpu_data->cpu_timer_interrupt_count, 1);
		rtclock_intr(state);
		goto out;
	}
	if (state->trapno == RISCV32_IRQ_SUPERVISOR_SOFTWARE) {
		riscv32_clear_supervisor_software_interrupt();
		(void)__sync_add_and_fetch(&cpu_data->cpu_ipi_count, 1);
		(void)riscv32_smp_ipi_interrupt();
		goto out;
	}
	if (state->trapno == RISCV32_IRQ_SUPERVISOR_EXTERNAL) {
		riscv32_plic_handle_external(state);
		goto out;
	}
	panic("riscv32: interrupt scause=0x%x sepc=0x%x",
	    state->exception.scause, state->exception.sepc);

out:
	cpu_data->cpu_interrupt_level--;
	cpu_data->cpu_int_state = NULL;
	cpu_data->cpu_iflag =
	    (state->csr.sstatus & RISCV32_SSTATUS_SPIE) != 0;
	if (riscv32_smp_kernel_preemption_enabled() &&
	    (state->csr.sstatus & RISCV32_SSTATUS_SPP) != 0 &&
	    cpu_data->cpu_interrupt_level == 0 &&
	    get_preemption_level() == 0 && current_thread() != THREAD_NULL &&
	    (current_thread()->state & TH_IDLE) == 0 &&
	    (*ast_pending() & (AST_PREEMPT | AST_URGENT)) ==
	    (AST_PREEMPT | AST_URGENT)) {
		(void)__sync_add_and_fetch(
		    &cpu_data->cpu_ast_preemption_count, 1);
		ast_taken(AST_PREEMPTION, FALSE);
	}
}
