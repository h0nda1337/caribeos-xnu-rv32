#ifdef KERNEL_PRIVATE

#ifndef _RISCV32_SIMPLE_LOCK_TYPES_H_
#define _RISCV32_SIMPLE_LOCK_TYPES_H_

#include <mach/boolean.h>
#include <kern/kern_types.h>
#include <stdint.h>

typedef struct hw_lock_data {
	volatile uint32_t	lock_data;
} hw_lock_data_t, *hw_lock_t;

typedef struct slock {
	hw_lock_data_t		interlock;
} usimple_lock_data_t, *usimple_lock_t;

#define USIMPLE_LOCK_NULL	((usimple_lock_t)0)

#if !defined(decl_simple_lock_data)
typedef usimple_lock_data_t	*simple_lock_t;
typedef usimple_lock_data_t	simple_lock_data_t;

#define decl_simple_lock_data(class,name) \
	class simple_lock_data_t name;
#endif

#endif /* _RISCV32_SIMPLE_LOCK_TYPES_H_ */

#endif /* KERNEL_PRIVATE */
