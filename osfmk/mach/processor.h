#ifndef _MACH_PROCESSOR_H_
#define _MACH_PROCESSOR_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/processor_info.h>

kern_return_t processor_info(processor_t processor,
    processor_flavor_t flavor, host_t *host, processor_info_t info,
    mach_msg_type_number_t *count);
kern_return_t processor_start(processor_t processor);
kern_return_t processor_exit(processor_t processor);
kern_return_t processor_control(processor_t processor, processor_info_t info,
    mach_msg_type_number_t count);
kern_return_t processor_get_assignment(processor_t processor,
    processor_set_t *pset);
kern_return_t processor_set_create(host_t host, processor_set_t *new_set,
    processor_set_t *new_name);
kern_return_t processor_set_destroy(processor_set_t pset);
kern_return_t processor_set_info(processor_set_t pset, int flavor,
    host_t *host, processor_set_info_t info, mach_msg_type_number_t *count);
kern_return_t processor_set_statistics(processor_set_t pset, int flavor,
    processor_set_info_t info, mach_msg_type_number_t *count);
kern_return_t processor_set_max_priority(processor_set_t pset,
    int max_priority, boolean_t change_threads);
kern_return_t processor_set_policy_enable(processor_set_t pset, int policy);
kern_return_t processor_set_policy_disable(processor_set_t pset, int policy,
    boolean_t change_threads);
kern_return_t processor_set_tasks(processor_set_t pset,
    task_array_t *task_list, mach_msg_type_number_t *count);
kern_return_t processor_set_threads(processor_set_t pset,
    thread_act_array_t *thread_list, mach_msg_type_number_t *count);
kern_return_t processor_set_policy_control(processor_set_t pset, int flavor,
    processor_set_info_t policy_info, mach_msg_type_number_t count,
    boolean_t change);

#endif /* _MACH_PROCESSOR_H_ */
