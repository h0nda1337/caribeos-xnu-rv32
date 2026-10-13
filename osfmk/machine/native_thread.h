/* SPDX-License-Identifier: APSL-2.0
 * CARIBEOS MODIFICATION NOTICE: h0nda1337, 2026-10-13.
 * Architecture forwarding header; see APPLE_LICENSE and the target header.
 */
#ifndef _MACHINE_NATIVE_THREAD_H_
#define _MACHINE_NATIVE_THREAD_H_
#if defined(__RISCV32__)
#include <riscv32/native_thread.h>
#else
#error native user-thread interface is available only in the CaribeOS RV32 port
#endif
#endif
