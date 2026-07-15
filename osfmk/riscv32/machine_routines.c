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
 */

#line 1
#include <mach/mach_types.h>
#include <mach/machine.h>
#include <kern/clock.h>
#include <kern/debug.h>
#include <kern/machine.h>
#include <kern/processor.h>
#include <kern/startup.h>
#include <kern/thread.h>
#include <machine/bootinfo.h>
#include <machine/cpu_data.h>
#include <machine/early_print.h>
#include <machine/hfsplus.h>
#include <machine/linux_exec.h>
#include <machine/machine_cpu.h>
#include <machine/machine_routines.h>
#include <machine/pal_routines.h>
#include <machine/plic.h>
#include <machine/trap.h>
#include <machine/timer.h>
#include <machine/tty.h>
#include <machine/umode_smoke.h>
#include <machine/virtio_blk.h>
#include <machine/virtio_mmio.h>
#include <riscv32/sbi.h>
#include <vm/pmap.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static int riscv32_max_cpus = 1;
static uint8_t riscv32_storage_probe_buf[4] __attribute__((aligned(4)));
unsigned int LockTimeOut = 1000000U;
unsigned int LockTimeOutTSC = 1000000U;

#define RISCV32_SSTATUS_SIE	0x00000002U
#define RISCV32_SIE_SSIE	0x00000002U
#define RISCV32_IDLE_POLL_SPINS	256U

static boolean_t
riscv32_has_sbi_extension(uint32_t extension_bit)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();

	return bootinfo != (const riscv32_bootinfo_t *)0 &&
	    (bootinfo->sbi_extensions & extension_bit) != 0;
}

kern_return_t
riscv32_send_ipi_to_hart(uint32_t hartid)
{
	struct riscv32_sbi_ret ret;

	if (!riscv32_has_sbi_extension(RISCV32_SBI_EXTBIT_IPI)) {
		return KERN_NOT_SUPPORTED;
	}
	ret = riscv32_sbi_send_ipi(1U, hartid);
	return ret.error == RISCV32_SBI_SUCCESS ? KERN_SUCCESS : KERN_FAILURE;
}

static void
riscv32_request_system_reset(boolean_t reboot)
{
	uint32_t reset_type = reboot ?
	    RISCV32_SBI_SRST_RESET_TYPE_COLD_REBOOT :
	    RISCV32_SBI_SRST_RESET_TYPE_SHUTDOWN;

	if (!riscv32_has_sbi_extension(RISCV32_SBI_EXTBIT_SRST)) {
		return;
	}
	(void)riscv32_sbi_system_reset(reset_type,
	    RISCV32_SBI_SRST_RESET_REASON_NONE);
}

static void
riscv32_trap_smoke(void)
{
	if (!riscv32_boot_arg_has("trap-smoke")) {
		return;
	}

	riscv32_early_puts("[riscv32] trap-smoke: issuing illegal instruction\n");
	__asm__ volatile(".word 0" ::: "memory");
	panic("riscv32: trap-smoke returned");
}

void
machine_init(void)
{
	cpu_machine_init();
	if (riscv32_boot_arg_has("smp-smoke")) {
		riscv32_smp_smoke_start();
	}
	if (riscv32_boot_arg_has("umode-smoke")) {
		riscv32_umode_smoke();
	}
	if (riscv32_boot_arg_has("linux-umode-smoke")) {
		(void)riscv32_linux_exec_umode_smoke();
	}
}

void
slave_machine_init(void *machine_param)
{
	cpu_data_t *cpu_data = current_cpu_datap();
	uint32_t ssie = RISCV32_SIE_SSIE;

	(void)machine_param;
	cpu_init();
	riscv32_trap_init();
	__asm__ volatile("csrc sip, %0" :: "r"(ssie) : "memory");
	__asm__ volatile("csrs sie, %0" :: "r"(ssie) : "memory");
	riscv32_timer_enable();
	riscv32_sbi_set_timer(riscv32_timer_read() + 100000U);
	cpu_data->cpu_running = TRUE;
	cpu_data->cpu_boot_complete = TRUE;
	cpu_data->cpu_iflag = TRUE;
	(void)ml_set_interrupts_enabled(TRUE);
}

