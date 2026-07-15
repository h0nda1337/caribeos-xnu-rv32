#ifndef _MACH_THREAD_ACT_H_
#define _MACH_THREAD_ACT_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/policy.h>
#include <mach/thread_policy.h>
#include <mach/thread_status.h>

kern_return_t thread_terminate(thread_act_t target_thread);
kern_return_t thread_policy_set(thread_t thread,
    thread_policy_flavor_t flavor, thread_policy_t policy_info,
    mach_msg_type_number_t count);
kern_return_t thread_policy_get(thread_t thread,
    thread_policy_flavor_t flavor, thread_policy_t policy_info,
    mach_msg_type_number_t *count, boolean_t *get_default);
kern_return_t thread_set_policy(thread_t thread, processor_set_t pset,
    policy_t policy, policy_base_t base, mach_msg_type_number_t base_count,
    policy_limit_t limit, mach_msg_type_number_t limit_count);
kern_return_t thread_policy(thread_t thread, policy_t policy,
    policy_base_t base, mach_msg_type_number_t count, boolean_t set_limit);

#endif /* _MACH_THREAD_ACT_H_ */
