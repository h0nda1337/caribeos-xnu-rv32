#ifdef PRIVATE

#ifndef _MACH_RISCV32_SYSCALL_SW_H_
#define _MACH_RISCV32_SYSCALL_SW_H_

#define SYSCALL_CLASS_SHIFT	24
#define SYSCALL_CLASS_MASK	(0xff << SYSCALL_CLASS_SHIFT)
#define SYSCALL_NUMBER_MASK	(~SYSCALL_CLASS_MASK)

#define SYSCALL_CLASS_NONE	0
#define SYSCALL_CLASS_MACH	1
#define SYSCALL_CLASS_UNIX	2
#define SYSCALL_CLASS_MDEP	3
#define SYSCALL_CLASS_DIAG	4
#define SYSCALL_CLASS_IPC	5
#define SYSCALL_CLASS_LINUX	6

#define SYSCALL_CONSTRUCT_MACH(syscall_number)	\
	((SYSCALL_CLASS_MACH << SYSCALL_CLASS_SHIFT) | \
	 (SYSCALL_NUMBER_MASK & (syscall_number)))
#define SYSCALL_CONSTRUCT_UNIX(syscall_number)	\
	((SYSCALL_CLASS_UNIX << SYSCALL_CLASS_SHIFT) | \
	 (SYSCALL_NUMBER_MASK & (syscall_number)))
#define SYSCALL_CONSTRUCT_LINUX(syscall_number)	\
	((SYSCALL_CLASS_LINUX << SYSCALL_CLASS_SHIFT) | \
	 (SYSCALL_NUMBER_MASK & (syscall_number)))

#ifndef KERNEL
#define UNIX_SYSCALL_TRAP	ecall
#define MACHDEP_SYSCALL_TRAP	ecall
#define kernel_trap(trap_name,trap_number,number_args)
#endif

#endif /* _MACH_RISCV32_SYSCALL_SW_H_ */

#endif /* PRIVATE */
