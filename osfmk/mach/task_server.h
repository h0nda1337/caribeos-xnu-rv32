#ifndef _MACH_TASK_SERVER_H_
#define _MACH_TASK_SERVER_H_

#include <mach/kern_return.h>
#include <mach/exception_types.h>
#include <mach/mach_types.h>
#include <mach/mig.h>
#include <mach/policy.h>
#include <mach/task_info.h>
#include <mach/task_policy.h>
#include <mach/thread_status.h>
#include <mach_debug/zone_info.h>

extern const struct mig_subsystem task_subsystem;

kern_return_t task_create(task_t parent_task, ledger_port_array_t ledger_ports,
    mach_msg_type_number_t num_ledger_ports, boolean_t inherit_memory,
    task_t *child_task);
kern_return_t thread_create_from_user(task_t task, thread_t *new_thread);
kern_return_t thread_create_running_from_user(task_t task, int flavor,
    thread_state_t new_state, mach_msg_type_number_t new_state_count,
    thread_t *new_thread);
kern_return_t task_terminate(task_t task);
kern_return_t task_threads(task_t task, thread_act_array_t *threads_out,
    mach_msg_type_number_t *count);
kern_return_t task_suspend(task_t task);
kern_return_t task_resume(task_t task);
kern_return_t mach_ports_register(task_t task, mach_port_array_t memory,
    mach_msg_type_number_t portsCnt);
kern_return_t mach_ports_lookup(task_t task, mach_port_array_t *portsp,
    mach_msg_type_number_t *portsCnt);
kern_return_t task_get_special_port(task_t task, int which_port,
    mach_port_t *special_port);
kern_return_t task_set_special_port(task_t task, int which_port,
    mach_port_t special_port);
kern_return_t task_set_info(task_t task, task_flavor_t flavor,
    task_info_t task_info_in, mach_msg_type_number_t task_info_count);
kern_return_t task_info(task_t task, task_flavor_t flavor,
    task_info_t task_info_out, mach_msg_type_number_t *task_info_count);
kern_return_t task_assign(task_t task, processor_set_t new_pset,
    boolean_t assign_threads);
kern_return_t task_assign_default(task_t task, boolean_t assign_threads);
kern_return_t task_get_assignment(task_t task, processor_set_t *pset);
kern_return_t task_policy(task_t task, policy_t policy_id,
    policy_base_t base, mach_msg_type_number_t count, boolean_t set_limit,
    boolean_t change);
kern_return_t task_policy_set(task_t task, task_policy_flavor_t flavor,
    task_policy_t policy_info, mach_msg_type_number_t count);
kern_return_t task_policy_get(task_t task, task_policy_flavor_t flavor,
    task_policy_t policy_info, mach_msg_type_number_t *count,
    boolean_t *get_default);
kern_return_t task_set_policy(task_t task, processor_set_t pset,
    policy_t policy_id, policy_base_t base,
    mach_msg_type_number_t base_count, policy_limit_t limit,
    mach_msg_type_number_t limit_count, boolean_t change);
kern_return_t task_set_emulation_vector(task_t task, int vector_start,
    emulation_vector_t emulation_vector,
    mach_msg_type_number_t emulation_vector_count);
kern_return_t task_get_emulation_vector(task_t task, int *vector_start,
    emulation_vector_t *emulation_vector,
    mach_msg_type_number_t *emulation_vector_count);
kern_return_t task_set_emulation(task_t task, vm_offset_t routine_entry_pt,
    int routine_number);
kern_return_t task_set_ras_pc(task_t task, vm_offset_t pc, vm_offset_t endpc);
kern_return_t task_set_state(task_t task, int flavor, thread_state_t state,
    mach_msg_type_number_t state_count);
kern_return_t task_get_state(task_t task, int flavor, thread_state_t state,
    mach_msg_type_number_t *state_count);
kern_return_t task_set_exception_ports(task_t task,
    exception_mask_t exception_mask, mach_port_t new_port,
    exception_behavior_t new_behavior, thread_state_flavor_t new_flavor);
kern_return_t task_swap_exception_ports(task_t task,
    exception_mask_t exception_mask, mach_port_t new_port,
    exception_behavior_t new_behavior, thread_state_flavor_t new_flavor,
    exception_mask_array_t masks, mach_msg_type_number_t *CountCnt,
    exception_port_array_t ports, exception_behavior_array_t behaviors,
    thread_state_flavor_array_t flavors);
kern_return_t task_get_exception_ports(task_t task,
    exception_mask_t exception_mask, exception_mask_array_t masks,
    mach_msg_type_number_t *CountCnt, exception_port_array_t ports,
    exception_behavior_array_t behaviors, thread_state_flavor_array_t flavors);
kern_return_t task_zone_info(task_t task,
    mach_zone_name_array_t *namesp, mach_msg_type_number_t *namesCntp,
    task_zone_info_array_t *infop, mach_msg_type_number_t *infoCntp);

#endif /* _MACH_TASK_SERVER_H_ */
