#ifndef _MACH_HOST_PRIV_SERVER_H_
#define _MACH_HOST_PRIV_SERVER_H_

#include <mach/clock_types.h>
#include <mach/host_priv.h>
#include <mach/host_info.h>
#include <mach/kmod.h>
#include <mach/mig.h>
#include <mach/processor_info.h>
#include <kern/kern_types.h>

extern const struct mig_subsystem host_priv_subsystem;

kern_return_t host_get_boot_info(host_priv_t host_priv,
    kernel_boot_info_t boot_info);
kern_return_t host_reboot(host_priv_t host_priv, int options);
kern_return_t host_processors(host_priv_t host_priv,
    processor_array_t *out_array, mach_msg_type_number_t *countp);
kern_return_t host_priv_statistics(host_priv_t host_priv,
    host_flavor_t flavor, host_info_t info, mach_msg_type_number_t *count);
kern_return_t host_processor_sets(host_priv_t host_priv,
    processor_set_name_array_t *pset_list, mach_msg_type_number_t *count);
kern_return_t host_processor_set_priv(host_priv_t host_priv,
    processor_set_t pset_name, processor_set_t *pset);
kern_return_t host_get_clock_control(host_priv_t host_priv,
    clock_id_t clock_id, clock_t *clock);
kern_return_t host_default_memory_manager(host_priv_t host_priv,
    memory_object_default_t *default_manager,
    memory_object_cluster_size_t cluster_size);
kern_return_t thread_wire(host_priv_t host_priv, thread_t thread,
    boolean_t wired);
kern_return_t kmod_create(host_priv_t host_priv, vm_address_t addr,
    kmod_t *id);
kern_return_t kmod_destroy(host_priv_t host_priv, kmod_t id);
kern_return_t kmod_control(host_priv_t host_priv, kmod_t id,
    kmod_control_flavor_t flavor, kmod_args_t *data,
    mach_msg_type_number_t *dataCount);

#endif /* _MACH_HOST_PRIV_SERVER_H_ */
