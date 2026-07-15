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
#ifndef _RISCV32_MACHINE_ROUTINES_H_
#define _RISCV32_MACHINE_ROUTINES_H_

#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <kern/kern_types.h>
#include <pexpert/pexpert.h>
#include <sys/cdefs.h>
#include <sys/appleapiopts.h>

__BEGIN_DECLS

#if defined(XNU_KERNEL_PRIVATE) || defined(MACH_KERNEL_PRIVATE)
extern unsigned int LockTimeOut;
extern unsigned int LockTimeOutTSC;

boolean_t ml_is64bit(void);
boolean_t ml_thread_is64bit(thread_t thread);
boolean_t ml_state_is64bit(void *state);
void ml_fp_setvalid(boolean_t value);
void ml_cpu_set_ldt(int ldt);
void ml_init_interrupt(void);
void ml_cause_interrupt(void);
void ml_install_interrupt_handler(void *nub, int source, void *target,
    IOInterruptHandler handler, void *refCon);
void ml_get_timebase(unsigned long long *timestamp);
void ml_init_lock_timeout(void);
void ml_init_delay_spin_threshold(int threshold);
boolean_t ml_delay_should_spin(uint64_t interval);
vm_offset_t ml_static_ptovirt(vm_offset_t paddr);
void ml_static_mfree(vm_offset_t vaddr, vm_size_t size);
vm_offset_t ml_static_malloc(vm_size_t size);
vm_offset_t ml_vtophys(vm_offset_t vaddr);
vm_size_t ml_nofault_copy(vm_offset_t virtsrc, vm_offset_t virtdst,
    vm_size_t size);
uint64_t ml_cpu_cache_size(unsigned int level);
uint64_t ml_cpu_cache_sharing(unsigned int level);
void ml_init_max_cpus(unsigned long max_cpus);
void ml_cpu_up(void);
void ml_cpu_down(void);
void bzero_phys_nc(addr64_t phys_address, uint32_t length);
boolean_t machine_timeout_suspended(void);
kern_return_t ml_interrupt_prewarm(uint64_t deadline);
void ml_get_power_state(boolean_t *icp, boolean_t *pidlep);
#endif

#ifdef KERNEL_PRIVATE
typedef void (*time_base_enable_t)(cpu_id_t cpu_id, boolean_t enable);
typedef void (*ipi_handler_t)(void);

struct ml_processor_info {
	cpu_id_t		cpu_id;
	boolean_t		boot_cpu;
	vm_offset_t		start_paddr;
	boolean_t		supports_nap;
	unsigned long		l2cr_value;
	time_base_enable_t	time_base_enable;
};
typedef struct ml_processor_info ml_processor_info_t;

kern_return_t ml_processor_register(cpu_id_t cpu_id, uint32_t hart_id,
    processor_t *processor_out, boolean_t boot_cpu, boolean_t start);
boolean_t ml_probe_read(vm_offset_t paddr, unsigned int *val);
boolean_t ml_probe_read_64(addr64_t paddr, unsigned int *val);
unsigned int ml_phys_read_byte(vm_offset_t paddr);
unsigned int ml_phys_read_byte_64(addr64_t paddr);
unsigned int ml_phys_read_half(vm_offset_t paddr);
unsigned int ml_phys_read_half_64(addr64_t paddr);
unsigned int ml_phys_read(vm_offset_t paddr);
unsigned int ml_phys_read_64(addr64_t paddr);
unsigned int ml_phys_read_word(vm_offset_t paddr);
unsigned int ml_phys_read_word_64(addr64_t paddr);
unsigned long long ml_phys_read_double(vm_offset_t paddr);
unsigned long long ml_phys_read_double_64(addr64_t paddr);
void ml_phys_write_byte(vm_offset_t paddr, unsigned int data);
void ml_phys_write_byte_64(addr64_t paddr, unsigned int data);
void ml_phys_write_half(vm_offset_t paddr, unsigned int data);
void ml_phys_write_half_64(addr64_t paddr, unsigned int data);
void ml_phys_write(vm_offset_t paddr, unsigned int data);
void ml_phys_write_64(addr64_t paddr, unsigned int data);
void ml_phys_write_word(vm_offset_t paddr, unsigned int data);
void ml_phys_write_word_64(addr64_t paddr, unsigned int data);
void ml_phys_write_double(vm_offset_t paddr, unsigned long long data);
void ml_phys_write_double_64(addr64_t paddr, unsigned long long data);

struct ml_cpu_info {
	uint32_t	vector_unit;
	uint32_t	cache_line_size;
	uint32_t	l1_icache_size;
	uint32_t	l1_dcache_size;
	uint32_t	l2_settings;
	uint32_t	l2_cache_size;
	uint32_t	l3_settings;
	uint32_t	l3_cache_size;
};
typedef struct ml_cpu_info ml_cpu_info_t;

void ml_cpu_get_info(ml_cpu_info_t *ml_cpu_info);
void ml_thread_policy(thread_t thread, unsigned policy_id,
    unsigned policy_info);
int ml_get_max_cpus(void);
uint64_t ml_cpu_int_event_time(void);
#endif

boolean_t ml_get_interrupts_enabled(void);
boolean_t ml_set_interrupts_enabled(boolean_t enable);
boolean_t ml_at_interrupt_context(void);
void bzero_phys(addr64_t phys_address, uint32_t length);
vm_offset_t ml_stack_remaining(void);
void ml_copy_phys(addr64_t src, addr64_t dst, vm_size_t bytes);

__END_DECLS

#endif /* _RISCV32_MACHINE_ROUTINES_H_ */
