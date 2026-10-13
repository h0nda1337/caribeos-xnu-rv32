/*
 * SPDX-License-Identifier: APSL-2.0
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * Please obtain a copy of the License at
 * https://opensource.apple.com/apsl/ and read it before using this file.
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 * CARIBEOS MODIFICATION NOTICE
 * Created by h0nda1337, 2026-10-13: native counting semaphores for RV32.
 */
#ifndef CARIBE_NATIVE_SYNC_H
#define CARIBE_NATIVE_SYNC_H
#include <stdint.h>
#define CARIBE_SYNC_ABI 0x00010000U
#define CARIBE_SYNC_CALL 0xc0805301U
#define CARIBE_SYNC_CREATE 1U
#define CARIBE_SYNC_DELETE 2U
#define CARIBE_SYNC_ACQUIRE 3U
#define CARIBE_SYNC_RELEASE 4U
#define CARIBE_SYNC_INFO 5U
#define CARIBE_SYNC_STATS 6U
#define CARIBE_SYNC_WAIT 1U
#define CARIBE_SYNC_GLOBAL 2U
#define CARIBE_SYNC_MAX_OBJECTS 64U
#define CARIBE_SYNC_MAX_VALUE 0x7fffffffU
struct caribe_sync_request {
    uint32_t abi, operation, id, units, value, flags, owner, holder;
    uint32_t timeout_low, timeout_high, live, waits, wakes, timeouts, acquires, releases;
    char name[64];
};
typedef char caribe_sync_abi_size[(sizeof(struct caribe_sync_request) == 128) ? 1 : -1];
#ifdef KERNEL
int riscv32_native_sync_ioctl(uint32_t request, uint32_t user_arg);
void riscv32_native_sync_release_owner(uint32_t pid);
#endif
#endif
