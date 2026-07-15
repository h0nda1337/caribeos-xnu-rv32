#ifdef KERNEL_PRIVATE

#ifndef _RISCV32_LOCK_H_
#define _RISCV32_LOCK_H_

#include <sys/appleapiopts.h>

#ifdef __APPLE_API_PRIVATE

#ifdef MACH_KERNEL_PRIVATE

#include <kern/macro_help.h>
#include <kern/assert.h>
#include <riscv32/locks.h>
#include <stdint.h>

typedef lck_rw_t lock_t;

#define bit_lock(bit,l)							\
	MACRO_BEGIN							\
		uint32_t __mask = (uint32_t)(1U << (bit));		\
		while (__sync_fetch_and_or((volatile uint32_t *)(l),	\
		    __mask) & __mask) {					\
		}							\
	MACRO_END

#define bit_unlock(bit,l)						\
	(void)__sync_fetch_and_and((volatile uint32_t *)(l),		\
	    (uint32_t)~(1U << (bit)))

#define i_bit_set(bit,l)						\
	(void)__sync_fetch_and_or((volatile uint32_t *)(l),		\
	    (uint32_t)(1U << (bit)))

#define i_bit_clear(bit,l)						\
	(void)__sync_fetch_and_and((volatile uint32_t *)(l),		\
	    (uint32_t)~(1U << (bit)))

static inline char
xchgb(volatile char *cp, char new_value)
{
	return __sync_lock_test_and_set(cp, new_value);
}

static inline void atomic_incl(volatile long *p, long delta)
{
	(void)__sync_fetch_and_add(p, delta);
}

static inline void atomic_decl(volatile long *p, long delta)
{
	(void)__sync_fetch_and_sub(p, delta);
}

static inline int atomic_decl_and_test(volatile long *p, long delta)
{
	return (__sync_sub_and_fetch(p, delta) == 0);
}

static inline long atomic_getl(const volatile long *p)
{
	return *p;
}

static inline void atomic_setl(volatile long *p, long value)
{
	*p = value;
}

#endif /* MACH_KERNEL_PRIVATE */

#endif /* __APPLE_API_PRIVATE */

#endif /* _RISCV32_LOCK_H_ */

#endif /* KERNEL_PRIVATE */
