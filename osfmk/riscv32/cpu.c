/*
 * SPDX-License-Identifier: APSL-2.0
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * https://opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 * CARIBEOS MODIFICATION NOTICE
 * This file was created as part of the experimental CaribeOS RV32IMACSU
 * port of Apple XNU 2050.48.11 on 2026-07-14.
 * 2026-10-12: bound the first secondary timer verification by elapsed time.
 */

#line 1
#include <mach/machine.h>
#include <mach/processor_info.h>
#include <mach/processor.h>
#include <mach/vm_param.h>
#include <kern/ast.h>
#include <kern/debug.h>
#include <kern/etimer.h>
#include <kern/kalloc.h>
#include <kern/machine.h>
#include <kern/processor.h>
#include <kern/sched_prim.h>
#include <kern/simple_lock.h>
#include <kern/thread.h>
#include <kern/timer_call.h>
#include <machine/bootinfo.h>
#include <machine/cpu_capabilities.h>
#include <machine/cpu_data.h>
#include <machine/early_print.h>
#include <machine/machine_cpu.h>
#include <machine/machine_routines.h>
#include <machine/pal_routines.h>
#include <machine/trap.h>
#include <riscv32/sbi.h>
#include <riscv32/sv32.h>
#include <vm/pmap.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct processor processor_master;
typedef struct riscv32_secondary_boot {
	uint32_t stack_top;
	uint32_t slot;
	uint32_t satp;
	volatile uint32_t stage;
	uint32_t cpu_data;
	uint32_t int_stack_top;
	uint32_t mode;
	uint32_t observed_sp;
	uint32_t observed_sscratch;
	uint32_t observed_stvec;
} riscv32_secondary_boot_t;

static struct processor riscv32_processor_secondary[RISCV32_MAX_CPUS - 1];
static cpu_data_t riscv32_cpu_data_secondary[RISCV32_MAX_CPUS - 1];
static uint8_t
    riscv32_secondary_stacks[RISCV32_MAX_CPUS - 1][KERNEL_STACK_SIZE]
    __attribute__((aligned(PAGE_SIZE)));
static uint8_t
    riscv32_interrupt_stacks[RISCV32_MAX_CPUS][KERNEL_STACK_SIZE]
    __attribute__((aligned(PAGE_SIZE)));
static riscv32_secondary_boot_t
    riscv32_secondary_boot[RISCV32_MAX_CPUS - 1];
static uint32_t riscv32_next_cpu_slot = 1;
static boolean_t riscv32_cpu_allow_smp_smoke;

#define RISCV32_SECONDARY_MODE_PARK	0U
#define RISCV32_SECONDARY_MODE_IPI	1U
#define RISCV32_SECONDARY_MODE_SCHEDULER	2U
#define RISCV32_SIE_SSIE		0x00000002U
#define RISCV32_SSTATUS_SIE		0x00000002U
#define RISCV32_SMP_GATE_B_ROUNDS	64U
#define RISCV32_SMP_GATE_D_ROUNDS	10000U
#define RISCV32_SMP_GATE_D_PAYLOAD_XOR	0xc41be000U
#define RISCV32_SMP_GATE_E_ROUNDS	1024U
#define RISCV32_SMP_GATE_E_ALIAS	0xffc00000U
#define RISCV32_SMP_GATE_E_VALUE_A	0x43415241U
#define RISCV32_SMP_GATE_E_VALUE_B	0x43415242U
#define RISCV32_SMP_GATE_F_ROUNDS	50000U
#define RISCV32_SMP_GATE_F_CPUS		2U

static volatile uint32_t riscv32_gate_b_active;
static volatile uint32_t riscv32_gate_b_request_sequence;
static volatile uint32_t riscv32_gate_b_response_sequence;
static volatile uint32_t riscv32_gate_b_boot_sequence;
static volatile uint32_t riscv32_gate_b_secondary_sequence;
static volatile uint32_t riscv32_gate_b_boot_count;
static volatile uint32_t riscv32_gate_b_secondary_count;
static volatile uint32_t riscv32_gate_b_unexpected_count;
static volatile uint32_t riscv32_gate_d_request_sequence;
static volatile uint32_t riscv32_gate_d_response_sequence;
static volatile uint32_t riscv32_gate_d_payload;
static volatile uint32_t riscv32_gate_d_completed;
static volatile uint32_t riscv32_gate_d_errors;
static volatile uint32_t riscv32_gate_d_worker_started;
static volatile uint32_t riscv32_gate_d_worker_done;
static volatile uint32_t riscv32_gate_d_worker_cpu;
static volatile uint32_t riscv32_gate_d_worker_thread;
static uint32_t riscv32_gate_d_wait_event;
static uint8_t riscv32_gate_e_page_a[PAGE_SIZE]
    __attribute__((aligned(PAGE_SIZE)));
static uint8_t riscv32_gate_e_page_b[PAGE_SIZE]
    __attribute__((aligned(PAGE_SIZE)));
static volatile uint32_t riscv32_gate_e_request_sequence;
static volatile uint32_t riscv32_gate_e_response_sequence;
static volatile uint32_t riscv32_gate_e_observations;
static volatile uint32_t riscv32_gate_e_stale;
static volatile uint32_t riscv32_gate_e_errors;
static volatile uint32_t riscv32_gate_e_worker_started;
static volatile uint32_t riscv32_gate_e_worker_done;
static volatile uint32_t riscv32_gate_e_worker_cpu;
static volatile uint32_t riscv32_gate_e_initial_value;
static uint32_t riscv32_gate_e_wait_event;
static hw_lock_data_t riscv32_gate_f_lock;
static volatile uint32_t riscv32_gate_f_start;
static volatile uint32_t riscv32_gate_f_ready;
static volatile uint32_t riscv32_gate_f_active_workers;
static volatile uint32_t riscv32_gate_f_done;
static volatile uint32_t riscv32_gate_f_iterations[RISCV32_SMP_GATE_F_CPUS];
static volatile uint32_t riscv32_gate_f_saw_peer[RISCV32_SMP_GATE_F_CPUS];
static volatile uint32_t riscv32_gate_f_locked_total;
static volatile uint32_t riscv32_gate_f_critical_owner;
static volatile uint32_t riscv32_gate_f_overlap;
static volatile uint32_t riscv32_gate_f_errors;
static volatile uint32_t riscv32_gate_f_worker_cpu;
static volatile uint32_t riscv32_gate_f_preempt_started;
static volatile uint32_t riscv32_gate_f_preempt_cpu;
static volatile uint32_t riscv32_gate_f_preempt_before_release;
static uint32_t riscv32_gate_f_wait_event;
static uint32_t riscv32_gate_f_preempt_wait_event;
static boolean_t riscv32_secondary_processor_initialized;

extern void riscv32_secondary_entry(void);
extern void riscv32_secondary_entry_c(uint32_t hartid, uint32_t slot);
extern void slave_main(void *machine_param);

static boolean_t
riscv32_smp_scheduler_requested(void)
{
	return riscv32_boot_arg_has("smp-gate-c") ||
	    riscv32_boot_arg_has("smp-gate-d") ||
	    riscv32_boot_arg_has("smp-gate-e") ||
	    riscv32_boot_arg_has("smp-gate-f");
}

static boolean_t
riscv32_smp_gate_d_requested(void)
{
	return riscv32_boot_arg_has("smp-gate-d") ||
	    riscv32_boot_arg_has("smp-gate-e") ||
	    riscv32_boot_arg_has("smp-gate-f");
}

static boolean_t
riscv32_smp_gate_e_requested(void)
{
	return riscv32_boot_arg_has("smp-gate-e") ||
	    riscv32_boot_arg_has("smp-gate-f");
}

static boolean_t
riscv32_smp_gate_f_requested(void)
{
	return riscv32_smp_kernel_preemption_enabled();
}

boolean_t
riscv32_smp_kernel_preemption_enabled(void)
{
	return riscv32_boot_arg_has("smp-gate-f");
}

cpu_data_t cpu_data_master = {
	.cpu_this = &cpu_data_master,
	.cpu_number = 0,
	.cpu_phys_number = 0,
	.cpu_id = 0,
	.cpu_running = TRUE,
	.cpu_type = CPU_TYPE_RISCV32,
	.cpu_subtype = CPU_SUBTYPE_RISCV32_ALL,
	.cpu_threadtype = CPU_THREADTYPE_NONE,
	.cpu_iflag = TRUE,
	.cpu_processor = &processor_master,
};

cpu_data_t *cpu_data_ptr[RISCV32_MAX_CPUS] = {
	&cpu_data_master,
};

typedef char riscv32_pal_scratch_t0_offset[
    offsetof(struct pal_cpu_data, scratch_t0) == 0 ? 1 : -1];
typedef char riscv32_pal_scratch_t1_offset[
    offsetof(struct pal_cpu_data, scratch_t1) == 4 ? 1 : -1];
typedef char riscv32_pal_scratch_t2_offset[
    offsetof(struct pal_cpu_data, scratch_t2) == 8 ? 1 : -1];
typedef char riscv32_pal_scratch_tp_offset[
    offsetof(struct pal_cpu_data, scratch_tp) == 12 ? 1 : -1];
typedef char riscv32_pal_scratch_sp_offset[
    offsetof(struct pal_cpu_data, scratch_sp) == 16 ? 1 : -1];
typedef char riscv32_pal_trap_stack_offset[
    offsetof(struct pal_cpu_data, trap_stack_top) == 20 ? 1 : -1];
typedef char riscv32_cpu_active_thread_offset[
    offsetof(cpu_data_t, cpu_active_thread) == 28 ? 1 : -1];
typedef char riscv32_cpu_kernel_stack_offset[
    offsetof(cpu_data_t, cpu_kernel_stack) == 48 ? 1 : -1];

