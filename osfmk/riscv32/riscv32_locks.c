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
 * port of Apple XNU 2050.48.11 on 2026-07-14.
 */

#line 1
#include <kern/debug.h>
#include <kern/kalloc.h>
#include <kern/locks.h>
#include <kern/simple_lock.h>
#include <kern/thread.h>
#include <machine/machine_cpu.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

unsigned int LcksOpts = 0;

static inline uintptr_t
riscv32_lock_owner(void)
{
	thread_t thread = current_thread();
	return thread != THREAD_NULL ? (uintptr_t)thread : 1;
}

void
hw_lock_init(hw_lock_t lock)
{
	lock->lock_data = 0;
}

unsigned int
hw_lock_try(hw_lock_t lock)
{
	return __sync_bool_compare_and_swap(&lock->lock_data, 0,
	    (uint32_t)riscv32_lock_owner());
}

void
hw_lock_lock(hw_lock_t lock)
{
	while (!hw_lock_try(lock)) {
		cpu_pause();
	}
}

unsigned int
hw_lock_to(hw_lock_t lock, unsigned int timeout)
{
	while (timeout-- != 0) {
		if (hw_lock_try(lock)) {
			return 1;
		}
		cpu_pause();
	}
	return 0;
}

void
hw_lock_unlock(hw_lock_t lock)
{
	__sync_lock_release(&lock->lock_data);
}

unsigned int
hw_lock_held(hw_lock_t lock)
{
	return lock->lock_data != 0;
}

uint32_t
hw_atomic_add(volatile uint32_t *dest, uint32_t delt)
{
	return __sync_add_and_fetch(dest, delt);
}

uint32_t
hw_atomic_sub(volatile uint32_t *dest, uint32_t delt)
{
	return __sync_sub_and_fetch(dest, delt);
}

uint32_t
hw_atomic_or(volatile uint32_t *dest, uint32_t mask)
{
	return __sync_or_and_fetch(dest, mask);
}

uint32_t
hw_atomic_and(volatile uint32_t *dest, uint32_t mask)
{
	return __sync_and_and_fetch(dest, mask);
}

void
hw_atomic_or_noret(volatile uint32_t *dest, uint32_t mask)
{
	(void)__sync_or_and_fetch(dest, mask);
}

void
hw_atomic_and_noret(volatile uint32_t *dest, uint32_t mask)
{
	(void)__sync_and_and_fetch(dest, mask);
}

uint32_t
hw_compare_and_store(uint32_t oldval, uint32_t newval, volatile uint32_t *dest)
{
	return __sync_bool_compare_and_swap(dest, oldval, newval);
}

void
hw_queue_atomic(unsigned int *anchor, unsigned int *elem, unsigned int disp)
{
	volatile uint32_t *head = (volatile uint32_t *)anchor;
	volatile uint32_t *next = (volatile uint32_t *)((uintptr_t)elem + disp);
	uint32_t old_head;

	do {
		old_head = *head;
		*next = old_head;
	} while (!hw_compare_and_store(old_head, (uint32_t)(uintptr_t)elem, head));
}

void
hw_queue_atomic_list(unsigned int *anchor, unsigned int *first,
    unsigned int *last, unsigned int disp)
{
	volatile uint32_t *head = (volatile uint32_t *)anchor;
	volatile uint32_t *last_next =
	    (volatile uint32_t *)((uintptr_t)last + disp);
	uint32_t old_head;

	do {
		old_head = *head;
		*last_next = old_head;
	} while (!hw_compare_and_store(old_head, (uint32_t)(uintptr_t)first,
	    head));
}

unsigned int *
hw_dequeue_atomic(unsigned int *anchor, unsigned int disp)
{
	volatile uint32_t *head = (volatile uint32_t *)anchor;
	unsigned int *elem;
	uint32_t old_head;
	uint32_t new_head;

	do {
		old_head = *head;
		if (old_head == 0) {
			return NULL;
		}
		elem = (unsigned int *)(uintptr_t)old_head;
		new_head = *(uint32_t *)((uintptr_t)elem + disp);
	} while (!hw_compare_and_store(old_head, new_head, head));

	return elem;
}

