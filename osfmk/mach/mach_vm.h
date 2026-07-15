#ifndef _MACH_MACH_VM_H_
#define _MACH_MACH_VM_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/memory_object_types.h>
#include <mach/std_types.h>
#include <mach/vm_attributes.h>
#include <mach/vm_behavior.h>
#include <mach/vm_inherit.h>
#include <mach/vm_prot.h>
#include <mach/vm_region.h>
#include <mach/vm_sync.h>

kern_return_t mach_vm_allocate(vm_map_t map, mach_vm_offset_t *addr,
    mach_vm_size_t size, int flags);
kern_return_t mach_vm_deallocate(vm_map_t map, mach_vm_offset_t start,
    mach_vm_size_t size);
kern_return_t mach_vm_inherit(vm_map_t map, mach_vm_offset_t start,
    mach_vm_size_t size, vm_inherit_t new_inheritance);
kern_return_t mach_vm_protect(vm_map_t map, mach_vm_offset_t start,
    mach_vm_size_t size, boolean_t set_maximum, vm_prot_t new_protection);
kern_return_t mach_vm_machine_attribute(vm_map_t map, mach_vm_address_t addr,
    mach_vm_size_t size, vm_machine_attribute_t attribute,
    vm_machine_attribute_val_t *value);
kern_return_t mach_vm_read(vm_map_t map, mach_vm_address_t addr,
    mach_vm_size_t size, pointer_t *data, mach_msg_type_number_t *data_size);
kern_return_t mach_vm_read_list(vm_map_t map, mach_vm_read_entry_t data_list,
    natural_t count);
kern_return_t mach_vm_read_overwrite(vm_map_t map, mach_vm_address_t address,
    mach_vm_size_t size, mach_vm_address_t data, mach_vm_size_t *data_size);
kern_return_t mach_vm_write(vm_map_t map, mach_vm_address_t address,
    pointer_t data, mach_msg_type_number_t size);
kern_return_t mach_vm_copy(vm_map_t map, mach_vm_address_t source_address,
    mach_vm_size_t size, mach_vm_address_t dest_address);
kern_return_t mach_vm_map(vm_map_t target_map, mach_vm_offset_t *address,
    mach_vm_size_t initial_size, mach_vm_offset_t mask, int flags,
    ipc_port_t port, vm_object_offset_t offset, boolean_t copy,
    vm_prot_t cur_protection, vm_prot_t max_protection,
    vm_inherit_t inheritance);
kern_return_t mach_vm_remap(vm_map_t target_map, mach_vm_offset_t *address,
    mach_vm_size_t size, mach_vm_offset_t mask, int flags, vm_map_t src_map,
    mach_vm_offset_t memory_address, boolean_t copy,
    vm_prot_t *cur_protection, vm_prot_t *max_protection,
    vm_inherit_t inheritance);
kern_return_t mach_vm_msync(vm_map_t map, mach_vm_address_t address,
    mach_vm_size_t size, vm_sync_t sync_flags);
kern_return_t mach_vm_behavior_set(vm_map_t map, mach_vm_offset_t start,
    mach_vm_size_t size, vm_behavior_t new_behavior);
kern_return_t mach_vm_region(vm_map_t map, mach_vm_offset_t *address,
    mach_vm_size_t *size, vm_region_flavor_t flavor, vm_region_info_t info,
    mach_msg_type_number_t *count, mach_port_t *object_name);
kern_return_t mach_vm_region_recurse(vm_map_t map, mach_vm_address_t *address,
    mach_vm_size_t *size, uint32_t *depth, vm_region_recurse_info_t info,
    mach_msg_type_number_t *infoCnt);
kern_return_t _mach_make_memory_entry(vm_map_t target_map,
    memory_object_size_t *size, memory_object_offset_t offset,
    vm_prot_t permission, ipc_port_t *object_handle, ipc_port_t parent_entry);

#endif /* _MACH_MACH_VM_H_ */
