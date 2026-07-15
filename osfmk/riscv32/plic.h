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
#ifndef _RISCV32_PLIC_H_
#define _RISCV32_PLIC_H_

#include <mach/riscv32/thread_status.h>
#include <pexpert/pexpert.h>
#include <stdint.h>

#define RISCV32_PLIC_MAX_IRQ	128U

void riscv32_plic_init(void);
void riscv32_plic_enable_external_interrupts(void);
void riscv32_plic_enable_irq(uint32_t irq, uint32_t priority);
void riscv32_plic_disable_irq(uint32_t irq);
uint32_t riscv32_plic_claim(void);
void riscv32_plic_complete(uint32_t irq);
void riscv32_plic_register_irq(int source, void *nub, void *target,
    IOInterruptHandler handler, void *refCon);
void riscv32_plic_handle_external(riscv_saved_state_t *state);

#endif /* _RISCV32_PLIC_H_ */
