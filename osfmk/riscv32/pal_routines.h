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
#ifndef _RISCV32_PAL_ROUTINES_H_
#define _RISCV32_PAL_ROUTINES_H_

#include <stdint.h>
#include <stddef.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/riscv32/thread_status.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct pal_efi_registers {
	uint64_t a0;
	uint64_t a1;
	uint64_t a2;
	uint64_t a3;
	uint64_t a4;
};

kern_return_t pal_efi_call_in_64bit_mode(uint64_t func,
    struct pal_efi_registers *efi_reg, void *stack_contents,
    size_t stack_contents_size, uint64_t *efi_status);
kern_return_t pal_efi_call_in_32bit_mode(uint32_t func,
    struct pal_efi_registers *efi_reg, void *stack_contents,
    size_t stack_contents_size, uint32_t *efi_status);
boolean_t pal_machine_sleep(uint8_t type_a, uint8_t type_b,
    uint32_t bit_position, uint32_t disable_mask, uint32_t enable_mask);

#if defined(XNU_KERNEL_PRIVATE) || defined(MACH_KERNEL_PRIVATE)
typedef uint32_t pal_cr_t;
typedef enum { DIRTY, VALID } pal_cache_state_t;

struct pal_cpu_data {
	uintptr_t scratch_t0;
	uintptr_t scratch_t1;
	uintptr_t scratch_t2;
	uintptr_t scratch_tp;
	uintptr_t scratch_sp;
	uintptr_t trap_stack_top;
};

struct pal_pcb {
	uintptr_t kernel_sp;
	uintptr_t saved_pc;
};

extern int pal_serial_init(void);
extern void pal_serial_putc(char c);
extern int pal_serial_getc(void);
extern void pal_set_signal_delivery(thread_t thread);
extern void pal_get_control_registers(pal_cr_t *satp, pal_cr_t *sstatus,
    pal_cr_t *stvec, pal_cr_t *sie);
extern void pal_dbg_page_fault(thread_t thread, user_addr_t vaddr,
    kern_return_t kr);
extern void pal_dbg_set_task_name(task_t task);
extern void pal_syscall_restart(thread_t thread, riscv_saved_state_t *state);
extern void pal_execve_return(thread_t thread);
extern void pal_thread_terminate_self(thread_t thread);
extern void pal_ast_check(thread_t thread);
extern void pal_get_kern_regs(riscv_saved_state_t *state);
extern void pal_switch_pmap(thread_t thread, pmap_t pmap, boolean_t user);
extern void pal_hlt(void);
extern void pal_sti(void);
extern void pal_cli(void);
extern void pal_register_cache_state(thread_t thread, pal_cache_state_t state);
extern void pal_preemption_assert(void);
extern void hibernate_pal_prepare(void);
extern void pal_efi_hibernate_prepare(void);
#define panic_display_pal_info() do { } while (0)

#define PAL_RISCV_PROPERTY_NAME "riscv_platform_matching"
static inline void
pal_get_resource_property(const char **property_name, int *property_value)
{
	*property_name = PAL_RISCV_PROPERTY_NAME;
	*property_value = 0;
}
#endif

#if defined(__cplusplus)
}
#endif

#endif /* _RISCV32_PAL_ROUTINES_H_ */
