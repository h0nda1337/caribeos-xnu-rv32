#ifndef _RISCV32_THREAD_H_
#define _RISCV32_THREAD_H_

#include <mach/boolean.h>
#include <mach/riscv32/vm_types.h>
#include <mach/thread_status.h>
#include <kern/lock.h>
#include <machine/pal_routines.h>
#include <machine/cpu_data.h>
#include <riscv32/user_exception.h>

struct riscv_kernel_state {
	uint32_t ra;
	uint32_t sp;
	uint32_t s[12];
	uint32_t pad[2];
};

extern unsigned int _MachineStateCount[];

struct machine_thread {
	void			*sf;
	riscv_saved_state_t	*iss;
	riscv32_user_exception_t	last_user_exception;
	void			*ifps;
	void			*ids;
	decl_simple_lock_data(,lock)
	uint32_t		arg_store_valid;
#ifdef MACH_BSD
	uint64_t		cthread_self;
	unsigned long		uldt_selector;
#endif
	struct pal_pcb		pal_pcb;
	uint32_t		specFlags;
#define OnProc		0x1
#define CopyIOActive	0x2
	vm_offset_t		kstackptr;
	vm_offset_t		copyio_recover;
	uint32_t		copyio_fault_addr;
	uint32_t		copyio_fault_code;
	int			copyio_error;
	struct pmap		*user_pmap;
	uint32_t		*deallocate_sequence;
};

typedef struct machine_thread *pcb_t;

#define THREAD_TO_PCB(Thr)	(&(Thr)->machine)
#define USER_STATE(Thr)		((Thr)->machine.iss)
#define user_pc(Thr)		(USER_STATE(Thr)->thread.pc)

extern void *get_user_regs(thread_t thread);
extern void *act_thread_csave(void);
extern void act_thread_catt(void *ctx);
extern void act_thread_cfree(void *ctx);
extern uint32_t riscv32_reserved_stack_handoff_count(void);

#define STACK_IKS(stack) \
	((struct riscv_kernel_state *)((stack) + kernel_stack_size) - 1)

static inline vm_offset_t
current_stack_depth(void)
{
	register vm_offset_t stack_ptr __asm__("sp");
	return current_cpu_datap()->cpu_kernel_stack +
	    sizeof(struct riscv_kernel_state) - stack_ptr;
}

#define GET_RETURN_PC(addr)	(__builtin_return_address(0))

#endif /* _RISCV32_THREAD_H_ */
