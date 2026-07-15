#ifndef _MACH_VM_MAP_H_
#define _MACH_VM_MAP_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>

kern_return_t vm_allocate(vm_map_t target_map, vm_offset_t *address,
    vm_size_t size, int flags);
kern_return_t vm_deallocate(vm_map_t target_map, vm_offset_t address,
    vm_size_t size);

#endif /* _MACH_VM_MAP_H_ */
