/* SPDX-License-Identifier: APSL-2.0 */
/* CaribeOS modification: native RV32 semaphore header forwarding, 2026-10-13. */
#ifndef _MACHINE_NATIVE_SYNC_H_
#define _MACHINE_NATIVE_SYNC_H_
#if defined(__riscv) || defined(__riscv32__)
#include <riscv32/native_sync.h>
#endif
#endif
