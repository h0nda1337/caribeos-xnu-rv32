#ifdef KERNEL_PRIVATE

#ifndef _RISCV32_CPU_AFFINITY_H_
#define _RISCV32_CPU_AFFINITY_H_

#include <kern/kern_types.h>
#include <stdint.h>

extern int ml_get_max_affinity_sets(void);
extern processor_set_t ml_affinity_to_pset(uint32_t affinity_num);

#endif /* _RISCV32_CPU_AFFINITY_H_ */

#endif /* KERNEL_PRIVATE */
