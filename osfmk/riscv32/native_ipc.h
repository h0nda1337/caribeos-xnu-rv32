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
 * Created for the experimental CaribeOS XNU RV32 port on 2026-10-12.
 * Native message ports; no Wayland, X11 or Linux kernel dependency.
 */
#ifndef CARIBE_NATIVE_IPC_H
#define CARIBE_NATIVE_IPC_H
#include <stdint.h>
#define CARIBE_IPC_ABI 0x00010000U
#define CARIBE_IPC_CALL 0xc0804901U
#define CARIBE_IPC_CREATE 1U
#define CARIBE_IPC_DELETE 2U
#define CARIBE_IPC_SEND 3U
#define CARIBE_IPC_RECEIVE 4U
#define CARIBE_IPC_PEEK 5U
#define CARIBE_IPC_FIND 6U
#define CARIBE_IPC_PUBLIC 7U
#define CARIBE_IPC_INFO 8U
#define CARIBE_IPC_STATS 9U
#define CARIBE_IPC_SET_SERVER 10U
#define CARIBE_IPC_GET_SERVER 11U
#define CARIBE_IPC_ENUM 12U
#define CARIBE_IPC_WAIT 1U
#define CARIBE_IPC_MAX_PORTS 32U
#define CARIBE_IPC_MAX_MESSAGES 64U
#define CARIBE_IPC_MAX_PAYLOAD 8192U
#define CARIBE_IPC_MAX_QUEUE 32U
struct caribe_ipc_request {
    uint32_t abi, operation, id, code, buffer, size, limit, flags;
    uint32_t owner, queued, ports, messages, sends, receives;
    uint32_t timeout_low, timeout_high;
    char name[64];
};
typedef char caribe_ipc_abi_size[(sizeof(struct caribe_ipc_request) == 128) ? 1 : -1];
#ifdef KERNEL
int riscv32_native_ipc_ioctl(uint32_t request, uint32_t user_arg);
void riscv32_native_ipc_release_owner(uint32_t pid);
#endif
#endif
