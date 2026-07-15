/*
 * SPDX-License-Identifier: APSL-2.0
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * https://opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 * CARIBEOS MODIFICATION NOTICE
 * This file was created as part of the experimental CaribeOS RV32IMACSU
 * port of Apple XNU 2050.48.11 on 2026-07-14.
 */

#line 1
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