static void
riscv32_log_early_storage(void)
{
	const riscv32_hfsplus_probe_t *root;
	riscv32_hfsplus_file_t kernel_file;
	uint64_t capacity;

	capacity = riscv32_virtio_blk_capacity();
	riscv32_early_puts("[riscv32] virtio-blk sectors=");
	riscv32_early_putdec((uint32_t)capacity);
	riscv32_early_puts("\n");

	if (riscv32_hfsplus_probe_root() != 0) {
		riscv32_early_puts("[riscv32] HFS+ probe failed\n");
		return;
	}

	root = riscv32_hfsplus_root_info();
	riscv32_early_puts("[riscv32] HFS+ sig=0x");
	riscv32_early_puthex32(root->signature);
	riscv32_early_puts(" block=");
	riscv32_early_putdec(root->block_size);
	riscv32_early_puts(" catNode=");
	riscv32_early_putdec(root->catalog_node_size);
	riscv32_early_puts("\n");

	if (riscv32_hfsplus_find_path("/kernel.elf", &kernel_file) != 0) {
		riscv32_early_puts("[riscv32] /kernel.elf lookup failed\n");
		return;
	}

	riscv32_early_puts("[riscv32] /kernel.elf size=");
	riscv32_early_putdec((uint32_t)kernel_file.logical_size);
	riscv32_early_puts(" ext0=");
	riscv32_early_putdec(kernel_file.extents[0].start_block);
	riscv32_early_puts("+");
	riscv32_early_putdec(kernel_file.extents[0].block_count);
	if (riscv32_hfsplus_read_file(&kernel_file, 0,
	    riscv32_storage_probe_buf, sizeof(riscv32_storage_probe_buf)) == 0) {
		uint32_t head = ((uint32_t)riscv32_storage_probe_buf[0] << 24) |
		    ((uint32_t)riscv32_storage_probe_buf[1] << 16) |
		    ((uint32_t)riscv32_storage_probe_buf[2] << 8) |
		    (uint32_t)riscv32_storage_probe_buf[3];

		riscv32_early_puts(" head=0x");
		riscv32_early_puthex32(head);
	}
	riscv32_early_puts("\n");
}

static inline uint32_t
riscv32_read_sstatus(void)
{
	uint32_t value;

	__asm__ volatile("csrr %0, sstatus" : "=r"(value));
	return value;
}

static inline void
riscv32_write_sstatus(uint32_t value)
{
	__asm__ volatile("csrw sstatus, %0" :: "r"(value) : "memory");
}

boolean_t
ml_is64bit(void)
{
	return FALSE;
}

boolean_t
ml_thread_is64bit(thread_t thread)
{
	(void)thread;
	return FALSE;
}

boolean_t
ml_state_is64bit(void *state)
{
	(void)state;
	return FALSE;
}

void
ml_fp_setvalid(boolean_t value)
{
	(void)value;
}

void
ml_cpu_set_ldt(int ldt)
{
	(void)ldt;
}

void
ml_init_interrupt(void)
{
	riscv32_trap_init();
	riscv32_trap_smoke();
	riscv32_plic_init();
	riscv32_tty_init();
	riscv32_virtio_mmio_init();
	if (riscv32_virtio_blk_init() == 0) {
		riscv32_log_early_storage();
	}
	riscv32_plic_enable_external_interrupts();
	(void)ml_set_interrupts_enabled(TRUE);
}

void
ml_cause_interrupt(void)
{
	riscv32_send_ipi_to_hart((uint32_t)get_cpu_phys_number());
}

void
ml_install_interrupt_handler(void *nub, int source, void *target,
    IOInterruptHandler handler, void *refCon)
{
	riscv32_plic_register_irq(source, nub, target, handler, refCon);
}

void
ml_get_timebase(unsigned long long *timestamp)
{
	*timestamp = mach_absolute_time();
}

void
ml_init_lock_timeout(void)
{
	LockTimeOut = 1000000U;
	LockTimeOutTSC = 1000000U;
}

void
ml_init_delay_spin_threshold(int threshold)
{
	(void)threshold;
}

boolean_t
ml_delay_should_spin(uint64_t interval)
{
	(void)interval;
	return TRUE;
}

vm_offset_t
ml_static_ptovirt(vm_offset_t paddr)
{
	return riscv32_phys_to_kva((addr64_t)paddr);
}

void
ml_static_mfree(vm_offset_t vaddr, vm_size_t size)
{
	(void)vaddr;
	(void)size;
}

vm_offset_t
ml_static_malloc(vm_size_t size)
{
	(void)size;
	return 0;
}

vm_offset_t
ml_vtophys(vm_offset_t vaddr)
{
	addr64_t phys = riscv32_kva_to_phys(vaddr);
	if (phys <= UINT32_MAX) {
		return (vm_offset_t)phys;
	}
	return pmap_extract(kernel_pmap, vaddr);
}

