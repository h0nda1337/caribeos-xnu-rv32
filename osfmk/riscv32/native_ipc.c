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
 * Bounded message ports with scheduler waits and process-owned lifecycle.
 * Modified 2026-10-13: independent native-thread waits and preemption-safe locks.
 */
#include <machine/native_ipc.h>
#include <machine/copyio.h>
#include <machine/linux_process.h>
#include <machine/linux_signal.h>
#include <machine/native_thread.h>
#include <kern/clock.h>
#include <kern/misc_protos.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <mach/mach_time.h>
#include <string.h>

/* Linux-personality errno values, not the BSD errno numbering. */
#define IPC_PERM 1
#define IPC_NOENT 2
#define IPC_INTR 4
#define IPC_AGAIN 11
#define IPC_FAULT 14
#define IPC_EXIST 17
#define IPC_INVAL 22
#define IPC_NOSPC 28
#define IPC_NOTTY 25
#define IPC_MSGSIZE 90
#define IPC_TIMEDOUT 110
typedef struct {
    uint32_t owner, generation, count, limit, public_port;
    uint32_t read_sequence, write_sequence;
    uint32_t queue[CARIBE_IPC_MAX_QUEUE];
    char name[64];
} caribe_ipc_port_t;
typedef struct {
    uint32_t used, code, size;
    uint8_t bytes[CARIBE_IPC_MAX_PAYLOAD];
} ipc_message_t;
static caribe_ipc_port_t ports[CARIBE_IPC_MAX_PORTS];
static ipc_message_t messages[CARIBE_IPC_MAX_MESSAGES];
static decl_simple_lock_data(, ipc_lock);
/* Raw native threads can compete on the same hart at kernel preemption points. */
static void ipc_lock_queue(void)
{
    disable_preemption();
    simple_lock(&ipc_lock);
}
static void ipc_unlock_queue(void)
{
    simple_unlock(&ipc_lock);
    enable_preemption();
}
static volatile uint32_t initialized;
static uint32_t sends, receives, server_id, blocks, wakeups, timeouts;

static void ipc_init(void)
{
    if (__sync_fetch_and_add(&initialized, 0U) == 2U) return;
    disable_preemption();
    if (__sync_bool_compare_and_swap(&initialized, 0U, 1U)) {
        simple_lock_init(&ipc_lock, 0);
        __sync_synchronize();
        initialized = 2U;
    } else {
        while (__sync_fetch_and_add(&initialized, 0U) != 2U)
            __asm__ volatile("nop");
    }
    enable_preemption();
}

static uint32_t port_id(uint32_t slot)
{
    return (ports[slot].generation << 6) | (slot + 1U);
}

static caribe_ipc_port_t *get_port(uint32_t id)
{
    uint32_t slot = (id & 63U);
    if (slot == 0U || slot > CARIBE_IPC_MAX_PORTS) return 0;
    slot--;
    if (ports[slot].owner == 0U || port_id(slot) != id) return 0;
    return &ports[slot];
}

static void wake_port(caribe_ipc_port_t *port)
{
    __sync_add_and_fetch(&port->read_sequence, 1U);
    __sync_add_and_fetch(&port->write_sequence, 1U);
    thread_wakeup((event_t)&port->read_sequence);
    thread_wakeup((event_t)&port->write_sequence);
}

static void delete_port_locked(uint32_t slot)
{
    caribe_ipc_port_t *port = &ports[slot];
    uint32_t i;
    for (i = 0; i < port->count; i++) {
        ipc_message_t *message = &messages[port->queue[i]];
        memset(message, 0, sizeof(*message));
    }
    if (server_id == port_id(slot)) server_id = 0;
    port->count = 0;
    port->owner = 0;
    port->public_port = 0;
    memset(port->name, 0, sizeof(port->name));
    wake_port(port);
    /* Pool pressure can have blocked a sender to any other port. */
    for (i = 0; i < CARIBE_IPC_MAX_PORTS; i++) {
        __sync_add_and_fetch(&ports[i].write_sequence, 1U);
        thread_wakeup((event_t)&ports[i].write_sequence);
    }
}

void riscv32_native_ipc_release_owner(uint32_t pid)
{
    uint32_t i;
    if (pid == 0U || __sync_fetch_and_add(&initialized, 0U) != 2U) return;
    ipc_lock_queue();
    for (i = 0; i < CARIBE_IPC_MAX_PORTS; i++)
        if (ports[i].owner == pid) delete_port_locked(i);
    ipc_unlock_queue();
}