void
usimple_lock_init(usimple_lock_t lock, unsigned short tag)
{
	(void)tag;
	hw_lock_init(&lock->interlock);
}

void
usimple_lock(usimple_lock_t lock)
{
	hw_lock_lock(&lock->interlock);
}

void
usimple_unlock(usimple_lock_t lock)
{
	hw_lock_unlock(&lock->interlock);
}

unsigned int
usimple_lock_try(usimple_lock_t lock)
{
	return hw_lock_try(&lock->interlock);
}

lck_spin_t *
lck_spin_alloc_init(lck_grp_t *grp, lck_attr_t *attr)
{
	lck_spin_t *lock = kalloc(sizeof(*lock));
	if (lock != NULL) {
		lck_spin_init(lock, grp, attr);
	}
	return lock;
}

void
lck_spin_free(lck_spin_t *lock, lck_grp_t *grp)
{
	lck_spin_destroy(lock, grp);
	kfree(lock, sizeof(*lock));
}

void
lck_spin_init(lck_spin_t *lock, lck_grp_t *grp, lck_attr_t *attr)
{
	(void)attr;
	lock->interlock = 0;
	lck_grp_reference(grp);
	lck_grp_lckcnt_incr(grp, LCK_TYPE_SPIN);
}

void
lck_spin_destroy(lck_spin_t *lock, lck_grp_t *grp)
{
	lock->interlock = LCK_SPIN_TAG_DESTROYED;
	lck_grp_lckcnt_decr(grp, LCK_TYPE_SPIN);
	lck_grp_deallocate(grp);
}

void
lck_spin_lock(lck_spin_t *lock)
{
	while (!__sync_bool_compare_and_swap(&lock->interlock, 0,
	    riscv32_lock_owner())) {
		cpu_pause();
	}
}

void
lck_spin_unlock(lck_spin_t *lock)
{
	__sync_lock_release(&lock->interlock);
}

boolean_t
lck_spin_try_lock(lck_spin_t *lock)
{
	return __sync_bool_compare_and_swap(&lock->interlock, 0,
	    riscv32_lock_owner());
}

static lck_mtx_t *
riscv32_mtx_real(lck_mtx_t *lock)
{
	return (lock->lck_mtx_tag == LCK_MTX_TAG_INDIRECT) ?
	    &lock->lck_mtx_ptr->lck_mtx : lock;
}

lck_mtx_t *
lck_mtx_alloc_init(lck_grp_t *grp, lck_attr_t *attr)
{
	lck_mtx_t *lock = kalloc(sizeof(*lock));
	if (lock != NULL) {
		lck_mtx_init(lock, grp, attr);
	}
	return lock;
}

void
lck_mtx_free(lck_mtx_t *lock, lck_grp_t *grp)
{
	lck_mtx_destroy(lock, grp);
	kfree(lock, sizeof(*lock));
}

void
lck_mtx_init_ext(lck_mtx_t *lock, lck_mtx_ext_t *lock_ext,
    lck_grp_t *grp, lck_attr_t *attr)
{
	memset(lock_ext, 0, sizeof(*lock_ext));
	lck_mtx_init(&lock_ext->lck_mtx, grp, attr);
	lock->lck_mtx_tag = LCK_MTX_TAG_INDIRECT;
	lock->lck_mtx_ptr = lock_ext;
	lock->lck_mtx_is_ext = 1;
	lock_ext->lck_mtx_grp = grp;
	lock_ext->lck_mtx_attr = attr != LCK_ATTR_NULL ? attr->lck_attr_val : 0;
}

void
lck_mtx_init(lck_mtx_t *lock, lck_grp_t *grp, lck_attr_t *attr)
{
	(void)attr;
	memset(lock, 0, sizeof(*lock));
	lock->lck_mtx_tag = MUTEX_TAG;
	lck_grp_reference(grp);
	lck_grp_lckcnt_incr(grp, LCK_TYPE_MTX);
}

