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
