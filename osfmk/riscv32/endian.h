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
#ifndef _RISCV32_ENDIAN_H_
#define _RISCV32_ENDIAN_H_

#define LITTLE_ENDIAN	1234
#define BIG_ENDIAN	4321
#define PDP_ENDIAN	3412

#define BYTE_ORDER	LITTLE_ENDIAN
#define ENDIAN		LITTLE

#if !defined(ntohs)
static __inline__ unsigned short
ntohs(unsigned short value)
{
	return (unsigned short)((value << 8) | (value >> 8));
}
#endif

#if !defined(htons)
unsigned short htons(unsigned short);
#define htons ntohs
#endif

#if !defined(ntohl)
static __inline__ unsigned long
ntohl(unsigned long value)
{
	return ((value & 0x000000ffUL) << 24) |
	       ((value & 0x0000ff00UL) << 8) |
	       ((value & 0x00ff0000UL) >> 8) |
	       ((value & 0xff000000UL) >> 24);
}
#endif

#if !defined(htonl)
unsigned long htonl(unsigned long);
#define htonl ntohl
#endif

#define NTOHL(x)	(x) = ntohl((unsigned long)(x))
#define NTOHS(x)	(x) = ntohs((unsigned short)(x))
#define HTONL(x)	(x) = htonl((unsigned long)(x))
#define HTONS(x)	(x) = htons((unsigned short)(x))

#endif /* _RISCV32_ENDIAN_H_ */
