/*
 * SPDX-License-Identifier: APSL-2.0
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
 * CARIBEOS MODIFICATION NOTICE
 * Created by h0nda1337, 2026-10-13. Opt-in private futex WAIT/WAKE only.
 */
#include <machine/native_thread.h>
#include <machine/bootinfo.h>
#include <machine/copyio.h>
#include <machine/cpu_data.h>
#include <machine/linux_process.h>
#include <machine/linux_signal.h>
#include <machine/pmap.h>
#include <kern/clock.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <mach/mach_time.h>
#include <vm/pmap.h>

#define FUTEX_PRIVATE 128U
#define FUTEX_WAIT 0U
#define FUTEX_WAKE 1U
#define FUTEX_SLOTS 64U
typedef struct {
    pmap_t pmap;
    uint32_t address, notified;
    thread_t thread;
} private_waiter_t;
static private_waiter_t waiters[FUTEX_SLOTS];
static decl_simple_lock_data(, futex_lock);
/* RV32 simple locks are atomic but do not suppress same-hart kernel
   preemption. Never switch to a competing user thread while owning this lock. */
static void futex_lock_queue(void)
{
    disable_preemption();
    simple_lock(&futex_lock);
}
static void futex_unlock_queue(void)
{
    simple_unlock(&futex_lock);
    enable_preemption();
}
static volatile uint32_t initialized;

static void futex_init(void)
{
    if (__sync_fetch_and_add(&initialized, 0U) == 2U) return;
    disable_preemption();
    if (__sync_bool_compare_and_swap(&initialized, 0U, 1U)) {
        simple_lock_init(&futex_lock, 0);
        __sync_synchronize(); initialized = 2U;
    } else while (__sync_fetch_and_add(&initialized, 0U) != 2U) __asm__ volatile("nop");
    enable_preemption();
}
static int futex_finish(riscv_saved_state_t *state, int value)
{
    state->thread.x[RISCV_X_A0] = (uint32_t)value;
    state->thread.x[RISCV_X_A1] = value < 0 ? (uint32_t)-value : 0;
    return TRUE;
}
uint32_t riscv32_native_futex_owner_waiters(struct pmap *pmap)
{
    uint32_t i, count = 0;
    if (__sync_fetch_and_add(&initialized, 0U) != 2U) return 0;
    futex_lock_queue();
    for (i = 0; i < FUTEX_SLOTS; i++) count += waiters[i].pmap == pmap;
    futex_unlock_queue();
    return count;
}
boolean_t riscv32_native_futex_dispatch(riscv_saved_state_t *state, boolean_t time64)
{
    riscv32_linux_process_t *process;
    private_waiter_t *waiter = 0;
    uint32_t address = state->thread.x[RISCV_X_A0], op = state->thread.x[RISCV_X_A1];
    uint32_t value = state->thread.x[RISCV_X_A2], timeout = state->thread.x[RISCV_X_A3];
    uint32_t i, current, count = 0;
    uint64_t deadline = 0, interval, now;
    vm_offset_t kva;
    int result = 0;
    if (!riscv32_boot_arg_has("native-futex")) return FALSE;
    process = riscv32_linux_process_current();
    if (!process) return futex_finish(state, -1);
    /* Do not silently treat shared, PI, requeue or bitset futexes as private. */
    if (op != (FUTEX_PRIVATE | FUTEX_WAIT) && op != (FUTEX_PRIVATE | FUTEX_WAKE))
        return futex_finish(state, -38);
    if (address & 3U) return futex_finish(state, -22);
    if (!riscv32_pmap_user_range_accessible(process->pmap, address, 4U, FALSE))
        return futex_finish(state, -14);
    kva = riscv32_phys_to_kva(pmap_extract(process->pmap, address));
    if (!kva) return futex_finish(state, -14);
    if (op == (FUTEX_PRIVATE | FUTEX_WAIT) && timeout) {
        int64_t seconds, nanos;
        if (time64) {
            int64_t timespec[2];
            if (riscv32_copyin(timeout, timespec, sizeof(timespec))) return futex_finish(state, -14);
            seconds = timespec[0]; nanos = timespec[1];
        } else {
            int32_t timespec[2];
            if (riscv32_copyin(timeout, timespec, sizeof(timespec))) return futex_finish(state, -14);
            seconds = timespec[0]; nanos = timespec[1];
        }
        if (seconds < 0 || nanos < 0 || nanos >= 1000000000LL) return futex_finish(state, -22);
        /* Saturate a very long valid relative timeout, never wrap it into the past. */
        if ((uint64_t)seconds > (UINT64_MAX - (uint64_t)nanos) / 1000000000ULL)
            interval = UINT64_MAX;
        else nanoseconds_to_absolutetime((uint64_t)seconds * 1000000000ULL + (uint64_t)nanos, &interval);
        now = mach_absolute_time();
        deadline = interval > UINT64_MAX - now ? UINT64_MAX : now + interval;
    }
    futex_init();
    futex_lock_queue();
    if (op == (FUTEX_PRIVATE | FUTEX_WAKE)) {
        if ((int32_t)value < 0) result = -22;
        else for (i = 0; i < FUTEX_SLOTS && count < value; i++) {
            private_waiter_t *other = &waiters[i];
            if (other->pmap == process->pmap && other->address == address && !other->notified) {
                other->notified = 1;
                count++;
                thread_wakeup((event_t)other);
            }
        }
        futex_unlock_queue();
        return futex_finish(state, result ? result : (int)count);
    }
    /* An aligned RV32A load and waiter publication share the wake lock. The
       waiter is armed before unlocking, including the wake-before-block race. */
    current = __atomic_load_n((uint32_t *)(uintptr_t)kva, __ATOMIC_SEQ_CST);
    if (current != value) result = -11;
    else if (timeout && mach_absolute_time() >= deadline) result = -110;
    else {
        for (i = 0; i < FUTEX_SLOTS; i++) if (!waiters[i].pmap) { waiter = &waiters[i]; break; }
        if (!waiter) result = -11;
    }
    if (result) { futex_unlock_queue(); return futex_finish(state, result); }
    waiter->pmap = process->pmap; waiter->address = address;
    waiter->thread = current_thread(); waiter->notified = 0;
    for (;;) {
        uint64_t poll_deadline;
        if (waiter->notified) break;
        if (!current_thread()->active || (!riscv32_native_thread_current() &&
            riscv32_linux_signal_has_deliverable(process))) { result = -4; break; }
        now = mach_absolute_time();
        if (timeout && now >= deadline) { result = -110; break; }
        nanoseconds_to_absolutetime(20000000ULL, &interval);
        poll_deadline = now + interval;
        if (timeout && deadline < poll_deadline) poll_deadline = deadline;
        assert_wait_deadline((event_t)waiter, THREAD_UNINT, poll_deadline);
        futex_unlock_queue();
        thread_block(THREAD_CONTINUE_NULL);
        futex_lock_queue();
    }
    waiter->pmap = PMAP_NULL; waiter->thread = THREAD_NULL;
    waiter->address = waiter->notified = 0;
    futex_unlock_queue();
    return futex_finish(state, result);
}
