#ifndef _RISCV32_CPU_NUMBER_H_
#define _RISCV32_CPU_NUMBER_H_

#ifdef KERNEL_PRIVATE
#include <machine/cpu_data.h>
#define cpu_number()	get_cpu_number()
#endif

#endif /* _RISCV32_CPU_NUMBER_H_ */