vm_size_t
ml_nofault_copy(vm_offset_t virtsrc, vm_offset_t virtdst, vm_size_t size)
{
	const uint8_t *src = (const uint8_t *)(uintptr_t)virtsrc;
	uint8_t *dst = (uint8_t *)(uintptr_t)virtdst;
	vm_size_t copied;

	for (copied = 0; copied < size; copied++) {
		dst[copied] = src[copied];
	}
	return copied;
}

uint64_t
ml_cpu_cache_size(unsigned int level)
{
	(void)level;
	return 0;
}

uint64_t
ml_cpu_cache_sharing(unsigned int level)
{
	(void)level;
	return 1;
}

void
ml_init_max_cpus(unsigned long max_cpus)
{
	riscv32_max_cpus = max_cpus != 0 ? (int)max_cpus : 1;
}

void
ml_cpu_up(void)
{
}

void
ml_cpu_down(void)
{
}

void
bzero_phys_nc(addr64_t phys_address, uint32_t length)
{
	bzero_phys(phys_address, length);
}

boolean_t
machine_timeout_suspended(void)
{
	return FALSE;
}

kern_return_t
ml_interrupt_prewarm(uint64_t deadline)
{
	(void)deadline;
	return KERN_SUCCESS;
}

void
ml_get_power_state(boolean_t *icp, boolean_t *pidlep)
{
	if (icp != NULL) {
		*icp = ml_at_interrupt_context();
	}
	if (pidlep != NULL) {
		*pidlep = FALSE;
	}
}

kern_return_t
ml_processor_register(cpu_id_t cpu_id, uint32_t hart_id,
    processor_t *processor_out, boolean_t boot_cpu, boolean_t start)
{
	(void)cpu_id;
	(void)hart_id;
	(void)boot_cpu;
	(void)start;
	*processor_out = current_processor();
	return KERN_SUCCESS;
}

boolean_t
ml_probe_read(vm_offset_t paddr, unsigned int *val)
{
	if (val != NULL) {
		*val = ml_phys_read(paddr);
	}
	return TRUE;
}

boolean_t
ml_probe_read_64(addr64_t paddr, unsigned int *val)
{
	if (val != NULL) {
		*val = ml_phys_read_64(paddr);
	}
	return TRUE;
}

static void *
riscv32_phys_ptr(addr64_t paddr, vm_size_t size)
{
	vm_offset_t kva;

	if (size == 0) {
		return NULL;
	}
	if (paddr > UINT32_MAX || paddr + size - 1 > UINT32_MAX) {
		panic("riscv32: physical address outside RV32 range");
	}
	kva = riscv32_phys_to_kva(paddr);
	if (kva == 0 && paddr != 0) {
		panic("riscv32: physical address outside direct window");
	}
	return (void *)(uintptr_t)kva;
}

unsigned int
ml_phys_read_byte(vm_offset_t paddr)
{
	volatile uint8_t *ptr = riscv32_phys_ptr(paddr, sizeof(*ptr));
	return *ptr;
}

unsigned int
ml_phys_read_byte_64(addr64_t paddr)
{
	return ml_phys_read_byte((vm_offset_t)paddr);
}

unsigned int
ml_phys_read_half(vm_offset_t paddr)
{
	volatile uint16_t *ptr = riscv32_phys_ptr(paddr, sizeof(*ptr));
	return *ptr;
}

unsigned int
ml_phys_read_half_64(addr64_t paddr)
{
	return ml_phys_read_half((vm_offset_t)paddr);
}

unsigned int
ml_phys_read(vm_offset_t paddr)
{
	volatile uint32_t *ptr = riscv32_phys_ptr(paddr, sizeof(*ptr));
	return *ptr;
}

unsigned int
ml_phys_read_64(addr64_t paddr)
{
	return ml_phys_read((vm_offset_t)paddr);
}

unsigned int
ml_phys_read_word(vm_offset_t paddr)
{
	return ml_phys_read(paddr);
}

unsigned int
ml_phys_read_word_64(addr64_t paddr)
{
	return ml_phys_read((vm_offset_t)paddr);
}

unsigned long long
ml_phys_read_double(vm_offset_t paddr)
{
	volatile uint64_t *ptr = riscv32_phys_ptr(paddr, sizeof(*ptr));
	return *ptr;
}

unsigned long long
ml_phys_read_double_64(addr64_t paddr)
{
	return ml_phys_read_double((vm_offset_t)paddr);
}

void
ml_phys_write_byte(vm_offset_t paddr, unsigned int data)
{
	volatile uint8_t *ptr = riscv32_phys_ptr(paddr, sizeof(*ptr));
	*ptr = (uint8_t)data;
}

