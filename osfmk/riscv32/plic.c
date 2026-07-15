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
#include <kern/debug.h>
#include <machine/bootinfo.h>
#include <machine/plic.h>
#include <stddef.h>

#define RISCV32_PLIC_DEFAULT_BASE	0x0c000000U
#define RISCV32_PLIC_PRIORITY_BASE	0x000000U
#define RISCV32_PLIC_PENDING_BASE	0x001000U
#define RISCV32_PLIC_ENABLE_BASE	0x002000U
#define RISCV32_PLIC_ENABLE_STRIDE	0x80U
#define RISCV32_PLIC_CONTEXT_BASE	0x200000U
#define RISCV32_PLIC_CONTEXT_STRIDE	0x1000U
#define RISCV32_PLIC_THRESHOLD		0x0U
#define RISCV32_PLIC_CLAIM_COMPLETE	0x4U
#define RISCV32_SIE_SEIE		0x00000200U

typedef struct riscv32_plic_handler {
	void *nub;
	void *target;
	void *refCon;
	IOInterruptHandler handler;
} riscv32_plic_handler_t;

static uint32_t riscv32_plic_base = RISCV32_PLIC_DEFAULT_BASE;
static uint32_t riscv32_plic_context = 1;
static riscv32_plic_handler_t riscv32_plic_handlers[RISCV32_PLIC_MAX_IRQ];

static volatile uint32_t *
riscv32_plic_reg(uint32_t offset)
{
	return (volatile uint32_t *)(uintptr_t)(riscv32_plic_base + offset);
}

static uint32_t
riscv32_plic_context_offset(uint32_t reg)
{
	return RISCV32_PLIC_CONTEXT_BASE +
	    riscv32_plic_context * RISCV32_PLIC_CONTEXT_STRIDE + reg;
}

static uint32_t
riscv32_plic_enable_offset(uint32_t irq)
{
	return RISCV32_PLIC_ENABLE_BASE +
	    riscv32_plic_context * RISCV32_PLIC_ENABLE_STRIDE +
	    (irq / 32U) * sizeof(uint32_t);
}

void
riscv32_plic_init(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();

	if (bootinfo->plic_base != 0) {
		riscv32_plic_base = bootinfo->plic_base;
	}
	riscv32_plic_context = bootinfo->hartid * 2U + 1U;
	*riscv32_plic_reg(riscv32_plic_context_offset(
	    RISCV32_PLIC_THRESHOLD)) = 0;
}

void
riscv32_plic_enable_external_interrupts(void)
{
	uint32_t mask = RISCV32_SIE_SEIE;

	__asm__ volatile("csrs sie, %0" :: "r"(mask) : "memory");
}

void
riscv32_plic_enable_irq(uint32_t irq, uint32_t priority)
{
	volatile uint32_t *enable;
	uint32_t bit;

	if (irq == 0 || irq >= RISCV32_PLIC_MAX_IRQ) {
		return;
	}
	if (priority == 0) {
		priority = 1;
	}

	*riscv32_plic_reg(RISCV32_PLIC_PRIORITY_BASE +
	    irq * sizeof(uint32_t)) = priority;
	enable = riscv32_plic_reg(riscv32_plic_enable_offset(irq));
	bit = 1U << (irq % 32U);
	*enable = *enable | bit;
}

void
riscv32_plic_disable_irq(uint32_t irq)
{
	volatile uint32_t *enable;
	uint32_t bit;

	if (irq == 0 || irq >= RISCV32_PLIC_MAX_IRQ) {
		return;
	}

	enable = riscv32_plic_reg(riscv32_plic_enable_offset(irq));
	bit = 1U << (irq % 32U);
	*enable = *enable & ~bit;
}

uint32_t
riscv32_plic_claim(void)
{
	return *riscv32_plic_reg(riscv32_plic_context_offset(
	    RISCV32_PLIC_CLAIM_COMPLETE));
}

void
riscv32_plic_complete(uint32_t irq)
{
	*riscv32_plic_reg(riscv32_plic_context_offset(
	    RISCV32_PLIC_CLAIM_COMPLETE)) = irq;
}

void
riscv32_plic_register_irq(int source, void *nub, void *target,
    IOInterruptHandler handler, void *refCon)
{
	riscv32_plic_handler_t *slot;

	if (source <= 0 || source >= (int)RISCV32_PLIC_MAX_IRQ) {
		panic("riscv32: invalid PLIC source %d", source);
	}

	slot = &riscv32_plic_handlers[source];
	slot->nub = nub;
	slot->target = target;
	slot->refCon = refCon;
	slot->handler = handler;
	riscv32_plic_enable_irq((uint32_t)source, 1);
}

void
riscv32_plic_handle_external(riscv_saved_state_t *state)
{
	uint32_t irq;
	riscv32_plic_handler_t *slot;

	(void)state;
	irq = riscv32_plic_claim();
	if (irq == 0) {
		return;
	}

	if (irq < RISCV32_PLIC_MAX_IRQ) {
		slot = &riscv32_plic_handlers[irq];
		if (slot->handler != NULL) {
			slot->handler(slot->target, slot->refCon, slot->nub,
			    (int)irq);
		}
	}

	riscv32_plic_complete(irq);
}
