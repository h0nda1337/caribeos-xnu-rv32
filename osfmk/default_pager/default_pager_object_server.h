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
#ifndef _DEFAULT_PAGER_OBJECT_SERVER_H_
#define _DEFAULT_PAGER_OBJECT_SERVER_H_

#include <mach/kern_return.h>
#include <mach/mig.h>
#include <mach/port.h>
#include <default_pager/default_pager_types.h>

extern const struct mig_subsystem default_pager_object_subsystem;

kern_return_t default_pager_triggers(
	mach_port_t default_pager,
	int hi_wat,
	int lo_wat,
	int flags,
	mach_port_t trigger_port);
kern_return_t default_pager_info(
	memory_object_default_t pager,
	default_pager_info_t *infop);
kern_return_t default_pager_info_64(
	memory_object_default_t pager,
	default_pager_info_64_t *infop);
kern_return_t default_pager_object_create(
	default_pager_t default_pager,
	vm_size_t size,
	memory_object_t *mem_objp);
kern_return_t default_pager_objects(
	default_pager_t default_pager,
	default_pager_object_array_t *objectsp,
	mach_msg_type_number_t *ocountp,
	mach_port_array_t *portsp,
	mach_msg_type_number_t *pcountp);
kern_return_t default_pager_object_pages(
	default_pager_t default_pager,
	mach_port_t memory_object,
	default_pager_page_array_t *pagesp,
	mach_msg_type_number_t *countp);
kern_return_t default_pager_backing_store_create(
	memory_object_default_t pager,
	int priority,
	int clsize,
	mach_port_t *backing_store);
kern_return_t default_pager_backing_store_delete(
	mach_port_t backing_store);
kern_return_t default_pager_backing_store_info(
	mach_port_t backing_store,
	backing_store_flavor_t flavor,
	backing_store_info_t info,
	mach_msg_type_number_t *size);
kern_return_t default_pager_add_file(
	mach_port_t backing_store,
	vnode_ptr_t vp,
	int record_size,
	vm_size_t size);

#endif /* _DEFAULT_PAGER_OBJECT_SERVER_H_ */
