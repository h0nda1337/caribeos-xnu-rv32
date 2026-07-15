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
#ifndef _MACH_CLOCK_SERVER_H_
#define _MACH_CLOCK_SERVER_H_

#include <mach/clock_types.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig.h>
#ifdef KERNEL_PRIVATE
#include <kern/kern_types.h>
#endif

extern const struct mig_subsystem clock_subsystem;

kern_return_t clock_get_time(clock_t clock, mach_timespec_t *cur_time);
kern_return_t clock_get_attributes(clock_t clock, clock_flavor_t flavor,
    clock_attr_t attr, mach_msg_type_number_t *count);
kern_return_t clock_alarm(clock_t clock, alarm_type_t alarm_type,
    mach_timespec_t alarm_time, ipc_port_t alarm_port,
    mach_msg_type_name_t alarm_port_type);

#endif /* _MACH_CLOCK_SERVER_H_ */
