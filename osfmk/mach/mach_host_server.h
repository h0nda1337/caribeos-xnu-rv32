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
#ifndef _MACH_MACH_HOST_SERVER_H_
#define _MACH_MACH_HOST_SERVER_H_

#include <mach/clock_types.h>
#include <mach/kern_return.h>
#include <mach/host_info.h>
#include <mach/mach_types.h>
#include <mach/mig.h>
#include <mach/processor_info.h>
#include <mach_debug/lockgroup_info.h>
#include <mach_debug/zone_info.h>
#include <kern/kern_types.h>

extern const struct mig_subsystem mach_host_subsystem;

kern_return_t host_info(host_t host, host_flavor_t flavor, host_info_t info,
    mach_msg_type_number_t *count);
kern_return_t host_statistics(host_t host, host_flavor_t flavor,
    host_info_t info, mach_msg_type_number_t *count);
kern_return_t host_statistics64(host_t host, host_flavor_t flavor,
    host_info64_t info, mach_msg_type_number_t *count);
kern_return_t host_page_size(host_t host, vm_size_t *out_page_size);
kern_return_t host_kernel_version(host_t host, kernel_version_t out_version);
kern_return_t host_processor_info(host_t host, processor_flavor_t flavor,
    natural_t *out_pcount, processor_info_array_t *out_array,
    mach_msg_type_number_t *out_array_count);
kern_return_t host_get_io_master(host_t host, io_master_t *io_masterp);
kern_return_t mach_zone_info(host_priv_t host,
    mach_zone_name_array_t *namesp, mach_msg_type_number_t *namesCntp,
    mach_zone_info_array_t *infop, mach_msg_type_number_t *infoCntp);
kern_return_t host_zone_info(host_priv_t host, zone_name_array_t *namesp,
    mach_msg_type_number_t *namesCntp, zone_info_array_t *infop,
    mach_msg_type_number_t *infoCntp);
kern_return_t host_lockgroup_info(host_t host,
    lockgroup_info_array_t *lockgroup_infop,
    mach_msg_type_number_t *lockgroup_infoCntp);
kern_return_t host_get_clock_service(host_t host, clock_id_t clock_id,
    clock_t *clock);
kern_return_t mach_zone_force_gc(host_t host);
kern_return_t host_get_special_port(host_priv_t host_priv, int node,
    int id, mach_port_t *portp);
kern_return_t host_set_special_port(host_priv_t host_priv, int id,
    mach_port_t port);

#endif /* _MACH_MACH_HOST_SERVER_H_ */
