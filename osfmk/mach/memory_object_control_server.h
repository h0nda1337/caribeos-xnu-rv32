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
#ifndef _MACH_MEMORY_OBJECT_CONTROL_SERVER_H_
#define _MACH_MEMORY_OBJECT_CONTROL_SERVER_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/memory_object_types.h>
#include <mach/vm_prot.h>

extern const struct mig_subsystem memory_object_control_subsystem;

kern_return_t memory_object_get_attributes(
    memory_object_control_t control,
    memory_object_flavor_t flavor,
    memory_object_info_t attributes,
    mach_msg_type_number_t *count);
kern_return_t memory_object_change_attributes(
    memory_object_control_t control,
    memory_object_flavor_t flavor,
    memory_object_info_t attributes,
    mach_msg_type_number_t count);
kern_return_t memory_object_synchronize_completed(
    memory_object_control_t control,
    memory_object_offset_t offset,
    memory_object_size_t length);
kern_return_t memory_object_lock_request(
    memory_object_control_t control,
    memory_object_offset_t offset,
    memory_object_size_t size,
    memory_object_offset_t *resid_offset,
    int *io_errno,
    memory_object_return_t should_return,
    int flags,
    vm_prot_t prot);
kern_return_t memory_object_destroy(
    memory_object_control_t control,
    kern_return_t reason);
kern_return_t memory_object_upl_request(
    memory_object_control_t control,
    memory_object_offset_t offset,
    upl_size_t size,
    upl_t *upl_ptr,
    upl_page_info_array_t user_page_list,
    unsigned int *page_list_count,
    int cntrl_flags);
kern_return_t memory_object_super_upl_request(
    memory_object_control_t control,
    memory_object_offset_t offset,
    upl_size_t size,
    upl_size_t super_cluster,
    upl_t *upl,
    upl_page_info_t *user_page_list,
    unsigned int *page_list_count,
    int cntrl_flags);
kern_return_t memory_object_cluster_size(
    memory_object_control_t control,
    memory_object_offset_t *start,
    vm_size_t *length,
    uint32_t *io_streaming,
    memory_object_fault_info_t fault_info);
kern_return_t memory_object_page_op(
    memory_object_control_t control,
    memory_object_offset_t offset,
    int ops,
    ppnum_t *phys_entry,
    int *flags);
kern_return_t memory_object_release_name(
    memory_object_control_t control,
    int flags);
kern_return_t memory_object_range_op(
    memory_object_control_t control,
    memory_object_offset_t offset_beg,
    memory_object_offset_t offset_end,
    int ops,
    int *range);

#endif /* _MACH_MEMORY_OBJECT_CONTROL_SERVER_H_ */
