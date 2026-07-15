#ifndef _MACH_LOCK_SET_SERVER_H_
#define _MACH_LOCK_SET_SERVER_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/mig.h>

extern const struct mig_subsystem lock_set_subsystem;

kern_return_t lock_set_create(task_t task, lock_set_t *new_lock_set,
    int n_ulocks, int policy);
kern_return_t lock_set_destroy(task_t task, lock_set_t lock_set);
kern_return_t lock_acquire(lock_set_t lock_set, int lock_id);
kern_return_t lock_release(lock_set_t lock_set, int lock_id);
kern_return_t lock_try(lock_set_t lock_set, int lock_id);
kern_return_t lock_make_stable(lock_set_t lock_set, int lock_id);
kern_return_t lock_handoff(lock_set_t lock_set, int lock_id);
kern_return_t lock_handoff_accept(lock_set_t lock_set, int lock_id);

#endif /* _MACH_LOCK_SET_SERVER_H_ */
