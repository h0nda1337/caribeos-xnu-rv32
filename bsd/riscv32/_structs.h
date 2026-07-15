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
/*
 * Minimal BSD machine context structures for CaribeOS RV32.
 */

#ifndef _BSD_RISCV32_STRUCTS_H_
#define _BSD_RISCV32_STRUCTS_H_

#include <sys/appleapiopts.h>
#include <mach/riscv32/thread_status.h>

#ifdef __need_mcontext_t
#ifndef __need_struct_mcontext
#define __need_struct_mcontext
#endif /* __need_struct_mcontext */
#endif /* __need_mcontext_t */

#ifdef __need_struct_mcontext
#undef __need_struct_mcontext

#ifndef _STRUCT_MCONTEXT32
#if __DARWIN_UNIX03
#define _STRUCT_MCONTEXT32 struct __darwin_riscv32_mcontext
_STRUCT_MCONTEXT32
{
	riscv_thread_state_t    __ss;
	riscv_exception_state_t __es;
	riscv_csr_state_t       __cs;
};
#else /* !__DARWIN_UNIX03 */
#define _STRUCT_MCONTEXT32 struct mcontext32
_STRUCT_MCONTEXT32
{
	riscv_thread_state_t    ss;
	riscv_exception_state_t es;
	riscv_csr_state_t       cs;
};
#endif /* __DARWIN_UNIX03 */
#endif /* _STRUCT_MCONTEXT32 */

#ifndef _STRUCT_MCONTEXT
#define _STRUCT_MCONTEXT _STRUCT_MCONTEXT32
#endif /* _STRUCT_MCONTEXT */
#endif /* __need_struct_mcontext */

#ifdef __need_struct_mcontext64
#undef __need_struct_mcontext64

#ifndef _STRUCT_MCONTEXT64
#if __DARWIN_UNIX03
#define _STRUCT_MCONTEXT64 struct __darwin_riscv64_compat_mcontext
_STRUCT_MCONTEXT64
{
	riscv_thread_state_t    __ss;
	riscv_exception_state_t __es;
	riscv_csr_state_t       __cs;
};
#else /* !__DARWIN_UNIX03 */
#define _STRUCT_MCONTEXT64 struct mcontext64
_STRUCT_MCONTEXT64
{
	riscv_thread_state_t    ss;
	riscv_exception_state_t es;
	riscv_csr_state_t       cs;
};
#endif /* __DARWIN_UNIX03 */
#endif /* _STRUCT_MCONTEXT64 */
#endif /* __need_struct_mcontext64 */

#ifdef __need_mcontext_t
#undef __need_mcontext_t
#ifndef _MCONTEXT_T
#define _MCONTEXT_T
typedef _STRUCT_MCONTEXT32 *mcontext_t;
#endif /* _MCONTEXT_T */
#endif /* __need_mcontext_t */

#endif /* _BSD_RISCV32_STRUCTS_H_ */
