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
#include <mach/exc.h>

kern_return_t
exception_raise(mach_port_t exception_port, mach_port_t thread,
    mach_port_t task, exception_type_t exception, exception_data_t code,
    mach_msg_type_number_t codeCnt)
{
	(void)exception_port;
	(void)thread;
	(void)task;
	(void)exception;
	(void)code;
	(void)codeCnt;
	return KERN_FAILURE;
}

kern_return_t
exception_raise_state(mach_port_t exception_port, exception_type_t exception,
    exception_data_t code, mach_msg_type_number_t codeCnt, int *flavor,
    thread_state_t old_state, mach_msg_type_number_t old_stateCnt,
    thread_state_t new_state, mach_msg_type_number_t *new_stateCnt)
{
	(void)exception_port;
	(void)exception;
	(void)code;
	(void)codeCnt;
	(void)flavor;
	(void)old_state;
	(void)old_stateCnt;
	(void)new_state;
	(void)new_stateCnt;
	return KERN_FAILURE;
}

kern_return_t
exception_raise_state_identity(mach_port_t exception_port, mach_port_t thread,
    mach_port_t task, exception_type_t exception, exception_data_t code,
    mach_msg_type_number_t codeCnt, int *flavor, thread_state_t old_state,
    mach_msg_type_number_t old_stateCnt, thread_state_t new_state,
    mach_msg_type_number_t *new_stateCnt)
{
	(void)exception_port;
	(void)thread;
	(void)task;
	(void)exception;
	(void)code;
	(void)codeCnt;
	(void)flavor;
	(void)old_state;
	(void)old_stateCnt;
	(void)new_state;
	(void)new_stateCnt;
	return KERN_FAILURE;
}
