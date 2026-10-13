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
 * Created by h0nda1337, 2026-10-13. Counting objects use real XNU scheduler
 * waits and RV32A ordering. This does not implement native user threads.
 */
#include <machine/native_sync.h>
#include <machine/native_thread.h>
#include <machine/copyio.h>
#include <machine/cpu_data.h>
#include <machine/linux_process.h>
#include <machine/linux_signal.h>
#include <kern/clock.h>
#include <kern/misc_protos.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <mach/mach_time.h>
#include <string.h>

/* Linux-personality errno numbers, not BSD errno numbers. */
#define SYNC_PERM 1
#define SYNC_INTR 4
#define SYNC_AGAIN 11
#define SYNC_FAULT 14
#define SYNC_INVAL 22
#define SYNC_NOTTY 25
#define SYNC_NOSPC 28
#define SYNC_REMOVED 43
#define SYNC_OVERFLOW 75
#define SYNC_TIMEDOUT 110
typedef struct {
    uint32_t owner, generation, value, global, holder, sequence;
    char name[64];
} native_semaphore_t;
static native_semaphore_t objects[CARIBE_SYNC_MAX_OBJECTS];
static decl_simple_lock_data(, sync_lock);
/* RV32 simple locks are atomic but do not suppress same-hart kernel
   preemption. Never switch to a competing user thread while owning this lock. */
static void sync_lock_queue(void)
{
    disable_preemption();
    simple_lock(&sync_lock);
}
static void sync_unlock_queue(void)
{
    simple_unlock(&sync_lock);
    enable_preemption();
}
static volatile uint32_t initialized;
static uint32_t waits, wakes, timeouts, acquires, releases;