void
ml_phys_write_byte_64(addr64_t paddr, unsigned int data)
{
	ml_phys_write_byte((vm_offset_t)paddr, data);
}

void
ml_phys_write_half(vm_offset_t paddr, unsigned int data)
{
	volatile uint16_t *ptr = riscv32_phys_ptr(paddr, sizeof(*ptr));
	*ptr = (uint16_t)data;
}

void
ml_phys_write_half_64(addr64_t paddr, unsigned int data)
{
	ml_phys_write_half((vm_offset_t)paddr, data);
}

void
ml_phys_write(vm_offset_t paddr, unsigned int data)
{
	volatile uint32_t *ptr = riscv32_phys_ptr(paddr, sizeof(*ptr));
	*ptr = data;
}

void
ml_phys_write_64(addr64_t paddr, unsigned int data)
{
	ml_phys_write((vm_offset_t)paddr, data);
}

void
ml_phys_write_word(vm_offset_t paddr, unsigned int data)
{
	ml_phys_write(paddr, data);
}

void
ml_phys_write_word_64(addr64_t paddr, unsigned int data)
{
	ml_phys_write((vm_offset_t)paddr, data);
}

void
ml_phys_write_double(vm_offset_t paddr, unsigned long long data)
{
	volatile uint64_t *ptr = riscv32_phys_ptr(paddr, sizeof(*ptr));
	*ptr = data;
}

void
ml_phys_write_double_64(addr64_t paddr, unsigned long long data)
{
	ml_phys_write_double((vm_offset_t)paddr, data);
}

void
ml_cpu_get_info(ml_cpu_info_t *info)
{
	info->vector_unit = 0;
	info->cache_line_size = 64;
	info->l1_icache_size = 0;
	info->l1_dcache_size = 0;
	info->l2_settings = 0;
	info->l2_cache_size = 0;
	info->l3_settings = 0;
	info->l3_cache_size = 0;
}

void
ml_thread_policy(thread_t thread, unsigned policy_id, unsigned policy_info)
{
	(void)thread;
	(void)policy_id;
	(void)policy_info;
}

int
ml_get_max_cpus(void)
{
	return riscv32_max_cpus;
}

uint64_t
ml_cpu_int_event_time(void)
{
	return current_cpu_datap()->cpu_int_event_time;
}

boolean_t
ml_get_interrupts_enabled(void)
{
	return (riscv32_read_sstatus() & RISCV32_SSTATUS_SIE) != 0;
}

boolean_t
ml_set_interrupts_enabled(boolean_t enable)
{
	uint32_t sstatus = riscv32_read_sstatus();
	boolean_t old = (sstatus & RISCV32_SSTATUS_SIE) != 0;

	current_cpu_datap()->cpu_iflag = enable;
	if (enable) {
		sstatus |= RISCV32_SSTATUS_SIE;
	} else {
		sstatus &= ~RISCV32_SSTATUS_SIE;
	}
	riscv32_write_sstatus(sstatus);
	return old;
}

boolean_t
ml_at_interrupt_context(void)
{
	return get_interrupt_level() != 0;
}

void
bzero_phys(addr64_t phys_address, uint32_t length)
{
	if (length == 0) {
		return;
	}
	memset(riscv32_phys_ptr(phys_address, length), 0, length);
}

vm_offset_t
ml_stack_remaining(void)
{
	return 0;
}

void
ml_copy_phys(addr64_t src, addr64_t dst, vm_size_t bytes)
{
	if (bytes == 0) {
		return;
	}
	memcpy(riscv32_phys_ptr(dst, bytes), riscv32_phys_ptr(src, bytes),
	    bytes);
}

void
machine_delay_until(uint64_t interval, uint64_t deadline)
{
	(void)interval;
	while (mach_absolute_time() < deadline) {
		cpu_pause();
	}
}

void
init_ast_check(processor_t processor)
{
	cpu_data_t *cpu_data;
	int cpu;

	if (processor == PROCESSOR_NULL) {
		return;
	}
	cpu = processor->cpu_id;
	cpu_data = cpu_datap(cpu);
	if (cpu_data == NULL) {
		return;
	}
	cpu_data->cpu_signals = 0;
	cpu_data->cpu_prior_signals = 0;
	cpu_data->cpu_pending_ast = 0;
	cpu_data->cpu_ast_ipi_count = 0;
	cpu_data->cpu_ast_preemption_count = 0;
	cpu_data->cpu_ast_signal_count = 0;
	cpu_data->cpu_wakeup_ipi_count = 0;
	cpu_data->cpu_unhandled_ipi_count = 0;
	__sync_synchronize();
}