static void
riscv32_cpu_timer_queue_init(cpu_data_t *cpu_data)
{
	if (cpu_data->cpu_timer_initialized) {
		return;
	}
	timer_call_initialize_queue(&cpu_data->rtclock_timer.queue);
	cpu_data->rtclock_timer.deadline = EndOfAllTime;
	cpu_data->cpu_timer_initialized = TRUE;
}

static void
riscv32_cpu_sync_boot_data(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();

	if (bootinfo == (const riscv32_bootinfo_t *)0) {
		return;
	}
	cpu_data_master.cpu_phys_number = (int)bootinfo->hartid;
	cpu_data_master.cpu_id = (cpu_id_t)(uintptr_t)bootinfo->hartid;
	cpu_data_master.cpu_type = CPU_TYPE_RISCV32;
	cpu_data_master.cpu_subtype = CPU_SUBTYPE_RISCV32_ALL;
	cpu_data_master.cpu_threadtype = CPU_THREADTYPE_NONE;
	cpu_data_master.cpu_is64bit = FALSE;
}

void
cpu_init(void)
{
	cpu_data_t *cpu_data = current_cpu_datap();

	if (cpu_data == &cpu_data_master) {
		riscv32_cpu_sync_boot_data();
		if (cpu_data->cpu_int_stack_top == 0) {
			cpu_data->cpu_int_stack_top = (vm_offset_t)(uintptr_t)
			    &riscv32_interrupt_stacks[0][KERNEL_STACK_SIZE];
		}
	}
	riscv32_cpu_timer_queue_init(cpu_data);
}

kern_return_t
cpu_control(int slot_num, processor_info_t info, unsigned int count)
{
	(void)slot_num;
	(void)info;
	(void)count;
	return KERN_FAILURE;
}

