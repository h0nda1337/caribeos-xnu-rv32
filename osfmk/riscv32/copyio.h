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
#ifndef _RISCV32_COPYIO_H_
#define _RISCV32_COPYIO_H_

#include <mach/boolean.h>
#include <stdint.h>

#define RISCV32_COPYIO_EFAULT	14
#define RISCV32_COPYIO_ENAMETOOLONG 63

struct pmap;

boolean_t riscv32_user_range_valid(uint32_t user_addr, uint32_t len);
void riscv32_copyio_set_user_pmap(struct pmap *pmap);
int riscv32_copyin(uint32_t user_addr, void *kernel_addr, uint32_t len);
int riscv32_copyout(const void *kernel_addr, uint32_t user_addr, uint32_t len);
int copyoutstr(const void *kernel_addr, uint64_t user_addr, uint32_t max,
    uint32_t *actual);

#endif /* _RISCV32_COPYIO_H_ */
