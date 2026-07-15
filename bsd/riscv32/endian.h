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
#ifndef _BSD_RISCV32_ENDIAN_H_
#define _BSD_RISCV32_ENDIAN_H_

#include <sys/cdefs.h>

#define _QUAD_HIGHWORD	1
#define _QUAD_LOWWORD	0

#define __DARWIN_LITTLE_ENDIAN	1234
#define __DARWIN_BIG_ENDIAN	4321
#define __DARWIN_PDP_ENDIAN	3412

#define __DARWIN_BYTE_ORDER	__DARWIN_LITTLE_ENDIAN

#if defined(KERNEL) || (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))

#define LITTLE_ENDIAN	__DARWIN_LITTLE_ENDIAN
#define BIG_ENDIAN	__DARWIN_BIG_ENDIAN
#define PDP_ENDIAN	__DARWIN_PDP_ENDIAN
#define BYTE_ORDER	__DARWIN_BYTE_ORDER

#include <sys/_endian.h>

#endif

#endif /* _BSD_RISCV32_ENDIAN_H_ */
