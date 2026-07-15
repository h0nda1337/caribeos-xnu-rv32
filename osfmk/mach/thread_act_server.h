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