static int port_wait(riscv32_linux_process_t *process, uint32_t *sequence,
    uint32_t expected, uint64_t deadline)
{
    boolean_t native = riscv32_native_thread_current();
    if (native ? !current_thread()->active : riscv32_linux_signal_has_deliverable(process)) return IPC_INTR;
    if (!native && !riscv32_linux_process_transition(process, RISCV32_LINUX_PROCESS_SLEEPING))
        panic("riscv32: IPC cannot sleep");
    assert_wait_deadline((event_t)sequence, THREAD_UNINT, deadline);
    __sync_synchronize();
    if (__sync_fetch_and_add(sequence, 0U) == expected &&
        (native ? current_thread()->active : !riscv32_linux_signal_has_deliverable(process))) {
        __sync_add_and_fetch(&blocks, 1U);
        thread_block(THREAD_CONTINUE_NULL);
        __sync_add_and_fetch(&wakeups, 1U);
    } else
        (void)clear_wait(current_thread(), THREAD_AWAKENED);
    if (!native && (!riscv32_linux_process_transition(process, RISCV32_LINUX_PROCESS_RUNNABLE) ||
        !riscv32_linux_process_transition(process, RISCV32_LINUX_PROCESS_RUNNING)))
        panic("riscv32: IPC cannot resume");
    if (native ? !current_thread()->active : riscv32_linux_signal_has_deliverable(process)) return IPC_INTR;
    if (mach_absolute_time() >= deadline) {
        __sync_add_and_fetch(&timeouts, 1U);
        return IPC_TIMEDOUT;
    }
    return 0;
}