void
lck_mtx_destroy(lck_mtx_t *lock, lck_grp_t *grp)
{
	riscv32_mtx_real(lock)->lck_mtx_tag = LCK_MTX_TAG_DESTROYED;
	lck_grp_lckcnt_decr(grp, LCK_TYPE_MTX);
	lck_grp_deallocate(grp);
}

void
lck_mtx_lock(lck_mtx_t *lock)
{
	lck_mtx_t *mutex = riscv32_mtx_real(lock);

	while (!__sync_bool_compare_and_swap(&mutex->lck_mtx_owner, 0,
	    riscv32_lock_owner())) {
		cpu_pause();
	}
}

void
lck_mtx_unlock(lck_mtx_t *lock)
{
	lck_mtx_t *mutex = riscv32_mtx_real(lock);
	__sync_lock_release(&mutex->lck_mtx_owner);
}

boolean_t
lck_mtx_try_lock(lck_mtx_t *lock)
{
	lck_mtx_t *mutex = riscv32_mtx_real(lock);
	return __sync_bool_compare_and_swap(&mutex->lck_mtx_owner, 0,
	    riscv32_lock_owner());
}

boolean_t
lck_mtx_ilk_unlock(lck_mtx_t *lock)
{
	lck_mtx_unlock(lock);
	return TRUE;
}

void
lck_mtx_assert(lck_mtx_t *lock, unsigned int type)
{
	boolean_t held = riscv32_mtx_real(lock)->lck_mtx_owner != 0;

	if ((type == LCK_MTX_ASSERT_OWNED && !held) ||
	    (type == LCK_MTX_ASSERT_NOTOWNED && held)) {
		panic("riscv32: lck_mtx_assert failed");
	}
}

static void
riscv32_rw_interlock(lck_rw_t *lock)
{
	while (__sync_lock_test_and_set(&lock->lck_rw_interlock, 1) != 0) {
		cpu_pause();
	}
}

static void
riscv32_rw_unlock_interlock(lck_rw_t *lock)
{
	__sync_lock_release(&lock->lck_rw_interlock);
}

lck_rw_t *
lck_rw_alloc_init(lck_grp_t *grp, lck_attr_t *attr)
{
	lck_rw_t *lock = kalloc(sizeof(*lock));
	if (lock != NULL) {
		lck_rw_init(lock, grp, attr);
	}
	return lock;
}

void
lck_rw_free(lck_rw_t *lock, lck_grp_t *grp)
{
	lck_rw_destroy(lock, grp);
	kfree(lock, sizeof(*lock));
}

void
lck_rw_init(lck_rw_t *lock, lck_grp_t *grp, lck_attr_t *attr)
{
	(void)attr;
	memset(lock, 0, sizeof(*lock));
	lock->lck_rw_can_sleep = TRUE;
	lck_grp_reference(grp);
	lck_grp_lckcnt_incr(grp, LCK_TYPE_RW);
}

void
lck_rw_destroy(lck_rw_t *lock, lck_grp_t *grp)
{
	lock->lck_rw_tag = LCK_RW_TAG_DESTROYED;
	lck_grp_lckcnt_decr(grp, LCK_TYPE_RW);
	lck_grp_deallocate(grp);
}

void
lck_rw_lock_shared(lck_rw_t *lock)
{
	for (;;) {
		riscv32_rw_interlock(lock);
		if (!lock->lck_rw_want_write) {
			lock->lck_rw_shared_count++;
			riscv32_rw_unlock_interlock(lock);
			return;
		}
		riscv32_rw_unlock_interlock(lock);
		cpu_pause();
	}
}

void
lck_rw_unlock_shared(lck_rw_t *lock)
{
	riscv32_rw_interlock(lock);
	if (lock->lck_rw_shared_count == 0) {
		panic("riscv32: lck_rw_unlock_shared without reader");
	}
	lock->lck_rw_shared_count--;
	riscv32_rw_unlock_interlock(lock);
}

