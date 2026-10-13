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
 * Created for native Syllable shared areas on XNU RV32, 2026-10-12.
 * Bounded wired shared objects and independently protected Sv32 views.
 */
#include <machine/native_area.h>
#include <machine/copyio.h>
#include <machine/linux_process.h>
#include <machine/pmap.h>
#include <kern/misc_protos.h>
#include <kern/thread.h>
#include <vm/pmap.h>
#include <string.h>

#define AREA_PERM 1
#define AREA_NOENT 2
#define AREA_NOMEM 12
#define AREA_FAULT 14
#define AREA_EXIST 17
#define AREA_INVAL 22
#define AREA_NOTTY 25
#define AREA_NOSPC 28
typedef struct {
    uint32_t count, references;
    uint32_t pages[CARIBE_AREA_MAX_PAGES];
} caribe_area_object_t;
typedef struct {
    uint32_t owner, generation, handle, object, address, size, protection;
    pmap_t pmap;
    char name[64];
} caribe_area_view_t;
static caribe_area_object_t objects[CARIBE_AREA_MAX_OBJECTS];
static caribe_area_view_t views[CARIBE_AREA_MAX_VIEWS];
static uint8_t backing[CARIBE_AREA_POOL_PAGES][4096] __attribute__((aligned(4096)));
static uint32_t page_used[CARIBE_AREA_POOL_PAGES];
static decl_simple_lock_data(, area_lock);
static volatile uint32_t initialized;
static uint32_t allocations, frees;

