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
