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
#ifndef _MACH_HOST_SECURITY_SERVER_H_
#define _MACH_HOST_SECURITY_SERVER_H_

#include <mach/host_security.h>
#include <mach/message.h>
#include <mach/mig.h>

extern const struct mig_subsystem host_security_subsystem;

kern_return_t host_security_create_task_token(
	host_security_t host_security,
	task_t parent_task,
	security_token_t sec_token,
	audit_token_t audit_token,
	host_priv_t host_priv,
	ledger_port_array_t ledger_ports,
	mach_msg_type_number_t num_ledger_ports,
	boolean_t inherit_memory,
	task_t *child_task);

kern_return_t host_security_set_task_token(
	host_security_t host_security,
	task_t task,
	security_token_t sec_token,
	audit_token_t audit_token,
	host_priv_t host_priv);

#endif /* _MACH_HOST_SECURITY_SERVER_H_ */
