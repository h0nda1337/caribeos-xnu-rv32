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
#include <mach/mig.h>

/*
 * Stage0 RV32 MIG subsystem descriptors.
 *
 * The normal XNU build emits these from .defs files.  Until the RV32
 * generator path is wired in, provide inert subsystem tables so the kernel
 * object demux registry can link while the actual server routines remain
 * outside the early boot path.
 */

#define RV32_MIG_SUBSYSTEM(name, base) \
	const struct mig_subsystem name = { 0, (base), (base), 0, 0, {{0}} }

RV32_MIG_SUBSYSTEM(mach_host_subsystem, 200);
RV32_MIG_SUBSYSTEM(host_priv_subsystem, 400);
RV32_MIG_SUBSYSTEM(host_security_subsystem, 600);
RV32_MIG_SUBSYSTEM(clock_subsystem, 1000);
RV32_MIG_SUBSYSTEM(clock_priv_subsystem, 1200);
RV32_MIG_SUBSYSTEM(default_pager_object_subsystem, 2275);
RV32_MIG_SUBSYSTEM(memory_object_name_subsystem, 2600);
RV32_MIG_SUBSYSTEM(is_iokit_subsystem, 2800);
RV32_MIG_SUBSYSTEM(processor_subsystem, 3000);
RV32_MIG_SUBSYSTEM(mach_port_subsystem, 3200);
RV32_MIG_SUBSYSTEM(task_subsystem, 3400);
RV32_MIG_SUBSYSTEM(thread_act_subsystem, 3600);
RV32_MIG_SUBSYSTEM(vm32_map_subsystem, 3800);
RV32_MIG_SUBSYSTEM(processor_set_subsystem, 4000);
RV32_MIG_SUBSYSTEM(mach_vm_subsystem, 4800);
RV32_MIG_SUBSYSTEM(security_subsystem, 5200);
RV32_MIG_SUBSYSTEM(UNDReply_subsystem, 6200);
RV32_MIG_SUBSYSTEM(lock_set_subsystem, 617000);