static void sync_init(void)
{
    if (__sync_fetch_and_add(&initialized, 0U) == 2U) return;
    disable_preemption();
    if (__sync_bool_compare_and_swap(&initialized, 0U, 1U)) {
        simple_lock_init(&sync_lock, 0);
        __sync_synchronize();
        initialized = 2U;
    } else {
        while (__sync_fetch_and_add(&initialized, 0U) != 2U)
            __asm__ volatile("nop");
    }
    enable_preemption();
}
static uint32_t object_id(uint32_t slot)
{
    return (objects[slot].generation << 7) | (slot + 1U);
}
static native_semaphore_t *get_object(uint32_t id)
{
    uint32_t slot = id & 127U;
    if (slot == 0U || slot > CARIBE_SYNC_MAX_OBJECTS) return 0;
    slot--;
    if (objects[slot].owner == 0U || object_id(slot) != id) return 0;
    return &objects[slot];
}
static void wake_object(native_semaphore_t *object)
{
    __sync_add_and_fetch(&object->sequence, 1U);
    thread_wakeup((event_t)&object->sequence);
}
static void delete_locked(native_semaphore_t *object)
{
    object->owner = object->value = object->global = object->holder = 0;
    memset(object->name, 0, sizeof(object->name));
    wake_object(object);
}
void riscv32_native_sync_release_owner(uint32_t pid)
{
    uint32_t i;
    if (!pid || __sync_fetch_and_add(&initialized, 0U) != 2U) return;
    sync_lock_queue();
    for (i = 0; i < CARIBE_SYNC_MAX_OBJECTS; i++)
        if (objects[i].owner == pid) delete_locked(&objects[i]);
    sync_unlock_queue();
}
static int sync_wait(riscv32_linux_process_t *process, uint32_t *sequence,
    uint32_t expected, uint64_t deadline)
{
    boolean_t native = riscv32_native_thread_current();
    if (native && !current_thread()->active) return SYNC_INTR;
    if (!native && riscv32_linux_signal_has_deliverable(process)) return SYNC_INTR;
    if (!native && !riscv32_linux_process_transition(process, RISCV32_LINUX_PROCESS_SLEEPING))
        panic("riscv32: native sync cannot sleep");
    assert_wait_deadline((event_t)sequence, THREAD_UNINT, deadline);
    __sync_synchronize();
    if (__sync_fetch_and_add(sequence, 0U) == expected &&
        (native ? current_thread()->active : !riscv32_linux_signal_has_deliverable(process))) {
        __sync_add_and_fetch(&waits, 1U);
        thread_block(THREAD_CONTINUE_NULL);
        __sync_add_and_fetch(&wakes, 1U);
    } else {
        (void)clear_wait(current_thread(), THREAD_AWAKENED);
    }
    if (!native && (!riscv32_linux_process_transition(process, RISCV32_LINUX_PROCESS_RUNNABLE) ||
        !riscv32_linux_process_transition(process, RISCV32_LINUX_PROCESS_RUNNING)))
        panic("riscv32: native sync cannot resume");
    if (native ? !current_thread()->active : riscv32_linux_signal_has_deliverable(process)) return SYNC_INTR;
    if (mach_absolute_time() >= deadline) {
        __sync_add_and_fetch(&timeouts, 1U);
        return SYNC_TIMEDOUT;
    }
    return 0;
}
int riscv32_native_sync_ioctl(uint32_t request, uint32_t user_arg)
{
    struct caribe_sync_request arg;
    riscv32_linux_process_t *process = riscv32_linux_process_current();
    native_semaphore_t *object;
    uint32_t i, expected;
    uint64_t timeout, interval, deadline = 0;
    int error, waited = 0;
    if (request != CARIBE_SYNC_CALL) return SYNC_NOTTY;
    if (!process) return SYNC_PERM;
    if (riscv32_copyin(user_arg, &arg, sizeof(arg))) return SYNC_FAULT;
    if (arg.abi != CARIBE_SYNC_ABI || (arg.flags & ~(CARIBE_SYNC_WAIT | CARIBE_SYNC_GLOBAL))) return SYNC_INVAL;
    timeout = ((uint64_t)arg.timeout_high << 32) | arg.timeout_low;
    if (timeout > 60000000ULL) return SYNC_INVAL;
    if (arg.operation == CARIBE_SYNC_CREATE) {
        if (arg.flags & CARIBE_SYNC_WAIT || arg.value > CARIBE_SYNC_MAX_VALUE ||
            !arg.name[0] || arg.name[63] || timeout) return SYNC_INVAL;
    } else if (arg.flags & CARIBE_SYNC_GLOBAL) return SYNC_INVAL;
    if (!(arg.flags & CARIBE_SYNC_WAIT) && timeout) return SYNC_INVAL;
    if (arg.flags & CARIBE_SYNC_WAIT) {
        if (arg.operation != CARIBE_SYNC_ACQUIRE || timeout == 0) return SYNC_INVAL;
        nanoseconds_to_absolutetime(timeout * 1000ULL, &interval);
        deadline = mach_absolute_time() + interval;
    }
    if ((arg.operation == CARIBE_SYNC_ACQUIRE || arg.operation == CARIBE_SYNC_RELEASE) &&
        (arg.units == 0 || arg.units > CARIBE_SYNC_MAX_VALUE)) return SYNC_INVAL;
    sync_init();
retry:
    sync_lock_queue();
    error = 0;
    if (arg.operation == CARIBE_SYNC_STATS) {
        arg.live = 0;
        for (i = 0; i < CARIBE_SYNC_MAX_OBJECTS; i++) arg.live += objects[i].owner != 0;
        arg.waits = __sync_fetch_and_add(&waits, 0U);
        arg.wakes = __sync_fetch_and_add(&wakes, 0U);
        arg.timeouts = __sync_fetch_and_add(&timeouts, 0U);
        arg.acquires = acquires;
        arg.releases = releases;
        goto output;
    }
    if (arg.operation == CARIBE_SYNC_CREATE) {
        for (i = 0; i < CARIBE_SYNC_MAX_OBJECTS; i++) if (!objects[i].owner) {
            object = &objects[i];
            object->generation = (object->generation + 1U) & 0xffffffU;
            if (!object->generation) object->generation = 1U;
            arg.id = object_id(i);
            if (riscv32_copyout(&arg, user_arg, sizeof(arg))) { error = SYNC_FAULT; goto done; }
            object->owner = process->pid;
            object->holder = 0;
            object->value = arg.value;
            object->global = (arg.flags & CARIBE_SYNC_GLOBAL) != 0;
            memcpy(object->name, arg.name, sizeof(object->name));
            goto done;
        }
        error = SYNC_NOSPC;
        goto done;
    }
    object = get_object(arg.id);
    if (!object) { error = waited ? SYNC_REMOVED : SYNC_INVAL; goto done; }
    if (object->owner != process->pid && !object->global) { error = SYNC_PERM; goto done; }
    switch (arg.operation) {
    case CARIBE_SYNC_DELETE:
        if (object->owner != process->pid) { error = SYNC_PERM; break; }
        delete_locked(object);
        break;
    case CARIBE_SYNC_ACQUIRE:
        if (object->value < arg.units) {
            expected = __sync_fetch_and_add(&object->sequence, 0U);
            sync_unlock_queue();
            if (!(arg.flags & CARIBE_SYNC_WAIT)) return SYNC_AGAIN;
            error = sync_wait(process, &object->sequence, expected, deadline);
            if (error) return error;
            waited = 1;
            goto retry;
        }
        arg.value = object->value - arg.units;
        arg.owner = object->owner;
        arg.holder = riscv32_native_thread_identity();
        if (riscv32_copyout(&arg, user_arg, sizeof(arg))) { error = SYNC_FAULT; break; }
        object->value = arg.value;
        object->holder = arg.holder;
        __sync_synchronize();
        acquires++;
        break;
    case CARIBE_SYNC_RELEASE:
        if (arg.units > CARIBE_SYNC_MAX_VALUE - object->value) { error = SYNC_OVERFLOW; break; }
        arg.value = object->value + arg.units;
        if (riscv32_copyout(&arg, user_arg, sizeof(arg))) { error = SYNC_FAULT; break; }
        __sync_synchronize();
        object->value = arg.value;
        releases++;
        wake_object(object);
        break;
    case CARIBE_SYNC_INFO:
        arg.value = object->value;
        arg.owner = object->owner;
        arg.holder = object->holder;
        arg.flags = object->global ? CARIBE_SYNC_GLOBAL : 0;
        memcpy(arg.name, object->name, sizeof(arg.name));
        goto output;
    default:
        error = SYNC_INVAL;
    }
done:
    sync_unlock_queue();
    return error;
output:
    error = riscv32_copyout(&arg, user_arg, sizeof(arg)) ? SYNC_FAULT : 0;
    goto done;
}
