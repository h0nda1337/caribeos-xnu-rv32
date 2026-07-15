#ifndef _MACH_MEMORY_OBJECT_DEFAULT_H_
#define _MACH_MEMORY_OBJECT_DEFAULT_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/memory_object_types.h>

kern_return_t memory_object_create(
	memory_object_default_t default_memory_manager,
	vm_size_t new_memory_object_size,
	memory_object_t *new_memory_object);

#endif /* _MACH_MEMORY_OBJECT_DEFAULT_H_ */
