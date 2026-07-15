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
#ifndef _MACH_UPL_H_
#define _MACH_UPL_H_

#include <mach/memory_object_types.h>
#include <mach/vm_types.h>

__BEGIN_DECLS

kern_return_t upl_commit_range(
	upl_t upl,
	upl_offset_t offset,
	upl_size_t size,
	int flags,
	upl_page_info_t *page_list,
	mach_msg_type_number_t count,
	boolean_t *empty);

kern_return_t upl_commit(
	upl_t upl,
	upl_page_info_t *page_list,
	mach_msg_type_number_t count);

kern_return_t upl_abort_range(
	upl_t upl,
	upl_offset_t offset,
	upl_size_t size,
	int error,
	boolean_t *empty);

kern_return_t upl_abort(
	upl_t upl,
	int error);

__END_DECLS

#endif /* _MACH_UPL_H_ */
