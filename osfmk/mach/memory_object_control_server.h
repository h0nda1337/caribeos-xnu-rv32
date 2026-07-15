#ifndef _MACH_MEMORY_OBJECT_CONTROL_SERVER_H_
#define _MACH_MEMORY_OBJECT_CONTROL_SERVER_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/memory_object_types.h>
#include <mach/vm_prot.h>

extern const struct mig_subsystem memory_object_control_subsystem;

kern_return_t memory_object_get_attributes(
    memory_object_control_t control,
    memory_object_flavor_t flavor,
    memory_object_info_t attributes,
    mach_msg_type_number_t *count);
kern_return_t memory_object_change_attributes(
    memory_object_control_t control,
    memory_object_flavor_t flavor,
    memory_object_info_t attributes,
    mach_msg_type_number_t count);
kern_return_t memory_object_synchronize_completed(
    memory_object_control_t control,
    memory_object_offset_t offset,
    memory_object_size_t length);
kern_return_t memory_object_lock_request(
    memory_object_control_t control,
    memory_object_offset_t offset,
    memory_object_size_t size,
    memory_object_offset_t *resid_offset,
    int *io_errno,
    memory_object_return_t should_return,
    int flags,
    vm_prot_t prot);
kern_return_t memory_object_destroy(
    memory_object_control_t control,
    kern_return_t reason);
kern_return_t memory_object_upl_request(
    memory_object_control_t control,
    memory_object_offset_t offset,
    upl_size_t size,
    upl_t *upl_ptr,
    upl_page_info_array_t user_page_list,
    unsigned int *page_list_count,
    int cntrl_flags);
kern_return_t memory_object_super_upl_request(
    memory_object_control_t control,
    memory_object_offset_t offset,
    upl_size_t size,
    upl_size_t super_cluster,
    upl_t *upl,
    upl_page_info_t *user_page_list,
    unsigned int *page_list_count,
    int cntrl_flags);
kern_return_t memory_object_cluster_size(
    memory_object_control_t control,
    memory_object_offset_t *start,
    vm_size_t *length,
    uint32_t *io_streaming,
    memory_object_fault_info_t fault_info);
kern_return_t memory_object_page_op(
    memory_object_control_t control,
    memory_object_offset_t offset,
    int ops,
    ppnum_t *phys_entry,
    int *flags);
kern_return_t memory_object_release_name(
    memory_object_control_t control,
    int flags);
kern_return_t memory_object_range_op(
    memory_object_control_t control,
    memory_object_offset_t offset_beg,
    memory_object_offset_t offset_end,
    int ops,
    int *range);

#endif /* _MACH_MEMORY_OBJECT_CONTROL_SERVER_H_ */
