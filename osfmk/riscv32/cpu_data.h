#ifndef _RISCV32_CPU_DATA_H_
#define _RISCV32_CPU_DATA_H_

#include <mach_assert.h>
#include <mach/boolean.h>
#include <mach/mach_types.h>
#include <mach/riscv32/thread_status.h>
#include <kern/kern_types.h>
#include <kern/queue.h>
#include <machine/pal_routines.h>
#include <pexpert/pexpert.h>
#include <stdint.h>

#define RISCV32_MAX_CPUS	8

typedef enum {
	TASK_MAP_32BIT = 0
} task_map_t;

struct pmap;

typedef struct rtclock_timer {
	mpqueue_head_t	queue;
	uint64_t	deadline;
	uint64_t	when_set;
	boolean_t	has_expired;
} rtclock_timer_t;

typedef struct cpu_data {
	struct pal_cpu_data	cpu_pal_data;
#define cpu_pd cpu_pal_data
	struct cpu_data		*cpu_this;
	thread_t		cpu_active_thread;
	int			cpu_preemption_level;
	int			cpu_number;
	void			*cpu_int_state;
	vm_offset_t		cpu_active_stack;
	vm_offset_t		cpu_kernel_stack;
	vm_offset_t		cpu_int_stack_top;
	int			cpu_interrupt_level;
	int			cpu_phys_number;
	cpu_id_t		cpu_id;
	int			cpu_signals;
	int			cpu_prior_signals;
	uint32_t		cpu_pending_ast;
	volatile int		cpu_running;
	rtclock_timer_t		rtclock_timer;
	boolean_t		cpu_is64bit;
	volatile task_map_t	cpu_task_map;
	struct pmap		*cpu_active_pmap;
	struct processor	*cpu_processor;
	void			*cpu_chud;
	void			*cpu_console_buf;
	uint64_t		cpu_int_event_time;
	uint64_t		cpu_itime_total;
	uint64_t		cpu_rtime_total;
	uint64_t		debugger_entry_time;
	uint64_t		debugger_ipi_time;
	riscv_saved_state_t	*cpu_fatal_trap_state;
	riscv_saved_state_t	*cpu_post_fatal_trap_state;
	int			cpu_type;
	int			cpu_subtype;
	int			cpu_threadtype;
	boolean_t		cpu_iflag;
	boolean_t		cpu_boot_complete;
	boolean_t		cpu_timer_initialized;
	volatile uint32_t	cpu_ipi_count;
	volatile uint32_t	cpu_timer_interrupt_count;
	volatile uint32_t	cpu_idle_count;
	int			cpu_hibernate;
	volatile uint32_t	cpu_ast_ipi_count;
	volatile uint32_t	cpu_ast_preemption_count;
	volatile uint32_t	cpu_ast_signal_count;
	volatile uint32_t	cpu_wakeup_ipi_count;
	volatile uint32_t	cpu_unhandled_ipi_count;
} cpu_data_t;

extern cpu_data_t *cpu_data_ptr[RISCV32_MAX_CPUS];
extern cpu_data_t cpu_data_master;

static inline cpu_data_t *
current_cpu_datap(void)
{
	cpu_data_t *cpu_data;

	__asm__ volatile("mv %0, tp" : "=r"(cpu_data));
	return cpu_data;
}

static inline void
riscv32_set_current_cpu_datap(cpu_data_t *cpu_data)
{
	__asm__ volatile("mv tp, %0" :: "r"(cpu_data) : "memory");
}

static inline thread_t
get_active_thread(void)
{
	return current_cpu_datap()->cpu_active_thread;
}

#define current_thread_fast()	get_active_thread()
#define current_thread()	current_thread_fast()

static inline boolean_t
get_is64bit(void)
{
	return FALSE;
}

#define cpu_mode_is64bit()	FALSE

static inline int
get_preemption_level(void)
{
	return current_cpu_datap()->cpu_preemption_level;
}

static inline int
get_interrupt_level(void)
{
	return current_cpu_datap()->cpu_interrupt_level;
}

static inline int
get_cpu_number(void)
{
	return current_cpu_datap()->cpu_number;
}

static inline int
get_cpu_phys_number(void)
{
	return current_cpu_datap()->cpu_phys_number;
}

static inline void
disable_preemption(void)
{
	current_cpu_datap()->cpu_preemption_level++;
}

static inline void
enable_preemption(void)
{
	cpu_data_t *cpu_data = current_cpu_datap();

	if (cpu_data->cpu_preemption_level > 0) {
		cpu_data->cpu_preemption_level--;
	}
}

static inline void
enable_preemption_no_check(void)
{
	enable_preemption();
}

static inline void
mp_disable_preemption(void)
{
	disable_preemption();
}

static inline void
mp_enable_preemption(void)
{
	enable_preemption();
}

static inline void
mp_enable_preemption_no_check(void)
{
	enable_preemption_no_check();
}

static inline cpu_data_t *
cpu_datap(int cpu)
{
	if (cpu < 0 || cpu >= RISCV32_MAX_CPUS) {
		return (cpu_data_t *)0;
	}
	return cpu_data_ptr[cpu];
}

extern cpu_data_t *cpu_data_alloc(boolean_t is_boot_cpu);
extern void cpu_data_realloc(void);

#endif /* _RISCV32_CPU_DATA_H_ */