void
lck_rw_lock_exclusive(lck_rw_t *lock)
{
	for (;;) {
		riscv32_rw_interlock(lock);
		if (!lock->lck_rw_want_write &&
		    lock->lck_rw_shared_count == 0) {
			lock->lck_rw_want_write = TRUE;
			riscv32_rw_unlock_interlock(lock);
			return;
		}
		riscv32_rw_unlock_interlock(lock);
		cpu_pause();
	}
}

void
lck_rw_unlock_exclusive(lck_rw_t *lock)
{
	riscv32_rw_interlock(lock);
	lock->lck_rw_want_write = FALSE;
	riscv32_rw_unlock_interlock(lock);
}

void
lck_rw_lock(lck_rw_t *lock, lck_rw_type_t type)
{
	if (type == LCK_RW_TYPE_SHARED) {
		lck_rw_lock_shared(lock);
	} else {
		lck_rw_lock_exclusive(lock);
	}
}

void
lck_rw_unlock(lck_rw_t *lock, lck_rw_type_t type)
{
	if (type == LCK_RW_TYPE_SHARED) {
		lck_rw_unlock_shared(lock);
	} else {
		lck_rw_unlock_exclusive(lock);
	}
}

lck_rw_type_t
lck_rw_done(lck_rw_t *lock)
{
	lck_rw_type_t type;

	riscv32_rw_interlock(lock);
	type = lock->lck_rw_want_write ? LCK_RW_TYPE_EXCLUSIVE :
	    LCK_RW_TYPE_SHARED;
	riscv32_rw_unlock_interlock(lock);
	lck_rw_unlock(lock, type);
	return type;
}

boolean_t
lck_rw_try_lock_shared(lck_rw_t *lock)
{
	boolean_t ok = FALSE;

	riscv32_rw_interlock(lock);
	if (!lock->lck_rw_want_write) {
		lock->lck_rw_shared_count++;
		ok = TRUE;
	}
	riscv32_rw_unlock_interlock(lock);
	return ok;
}

boolean_t
lck_rw_try_lock_exclusive(lck_rw_t *lock)
{
	boolean_t ok = FALSE;

	riscv32_rw_interlock(lock);
	if (!lock->lck_rw_want_write && lock->lck_rw_shared_count == 0) {
		lock->lck_rw_want_write = TRUE;
		ok = TRUE;
	}
	riscv32_rw_unlock_interlock(lock);
	return ok;
}

boolean_t
lck_rw_try_lock(lck_rw_t *lock, lck_rw_type_t type)
{
	return (type == LCK_RW_TYPE_SHARED) ?
	    lck_rw_try_lock_shared(lock) : lck_rw_try_lock_exclusive(lock);
}

boolean_t
lck_rw_lock_shared_to_exclusive(lck_rw_t *lock)
{
	boolean_t ok = FALSE;

	riscv32_rw_interlock(lock);
	if (lock->lck_rw_shared_count != 0) {
		lock->lck_rw_shared_count--;
	}
	if (lock->lck_rw_shared_count == 0 && !lock->lck_rw_want_write) {
		lock->lck_rw_want_write = TRUE;
		ok = TRUE;
	}
	riscv32_rw_unlock_interlock(lock);
	return ok;
}

void
lck_rw_lock_exclusive_to_shared(lck_rw_t *lock)
{
	riscv32_rw_interlock(lock);
	lock->lck_rw_want_write = FALSE;
	lock->lck_rw_shared_count++;
	riscv32_rw_unlock_interlock(lock);
}

void
lck_rw_assert(lck_rw_t *lock, unsigned int type)
{
	boolean_t shared = lock->lck_rw_shared_count != 0;
	boolean_t exclusive = lock->lck_rw_want_write;

	if (((type & LCK_RW_ASSERT_SHARED) && !shared) ||
	    ((type & LCK_RW_ASSERT_EXCLUSIVE) && !exclusive)) {
		panic("riscv32: lck_rw_assert failed");
	}
}
