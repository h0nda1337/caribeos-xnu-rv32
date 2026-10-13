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
 * port of Apple XNU 2050.48.11 on 2026-10-12.
 */

#ifndef CARIBE_GRAPHICS_ABI_H
#define CARIBE_GRAPHICS_ABI_H
#include <stdint.h>
#define CARIBE_GFX_ABI 0x00010000U
#define CARIBE_GFX_GET_MODE 0x80184301U
#define CARIBE_GFX_BLIT 0x40204302U
#define CARIBE_GFX_PRESENT 0x00004303U
#define CARIBE_GFX_GET_EVENT 0x80104304U
#define CARIBE_GFX_GET_STATS 0x80104305U
#define CARIBE_GFX_FORMAT_XRGB8888 1U
struct caribe_gfx_mode {
  uint32_t abi, width, height, stride, format, capabilities;
};
struct caribe_gfx_blit {
  uint32_t abi, x, y, width, height, stride, pixels, reserved;
};
struct caribe_gfx_event {
  uint32_t device;
  uint16_t type, code;
  int32_t value;
  uint32_t sequence;
};
struct caribe_gfx_stats {
  uint32_t presents, blits, events, dropped;
};
typedef char
    caribe_gfx_mode_abi_size[(sizeof(struct caribe_gfx_mode) == 24) ? 1 : -1];
typedef char
    caribe_gfx_blit_abi_size[(sizeof(struct caribe_gfx_blit) == 32) ? 1 : -1];
typedef char
    caribe_gfx_event_abi_size[(sizeof(struct caribe_gfx_event) == 16) ? 1 : -1];
typedef char
    caribe_gfx_stats_abi_size[(sizeof(struct caribe_gfx_stats) == 16) ? 1 : -1];
#endif

#ifdef KERNEL
int riscv32_graphics_available(void);
int riscv32_graphics_ioctl(uint32_t request, uint32_t user_arg);
#endif