static void area_init(void)
{
    if (__sync_fetch_and_add(&initialized, 0U) == 2U) return;
    if (__sync_bool_compare_and_swap(&initialized, 0U, 1U)) {
        simple_lock_init(&area_lock, 0);
        __sync_synchronize();
        initialized = 2U;
    } else {
        while (__sync_fetch_and_add(&initialized, 0U) != 2U)
            __asm__ volatile("nop");
    }
}
int riscv32_native_area_reserved(uint32_t address, uint32_t size)
{
    return size && address < CARIBE_AREA_END &&
        (uint64_t)address + size > CARIBE_AREA_BASE;
}
static ppnum_t area_ppnum(uint32_t page)
{
    return (ppnum_t)(riscv32_kva_to_phys((vm_offset_t)(uintptr_t)backing[page]) >> PAGE_SHIFT);
}
static caribe_area_view_t *find_view(uint32_t id, uint32_t owner)
{
    uint32_t i;
    for (i = 0; i < CARIBE_AREA_MAX_VIEWS; i++) {
        if (views[i].owner && views[i].handle == id &&
            (!owner || views[i].owner == owner)) return &views[i];
    }
    return 0;
}
static void free_object(uint32_t slot)
{
    caribe_area_object_t *object = &objects[slot];
    uint32_t i;
    if (object->references) panic("riscv32: shared area freed with live views");
    for (i = 0; i < object->count; i++) {
        uint32_t page = object->pages[i];
        if (!page_used[page]) panic("riscv32: shared area double free");
        memset(backing[page], 0, sizeof(backing[page]));
        page_used[page] = 0;
        frees++;
    }
    memset(object, 0, sizeof(*object));
}
static void delete_view(caribe_area_view_t *view)
{
    uint32_t generation = view->generation;
    uint32_t object_slot = view->object;
    caribe_area_object_t *object = &objects[object_slot];
    /* pmap_remove completes local/RFENCE invalidation before the last zero/reuse. */
    pmap_remove(view->pmap, view->address, view->address + view->size);
    if (!object->references) panic("riscv32: shared area reference underflow");
    object->references--;
    memset(view, 0, sizeof(*view));
    view->generation = generation;
    if (!object->references) free_object(object_slot);
}
void riscv32_native_area_release_owner(uint32_t pid)
{
    uint32_t i;
    if (!pid || __sync_fetch_and_add(&initialized, 0U) != 2U) return;
    simple_lock(&area_lock);
    for (i = 0; i < CARIBE_AREA_MAX_VIEWS; i++)
        if (views[i].owner == pid) delete_view(&views[i]);
    simple_unlock(&area_lock);
}
int riscv32_native_area_inherit(uint32_t parent, uint32_t child, pmap_t pmap)
{
    uint32_t i, j, required = 0, available = 0;
    if (__sync_fetch_and_add(&initialized, 0U) != 2U) return 0;
    simple_lock(&area_lock);
    for (i = 0; i < CARIBE_AREA_MAX_VIEWS; i++) {
        required += views[i].owner == parent;
        available += views[i].owner == 0;
    }
    if (available < required) { simple_unlock(&area_lock); return AREA_NOSPC; }
    for (i = 0; i < CARIBE_AREA_MAX_VIEWS; i++) {
        if (views[i].owner != parent) continue;
        for (j = 0; j < CARIBE_AREA_MAX_VIEWS; j++) {
            if (!views[j].owner) {
                uint32_t generation = views[j].generation;
                views[j] = views[i];
                views[j].generation = generation;
                views[j].owner = child;
                views[j].pmap = pmap;
                /* Fork preserves the parent's virtual address and local area ID. */
                objects[views[j].object].references++;
                break;
            }
        }
    }
    __sync_synchronize();
    simple_unlock(&area_lock);
    return 0;
}
int riscv32_native_area_fork_page(uint32_t child, uint32_t vaddr, uint32_t source_page)
{
    uint32_t i;
    int shared = 0;
    if (__sync_fetch_and_add(&initialized, 0U) != 2U) return 0;
    simple_lock(&area_lock);
    for (i = 0; i < CARIBE_AREA_MAX_VIEWS; i++) {
        caribe_area_view_t *view = &views[i];
        if (view->owner == child && vaddr >= view->address &&
            vaddr < view->address + view->size) {
            uint32_t page = objects[view->object].pages[(vaddr - view->address) >> PAGE_SHIFT];
            if (area_ppnum(page) != source_page) panic("riscv32: shared fork backing mismatch");
            shared = 1;
            break;
        }
    }
    simple_unlock(&area_lock);
    return shared;
}
static int range_free(pmap_t pmap, uint32_t address, uint32_t size)
{
    uint32_t va;
    for (va = address; va < address + size; va += 4096)
        if (pmap_extract(pmap, va)) return 0;
    return 1;
}
static int choose_address(pmap_t pmap, struct caribe_area_request *arg)
{
    uint32_t address;
    if (arg->policy == CARIBE_AREA_EXACT) {
        if ((arg->address & 4095U) || arg->address < CARIBE_AREA_BASE ||
            (uint64_t)arg->address + arg->size > CARIBE_AREA_END) return AREA_INVAL;
        return range_free(pmap, arg->address, arg->size) ? 0 : AREA_EXIST;
    }
    if (arg->policy) return AREA_INVAL;
    for (address = CARIBE_AREA_BASE; address < CARIBE_AREA_END; address += CARIBE_AREA_SLOT_SIZE) {
        if (range_free(pmap, address, arg->size)) { arg->address = address; return 0; }
    }
    return AREA_NOMEM;
}
int riscv32_native_area_ioctl(uint32_t request, uint32_t user_arg)
{
    struct caribe_area_request arg;
    riscv32_linux_process_t *process = riscv32_linux_process_current();
    caribe_area_view_t *view, *source;
    caribe_area_object_t *object;
    uint32_t i, j, slot, object_slot, needed, available;
    int error = 0, creating;
    if (request != CARIBE_AREA_CALL) return AREA_NOTTY;
    if (!process || process->pmap == PMAP_NULL) return AREA_PERM;
    if (riscv32_copyin(user_arg, &arg, sizeof(arg))) return AREA_FAULT;
    if (arg.abi != CARIBE_AREA_ABI || arg.reserved[0] || arg.reserved[1] || arg.reserved[2]) return AREA_INVAL;
    area_init();
    simple_lock(&area_lock);
    if (arg.operation == CARIBE_AREA_STATS) {
        arg.objects = arg.views = arg.pages = 0;
        for (i = 0; i < CARIBE_AREA_MAX_OBJECTS; i++) arg.objects += objects[i].references != 0;
        for (i = 0; i < CARIBE_AREA_MAX_VIEWS; i++) arg.views += views[i].owner != 0;
        for (i = 0; i < CARIBE_AREA_POOL_PAGES; i++) arg.pages += page_used[i] != 0;
        arg.allocations = allocations;
        arg.frees = frees;
        goto output;
    }
    if (arg.operation == CARIBE_AREA_INFO || arg.operation == CARIBE_AREA_DELETE) {
        view = find_view(arg.id, process->pid);
        if (!view) { error = find_view(arg.id, 0) ? AREA_PERM : AREA_NOENT; goto done; }
        if (arg.operation == CARIBE_AREA_DELETE) { delete_view(view); goto done; }
        arg.address = view->address;
        arg.size = view->size;
        arg.protection = view->protection;
        arg.views = objects[view->object].references;
        memcpy(arg.name, view->name, sizeof(arg.name));
        goto output;
    }
    creating = arg.operation == CARIBE_AREA_CREATE;
    if (!creating && arg.operation != CARIBE_AREA_CLONE) { error = AREA_INVAL; goto done; }
    for (i = 0; i < sizeof(arg.name) && arg.name[i]; i++) { }
    if (i == sizeof(arg.name) || !arg.name[0] ||
        (arg.protection != VM_PROT_READ && arg.protection != (VM_PROT_READ | VM_PROT_WRITE))) {
        error = AREA_INVAL; goto done;
    }
    source = 0;
    object_slot = CARIBE_AREA_MAX_OBJECTS;
    if (!creating) {
        source = find_view(arg.source, process->pid);
        if (!source) source = find_view(arg.source, 0);
        if (!source) { error = AREA_NOENT; goto done; }
        if (arg.protection & ~source->protection) { error = AREA_PERM; goto done; }
        arg.size = source->size;
        object_slot = source->object;
    } else {
        if (!arg.size || arg.size > CARIBE_AREA_MAX_PAGES * 4096U) { error = AREA_INVAL; goto done; }
        arg.size = (arg.size + 4095U) & ~4095U;
        for (i = 0; i < CARIBE_AREA_MAX_OBJECTS; i++) {
            if (!objects[i].count) { object_slot = i; break; }
        }
        if (object_slot == CARIBE_AREA_MAX_OBJECTS) { error = AREA_NOSPC; goto done; }
        available = 0;
        for (i = 0; i < CARIBE_AREA_POOL_PAGES; i++) available += !page_used[i];
        if (available < arg.size / 4096U) { error = AREA_NOMEM; goto done; }
    }
    for (slot = 0; slot < CARIBE_AREA_MAX_VIEWS; slot++) {
        if (!views[slot].owner && views[slot].generation < 0x00ffffffU) break;
    }
    if (slot == CARIBE_AREA_MAX_VIEWS) { error = AREA_NOSPC; goto done; }
    error = choose_address(process->pmap, &arg);
    if (error) goto done;
    object = &objects[object_slot];
    needed = arg.size / 4096U;
    if (creating) {
        for (i = 0, j = 0; i < CARIBE_AREA_POOL_PAGES && j < needed; i++) {
            if (page_used[i]) continue;
            memset(backing[i], 0, sizeof(backing[i]));
            page_used[i] = 1;
            object->pages[j++] = i;
            allocations++;
        }
        object->count = needed;
    }
    for (i = 0; i < needed; i++) {
        uint32_t address = arg.address + i * 4096U;
        ppnum_t physical = area_ppnum(object->pages[i]);
        pmap_enter(process->pmap, address, physical, arg.protection, arg.protection, 0, TRUE);
        if ((pmap_extract(process->pmap, address) >> PAGE_SHIFT) != physical) {
            error = AREA_NOMEM;
            goto rollback;
        }
    }
    view = &views[slot];
    view->generation++;
    arg.id = (view->generation << 7) | (slot + 1U);
    if (riscv32_copyout(&arg, user_arg, sizeof(arg))) { error = AREA_FAULT; goto rollback; }
    view->handle = arg.id;
    view->owner = process->pid;
    view->pmap = process->pmap;
    view->object = object_slot;
    view->address = arg.address;
    view->size = arg.size;
    view->protection = arg.protection;
    memcpy(view->name, arg.name, sizeof(arg.name));
    object->references++;
    __sync_synchronize();
    goto done;
rollback:
    pmap_remove(process->pmap, arg.address, arg.address + arg.size);
    if (creating) free_object(object_slot);
    goto done;
output:
    error = riscv32_copyout(&arg, user_arg, sizeof(arg)) ? AREA_FAULT : 0;
done:
    simple_unlock(&area_lock);
    return error;
}
