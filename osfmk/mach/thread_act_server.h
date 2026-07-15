#ifndef _MACH_THREAD_ACT_SERVER_H_
#define _MACH_THREAD_ACT_SERVER_H_

#include <mach/thread_act.h>
#include <mach/exception_types.h>
#include <mach/mig.h>
#include <mach/thread_policy.h>

extern const struct mig_subsystem thread_act_subsystem;

kern_return_t thread_get_special_port(thread_t thread, int which,
    mach_port_t *portp);
kern_return_t thread_set_special_port(thread_t thread, int which,
    mach_port_t port);
kern_return_t thread_set_exception_ports(thread_t thread,
    exception_mask_t exception_mask, mach_port_t new_port,
    exception_behavior_t new_behavior, thread_state_flavor_t new_flavor);
kern_return_t thread_swap_exception_ports(thread_t thread,
    exception_mask_t exception_mask, mach_port_t new_port,
    exception_behavior_t new_behavior, thread_state_flavor_t new_flavor,
    exception_mask_array_t masks, mach_msg_type_number_t *CountCnt,
    exception_port_array_t ports, exception_behavior_array_t behaviors,
    thread_state_flavor_array_t flavors);
kern_return_t thread_get_exception_ports(thread_t thread,
    exception_mask_t exception_mask, exception_mask_array_t masks,
    mach_msg_type_number_t *CountCnt, exception_port_array_t ports,
    exception_behavior_array_t behaviors, thread_state_flavor_array_t flavors);
kern_return_t thread_suspend(thread_t thread);
kern_return_t thread_resume(thread_t thread);
kern_return_t thread_depress_abort(thread_t thread);
kern_return_t thread_abort(thread_t thread);
kern_return_t thread_abort_safely(thread_t thread);
kern_return_t thread_info(thread_t thread, thread_flavor_t flavor,
    thread_info_t thread_info_out, mach_msg_type_number_t *thread_info_count);
kern_return_t thread_get_state(thread_t thread, int flavor,
    thread_state_t state, mach_msg_type_number_t *state_count);
kern_return_t thread_set_state_from_user(thread_t thread, int flavor,
    thread_state_t state, mach_msg_type_number_t state_count);
kern_return_t thread_assign(thread_t thread, processor_set_t new_pset);
kern_return_t thread_assign_default(thread_t thread);
kern_return_t thread_get_assignment(thread_t thread, processor_set_t *pset);
kern_return_t thread_policy_set(thread_t thread,
    thread_policy_flavor_t flavor, thread_policy_t policy_info,
    mach_msg_type_number_t count);
kern_return_t thread_policy_get(thread_t thread,
    thread_policy_flavor_t flavor, thread_policy_t policy_info,
    mach_msg_type_number_t *count, boolean_t *get_default);
kern_return_t act_set_state_from_user(thread_t thread, int flavor,
    thread_state_t state, mach_msg_type_number_t count);
kern_return_t act_get_state(thread_t thread, int flavor,
    thread_state_t state, mach_msg_type_number_t *count);

#endif /* _MACH_THREAD_ACT_SERVER_H_ */
