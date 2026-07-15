#ifndef _MACH_MEMORY_OBJECT_SERVER_H_
#define _MACH_MEMORY_OBJECT_SERVER_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/memory_object_types.h>
#include <mach/vm_prot.h>

extern const struct mig_subsystem memory_object_subsystem;

kern_return_t memory_object_init(
    memory_object_t memory_object,
    memory_object_control_t memory_control,
    memory_object_cluster_size_t memory_object_page_size);
kern_return_t memory_object_terminate(
    memory_object_t memory_object);
kern_return_t memory_object_data_request(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_cluster_size_t length,
    vm_prot_t desired_access,
    memory_object_fault_info_t fault_info);
kern_return_t memory_object_data_return(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_cluster_size_t size,
    memory_object_offset_t *resid_offset,
    int *io_error,
    boolean_t dirty,
    boolean_t kernel_copy,
    int upl_flags);
kern_return_t memory_object_data_initialize(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_cluster_size_t size);
kern_return_t memory_object_data_unlock(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_size_t size,
    vm_prot_t desired_access);
kern_return_t memory_object_synchronize(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_size_t size,
    vm_sync_t sync_flags);
kern_return_t memory_object_map(
    memory_object_t memory_object,
    vm_prot_t prot);
kern_return_t memory_object_last_unmap(
    memory_object_t memory_object);
kern_return_t memory_object_data_reclaim(
    memory_object_t memory_object,
    boolean_t reclaim_backing_store);

#endif /* _MACH_MEMORY_OBJECT_SERVER_H_ */