kern_return_t
cpu_info_count(processor_flavor_t flavor, unsigned int *count)
{
	if (count == NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	switch (flavor) {
	case PROCESSOR_BASIC_INFO:
		*count = PROCESSOR_BASIC_INFO_COUNT;
		return KERN_SUCCESS;
	case PROCESSOR_CPU_LOAD_INFO:
		*count = PROCESSOR_CPU_LOAD_INFO_COUNT;
		return KERN_SUCCESS;
	default:
		break;
	}

	*count = 0;
	return KERN_INVALID_ARGUMENT;
}

kern_return_t
cpu_info(processor_flavor_t flavor, int slot_num, processor_info_t info,
    unsigned int *count)
{
	cpu_data_t *cpu_data;

	if (info == NULL || count == NULL || slot_num < 0 ||
	    slot_num >= RISCV32_MAX_CPUS) {
		return KERN_INVALID_ARGUMENT;
	}

	cpu_data = cpu_datap(slot_num);
	if (cpu_data == NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	switch (flavor) {
	case PROCESSOR_BASIC_INFO:
	{
		processor_basic_info_t basic = (processor_basic_info_t)info;

		if (*count < PROCESSOR_BASIC_INFO_COUNT) {
			return KERN_FAILURE;
		}
		basic->cpu_type = CPU_TYPE_RISCV32;
		basic->cpu_subtype = CPU_SUBTYPE_RISCV32_ALL;
		basic->running = cpu_data->cpu_running != 0;
		basic->slot_num = slot_num;
		basic->is_master = slot_num == 0;
		*count = PROCESSOR_BASIC_INFO_COUNT;
		return KERN_SUCCESS;
	}
	case PROCESSOR_CPU_LOAD_INFO:
	{
		processor_cpu_load_info_t load =
		    (processor_cpu_load_info_t)info;

		if (*count < PROCESSOR_CPU_LOAD_INFO_COUNT) {
			return KERN_FAILURE;
		}
		memset(load, 0, sizeof(*load));
		load->cpu_ticks[CPU_STATE_IDLE] =
		    (unsigned int)(mach_absolute_time() / 10000000ULL);
		*count = PROCESSOR_CPU_LOAD_INFO_COUNT;
		return KERN_SUCCESS;
	}
	default:
		break;
	}

	*count = 0;
	return KERN_INVALID_ARGUMENT;
}

void
cpu_sleep(void)
{
	panic("riscv32: cpu_sleep unimplemented");
}

static cpu_data_t *
riscv32_cpu_ensure_slot(uint32_t slot)
{
	while (riscv32_next_cpu_slot <= slot) {
		if (cpu_data_alloc(FALSE) == NULL) {
			return NULL;
		}
	}
	return cpu_data_ptr[slot];
}

kern_return_t
cpu_start(int slot_num)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	struct riscv32_sbi_ret ret;
	cpu_data_t *cpu_data;
	uint32_t slot = (uint32_t)slot_num;
	uint32_t hartid;
	uint32_t start_addr;
	uint32_t opaque;

	if (slot_num == 0) {
		return KERN_SUCCESS;
	}
	if (slot_num < 0 || slot >= RISCV32_MAX_CPUS ||
	    slot >= bootinfo->cpu_count) {
		return KERN_FAILURE;
	}
	if (!riscv32_cpu_allow_smp_smoke &&
	    !riscv32_boot_arg_has("smp-start")) {
		return KERN_FAILURE;
	}
	if (!riscv32_sbi_probe_extension(RISCV32_SBI_EXT_HSM)) {
		riscv32_early_puts("[smp] SBI HSM extension unavailable\n");
		return KERN_NOT_SUPPORTED;
	}

	cpu_data = riscv32_cpu_ensure_slot(slot);
	if (cpu_data == NULL) {
		return KERN_RESOURCE_SHORTAGE;
	}

	hartid = (uint32_t)cpu_data->cpu_phys_number;
	if (hartid == bootinfo->hartid) {
		riscv32_early_puts("[smp] refusing to start boot hart=");
		riscv32_early_putdec(hartid);
		riscv32_early_puts(" as secondary slot=");
		riscv32_early_putdec(slot);
		riscv32_early_puts("\n");
		return KERN_INVALID_ARGUMENT;
	}
	riscv32_secondary_boot[slot - 1].stack_top = (uint32_t)
	    riscv32_kva_to_phys((vm_offset_t)(uintptr_t)
	    &riscv32_secondary_stacks[slot - 1][KERNEL_STACK_SIZE]);
	riscv32_secondary_boot[slot - 1].slot = slot;
	riscv32_secondary_boot[slot - 1].satp = riscv32_read_satp();
	riscv32_secondary_boot[slot - 1].stage = 0;
	riscv32_secondary_boot[slot - 1].cpu_data = (uint32_t)
	    riscv32_kva_to_phys((vm_offset_t)(uintptr_t)cpu_data);
	riscv32_secondary_boot[slot - 1].int_stack_top = (uint32_t)
	    riscv32_kva_to_phys((vm_offset_t)(uintptr_t)
	    &riscv32_interrupt_stacks[slot][KERNEL_STACK_SIZE]);
	if (riscv32_smp_scheduler_requested()) {
		riscv32_secondary_boot[slot - 1].mode =
		    RISCV32_SECONDARY_MODE_SCHEDULER;
	} else if (riscv32_boot_arg_has("smp-gate-b")) {
		riscv32_secondary_boot[slot - 1].mode =
		    RISCV32_SECONDARY_MODE_IPI;
	} else {
		riscv32_secondary_boot[slot - 1].mode =
		    RISCV32_SECONDARY_MODE_PARK;
	}
	riscv32_secondary_boot[slot - 1].observed_sp = 0;
	riscv32_secondary_boot[slot - 1].observed_sscratch = 0;
	riscv32_secondary_boot[slot - 1].observed_stvec = 0;
	__sync_synchronize();

	start_addr = (uint32_t)riscv32_kva_to_phys(
	    (vm_offset_t)(uintptr_t)riscv32_secondary_entry);
	opaque = (uint32_t)riscv32_kva_to_phys(
	    (vm_offset_t)(uintptr_t)&riscv32_secondary_boot[slot - 1]);

	ret = riscv32_sbi_hart_start(hartid, start_addr, opaque);
	if (ret.error == RISCV32_SBI_SUCCESS ||
	    ret.error == RISCV32_SBI_ERR_ALREADY_STARTED) {
		return KERN_SUCCESS;
	}

	riscv32_early_puts("[smp] hart_start failed hart=");
	riscv32_early_putdec(hartid);
	riscv32_early_puts(" error=");
	riscv32_early_putdec((uint32_t)(0 - ret.error));
	riscv32_early_puts("\n");
	return KERN_FAILURE;
}

static void
riscv32_smp_smoke_print_status(uint32_t hartid)
{
	struct riscv32_sbi_ret ret;

	ret = riscv32_sbi_hart_get_status(hartid);
	riscv32_early_puts("[smp-smoke] hsm hart=");
	riscv32_early_putdec(hartid);
	riscv32_early_puts(" status=");
	if (ret.error == RISCV32_SBI_SUCCESS) {
		riscv32_early_putdec((uint32_t)ret.value);
	} else {
		riscv32_early_puts("err:");
		riscv32_early_putdec((uint32_t)(0 - ret.error));
	}
	riscv32_early_puts("\n");
}

static long
riscv32_smp_smoke_wait_status(uint32_t hartid)
{
	struct riscv32_sbi_ret ret;
	uint32_t tries;

	ret.error = RISCV32_SBI_ERR_FAILED;
	ret.value = -1;
	for (tries = 0; tries < 100000; tries++) {
		ret = riscv32_sbi_hart_get_status(hartid);
		if (ret.error != RISCV32_SBI_SUCCESS ||
		    ret.value != RISCV32_SBI_HSM_START_PENDING) {
			break;
		}
		__asm__ volatile("nop" ::: "memory");
	}
	return ret.error == RISCV32_SBI_SUCCESS ? ret.value : ret.error;
}

static uint32_t
riscv32_smp_smoke_wait_stage(uint32_t slot, uint32_t target)
{
	uint32_t tries;
	volatile uint32_t *stage;

	if (slot == 0 || slot >= RISCV32_MAX_CPUS) {
		return 0;
	}
	stage = &riscv32_secondary_boot[slot - 1].stage;
	for (tries = 0; tries < 2000000; tries++) {
		if (*stage == 0xff || *stage >= target) {
			break;
		}
		__asm__ volatile("nop" ::: "memory");
	}
	return *stage;
}

static void
riscv32_smp_gate_b_reset(void)
{
	riscv32_gate_b_active = 0;
	riscv32_gate_b_request_sequence = 0;
	riscv32_gate_b_response_sequence = 0;
	riscv32_gate_b_boot_sequence = 0;
	riscv32_gate_b_secondary_sequence = 0;
	riscv32_gate_b_boot_count = 0;
	riscv32_gate_b_secondary_count = 0;
	riscv32_gate_b_unexpected_count = 0;
	__sync_synchronize();
}

boolean_t
riscv32_smp_ipi_interrupt(void)
{
	cpu_data_t *cpu_data = current_cpu_datap();
	processor_t processor;
	uint32_t slot = (uint32_t)get_cpu_number();
	uint32_t sequence;
	uint32_t signals;
	uint32_t handled = 0;

	if (riscv32_gate_b_active != 0 && slot == 1) {
		__sync_synchronize();
		sequence = riscv32_gate_b_request_sequence;
		if (sequence == 0 ||
		    sequence <= riscv32_gate_b_secondary_sequence) {
			(void)__sync_add_and_fetch(
			    &riscv32_gate_b_unexpected_count, 1);
			return TRUE;
		}
		riscv32_gate_b_secondary_sequence = sequence;
		(void)__sync_add_and_fetch(&riscv32_gate_b_secondary_count, 1);
		__sync_synchronize();
		riscv32_gate_b_response_sequence = sequence;
		__sync_synchronize();
		if (riscv32_send_ipi_to_hart(
		    riscv32_bootinfo()->hartid) != KERN_SUCCESS) {
			(void)__sync_add_and_fetch(
			    &riscv32_gate_b_unexpected_count, 1);
		}
		return TRUE;
	}

	if (riscv32_gate_b_active != 0 && slot == 0) {
		sequence = riscv32_gate_b_response_sequence;
		if (sequence == 0 || sequence <= riscv32_gate_b_boot_sequence) {
			(void)__sync_add_and_fetch(
			    &riscv32_gate_b_unexpected_count, 1);
			return TRUE;
		}
		riscv32_gate_b_boot_sequence = sequence;
		(void)__sync_add_and_fetch(&riscv32_gate_b_boot_count, 1);
		return TRUE;
	}

	if (riscv32_gate_b_active != 0) {
		(void)__sync_add_and_fetch(&riscv32_gate_b_unexpected_count, 1);
		return TRUE;
	}

	signals = (uint32_t)__sync_lock_test_and_set(&cpu_data->cpu_signals, 0);
	cpu_data->cpu_prior_signals = (int)signals;
	__sync_synchronize();
	if ((signals & RISCV32_CPU_SIGNAL_AST) != 0) {
		processor = cpu_data->cpu_processor;
		(void)__sync_add_and_fetch(&cpu_data->cpu_ast_ipi_count, 1);
		if (processor != PROCESSOR_NULL &&
		    processor->active_thread != THREAD_NULL) {
			ast_check(processor);
		} else {
			(void)__sync_add_and_fetch(
			    &cpu_data->cpu_unhandled_ipi_count, 1);
		}
		handled |= RISCV32_CPU_SIGNAL_AST;
	}
	if ((signals & ~RISCV32_CPU_SIGNAL_AST) != 0) {
		(void)__sync_add_and_fetch(&cpu_data->cpu_unhandled_ipi_count, 1);
	}
	if (signals == 0) {
		(void)__sync_add_and_fetch(&cpu_data->cpu_wakeup_ipi_count, 1);
		return TRUE;
	}
	return handled != 0;
}

static boolean_t
riscv32_smp_gate_b_wait_for(uint32_t sequence)
{
	uint32_t tries;

	for (tries = 0; tries < 2000000; tries++) {
		if (riscv32_gate_b_boot_sequence == sequence) {
			return TRUE;
		}
		__asm__ volatile("nop" ::: "memory");
	}
	return FALSE;
}

static boolean_t
riscv32_smp_gate_b_run(uint32_t slot)
{
	cpu_data_t *secondary = cpu_datap((int)slot);
	uint32_t sequence;
	uint32_t ssie = RISCV32_SIE_SSIE;
	uint32_t sstatus_sie = RISCV32_SSTATUS_SIE;
	boolean_t passed = TRUE;

	if (secondary == NULL ||
	    riscv32_secondary_boot[slot - 1].stage < 7) {
		return FALSE;
	}

	riscv32_smp_gate_b_reset();
	__asm__ volatile("csrc sip, %0" :: "r"(ssie) : "memory");
	__asm__ volatile("csrs sie, %0" :: "r"(ssie) : "memory");
	__asm__ volatile("csrs sstatus, %0" :: "r"(sstatus_sie) : "memory");
	riscv32_gate_b_active = 1;
	__sync_synchronize();

	for (sequence = 1; sequence <= RISCV32_SMP_GATE_B_ROUNDS;
	    sequence++) {
		riscv32_gate_b_request_sequence = sequence;
		__sync_synchronize();
		if (riscv32_send_ipi_to_hart(
		    (uint32_t)secondary->cpu_phys_number) != KERN_SUCCESS ||
		    !riscv32_smp_gate_b_wait_for(sequence)) {
			passed = FALSE;
			break;
		}
	}

	__sync_synchronize();
	passed = passed &&
	    riscv32_gate_b_boot_sequence == RISCV32_SMP_GATE_B_ROUNDS &&
	    riscv32_gate_b_secondary_sequence == RISCV32_SMP_GATE_B_ROUNDS &&
	    riscv32_gate_b_boot_count == RISCV32_SMP_GATE_B_ROUNDS &&
	    riscv32_gate_b_secondary_count == RISCV32_SMP_GATE_B_ROUNDS &&
	    riscv32_gate_b_unexpected_count == 0;
	riscv32_gate_b_active = 0;
	__sync_synchronize();

	riscv32_early_puts("[smp-gate-b] ping-pong rounds=");
	riscv32_early_putdec(RISCV32_SMP_GATE_B_ROUNDS);
	riscv32_early_puts(" hart0-acks=");
	riscv32_early_putdec(riscv32_gate_b_boot_count);
	riscv32_early_puts(" hart1-acks=");
	riscv32_early_putdec(riscv32_gate_b_secondary_count);
	riscv32_early_puts(" unexpected=");
	riscv32_early_putdec(riscv32_gate_b_unexpected_count);
	riscv32_early_puts("\n");
	if (passed) {
		riscv32_early_puts(
		    "[smp-gate-b] bidirectional SBI IPI/SSIP PASS\n");
	} else {
		riscv32_early_puts(
		    "[smp-gate-b] bidirectional SBI IPI/SSIP FAIL\n");
	}
	return passed;
}

static void
riscv32_smp_gate_a_print(uint32_t slot)
{
	cpu_data_t *cpu_data = cpu_datap((int)slot);

	if (cpu_data == NULL || slot == 0 || slot >= RISCV32_MAX_CPUS) {
		return;
	}
	riscv32_early_puts("[smp-gate-a] hart=");
	riscv32_early_putdec((uint32_t)cpu_data->cpu_phys_number);
	riscv32_early_puts(" slot=");
	riscv32_early_putdec(slot);
	riscv32_early_puts(" cpu_data=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)cpu_data);
	riscv32_early_puts(" stack=0x");
	riscv32_early_puthex32(
	    riscv32_secondary_boot[slot - 1].observed_sp);
	riscv32_early_puts(" int_stack=0x");
	riscv32_early_puthex32((uint32_t)cpu_data->cpu_int_stack_top);
	riscv32_early_puts("\n");
	riscv32_early_puts(
	    "[smp-gate-a] per-hart tp/sscratch/stvec/stacks PASS\n");
}

static void
riscv32_smp_gate_d_worker(void *parameter, wait_result_t wait_result)
{
	processor_t processor = (processor_t)parameter;
	thread_t thread = current_thread();
	uint32_t expected;
	uint32_t sequence;

	(void)wait_result;
	riscv32_gate_d_worker_cpu = (uint32_t)get_cpu_number();
	riscv32_gate_d_worker_thread = (uint32_t)(uintptr_t)thread;
	if (processor == PROCESSOR_NULL || get_cpu_number() != 1 ||
	    current_processor() != processor ||
	    thread->bound_processor != processor) {
		(void)__sync_add_and_fetch(&riscv32_gate_d_errors, 1);
	}
	__sync_synchronize();
	riscv32_gate_d_worker_started = 1;
	__sync_synchronize();

	for (expected = 1; expected <= RISCV32_SMP_GATE_D_ROUNDS;
	    expected++) {
		do {
			sequence = riscv32_gate_d_request_sequence;
			cpu_pause();
		} while (sequence < expected);
		__sync_synchronize();
		if (sequence != expected ||
		    riscv32_gate_d_payload !=
		    (expected ^ RISCV32_SMP_GATE_D_PAYLOAD_XOR)) {
			(void)__sync_add_and_fetch(&riscv32_gate_d_errors, 1);
		}
		(void)__sync_add_and_fetch(&riscv32_gate_d_completed, 1);
		__sync_synchronize();
		riscv32_gate_d_response_sequence = expected;
		__sync_synchronize();
	}

	riscv32_gate_d_worker_done = 1;
	__sync_synchronize();
	(void)assert_wait((event_t)&riscv32_gate_d_wait_event, THREAD_UNINT);
	(void)thread_block(THREAD_CONTINUE_NULL);
	panic("riscv32: Gate D worker unexpectedly resumed");
}

static boolean_t
riscv32_smp_gate_d_wait(volatile uint32_t *value, uint32_t target)
{
	uint32_t tries;

	for (tries = 0; tries < 20000000U; tries++) {
		if (*value == target) {
			__sync_synchronize();
			return TRUE;
		}
		cpu_pause();
	}
	return FALSE;
}

static boolean_t
riscv32_smp_gate_d_run(processor_t processor)
{
	thread_t thread;
	kern_return_t kr;
	spl_t s;
	uint32_t sequence;
	boolean_t passed;

	riscv32_gate_d_request_sequence = 0;
	riscv32_gate_d_response_sequence = 0;
	riscv32_gate_d_payload = 0;
	riscv32_gate_d_completed = 0;
	riscv32_gate_d_errors = 0;
	riscv32_gate_d_worker_started = 0;
	riscv32_gate_d_worker_done = 0;
	riscv32_gate_d_worker_cpu = UINT32_MAX;
	riscv32_gate_d_worker_thread = 0;
	__sync_synchronize();

	kr = kernel_thread_create(riscv32_smp_gate_d_worker, processor,
	    BASEPRI_KERNEL, &thread);
	if (kr != KERN_SUCCESS) {
		riscv32_early_puts("[smp-gate-d] kernel_thread_create FAIL kr=");
		riscv32_early_putdec((uint32_t)kr);
		riscv32_early_puts("\n");
		return FALSE;
	}
	s = splsched();
	thread_lock(thread);
	thread->bound_processor = processor;
	thread_unlock(thread);
	splx(s);
	thread_mtx_lock(thread);
	thread_start_internal(thread);
	thread_mtx_unlock(thread);

	if (!riscv32_smp_gate_d_wait(&riscv32_gate_d_worker_started, 1)) {
		riscv32_early_puts("[smp-gate-d] worker start timeout FAIL\n");
		thread_deallocate(thread);
		return FALSE;
	}
	for (sequence = 1; sequence <= RISCV32_SMP_GATE_D_ROUNDS;
	    sequence++) {
		riscv32_gate_d_payload =
		    sequence ^ RISCV32_SMP_GATE_D_PAYLOAD_XOR;
		__sync_synchronize();
		riscv32_gate_d_request_sequence = sequence;
		__sync_synchronize();
		if (!riscv32_smp_gate_d_wait(
		    &riscv32_gate_d_response_sequence, sequence)) {
			(void)__sync_add_and_fetch(&riscv32_gate_d_errors, 1);
			break;
		}
	}
	if (sequence > RISCV32_SMP_GATE_D_ROUNDS) {
		(void)riscv32_smp_gate_d_wait(&riscv32_gate_d_worker_done, 1);
	}
	passed = sequence > RISCV32_SMP_GATE_D_ROUNDS &&
	    riscv32_gate_d_worker_done == 1 &&
	    riscv32_gate_d_request_sequence == RISCV32_SMP_GATE_D_ROUNDS &&
	    riscv32_gate_d_response_sequence == RISCV32_SMP_GATE_D_ROUNDS &&
	    riscv32_gate_d_completed == RISCV32_SMP_GATE_D_ROUNDS &&
	    riscv32_gate_d_worker_cpu == 1 && riscv32_gate_d_errors == 0;

	riscv32_early_puts("[smp-gate-d] rounds=");
	riscv32_early_putdec(RISCV32_SMP_GATE_D_ROUNDS);
	riscv32_early_puts(" requests=");
	riscv32_early_putdec(riscv32_gate_d_request_sequence);
	riscv32_early_puts(" completions=");
	riscv32_early_putdec(riscv32_gate_d_completed);
	riscv32_early_puts(" cpu=");
	riscv32_early_putdec(riscv32_gate_d_worker_cpu);
	riscv32_early_puts(" errors=");
	riscv32_early_putdec(riscv32_gate_d_errors);
	riscv32_early_puts("\n");
	if (passed) {
		riscv32_early_puts(
		    "[smp-gate-d] bound XNU kernel thread synchronization PASS\n");
	} else {
		riscv32_early_puts(
		    "[smp-gate-d] bound XNU kernel thread synchronization FAIL\n");
	}
	thread_deallocate(thread);
	return passed;
}

static uint32_t
riscv32_smp_gate_e_expected(uint32_t sequence)
{
	return (sequence & 1U) != 0 ? RISCV32_SMP_GATE_E_VALUE_B :
	    RISCV32_SMP_GATE_E_VALUE_A;
}

static void
riscv32_smp_gate_e_worker(void *parameter, wait_result_t wait_result)
{
	processor_t processor = (processor_t)parameter;
	thread_t thread = current_thread();
	volatile uint32_t *alias =
	    (volatile uint32_t *)(uintptr_t)RISCV32_SMP_GATE_E_ALIAS;
	uint32_t expected;
	uint32_t sequence;
	uint32_t observed;

	(void)wait_result;
	riscv32_gate_e_worker_cpu = (uint32_t)get_cpu_number();
	if (processor == PROCESSOR_NULL || get_cpu_number() != 1 ||
	    current_processor() != processor ||
	    thread->bound_processor != processor ||
	    riscv32_pmap_current() != kernel_pmap) {
		(void)__sync_add_and_fetch(&riscv32_gate_e_errors, 1);
	}
	riscv32_gate_e_initial_value = *alias;
	__sync_synchronize();
	riscv32_gate_e_worker_started = 1;
	__sync_synchronize();

	for (expected = 1; expected <= RISCV32_SMP_GATE_E_ROUNDS;
	    expected++) {
		do {
			sequence = riscv32_gate_e_request_sequence;
			cpu_pause();
		} while (sequence < expected);
		__sync_synchronize();
		observed = *alias;
		if (sequence != expected ||
		    observed != riscv32_smp_gate_e_expected(expected)) {
			(void)__sync_add_and_fetch(&riscv32_gate_e_stale, 1);
		}
		(void)__sync_add_and_fetch(&riscv32_gate_e_observations, 1);
		__sync_synchronize();
		riscv32_gate_e_response_sequence = expected;
		__sync_synchronize();
	}
	riscv32_gate_e_worker_done = 1;
	__sync_synchronize();
	(void)assert_wait((event_t)&riscv32_gate_e_wait_event, THREAD_UNINT);
	(void)thread_block(THREAD_CONTINUE_NULL);
	panic("riscv32: Gate E worker unexpectedly resumed");
}

static boolean_t
riscv32_smp_gate_e_run(processor_t processor)
{
	volatile uint32_t *alias =
	    (volatile uint32_t *)(uintptr_t)RISCV32_SMP_GATE_E_ALIAS;
	thread_t thread;
	kern_return_t kr;
	spl_t s;
	ppnum_t page_a;
	ppnum_t page_b;
	cpu_set active_mask;
	uint32_t sequence;
	uint32_t expected;
	uint32_t local_before;
	uint32_t remote_before;
	uint32_t errors_before;
	uint32_t local_delta;
	uint32_t remote_delta;
	uint32_t remote_errors;
	boolean_t passed;

	*(volatile uint32_t *)(void *)riscv32_gate_e_page_a =
	    RISCV32_SMP_GATE_E_VALUE_A;
	*(volatile uint32_t *)(void *)riscv32_gate_e_page_b =
	    RISCV32_SMP_GATE_E_VALUE_B;
	page_a = (ppnum_t)(riscv32_kva_to_phys((vm_offset_t)(uintptr_t)
	    riscv32_gate_e_page_a) >> PAGE_SHIFT);
	page_b = (ppnum_t)(riscv32_kva_to_phys((vm_offset_t)(uintptr_t)
	    riscv32_gate_e_page_b) >> PAGE_SHIFT);
	riscv32_gate_e_request_sequence = 0;
	riscv32_gate_e_response_sequence = 0;
	riscv32_gate_e_observations = 0;
	riscv32_gate_e_stale = 0;
	riscv32_gate_e_errors = 0;
	riscv32_gate_e_worker_started = 0;
	riscv32_gate_e_worker_done = 0;
	riscv32_gate_e_worker_cpu = UINT32_MAX;
	riscv32_gate_e_initial_value = 0;
	__sync_synchronize();

	local_before = kernel_pmap->tlb_local_fence_count;
	remote_before = kernel_pmap->tlb_remote_fence_count;
	errors_before = kernel_pmap->tlb_remote_fence_errors;
	pmap_enter(kernel_pmap, RISCV32_SMP_GATE_E_ALIAS, page_a,
	    VM_PROT_READ | VM_PROT_WRITE, VM_PROT_NONE, 0, FALSE);
	if (*alias != RISCV32_SMP_GATE_E_VALUE_A) {
		(void)__sync_add_and_fetch(&riscv32_gate_e_errors, 1);
	}

	kr = kernel_thread_create(riscv32_smp_gate_e_worker, processor,
	    BASEPRI_KERNEL, &thread);
	if (kr != KERN_SUCCESS) {
		riscv32_early_puts("[smp-gate-e] kernel_thread_create FAIL kr=");
		riscv32_early_putdec((uint32_t)kr);
		riscv32_early_puts("\n");
		return FALSE;
	}
	s = splsched();
	thread_lock(thread);
	thread->bound_processor = processor;
	thread_unlock(thread);
	splx(s);
	thread_mtx_lock(thread);
	thread_start_internal(thread);
	thread_mtx_unlock(thread);
	if (!riscv32_smp_gate_d_wait(&riscv32_gate_e_worker_started, 1)) {
		riscv32_early_puts("[smp-gate-e] worker start timeout FAIL\n");
		thread_deallocate(thread);
		return FALSE;
	}

	active_mask = riscv32_pmap_active_cpu_mask(kernel_pmap);
	for (sequence = 1; sequence <= RISCV32_SMP_GATE_E_ROUNDS;
	    sequence++) {
		expected = riscv32_smp_gate_e_expected(sequence);
		pmap_enter(kernel_pmap, RISCV32_SMP_GATE_E_ALIAS,
		    (sequence & 1U) != 0 ? page_b : page_a,
		    VM_PROT_READ | VM_PROT_WRITE, VM_PROT_NONE, 0, FALSE);
		if (*alias != expected) {
			(void)__sync_add_and_fetch(&riscv32_gate_e_errors, 1);
		}
		__sync_synchronize();
		riscv32_gate_e_request_sequence = sequence;
		__sync_synchronize();
		if (!riscv32_smp_gate_d_wait(
		    &riscv32_gate_e_response_sequence, sequence)) {
			(void)__sync_add_and_fetch(&riscv32_gate_e_errors, 1);
			break;
		}
	}
	if (sequence > RISCV32_SMP_GATE_E_ROUNDS) {
		(void)riscv32_smp_gate_d_wait(&riscv32_gate_e_worker_done, 1);
	}
	pmap_remove(kernel_pmap, RISCV32_SMP_GATE_E_ALIAS,
	    RISCV32_SMP_GATE_E_ALIAS + PAGE_SIZE);
	local_delta = kernel_pmap->tlb_local_fence_count - local_before;
	remote_delta = kernel_pmap->tlb_remote_fence_count - remote_before;
	remote_errors = kernel_pmap->tlb_remote_fence_errors - errors_before;
	passed = sequence > RISCV32_SMP_GATE_E_ROUNDS &&
	    riscv32_gate_e_worker_done == 1 &&
	    riscv32_gate_e_worker_cpu == 1 &&
	    riscv32_gate_e_initial_value == RISCV32_SMP_GATE_E_VALUE_A &&
	    riscv32_gate_e_observations == RISCV32_SMP_GATE_E_ROUNDS &&
	    riscv32_gate_e_stale == 0 && riscv32_gate_e_errors == 0 &&
	    (active_mask & 0x3U) == 0x3U &&
	    local_delta >= RISCV32_SMP_GATE_E_ROUNDS &&
	    remote_delta >= RISCV32_SMP_GATE_E_ROUNDS && remote_errors == 0;

	riscv32_early_puts("[smp-gate-e] rounds=");
	riscv32_early_putdec(RISCV32_SMP_GATE_E_ROUNDS);
	riscv32_early_puts(" active-mask=0x");
	riscv32_early_puthex32(active_mask);
	riscv32_early_puts(" local-fences=");
	riscv32_early_putdec(local_delta);
	riscv32_early_puts(" remote-fences=");
	riscv32_early_putdec(remote_delta);
	riscv32_early_puts(" stale=");
	riscv32_early_putdec(riscv32_gate_e_stale);
	riscv32_early_puts(" errors=");
	riscv32_early_putdec(riscv32_gate_e_errors + remote_errors);
	riscv32_early_puts("\n");
	if (passed) {
		riscv32_early_puts(
		    "[smp-gate-e] shared translation RFENCE coherence PASS\n");
	} else {
		riscv32_early_puts(
		    "[smp-gate-e] shared translation RFENCE coherence FAIL\n");
	}
	thread_deallocate(thread);
	return passed;
}

static void
riscv32_smp_gate_f_locked_round(uint32_t cpu)
{
	uint32_t value;

	hw_lock_lock(&riscv32_gate_f_lock);
	if (riscv32_gate_f_critical_owner != UINT32_MAX) {
		(void)__sync_add_and_fetch(&riscv32_gate_f_overlap, 1);
	}
	riscv32_gate_f_critical_owner = cpu;
	__sync_synchronize();
	value = riscv32_gate_f_locked_total;
	riscv32_gate_f_locked_total = value + 1;
	__sync_synchronize();
	if (riscv32_gate_f_critical_owner != cpu) {
		(void)__sync_add_and_fetch(&riscv32_gate_f_errors, 1);
	}
	riscv32_gate_f_critical_owner = UINT32_MAX;
	hw_lock_unlock(&riscv32_gate_f_lock);
	(void)__sync_add_and_fetch(&riscv32_gate_f_iterations[cpu], 1);
}

static boolean_t
riscv32_smp_gate_f_wait_at_least(volatile uint32_t *value, uint32_t target)
{
	uint32_t tries;

	for (tries = 0; tries < 20000000U; tries++) {
		if (*value >= target) {
			__sync_synchronize();
			return TRUE;
		}
		cpu_pause();
	}
	return FALSE;
}

static void
riscv32_smp_gate_f_worker(void *parameter, wait_result_t wait_result)
{
	processor_t processor = (processor_t)parameter;
	thread_t thread = current_thread();
	uint32_t round;

	(void)wait_result;
	riscv32_gate_f_worker_cpu = (uint32_t)get_cpu_number();
	if (processor == PROCESSOR_NULL || get_cpu_number() != 1 ||
	    current_processor() != processor ||
	    thread->bound_processor != processor ||
	    riscv32_pmap_current() != kernel_pmap) {
		(void)__sync_add_and_fetch(&riscv32_gate_f_errors, 1);
	}
	__sync_synchronize();
	riscv32_gate_f_ready = 1;
	__sync_synchronize();
	while (riscv32_gate_f_start == 0) {
		cpu_pause();
	}
	(void)__sync_add_and_fetch(&riscv32_gate_f_active_workers, 1);
	if (!riscv32_smp_gate_f_wait_at_least(
	    &riscv32_gate_f_active_workers, RISCV32_SMP_GATE_F_CPUS)) {
		(void)__sync_add_and_fetch(&riscv32_gate_f_errors, 1);
	} else {
		riscv32_gate_f_saw_peer[1] = 1;
	}
	for (round = 0; round < RISCV32_SMP_GATE_F_ROUNDS; round++) {
		riscv32_smp_gate_f_locked_round(1);
	}
	(void)__sync_sub_and_fetch(&riscv32_gate_f_active_workers, 1);
	__sync_synchronize();
	riscv32_gate_f_done = 1;
	__sync_synchronize();
	(void)assert_wait((event_t)&riscv32_gate_f_wait_event, THREAD_UNINT);
	(void)thread_block(THREAD_CONTINUE_NULL);
	panic("riscv32: Gate F worker unexpectedly resumed");
}

static void
riscv32_smp_gate_f_preempt_worker(void *parameter, wait_result_t wait_result)
{
	processor_t processor = (processor_t)parameter;
	thread_t thread = current_thread();

	(void)wait_result;
	riscv32_gate_f_preempt_cpu = (uint32_t)get_cpu_number();
	if (processor == PROCESSOR_NULL || get_cpu_number() != 1 ||
	    current_processor() != processor ||
	    thread->bound_processor != processor ||
	    riscv32_pmap_current() != kernel_pmap) {
		(void)__sync_add_and_fetch(&riscv32_gate_f_errors, 1);
	}
	if (riscv32_gate_f_ready == 1 && riscv32_gate_f_start == 0) {
		riscv32_gate_f_preempt_before_release = 1;
	} else {
		(void)__sync_add_and_fetch(&riscv32_gate_f_errors, 1);
	}
	__sync_synchronize();
	riscv32_gate_f_preempt_started = 1;
	__sync_synchronize();
	(void)assert_wait((event_t)&riscv32_gate_f_preempt_wait_event,
	    THREAD_UNINT);
	(void)thread_block(THREAD_CONTINUE_NULL);
	panic("riscv32: Gate F preempt worker unexpectedly resumed");
}

static boolean_t
riscv32_smp_gate_f_run(processor_t secondary_processor)
{
	cpu_data_t *boot_cpu = cpu_datap(0);
	cpu_data_t *secondary_cpu = cpu_datap(1);
	thread_t current = current_thread();
	thread_t worker;
	thread_t preempt_worker = THREAD_NULL;
	kern_return_t kr;
	spl_t s;
	cpu_set active_mask;
	uint32_t online_mask = 0;
	uint32_t ast_before = secondary_cpu->cpu_ast_ipi_count;
	uint32_t ast_after = ast_before;
	uint32_t signal_before = secondary_cpu->cpu_ast_signal_count;
	uint32_t signal_after = signal_before;
	uint32_t preempt_before = secondary_cpu->cpu_ast_preemption_count;
	uint32_t preempt_after = preempt_before;
	uint32_t unhandled_before = secondary_cpu->cpu_unhandled_ipi_count;
	uint32_t processor_state_before = UINT32_MAX;
	uint32_t processor_priority_before = UINT32_MAX;
	uint32_t preempt_priority_before = UINT32_MAX;
	uint32_t processor_cpu_id_before = UINT32_MAX;
	uint32_t active_thread_before = 0;
	uint32_t next_thread_before = 0;
	uint32_t round;
	boolean_t passed;

	hw_lock_init(&riscv32_gate_f_lock);
	riscv32_gate_f_start = 0;
	riscv32_gate_f_ready = 0;
	riscv32_gate_f_active_workers = 0;
	riscv32_gate_f_done = 0;
	riscv32_gate_f_iterations[0] = 0;
	riscv32_gate_f_iterations[1] = 0;
	riscv32_gate_f_saw_peer[0] = 0;
	riscv32_gate_f_saw_peer[1] = 0;
	riscv32_gate_f_locked_total = 0;
	riscv32_gate_f_critical_owner = UINT32_MAX;
	riscv32_gate_f_overlap = 0;
	riscv32_gate_f_errors = 0;
	riscv32_gate_f_worker_cpu = UINT32_MAX;
	riscv32_gate_f_preempt_started = 0;
	riscv32_gate_f_preempt_cpu = UINT32_MAX;
	riscv32_gate_f_preempt_before_release = 0;
	__sync_synchronize();

	if (boot_cpu == NULL || secondary_cpu == NULL ||
	    secondary_processor == PROCESSOR_NULL || current == THREAD_NULL ||
	    get_cpu_number() != 0 || current_processor() != master_processor ||
	    current->bound_processor != master_processor) {
		riscv32_early_puts("[smp-gate-f] CPU0 prerequisites FAIL\n");
		return FALSE;
	}
	kr = kernel_thread_create(riscv32_smp_gate_f_worker,
	    secondary_processor, BASEPRI_KERNEL, &worker);
	if (kr != KERN_SUCCESS) {
		riscv32_early_puts("[smp-gate-f] kernel_thread_create FAIL kr=");
		riscv32_early_putdec((uint32_t)kr);
		riscv32_early_puts("\n");
		return FALSE;
	}
	s = splsched();
	thread_lock(worker);
	worker->bound_processor = secondary_processor;
	thread_unlock(worker);
	splx(s);
	thread_mtx_lock(worker);
	thread_start_internal(worker);
	thread_mtx_unlock(worker);
	if (!riscv32_smp_gate_d_wait(&riscv32_gate_f_ready, 1)) {
		riscv32_early_puts("[smp-gate-f] worker start timeout FAIL\n");
		thread_deallocate(worker);
		return FALSE;
	}

	kr = kernel_thread_create(riscv32_smp_gate_f_preempt_worker,
	    secondary_processor, MAXPRI_KERNEL, &preempt_worker);
	if (kr != KERN_SUCCESS) {
		riscv32_early_puts(
		    "[smp-gate-f] preempt kernel_thread_create FAIL kr=");
		riscv32_early_putdec((uint32_t)kr);
		riscv32_early_puts("\n");
		(void)__sync_add_and_fetch(&riscv32_gate_f_errors, 1);
	} else {
		s = splsched();
		thread_lock(preempt_worker);
		preempt_worker->bound_processor = secondary_processor;
		thread_unlock(preempt_worker);
		splx(s);
		ast_before = secondary_cpu->cpu_ast_ipi_count;
		signal_before = secondary_cpu->cpu_ast_signal_count;
		preempt_before = secondary_cpu->cpu_ast_preemption_count;
		unhandled_before = secondary_cpu->cpu_unhandled_ipi_count;
		processor_state_before = (uint32_t)secondary_processor->state;
		processor_priority_before =
		    (uint32_t)secondary_processor->current_pri;
		preempt_priority_before = (uint32_t)preempt_worker->sched_pri;
		processor_cpu_id_before = (uint32_t)secondary_processor->cpu_id;
		active_thread_before =
		    (uint32_t)(uintptr_t)secondary_processor->active_thread;
		next_thread_before =
		    (uint32_t)(uintptr_t)secondary_processor->next_thread;
		thread_mtx_lock(preempt_worker);
		thread_start_internal(preempt_worker);
		thread_mtx_unlock(preempt_worker);
		/* Exercise the architecture kick even if the run queue coalesced it. */
		cause_ast_check(secondary_processor);
		if (!riscv32_smp_gate_d_wait(
		    &riscv32_gate_f_preempt_started, 1)) {
			(void)__sync_add_and_fetch(&riscv32_gate_f_errors, 1);
		}
		ast_after = secondary_cpu->cpu_ast_ipi_count;
		signal_after = secondary_cpu->cpu_ast_signal_count;
		preempt_after = secondary_cpu->cpu_ast_preemption_count;
		if (signal_after <= signal_before || ast_after == 0 ||
		    preempt_after <= preempt_before ||
		    secondary_cpu->cpu_unhandled_ipi_count != unhandled_before) {
			(void)__sync_add_and_fetch(&riscv32_gate_f_errors, 1);
		}
	}
	if (preempt_worker == THREAD_NULL) {
		ast_before = secondary_cpu->cpu_ast_ipi_count;
		ast_after = ast_before;
		signal_before = secondary_cpu->cpu_ast_signal_count;
		signal_after = signal_before;
		preempt_before = secondary_cpu->cpu_ast_preemption_count;
		preempt_after = preempt_before;
		unhandled_before = secondary_cpu->cpu_unhandled_ipi_count;
	}

	__sync_synchronize();
	riscv32_gate_f_start = 1;
	(void)__sync_add_and_fetch(&riscv32_gate_f_active_workers, 1);
	if (!riscv32_smp_gate_f_wait_at_least(
	    &riscv32_gate_f_active_workers, RISCV32_SMP_GATE_F_CPUS)) {
		(void)__sync_add_and_fetch(&riscv32_gate_f_errors, 1);
	} else {
		riscv32_gate_f_saw_peer[0] = 1;
	}
	for (round = 0; round < RISCV32_SMP_GATE_F_ROUNDS; round++) {
		riscv32_smp_gate_f_locked_round(0);
	}
	(void)__sync_sub_and_fetch(&riscv32_gate_f_active_workers, 1);
	if (!riscv32_smp_gate_d_wait(&riscv32_gate_f_done, 1)) {
		(void)__sync_add_and_fetch(&riscv32_gate_f_errors, 1);
	}
	__sync_synchronize();

	active_mask = riscv32_pmap_active_cpu_mask(kernel_pmap);
	if (boot_cpu->cpu_running != 0 &&
	    master_processor->state != PROCESSOR_OFF_LINE &&
	    master_processor->state != PROCESSOR_START) {
		online_mask |= 1U;
	}
	if (secondary_cpu->cpu_running != 0 &&
	    secondary_processor->state != PROCESSOR_OFF_LINE &&
	    secondary_processor->state != PROCESSOR_START) {
		online_mask |= 2U;
	}
	passed = riscv32_gate_f_done == 1 &&
	    riscv32_gate_f_worker_cpu == 1 &&
	    riscv32_gate_f_iterations[0] == RISCV32_SMP_GATE_F_ROUNDS &&
	    riscv32_gate_f_iterations[1] == RISCV32_SMP_GATE_F_ROUNDS &&
	    riscv32_gate_f_locked_total ==
	    RISCV32_SMP_GATE_F_ROUNDS * RISCV32_SMP_GATE_F_CPUS &&
	    riscv32_gate_f_overlap == 0 && riscv32_gate_f_errors == 0 &&
	    riscv32_gate_f_saw_peer[0] == 1 &&
	    riscv32_gate_f_saw_peer[1] == 1 &&
	    riscv32_gate_f_active_workers == 0 && online_mask == 0x3U &&
	    (active_mask & 0x3U) == 0x3U &&
	    boot_cpu->cpu_timer_initialized &&
	    secondary_cpu->cpu_timer_initialized &&
	    boot_cpu->cpu_timer_interrupt_count != 0 &&
	    secondary_cpu->cpu_timer_interrupt_count != 0 &&
	    master_processor->idle_thread != THREAD_NULL &&
	    secondary_processor->idle_thread != THREAD_NULL &&
	    riscv32_gate_f_preempt_started == 1 &&
	    riscv32_gate_f_preempt_cpu == 1 &&
	    riscv32_gate_f_preempt_before_release == 1 &&
	    signal_after > signal_before && ast_after != 0 &&
	    preempt_after > preempt_before &&
	    secondary_cpu->cpu_unhandled_ipi_count == unhandled_before;

	riscv32_early_puts("[smp-gate-f] ast-signals=");
	riscv32_early_putdec(signal_after - signal_before);
	riscv32_early_puts(" ast-ipis-total=");
	riscv32_early_putdec(ast_after);
	riscv32_early_puts(" ast-ipis-delta=");
	riscv32_early_putdec(ast_after - ast_before);
	riscv32_early_puts(" remote-preemptions=");
	riscv32_early_putdec(preempt_after - preempt_before);
	riscv32_early_puts(" preempt-cpu=");
	riscv32_early_putdec(riscv32_gate_f_preempt_cpu);
	riscv32_early_puts(" before-release=");
	riscv32_early_putdec(riscv32_gate_f_preempt_before_release);
	riscv32_early_puts(" wake-ipis=");
	riscv32_early_putdec(secondary_cpu->cpu_wakeup_ipi_count);
	riscv32_early_puts(" prior-signals=0x");
	riscv32_early_puthex32((uint32_t)secondary_cpu->cpu_prior_signals);
	riscv32_early_puts(" unhandled=");
	riscv32_early_putdec(secondary_cpu->cpu_unhandled_ipi_count -
	    unhandled_before);
	riscv32_early_puts("\n");
	riscv32_early_puts("[smp-gate-f] preempt-state=");
	riscv32_early_putdec(processor_state_before);
	riscv32_early_puts(" current-pri=");
	riscv32_early_putdec(processor_priority_before);
	riscv32_early_puts(" new-pri=");
	riscv32_early_putdec(preempt_priority_before);
	riscv32_early_puts(" processor-cpu-id=");
	riscv32_early_putdec(processor_cpu_id_before);
	riscv32_early_puts(" active-thread=0x");
	riscv32_early_puthex32(active_thread_before);
	riscv32_early_puts(" expected-active=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)worker);
	riscv32_early_puts(" next-thread=0x");
	riscv32_early_puthex32(next_thread_before);
	riscv32_early_puts("\n");
	riscv32_early_puts("[smp-gate-f] workers=2 cpu0=");
	riscv32_early_putdec(riscv32_gate_f_iterations[0]);
	riscv32_early_puts(" cpu1=");
	riscv32_early_putdec(riscv32_gate_f_iterations[1]);
	riscv32_early_puts(" locked-total=");
	riscv32_early_putdec(riscv32_gate_f_locked_total);
	riscv32_early_puts(" overlap=");
	riscv32_early_putdec(riscv32_gate_f_overlap);
	riscv32_early_puts(" online-mask=0x");
	riscv32_early_puthex32(online_mask);
	riscv32_early_puts(" active-pmap-mask=0x");
	riscv32_early_puthex32(active_mask);
	riscv32_early_puts(" errors=");
	riscv32_early_putdec(riscv32_gate_f_errors);
	riscv32_early_puts("\n");
	if (passed) {
		riscv32_early_puts(
		    "[smp-gate-f] dual-processor kernel scheduler and RV32A locks PASS\n");
	} else {
		riscv32_early_puts(
		    "[smp-gate-f] dual-processor kernel scheduler and RV32A locks FAIL\n");
	}
	if (preempt_worker != THREAD_NULL) {
		thread_deallocate(preempt_worker);
	}
	thread_deallocate(worker);
	return passed;
}

void
riscv32_smp_smoke_start(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	uint32_t slot;
	uint32_t started = 0;

	if (bootinfo == (const riscv32_bootinfo_t *)0 ||
	    bootinfo->cpu_count <= 1) {
		riscv32_early_puts("[smp-smoke] no secondary harts in DTB\n");
		return;
	}
	if ((bootinfo->sbi_extensions & RISCV32_SBI_EXTBIT_HSM) == 0) {
		riscv32_early_puts("[smp-smoke] HSM unavailable\n");
		return;
	}

	riscv32_early_puts("[smp-smoke] attempting secondary starts cpus=");
	riscv32_early_putdec(bootinfo->cpu_count);
	riscv32_early_puts("\n");

	riscv32_cpu_allow_smp_smoke = TRUE;
	for (slot = 1; slot < bootinfo->cpu_count && slot < RISCV32_MAX_CPUS;
	    slot++) {
		kern_return_t kr;

		riscv32_early_puts("[smp-smoke] start slot=");
		riscv32_early_putdec(slot);
		riscv32_early_puts(" hart=");
		riscv32_early_putdec(bootinfo->hart_ids[slot]);
		riscv32_early_puts("\n");
		kr = cpu_start((int)slot);
		if (kr == KERN_SUCCESS) {
			long status;
			uint32_t stage;

			started++;
			status = riscv32_smp_smoke_wait_status(
			    bootinfo->hart_ids[slot]);
			riscv32_smp_smoke_print_status(bootinfo->hart_ids[slot]);
			if (status == RISCV32_SBI_HSM_STARTED) {
				riscv32_early_puts("[smp-smoke] hart started\n");
			}
			stage = riscv32_smp_smoke_wait_stage(slot, 6);
			riscv32_early_puts("[smp-smoke] bootstage=");
			riscv32_early_putdec(stage);
			riscv32_early_puts("\n");
			if (stage >= 6 && stage != 0xff) {
				riscv32_smp_gate_a_print(slot);
				riscv32_early_puts(
				    "[smp-smoke] secondary parked confirmed\n");
				if (riscv32_boot_arg_has("smp-gate-b")) {
					stage = riscv32_smp_smoke_wait_stage(slot, 7);
					if (stage >= 7 && stage != 0xff) {
						(void)riscv32_smp_gate_b_run(slot);
					} else {
						riscv32_early_puts(
						    "[smp-gate-b] secondary SSIP ready FAIL\n");
					}
				}
			}
		} else {
			riscv32_early_puts("[smp-smoke] start failed kr=");
			riscv32_early_putdec((uint32_t)kr);
			riscv32_early_puts("\n");
		}
	}
	riscv32_cpu_allow_smp_smoke = FALSE;

	riscv32_early_puts("[smp-smoke] secondary start requests=");
	riscv32_early_putdec(started);
	riscv32_early_puts("\n");
}

void
riscv32_smp_scheduler_start(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	cpu_data_t *cpu_data;
	processor_t processor;
	kern_return_t kr;
	uint32_t slot = 1;
	uint32_t stage;
	uint64_t timer_wait_started;
	boolean_t passed;
	boolean_t gate_d_passed;
	boolean_t gate_e_passed;

	if (!riscv32_smp_scheduler_requested()) {
		return;
	}
	if (get_cpu_number() != 0 || bootinfo == NULL ||
	    bootinfo->cpu_count <= slot) {
		riscv32_early_puts(
		    "[smp-gate-c] scheduler launch prerequisites FAIL\n");
		return;
	}

	cpu_data = riscv32_cpu_ensure_slot(slot);
	if (cpu_data == NULL) {
		riscv32_early_puts("[smp-gate-c] cpu_data allocation FAIL\n");
		return;
	}
	processor = cpu_data->cpu_processor;
	if (!riscv32_secondary_processor_initialized) {
		processor_init(processor, (int)slot,
		    current_processor()->processor_set);
		riscv32_secondary_processor_initialized = TRUE;
	}

	cpu_data->cpu_idle_count = 0;
	cpu_data->cpu_timer_interrupt_count = 0;
	cpu_data->cpu_ipi_count = 0;
	__sync_synchronize();
	riscv32_early_puts(
	    "[smp-gate-c] processor_start slot=1 via SBI HSM\n");
	kr = processor_start(processor);
	if (kr != KERN_SUCCESS) {
		riscv32_early_puts("[smp-gate-c] processor_start FAIL kr=");
		riscv32_early_putdec((uint32_t)kr);
		riscv32_early_puts("\n");
		return;
	}

	stage = riscv32_smp_smoke_wait_stage(slot, 8);
	if (stage >= 6 && stage != 0xff) {
		riscv32_smp_gate_a_print(slot);
	}
	/* Host/TCG speed must not decide whether the first local timer has fired. */
	timer_wait_started = mach_absolute_time();
	while (mach_absolute_time() - timer_wait_started < 1000000000ULL) {
		__sync_synchronize();
		if (cpu_data->cpu_idle_count != 0 &&
		    cpu_data->cpu_timer_interrupt_count != 0) {
			break;
		}
		__asm__ volatile("nop" ::: "memory");
	}
	__sync_synchronize();
	passed = stage >= 8 && stage != 0xff &&
	    processor->idle_thread != THREAD_NULL &&
	    cpu_data->cpu_idle_count != 0 &&
	    cpu_data->cpu_timer_interrupt_count != 0 &&
	    cpu_data->cpu_timer_initialized && cpu_data->cpu_running &&
	    processor->state != PROCESSOR_OFF_LINE &&
	    processor->state != PROCESSOR_START;

	riscv32_early_puts("[smp-gate-c] processor-state=");
	riscv32_early_putdec((uint32_t)processor->state);
	riscv32_early_puts(" idle-thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)processor->idle_thread);
	riscv32_early_puts(" idle-entries=");
	riscv32_early_putdec(cpu_data->cpu_idle_count);
	riscv32_early_puts(" timer-irqs=");
	riscv32_early_putdec(cpu_data->cpu_timer_interrupt_count);
	riscv32_early_puts("\n");
	if (passed) {
		riscv32_early_puts(
		    "[smp-gate-c] XNU idle thread and per-hart timer PASS\n");
	} else {
		riscv32_early_puts(
		    "[smp-gate-c] XNU idle thread and per-hart timer FAIL\n");
	}
	if (passed && riscv32_smp_gate_d_requested()) {
		gate_d_passed = riscv32_smp_gate_d_run(processor);
		if (gate_d_passed && riscv32_smp_gate_e_requested()) {
			gate_e_passed = riscv32_smp_gate_e_run(processor);
			if (gate_e_passed && riscv32_smp_gate_f_requested()) {
				(void)riscv32_smp_gate_f_run(processor);
			}
		}
	}
}

void
riscv32_smp_assert_userspace_cpu0(void)
{
	thread_t thread;
	boolean_t passed;

	if (!riscv32_smp_scheduler_requested()) {
		return;
	}
	thread = current_thread();
	passed = get_cpu_number() == 0 && thread != THREAD_NULL &&
	    thread->bound_processor == master_processor;
	if (!passed) {
		panic("riscv32: SMP userspace escaped boot CPU affinity");
	}
	riscv32_early_puts(
	    "[smp-affinity] Linux userspace bound to hart 0 PASS\n");
}

processor_t
riscv32_smp_processor_for_cpu(uint32_t cpu)
{
	cpu_data_t *cpu_data;
	processor_t processor;

	if (cpu == 0U) {
		return master_processor;
	}
	if (cpu >= RISCV32_MAX_CPUS || !riscv32_secondary_processor_initialized) {
		return PROCESSOR_NULL;
	}
	cpu_data = cpu_datap((int)cpu);
	if (cpu_data == (cpu_data_t *)0 || cpu_data->cpu_running == 0) {
		return PROCESSOR_NULL;
	}
	processor = cpu_data->cpu_processor;
	__sync_synchronize();
	if (processor == PROCESSOR_NULL || processor->cpu_id != (int)cpu ||
	    processor->state == PROCESSOR_OFF_LINE ||
	    processor->state == PROCESSOR_START) {
		return PROCESSOR_NULL;
	}
	return processor;
}

void
cpu_exit_wait(int slot_num)
{
	(void)slot_num;
}

void
cpu_machine_init(void)
{
	cpu_init();
	current_cpu_datap()->cpu_boot_complete = TRUE;
	current_cpu_datap()->cpu_running = TRUE;
	ml_init_interrupt();
}

void
handle_pending_TLB_flushes(void)
{
}

int
cpu_signal_handler(riscv_saved_state_t *regs)
{
	(void)regs;
	return 0;
}

kern_return_t
cpu_register(int *slot_nump)
{
	if (slot_nump != NULL) {
		*slot_nump = 0;
	}
	return KERN_SUCCESS;
}

processor_t
current_processor(void)
{
	return current_cpu_datap()->cpu_processor;
}

processor_t
cpu_to_processor(int cpu)
{
	cpu_data_t *cpu_data = cpu_datap(cpu);

	return cpu_data != NULL ? cpu_data->cpu_processor : PROCESSOR_NULL;
}

boolean_t
machine_processor_is_inactive(processor_t processor)
{
	cpu_data_t *cpu_data;

	if (processor == PROCESSOR_NULL) {
		return TRUE;
	}
	cpu_data = cpu_datap(processor->cpu_id);
	if (cpu_data == NULL) {
		return TRUE;
	}
	return cpu_data->cpu_running == 0;
}

processor_t
machine_choose_processor(processor_set_t pset, processor_t preferred)
{
	int cpu;
	int low = 0;
	int high = 0;

	if (preferred != PROCESSOR_NULL &&
	    !machine_processor_is_inactive(preferred)) {
		return preferred;
	}

	if (pset != PROCESSOR_SET_NULL) {
		low = pset->cpu_set_low;
		high = pset->cpu_set_hi;
	}

	for (cpu = low; cpu <= high && cpu < RISCV32_MAX_CPUS; cpu++) {
		processor_t processor = cpu_to_processor(cpu);

		if (processor != PROCESSOR_NULL &&
		    !machine_processor_is_inactive(processor)) {
			return processor;
		}
	}

	return current_processor();
}

ast_t *
ast_pending(void)
{
	return &current_cpu_datap()->cpu_pending_ast;
}

cpu_type_t
slot_type(int slot_num)
{
	(void)slot_num;
	return CPU_TYPE_RISCV32;
}

cpu_subtype_t
slot_subtype(int slot_num)
{
	(void)slot_num;
	return CPU_SUBTYPE_RISCV32_ALL;
}

cpu_threadtype_t
slot_threadtype(int slot_num)
{
	(void)slot_num;
	return CPU_THREADTYPE_NONE;
}

cpu_type_t
cpu_type(void)
{
	return CPU_TYPE_RISCV32;
}

cpu_subtype_t
cpu_subtype(void)
{
	return CPU_SUBTYPE_RISCV32_ALL;
}

cpu_threadtype_t
cpu_threadtype(void)
{
	return CPU_THREADTYPE_NONE;
}

cpu_data_t *
cpu_data_alloc(boolean_t is_boot_cpu)
{
	uint32_t slot;
	const riscv32_bootinfo_t *bootinfo;

	if (is_boot_cpu) {
		riscv32_cpu_sync_boot_data();
		cpu_data_master.cpu_this = &cpu_data_master;
		cpu_data_master.cpu_processor = &processor_master;
		cpu_data_master.cpu_active_pmap = kernel_pmap;
		cpu_data_master.cpu_int_stack_top = (vm_offset_t)(uintptr_t)
		    &riscv32_interrupt_stacks[0][KERNEL_STACK_SIZE];
		cpu_data_ptr[0] = &cpu_data_master;
		riscv32_set_current_cpu_datap(&cpu_data_master);
		riscv32_cpu_timer_queue_init(&cpu_data_master);
		return &cpu_data_master;
	}

	bootinfo = riscv32_bootinfo();
	if (riscv32_next_cpu_slot >= RISCV32_MAX_CPUS ||
	    riscv32_next_cpu_slot >= bootinfo->cpu_count) {
		return NULL;
	}

	slot = riscv32_next_cpu_slot++;
	memset(&riscv32_cpu_data_secondary[slot - 1], 0,
	    sizeof(riscv32_cpu_data_secondary[slot - 1]));
	riscv32_cpu_data_secondary[slot - 1].cpu_this =
	    &riscv32_cpu_data_secondary[slot - 1];
	riscv32_cpu_data_secondary[slot - 1].cpu_number = (int)slot;
	riscv32_cpu_data_secondary[slot - 1].cpu_phys_number =
	    (int)bootinfo->hart_ids[slot];
	riscv32_cpu_data_secondary[slot - 1].cpu_id =
	    (cpu_id_t)(uintptr_t)bootinfo->hart_ids[slot];
	riscv32_cpu_data_secondary[slot - 1].cpu_type = CPU_TYPE_RISCV32;
	riscv32_cpu_data_secondary[slot - 1].cpu_subtype =
	    CPU_SUBTYPE_RISCV32_ALL;
	riscv32_cpu_data_secondary[slot - 1].cpu_threadtype =
	    CPU_THREADTYPE_NONE;
	riscv32_cpu_data_secondary[slot - 1].cpu_processor =
	    &riscv32_processor_secondary[slot - 1];
	riscv32_cpu_data_secondary[slot - 1].cpu_active_pmap = kernel_pmap;
	riscv32_cpu_data_secondary[slot - 1].cpu_int_stack_top =
	    (vm_offset_t)(uintptr_t)
	    &riscv32_interrupt_stacks[slot][KERNEL_STACK_SIZE];
	riscv32_cpu_data_secondary[slot - 1].cpu_pal_data.trap_stack_top =
	    (uintptr_t)&riscv32_secondary_stacks[slot - 1][KERNEL_STACK_SIZE];
	riscv32_cpu_data_secondary[slot - 1].cpu_iflag = FALSE;
	riscv32_cpu_timer_queue_init(&riscv32_cpu_data_secondary[slot - 1]);
	cpu_data_ptr[slot] = &riscv32_cpu_data_secondary[slot - 1];
	__sync_synchronize();
	return cpu_data_ptr[slot];
}

void
cpu_data_realloc(void)
{
}

void
riscv32_secondary_entry_c(uint32_t hartid, uint32_t slot)
{
	cpu_data_t *cpu_data = cpu_datap((int)slot);
	uint32_t sscratch;
	uint32_t stvec;
	uintptr_t stack_pointer;
	boolean_t gate_a_ok;
	uint32_t mode = RISCV32_SECONDARY_MODE_PARK;

	if (slot > 0 && slot < RISCV32_MAX_CPUS) {
		riscv32_secondary_boot[slot - 1].stage = 3;
	}
	if (cpu_data != NULL) {
		riscv32_set_current_cpu_datap(cpu_data);
		riscv32_trap_init();
		cpu_data->cpu_running = TRUE;
		cpu_data->cpu_boot_complete = TRUE;
		cpu_data->cpu_phys_number = (int)hartid;
		cpu_data->cpu_id = (cpu_id_t)(uintptr_t)hartid;
		cpu_data->cpu_iflag = FALSE;
		riscv32_secondary_boot[slot - 1].stage = 4;
	}

	__asm__ volatile("csrr %0, sscratch" : "=r"(sscratch));
	__asm__ volatile("csrr %0, stvec" : "=r"(stvec));
	__asm__ volatile("mv %0, sp" : "=r"(stack_pointer));
	gate_a_ok = cpu_data != NULL && current_cpu_datap() == cpu_data &&
	    sscratch == (uint32_t)(uintptr_t)cpu_data &&
	    stvec == (uint32_t)(uintptr_t)riscv32_trap_vector &&
	    stack_pointer > (uintptr_t)&riscv32_secondary_stacks[slot - 1][0] &&
	    stack_pointer <= (uintptr_t)
	    &riscv32_secondary_stacks[slot - 1][KERNEL_STACK_SIZE] &&
	    cpu_data->cpu_int_stack_top == (vm_offset_t)(uintptr_t)
	    &riscv32_interrupt_stacks[slot][KERNEL_STACK_SIZE];

	if (slot > 0 && slot < RISCV32_MAX_CPUS) {
		mode = riscv32_secondary_boot[slot - 1].mode;
		riscv32_secondary_boot[slot - 1].observed_sp =
		    (uint32_t)stack_pointer;
		riscv32_secondary_boot[slot - 1].observed_sscratch = sscratch;
		riscv32_secondary_boot[slot - 1].observed_stvec = stvec;
		__sync_synchronize();
		riscv32_secondary_boot[slot - 1].stage = gate_a_ok ? 6 : 0xff;
	}

	if (gate_a_ok && mode == RISCV32_SECONDARY_MODE_IPI) {
		uint32_t ssie = RISCV32_SIE_SSIE;
		uint32_t sstatus_sie = RISCV32_SSTATUS_SIE;

		__asm__ volatile("csrc sip, %0" :: "r"(ssie) : "memory");
		__asm__ volatile("csrs sie, %0" :: "r"(ssie) : "memory");
		cpu_data->cpu_iflag = TRUE;
		__sync_synchronize();
		riscv32_secondary_boot[slot - 1].stage = 7;
		__asm__ volatile("csrs sstatus, %0" ::
		    "r"(sstatus_sie) : "memory");
	}
	if (gate_a_ok && mode == RISCV32_SECONDARY_MODE_SCHEDULER) {
		__sync_synchronize();
		riscv32_secondary_boot[slot - 1].stage = 8;
		slave_main(NULL);
		panic("riscv32: slave_main returned");
	}

	for (;;) {
		__asm__ volatile("wfi");
	}
}

uint64_t
_get_cpu_capabilities(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();

	if (bootinfo != (const riscv32_bootinfo_t *)0 &&
	    bootinfo->cpu_capabilities != 0) {
		return bootinfo->cpu_capabilities;
	}
	return kHasRV32I | kHasM | kHasA | kHasC | kHasSupervisor |
	    kHasUser | kUP | (1ULL << kNumCPUsShift);
}
