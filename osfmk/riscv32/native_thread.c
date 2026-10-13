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
 * Created by h0nda1337, 2026-10-13. Raw RV32 user entry, not pthread/TLS.
 */
#include <machine/native_thread.h>
#include <machine/bootinfo.h>
#include <machine/copyio.h>
#include <machine/cpu_data.h>
#include <machine/linux_process.h>
#include <machine/linux_signal.h>
#include <machine/pmap.h>
#include <machine/trap.h>
#include <kern/ast.h>
#include <kern/clock.h>
#include <kern/misc_protos.h>
#include <kern/sched_prim.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <mach/mach_time.h>
#include <mach/thread_act_server.h>
#include <string.h>
extern kern_return_t thread_create(task_t task, thread_t *new_thread);

#define NT_PERM 1
#define NT_INTR 4
#define NT_AGAIN 11
#define NT_FAULT 14
#define NT_BUSY 16
#define NT_INVAL 22
#define NT_NOTTY 25
#define NT_NOSPC 28
#define NT_NOTSUP 95
#define NT_TIMEDOUT 110
#define NT_RESERVED 1U
#define NT_REAPING 5U
typedef struct {
    riscv32_linux_process_t *owner;
    thread_t thread;
    uint32_t generation, state, status, sequence, destroyed;
    uint32_t stack_base, stack_size, tp;
} native_thread_t;
static native_thread_t objects[CARIBE_THREAD_MAX];
static decl_simple_lock_data(, native_thread_lock);
static volatile uint32_t initialized;
static uint32_t created, entered, exited, reaped, cancelled, faults, waits, preemptions;

