#ifndef _MACH_RISCV32_VM_TYPES_H_
#define _MACH_RISCV32_VM_TYPES_H_

#ifndef ASSEMBLER

#include <riscv32/_types.h>
#include <mach/riscv32/vm_param.h>
#include <stdint.h>

typedef __darwin_natural_t	natural_t;
typedef int			integer_t;

typedef natural_t		vm_offset_t;
typedef natural_t		vm_size_t;

typedef uint64_t		mach_vm_address_t;
typedef uint64_t		mach_vm_offset_t;
typedef uint64_t		mach_vm_size_t;

typedef uint64_t		vm_map_offset_t;
typedef uint64_t		vm_map_address_t;
typedef uint64_t		vm_map_size_t;

typedef mach_vm_address_t	mach_port_context_t;

#ifdef MACH_KERNEL_PRIVATE
#if VM32_SUPPORT
typedef uint32_t		vm32_address_t;
typedef uint32_t		vm32_offset_t;
typedef uint32_t		vm32_size_t;
#endif
#endif

#endif /* ASSEMBLER */

#define MACH_MSG_TYPE_INTEGER_T	MACH_MSG_TYPE_INTEGER_32

#endif /* _MACH_RISCV32_VM_TYPES_H_ */
