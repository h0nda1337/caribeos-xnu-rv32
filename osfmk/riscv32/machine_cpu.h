#ifndef _RISCV32_MACHINE_CPU_H_
#define _RISCV32_MACHINE_CPU_H_

#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <kern/kern_types.h>
#include <pexpert/pexpert.h>
#include <sys/cdefs.h>
#include <mach/riscv32/thread_status.h>

#define RISCV32_CPU_SIGNAL_AST	0x00000001U

__BEGIN_DECLS
void cpu_init(void);
void cpu_machine_init(void);
void riscv32_smp_smoke_start(void);
void riscv32_smp_scheduler_start(void);
void riscv32_smp_assert_userspace_cpu0(void);
processor_t riscv32_smp_processor_for_cpu(uint32_t cpu);
boolean_t riscv32_smp_ipi_interrupt(void);
boolean_t riscv32_smp_kernel_preemption_enabled(void);
kern_return_t riscv32_send_ipi_to_hart(uint32_t hartid);
void handle_pending_TLB_flushes(void);
int cpu_signal_handler(riscv_saved_state_t *regs);
kern_return_t cpu_register(int *slot_nump);
__END_DECLS

static inline void
cpu_halt(void)
{
	__asm__ volatile("wfi" ::: "memory");
}

static inline void
cpu_pause(void)
{
	__asm__ volatile("" ::: "memory");
}

#endif /* _RISCV32_MACHINE_CPU_H_ */