static void nt_init(void)
{
    if (__sync_fetch_and_add(&initialized, 0U) == 2U) return;
    if (__sync_bool_compare_and_swap(&initialized, 0U, 1U)) {
        simple_lock_init(&native_thread_lock, 0);
        __sync_synchronize(); initialized = 2U;
    } else while (__sync_fetch_and_add(&initialized, 0U) != 2U) __asm__ volatile("nop");
}
static uint32_t nt_id(native_thread_t *object)
{
    return 0x40000000U | (object->generation << 5) | (uint32_t)(object - objects + 1);
}
static native_thread_t *nt_lookup(uint32_t id)
{
    uint32_t slot = id & 31U;
    if (!slot || slot > CARIBE_THREAD_MAX) return 0;
    return objects[slot - 1].owner && nt_id(&objects[slot - 1]) == id ? &objects[slot - 1] : 0;
}
static native_thread_t *nt_current(void)
{
    thread_t thread = current_thread();
    return thread ? (native_thread_t *)thread->machine.native_context : 0;
}
boolean_t riscv32_native_thread_current(void) { return nt_current() != 0; }
uint32_t riscv32_native_thread_identity(void)
{
    native_thread_t *object = nt_current();
    riscv32_linux_process_t *process = riscv32_linux_process_current();
    return object ? nt_id(object) : process ? process->pid : 0;
}
boolean_t riscv32_native_thread_cpu_allowed(void)
{
    native_thread_t *object = nt_current();
    thread_t thread = current_thread();
    riscv32_linux_process_t *process = riscv32_linux_process_current();
    return object && object->owner == process && object->thread == thread && process &&
        process->task == thread->task && thread->machine.user_pmap == process->pmap &&
        get_cpu_number() == 0 && thread->bound_processor == master_processor;
}
boolean_t riscv32_native_thread_owner_busy(uint32_t pid)
{
    uint32_t i; boolean_t busy = FALSE;
    if (__sync_fetch_and_add(&initialized, 0U) != 2U) return FALSE;
    simple_lock(&native_thread_lock);
    for (i = 0; i < CARIBE_THREAD_MAX; i++)
        if (objects[i].owner && objects[i].owner->pid == pid) { busy = TRUE; break; }
    simple_unlock(&native_thread_lock);
    return busy;
}
static void nt_wake(native_thread_t *object)
{
    __sync_add_and_fetch(&object->sequence, 1U);
    thread_wakeup((event_t)&object->sequence);
}
static uint64_t nt_deadline(uint32_t microseconds)
{
    uint64_t interval;
    nanoseconds_to_absolutetime((uint64_t)microseconds * 1000ULL, &interval);
    return mach_absolute_time() + interval;
}
static int nt_wait(uint32_t *sequence, uint32_t expected, uint64_t deadline)
{
    if (mach_absolute_time() >= deadline) return NT_TIMEDOUT;
    assert_wait_deadline((event_t)sequence, THREAD_UNINT, deadline);
    __sync_synchronize();
    if (__sync_fetch_and_add(sequence, 0U) == expected) {
        __sync_add_and_fetch(&waits, 1U);
        thread_block(THREAD_CONTINUE_NULL);
    } else (void)clear_wait(current_thread(), THREAD_AWAKENED);
    return 0;
}
void riscv32_native_thread_exit(uint32_t status)
{
    native_thread_t *object = nt_current();
    if (!object || !riscv32_native_thread_cpu_allowed()) panic("native thread exit identity");
    simple_lock(&native_thread_lock);
    object->status = status;
    object->state = CARIBE_THREAD_EXITED;
    exited++;
    if (status == CARIBE_THREAD_CANCELLED && !current_thread()->active) cancelled++;
    nt_wake(object);
    simple_unlock(&native_thread_lock);
    simple_lock(&object->owner->lock);
    object->owner->live_thread_count--;
    simple_unlock(&object->owner->lock);
    (void)thread_terminate_internal(current_thread());
    thread_terminate_self();
    panic("native thread exit returned");
    __builtin_unreachable();
}
void riscv32_native_thread_user_boundary(void)
{
    native_thread_t *object = nt_current();
    riscv32_linux_process_t *process = riscv32_linux_process_current();
    if (!object && (!process || !riscv32_native_thread_owner_busy(process->pid))) return;
    if (object && !current_thread()->active) riscv32_native_thread_exit(CARIBE_THREAD_CANCELLED);
    /* Preserve whole-process STOP/CONT semantics without borrowing the main
       thread's signal frame or changing process lifecycle state from a worker. */
    while (object && process->state == RISCV32_LINUX_PROCESS_STOPPED) {
        uint32_t expected = __sync_fetch_and_add(&process->stop_sequence, 0U);
        assert_wait((event_t)&process->stop_sequence, THREAD_UNINT);
        __sync_synchronize();
        if (current_thread()->active && process->state == RISCV32_LINUX_PROCESS_STOPPED &&
            __sync_fetch_and_add(&process->stop_sequence, 0U) == expected)
            thread_block(THREAD_CONTINUE_NULL);
        else (void)clear_wait(current_thread(), THREAD_AWAKENED);
        if (!current_thread()->active) riscv32_native_thread_exit(CARIBE_THREAD_CANCELLED);
    }
    /* Only the opted-in raw-thread profile takes new user preemption paths. */
    if ((*ast_pending() & (AST_PREEMPT | AST_URGENT)) == (AST_PREEMPT | AST_URGENT) && get_preemption_level() == 0) {
        __sync_add_and_fetch(&preemptions, 1U);
        ast_taken(AST_PREEMPTION, FALSE);
    }
    if (object && !current_thread()->active) riscv32_native_thread_exit(CARIBE_THREAD_CANCELLED);
}
void riscv32_native_thread_fault(void)
{
    __sync_add_and_fetch(&faults, 1U);
    riscv32_native_thread_exit(CARIBE_THREAD_FAULTED);
}
void riscv32_native_thread_bootstrap(void)
{
    native_thread_t *object = nt_current();
    thread_t thread = current_thread();
    if (!riscv32_native_thread_cpu_allowed() || riscv32_pmap_current() != object->owner->pmap)
        panic("native thread bootstrap pmap/CPU");
    if (!thread->active) riscv32_native_thread_exit(CARIBE_THREAD_CANCELLED);
    __sync_add_and_fetch(&entered, 1U);
    riscv32_return_to_user(thread->machine.iss);
    panic("native thread U-mode entry returned");
    __builtin_unreachable();
}
static void nt_reap(native_thread_t *object)
{
    riscv32_linux_process_t *owner = object->owner;
    thread_t thread = object->thread;
    uint64_t deadline = nt_deadline(60000000U);
    /* The scheduler/reaper owns another reference until it releases the stack.
       Keep the slot and process alive until the final deallocation notification. */
    simple_lock(&native_thread_lock);
    object->state = NT_REAPING;
    simple_unlock(&native_thread_lock);
    thread_deallocate(thread);
    while (__sync_fetch_and_add(&object->destroyed, 0U) == 0U)
        if (nt_wait(&object->destroyed, 0U, deadline)) panic("native thread reaper timeout");
    simple_lock(&native_thread_lock);
    object->thread = THREAD_NULL;
    object->owner = 0;
    object->state = 0;
    reaped++;
    simple_unlock(&native_thread_lock);
    simple_lock(&owner->lock);
    owner->thread_count--;
    simple_unlock(&owner->lock);
    riscv32_linux_process_release(owner);
}
static void nt_cancel(native_thread_t *object)
{
    uint64_t deadline = nt_deadline(60000000U);
    if (object->state != CARIBE_THREAD_EXITED) {
        (void)thread_terminate_internal(object->thread);
        /* Native waits are non-interruptible: explicitly wake for cancellation. */
        (void)clear_wait(object->thread, THREAD_AWAKENED);
    }
    for (;;) {
        uint32_t sequence;
        simple_lock(&native_thread_lock);
        if (object->state == CARIBE_THREAD_EXITED) { simple_unlock(&native_thread_lock); break; }
        sequence = object->sequence;
        simple_unlock(&native_thread_lock);
        if (nt_wait(&object->sequence, sequence, deadline)) panic("native thread cancellation timeout");
    }
}
void riscv32_native_thread_release_owner(uint32_t pid)
{
    uint32_t i;
    if (__sync_fetch_and_add(&initialized, 0U) != 2U) return;
    for (i = 0; i < CARIBE_THREAD_MAX; i++) {
        native_thread_t *object = &objects[i];
        boolean_t owned;
        simple_lock(&native_thread_lock);
        owned = object->owner && object->owner->pid == pid;
        simple_unlock(&native_thread_lock);
        if (owned) { nt_cancel(object); nt_reap(object); }
    }
}
static int nt_create(riscv32_linux_process_t *process, struct caribe_thread_request *arg,
    uint32_t user_arg)
{
    native_thread_t *object = 0;
    thread_t thread;
    riscv_saved_state_t *state;
    uint32_t i, end;
    kern_return_t kr;
    spl_t s;
    if (!riscv32_boot_arg_has("native-threads")) return NT_NOTSUP;
    if ((arg->entry & 1U) || !riscv32_pmap_user_execute_accessible(process->pmap, arg->entry) ||
        (arg->stack_base & 4095U) || (arg->stack_size & 4095U) || arg->stack_size < 16384U ||
        arg->stack_size > 262144U || arg->stack_base > 0x80000000U - arg->stack_size ||
        !riscv32_pmap_user_range_accessible(process->pmap, arg->stack_base, arg->stack_size, TRUE) ||
        (arg->tp & 15U) || !riscv32_pmap_user_range_accessible(process->pmap, arg->tp, 16U, TRUE)) return NT_INVAL;
    end = arg->stack_base + arg->stack_size;
    if (arg->tp == current_thread()->machine.user_tp ||
        (arg->stack_base < process->exec_info.stack_top && end > process->exec_info.stack_base))
        return NT_BUSY;
    simple_lock(&native_thread_lock);
    for (i = 0; i < CARIBE_THREAD_MAX; i++) {
        native_thread_t *other = &objects[i];
        if (other->owner == process && (other->tp == arg->tp ||
            (arg->tp >= other->stack_base && arg->tp < other->stack_base + other->stack_size) ||
            (other->tp >= arg->stack_base && other->tp < end) ||
            (arg->stack_base < other->stack_base + other->stack_size && end > other->stack_base))) {
            simple_unlock(&native_thread_lock); return NT_BUSY;
        }
        if (!other->owner && !object) object = other;
    }
    if (!object) { simple_unlock(&native_thread_lock); return NT_NOSPC; }
    object->generation = object->generation % 0x1ffffffU + 1U;
    object->owner = process; object->state = NT_RESERVED; object->status = object->destroyed = 0;
    object->stack_base = arg->stack_base; object->stack_size = arg->stack_size; object->tp = arg->tp;
    simple_unlock(&native_thread_lock);
    riscv32_linux_process_retain(process);
    kr = thread_create(process->task, &thread);
    if (kr != KERN_SUCCESS) {
        simple_lock(&native_thread_lock); object->owner = 0; object->state = 0; simple_unlock(&native_thread_lock);
        riscv32_linux_process_release(process); return NT_AGAIN;
    }
    object->thread = thread;
    simple_lock(&process->lock);
    process->thread_count++; process->live_thread_count++;
    simple_unlock(&process->lock);
    thread->machine.native_context = object;
    thread->machine.deallocate_sequence = &object->destroyed;
    thread->machine.user_pmap = process->pmap;
    state = thread->machine.iss;
    memset(state, 0, sizeof(*state));
    state->thread.pc = state->exception.sepc = arg->entry;
    state->thread.x[RISCV_X_SP] = end;
    state->thread.x[RISCV_X_TP] = arg->tp;
    state->thread.x[RISCV_X_GP] = current_thread()->machine.user_gp;
    state->thread.x[RISCV_X_A0] = arg->argument;
    state->thread.x[RISCV_X_A1] = nt_id(object);
    s = splsched(); thread_lock(thread); thread->bound_processor = master_processor; thread_unlock(thread); splx(s);
    simple_lock(&native_thread_lock); object->state = CARIBE_THREAD_SUSPENDED; created++; simple_unlock(&native_thread_lock);
    arg->id = nt_id(object); arg->state = object->state; arg->owner = process->pid; arg->cpu = 0;
    if (riscv32_copyout(arg, user_arg, sizeof(*arg))) { nt_cancel(object); nt_reap(object); return NT_FAULT; }
    return 0;
}
int riscv32_native_thread_ioctl(uint32_t request, uint32_t user_arg)
{
    struct caribe_thread_request arg;
    riscv32_linux_process_t *process = riscv32_linux_process_current();
    native_thread_t *object;
    uint32_t i, sequence;
    uint64_t deadline;
    int error = 0;
    if (request != CARIBE_THREAD_CALL) return NT_NOTTY;
    if (!process || process->affinity_cpu != 0U) return NT_PERM;
    if (riscv32_copyin(user_arg, &arg, sizeof(arg))) return NT_FAULT;
    if (arg.abi != CARIBE_THREAD_ABI || arg.timeout_us > 60000000U) return NT_INVAL;
    for (i = 0; i < 10; i++) if (arg.reserved[i]) return NT_INVAL;
    if (arg.operation == CARIBE_THREAD_EXIT) {
        if (!nt_current()) return NT_PERM;
        riscv32_native_thread_exit(arg.status);
    }
    if (arg.operation == CARIBE_THREAD_YIELD) {
        thread_block(THREAD_CONTINUE_NULL); return 0;
    }
    if (nt_current() && arg.operation != CARIBE_THREAD_SELF && arg.operation != CARIBE_THREAD_INFO)
        return NT_NOTSUP;
    if (!riscv32_pmap_user_range_accessible(process->pmap, user_arg, sizeof(arg), TRUE)) return NT_FAULT;
    nt_init();
    if (arg.operation == CARIBE_THREAD_CREATE) return nt_create(process, &arg, user_arg);
    if (arg.operation == CARIBE_THREAD_SELF) { arg.id = riscv32_native_thread_identity(); arg.owner = process->pid; arg.cpu = get_cpu_number(); arg.tp = current_thread()->machine.user_tp; }
    else if (arg.operation == CARIBE_THREAD_STATS) {
        simple_lock(&native_thread_lock);
        arg.live = 0;
        for (i = 0; i < CARIBE_THREAD_MAX; i++) arg.live += objects[i].owner != 0;
        arg.created = created; arg.reaped = reaped; arg.entered = entered; arg.exited = exited;
        arg.cancelled = cancelled; arg.faults = faults;
        arg.waits = __sync_fetch_and_add(&waits, 0U); arg.preemptions = __sync_fetch_and_add(&preemptions, 0U);
        simple_unlock(&native_thread_lock);
    } else {
        simple_lock(&native_thread_lock); object = nt_lookup(arg.id);
        if (!object || object->owner != process) { simple_unlock(&native_thread_lock); return object ? NT_PERM : NT_INVAL; }
        arg.state = object->state; arg.status = object->status; arg.owner = process->pid; arg.cpu = 0;
        simple_unlock(&native_thread_lock);
        if (arg.operation == CARIBE_THREAD_START) {
            if (object->state != CARIBE_THREAD_SUSPENDED) return NT_BUSY;
            if (riscv32_copyout(&arg, user_arg, sizeof(arg))) return NT_FAULT;
            simple_lock(&native_thread_lock);
            object->state = CARIBE_THREAD_RUNNING;
            simple_unlock(&native_thread_lock);
            if (thread_resume(object->thread) != KERN_SUCCESS) panic("native thread start failed");
            return 0;
        } else if (arg.operation == CARIBE_THREAD_CANCEL) {
            if (riscv32_copyout(&arg, user_arg, sizeof(arg))) return NT_FAULT;
            nt_cancel(object); return 0;
        } else if (arg.operation == CARIBE_THREAD_JOIN) {
            deadline = nt_deadline(arg.timeout_us);
            for (;;) {
                simple_lock(&native_thread_lock);
                if (object->state == CARIBE_THREAD_EXITED) { simple_unlock(&native_thread_lock); break; }
                sequence = object->sequence;
                simple_unlock(&native_thread_lock);
                if (!arg.timeout_us) return NT_AGAIN;
                if (riscv32_linux_signal_has_deliverable(process)) return NT_INTR;
                if ((error = nt_wait(&object->sequence, sequence, deadline))) return error;
            }
            arg.state = CARIBE_THREAD_EXITED; arg.status = object->status;
            if (riscv32_copyout(&arg, user_arg, sizeof(arg))) return NT_FAULT;
            nt_reap(object); return 0;
        } else if (arg.operation != CARIBE_THREAD_INFO) return NT_INVAL;
    }
    return riscv32_copyout(&arg, user_arg, sizeof(arg)) ? NT_FAULT : error;
}
