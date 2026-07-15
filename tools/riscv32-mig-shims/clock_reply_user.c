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
 * port of Apple XNU 2050.48.11 on 2026-07-15.
 */

#line 1
#include <mach/clock_reply.h>

/*
 * Stage0 RISC-V build shim.
 *
 * The full XNU build generates this client stub from clock_reply.defs.
 * RV32 stage0 only needs the symbol so deprecated clock alarm code can link;
 * real IPC delivery will replace this once MIG generation is wired in.
 */

kern_return_t
clock_alarm_reply(
	mach_port_t alarm_port,
	mach_msg_type_name_t alarm_port_type,
	kern_return_t alarm_code,
	alarm_type_t alarm_type,
	mach_timespec_t alarm_time)
{
	(void)alarm_port;
	(void)alarm_port_type;
	(void)alarm_code;
	(void)alarm_type;
	(void)alarm_time;
	return KERN_SUCCESS;
}