void
cause_ast_check(processor_t processor)
{
	cpu_data_t *cpu_data;
	uint32_t previous;
	int cpu;

	if (processor == PROCESSOR_NULL) {
		return;
	}
	cpu = processor->cpu_id;
	if (cpu == get_cpu_number()) {
		return;
	}
	cpu_data = cpu_datap(cpu);
	if (cpu_data == NULL || cpu_data->cpu_running == 0) {
		return;
	}
	(void)__sync_add_and_fetch(&cpu_data->cpu_ast_signal_count, 1);
	previous = (uint32_t)__sync_fetch_and_or(&cpu_data->cpu_signals,
	    RISCV32_CPU_SIGNAL_AST);
	__sync_synchronize();
	if ((previous & RISCV32_CPU_SIGNAL_AST) == 0) {
		machine_signal_idle(processor);
	}
}

thread_t
machine_processor_shutdown(thread_t thread,
    void (*doshutdown)(processor_t), processor_t processor)
{
	doshutdown(processor);
	return thread;
}

void
machine_idle(void)
{
	cpu_data_t *cpu_data = current_cpu_datap();
	processor_t processor = current_processor();
	boolean_t interrupts_enabled;
	boolean_t slept = FALSE;
	uint32_t spins;

	(void)__sync_add_and_fetch(&cpu_data->cpu_idle_count, 1);
	if (cpu_data->cpu_number != 0 &&
	    cpu_data->cpu_timer_interrupt_count == 0) {
		riscv32_sbi_set_timer(riscv32_timer_read() + 1000U);
	}
	/*
	 * Gate 14 repeatedly hands a freshly runnable, CPU-bound user thread to
	 * the secondary while its idle thread is at splsched.  Polling here keeps
	 * that handoff deterministic: interrupts are enabled for a bounded window
	 * and the scheduler's outer idle loop is revisited without a blocking WFI.
	 * The boot CPU and non-stress SMP paths retain the normal low-power path.
	 */
	if (cpu_data->cpu_number != 0 &&
	    riscv32_linux_gate14_stress_enabled()) {
		interrupts_enabled = ml_set_interrupts_enabled(TRUE);
		for (spins = 0; spins < RISCV32_IDLE_POLL_SPINS; spins++) {
			__sync_synchronize();
			if (processor != PROCESSOR_NULL &&
			    (processor->state == PROCESSOR_DISPATCHING ||
			    processor->next_thread != THREAD_NULL)) {
				break;
			}
			__asm__ volatile("nop" ::: "memory");
		}
		(void)ml_set_interrupts_enabled(interrupts_enabled);
		return;
	}
	/*
	 * Keep global SIE clear across WFI.  Locally enabled pending interrupts
	 * still wake RISC-V, but cannot be serviced and cleared in the narrow
	 * window between the final work check and WFI.
	 */
	interrupts_enabled = ml_set_interrupts_enabled(FALSE);
	__sync_synchronize();
	if (processor == PROCESSOR_NULL ||
	    (processor->state != PROCESSOR_DISPATCHING &&
	    processor->next_thread == THREAD_NULL &&
	    __sync_fetch_and_add(&cpu_data->cpu_signals, 0) == 0)) {
		__asm__ volatile("wfi" ::: "memory");
		slept = TRUE;
	}
	if (slept) {
		(void)ml_set_interrupts_enabled(TRUE);
		__asm__ volatile("nop" ::: "memory");
	}
	(void)ml_set_interrupts_enabled(interrupts_enabled);
}

void
machine_track_platform_idle(boolean_t entry)
{
	(void)entry;
}

void
machine_signal_idle(processor_t processor)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	uint32_t slot;

	if (processor == PROCESSOR_NULL ||
	    bootinfo == (const riscv32_bootinfo_t *)0) {
		return;
	}
	slot = (uint32_t)processor->cpu_id;
	if (slot >= bootinfo->cpu_count || slot >= RISCV32_MAX_CPUS) {
		return;
	}
	riscv32_send_ipi_to_hart(bootinfo->hart_ids[slot]);
}

void
halt_cpu(void)
{
	for (;;) {
		cpu_halt();
	}
}

void
halt_all_cpus(boolean_t reboot)
{
	riscv32_request_system_reset(reboot);
	halt_cpu();
}

char *
machine_boot_info(char *buf, vm_size_t buf_len)
{
	(void)buf_len;
	return buf;
}

void
machine_callstack(natural_t *buf, vm_size_t callstack_max)
{
	vm_size_t i;

	for (i = 0; i < callstack_max; i++) {
		buf[i] = 0;
	}
}

void
consider_machine_collect(void)
{
}