int riscv32_native_ipc_ioctl(uint32_t request, uint32_t user_arg)
{
    struct caribe_ipc_request arg;
    riscv32_linux_process_t *process = riscv32_linux_process_current();
    caribe_ipc_port_t *port;
    uint32_t i, j, free_slot, *sequence, expected;
    uint64_t timeout, interval, deadline = 0;
    int error;
    if (request != CARIBE_IPC_CALL) return IPC_NOTTY;
    if (!process) return IPC_PERM;
    if (riscv32_native_thread_current() && !riscv32_native_thread_cpu_allowed()) return IPC_PERM;
    if (riscv32_copyin(user_arg, &arg, sizeof(arg))) return IPC_FAULT;
    if (arg.abi != CARIBE_IPC_ABI || (arg.flags & ~CARIBE_IPC_WAIT)) return IPC_INVAL;
    timeout = ((uint64_t)arg.timeout_high << 32) | arg.timeout_low;
    if (timeout > 60000000ULL) return IPC_INVAL;
    if (arg.flags & CARIBE_IPC_WAIT) {
        if (timeout == 0 || (arg.operation != CARIBE_IPC_SEND &&
            arg.operation != CARIBE_IPC_RECEIVE && arg.operation != CARIBE_IPC_PEEK)) return IPC_INVAL;
        nanoseconds_to_absolutetime(timeout * 1000ULL, &interval);
        deadline = mach_absolute_time() + interval;
    }
    ipc_init();
retry:
    ipc_lock_queue();
    error = 0;
    if (arg.operation == CARIBE_IPC_STATS) {
        arg.ports = arg.messages = 0;
        for (i = 0; i < CARIBE_IPC_MAX_PORTS; i++) arg.ports += ports[i].owner != 0;
        for (i = 0; i < CARIBE_IPC_MAX_MESSAGES; i++) arg.messages += messages[i].used != 0;
        arg.sends = sends;
        arg.receives = receives;
        arg.limit = __sync_fetch_and_add(&blocks, 0U);
        arg.flags = __sync_fetch_and_add(&wakeups, 0U);
        arg.code = __sync_fetch_and_add(&timeouts, 0U);
        goto output;
    }
    if (arg.operation == CARIBE_IPC_GET_SERVER) {
        if (!get_port(server_id)) { error = IPC_NOENT; goto done; }
        arg.id = server_id;
        goto output;
    }
    if (arg.operation == CARIBE_IPC_ENUM) {
        i = arg.id && arg.id != UINT32_MAX ? (arg.id & 63U) : 0U;
        arg.code = 0;
        for (; i < CARIBE_IPC_MAX_PORTS; i++) {
            if (ports[i].owner && (ports[i].owner == process->pid || ports[i].public_port)) {
                arg.id = port_id(i);
                arg.owner = ports[i].owner;
                arg.limit = ports[i].limit;
                arg.queued = ports[i].count;
                arg.messages = ports[i].public_port;
                memcpy(arg.name, ports[i].name, sizeof(arg.name));
                arg.code = 1;
                break;
            }
        }
        goto output;
    }
    if (arg.operation == CARIBE_IPC_CREATE || arg.operation == CARIBE_IPC_FIND) {
        for (i = 0; i < sizeof(arg.name) && arg.name[i]; i++) { }
        if (i == sizeof(arg.name) || !arg.name[0]) {
            error = IPC_INVAL; goto done;
        }
        if (arg.operation == CARIBE_IPC_FIND) {
            for (i = 0; i < CARIBE_IPC_MAX_PORTS; i++) {
                if (ports[i].owner && ports[i].public_port &&
                    !strcmp(ports[i].name, arg.name)) {
                    arg.id = port_id(i);
                    goto output;
                }
            }
            error = IPC_NOENT; goto done;
        }
        if (!arg.limit || arg.limit > CARIBE_IPC_MAX_QUEUE) {
            error = IPC_INVAL; goto done;
        }
        for (i = 0; i < CARIBE_IPC_MAX_PORTS; i++) {
            if (!ports[i].owner && ports[i].generation < 0x01ffffffU) {
                ports[i].generation++;
                arg.id = port_id(i);
                /* Failed copyout never publishes an unreachable port. */
                if (riscv32_copyout(&arg, user_arg, sizeof(arg))) {
                    error = IPC_FAULT; goto done;
                }
                ports[i].owner = process->pid;
                ports[i].limit = arg.limit;
                memcpy(ports[i].name, arg.name, sizeof(arg.name));
                goto done;
            }
        }
        error = IPC_NOSPC; goto done;
    }
    port = get_port(arg.id);
    if (!port) { error = IPC_NOENT; goto done; }
    switch (arg.operation) {
    case CARIBE_IPC_DELETE:
        if (port->owner != process->pid) { error = IPC_PERM; break; }
        delete_port_locked((arg.id & 63U) - 1U);
        break;
    case CARIBE_IPC_PUBLIC:
    case CARIBE_IPC_SET_SERVER:
        if (port->owner != process->pid) { error = IPC_PERM; break; }
        if (arg.operation == CARIBE_IPC_SET_SERVER && server_id && server_id != arg.id) {
            error = IPC_EXIST; break;
        }
        if (arg.operation == CARIBE_IPC_SET_SERVER || arg.code == 1U) {
            for (i = 0; i < CARIBE_IPC_MAX_PORTS; i++) {
                if (&ports[i] != port && ports[i].owner && ports[i].public_port &&
                    !strcmp(ports[i].name, port->name)) { error = IPC_EXIST; goto done; }
            }
            port->public_port = 1;
        } else if (arg.code == 0U) port->public_port = 0;
        else { error = IPC_INVAL; break; }
        if (arg.operation == CARIBE_IPC_SET_SERVER) server_id = arg.id;
        break;
    case CARIBE_IPC_INFO:
        if (!port->public_port && port->owner != process->pid) { error = IPC_PERM; break; }
        arg.owner = port->owner;
        arg.queued = port->count;
        arg.limit = port->limit;
        arg.code = port->public_port;
        memcpy(arg.name, port->name, sizeof(arg.name));
        goto output;
    case CARIBE_IPC_SEND:
        if (arg.size > CARIBE_IPC_MAX_PAYLOAD) { error = IPC_MSGSIZE; break; }
        free_slot = CARIBE_IPC_MAX_MESSAGES;
        for (i = 0; i < CARIBE_IPC_MAX_MESSAGES; i++) {
            if (!messages[i].used) { free_slot = i; break; }
        }
        if (port->count == port->limit || free_slot == CARIBE_IPC_MAX_MESSAGES) {
            sequence = &port->write_sequence;
            goto unavailable;
        }
        if (arg.size && riscv32_copyin(arg.buffer, messages[free_slot].bytes, arg.size)) {
            memset(&messages[free_slot], 0, sizeof(messages[free_slot]));
            error = IPC_FAULT; break;
        }
        messages[free_slot].used = 1;
        messages[free_slot].size = arg.size;
        messages[free_slot].code = arg.code;
        port->queue[port->count++] = free_slot;
        sends++;
        __sync_add_and_fetch(&port->read_sequence, 1U);
        thread_wakeup((event_t)&port->read_sequence);
        break;
    case CARIBE_IPC_PEEK:
    case CARIBE_IPC_RECEIVE:
        if (port->owner != process->pid) { error = IPC_PERM; break; }
        if (!port->count) {
            sequence = &port->read_sequence;
            goto unavailable;
        }
        i = port->queue[0];
        if (arg.operation == CARIBE_IPC_PEEK) {
            arg.size = messages[i].size;
            arg.code = messages[i].code;
            goto output;
        }
        if (arg.size < messages[i].size) { error = IPC_MSGSIZE; break; }
        arg.size = messages[i].size;
        arg.code = messages[i].code;
        if (arg.size && arg.buffer < user_arg + sizeof(arg) &&
            (uint64_t)arg.buffer + arg.size > user_arg) {
            error = IPC_INVAL; break;
        }
        if ((arg.size && riscv32_copyout(messages[i].bytes, arg.buffer, arg.size)) ||
            riscv32_copyout(&arg, user_arg, sizeof(arg))) {
            error = IPC_FAULT; break;
        }
        memset(&messages[i], 0, sizeof(messages[i]));
        for (j = 1; j < port->count; j++) port->queue[j-1] = port->queue[j];
        port->count--;
        receives++;
        for (j = 0; j < CARIBE_IPC_MAX_PORTS; j++) {
            __sync_add_and_fetch(&ports[j].write_sequence, 1U);
            thread_wakeup((event_t)&ports[j].write_sequence);
        }
        break;
    default: error = IPC_INVAL;
    }
done:
    ipc_unlock_queue();
    return error;
output:
    error = riscv32_copyout(&arg, user_arg, sizeof(arg)) ? IPC_FAULT : 0;
    goto done;
unavailable:
    expected = __sync_fetch_and_add(sequence, 0U);
    ipc_unlock_queue();
    if (!(arg.flags & CARIBE_IPC_WAIT)) return IPC_AGAIN;
    error = port_wait(process, sequence, expected, deadline);
    if (error) return error;
    goto retry;
}
