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
#ifndef _MACH_MEMORY_OBJECT_SERVER_H_
#define _MACH_MEMORY_OBJECT_SERVER_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/memory_object_types.h>
#include <mach/vm_prot.h>

extern const struct mig_subsystem memory_object_subsystem;

kern_return_t memory_object_init(
    memory_object_t memory_object,
    memory_object_control_t memory_control,
    memory_object_cluster_size_t memory_object_page_size);
kern_return_t memory_object_terminate(
    memory_object_t memory_object);
kern_return_t memory_object_data_request(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_cluster_size_t length,
    vm_prot_t desired_access,
    memory_object_fault_info_t fault_info);
kern_return_t memory_object_data_return(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_cluster_size_t size,
    memory_object_offset_t *resid_offset,
    int *io_error,
    boolean_t dirty,
    boolean_t kernel_copy,
    int upl_flags);
kern_return_t memory_object_data_initialize(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_cluster_size_t size);
kern_return_t memory_object_data_unlock(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_size_t size,
    vm_prot_t desired_access);
kern_return_t memory_object_synchronize(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_size_t size,
    vm_sync_t sync_flags);
kern_return_t memory_object_map(
    memory_object_t memory_object,
    vm_prot_t prot);
kern_return_t memory_object_last_unmap(
    memory_object_t memory_object);
kern_return_t memory_object_data_reclaim(
    memory_object_t memory_object,
    boolean_t reclaim_backing_store);

#endif /* _MACH_MEMORY_OBJECT_SERVER_H_ */
