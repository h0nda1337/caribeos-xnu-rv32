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
#ifndef _RISCV32_MACHLIMITS_H_
#define _RISCV32_MACHLIMITS_H_

#define CHAR_BIT	8
#define SCHAR_MAX	127
#define SCHAR_MIN	(-128)
#define UCHAR_MAX	255U
#define CHAR_MAX	127
#define CHAR_MIN	(-128)
#define USHRT_MAX	65535U
#define SHRT_MAX	32767
#define SHRT_MIN	(-32768)
#define UINT_MAX	0xffffffffU
#define INT_MAX		2147483647
#define INT_MIN		(-2147483647 - 1)
#define ULONG_MAX	UINT_MAX
#define LONG_MAX	INT_MAX
#define LONG_MIN	INT_MIN
#define MB_LEN_MAX	4

#endif /* _RISCV32_MACHLIMITS_H_ */
