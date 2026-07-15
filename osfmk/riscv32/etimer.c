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
#include <mach/mach_types.h>
#include <kern/clock.h>
#include <kern/etimer.h>
#include <kern/processor.h>
#include <kern/spl.h>
#include <kern/thread.h>
#include <kern/timer_queue.h>
#include <machine/cpu_data.h>
#include <machine/machine_routines.h>

void etimer_timer_expire(void *arg);

void
etimer_intr(int user_mode, uint64_t iaddr)
{
	uint64_t abstime;
	rtclock_timer_t *mytimer;
	cpu_data_t *cdp;

	(void)user_mode;
	(void)iaddr;

	cdp = current_cpu_datap();
	abstime = mach_absolute_time();
	mytimer = &cdp->rtclock_timer;

	if (mytimer->deadline <= abstime) {
		mytimer->has_expired = TRUE;
		mytimer->deadline = timer_queue_expire(&mytimer->queue,
		    abstime);
		mytimer->has_expired = FALSE;
		mytimer->when_set = mach_absolute_time();
	}

	etimer_resync_deadlines();
}

void
etimer_set_deadline(uint64_t deadline)
{
	rtclock_timer_t *mytimer;
	spl_t s;

	s = splclock();
	mytimer = &current_cpu_datap()->rtclock_timer;
	mytimer->deadline = deadline;
	mytimer->when_set = mach_absolute_time();
	etimer_resync_deadlines();
	splx(s);
}

void
etimer_resync_deadlines(void)
{
	uint64_t deadline = EndOfAllTime;
	rtclock_timer_t *mytimer;
	spl_t s;

	s = splclock();
	mytimer = &current_cpu_datap()->rtclock_timer;
	if (!mytimer->has_expired &&
	    0 < mytimer->deadline && mytimer->deadline < EndOfAllTime) {
		deadline = mytimer->deadline;
	}
	(void)setPop(deadline);
	splx(s);
}

void
etimer_timer_expire(void *arg)
{
	rtclock_timer_t *mytimer;
	uint64_t abstime;

	(void)arg;
	mytimer = &current_cpu_datap()->rtclock_timer;
	abstime = mach_absolute_time();
	mytimer->has_expired = TRUE;
	mytimer->deadline = timer_queue_expire(&mytimer->queue, abstime);
	mytimer->has_expired = FALSE;
	mytimer->when_set = mach_absolute_time();
	etimer_resync_deadlines();
}

uint64_t
timer_call_slop(uint64_t deadline)
{
	uint64_t now = mach_absolute_time();

	if (deadline <= now) {
		return 0;
	}
	return (deadline - now) >> 3;
}

mpqueue_head_t *
timer_queue_assign(uint64_t deadline)
{
	cpu_data_t *cdp = current_cpu_datap();

	if (deadline < cdp->rtclock_timer.deadline) {
		etimer_set_deadline(deadline);
	}
	return &cdp->rtclock_timer.queue;
}

void
timer_queue_cancel(mpqueue_head_t *queue, uint64_t deadline,
    uint64_t new_deadline)
{
	if (queue == &current_cpu_datap()->rtclock_timer.queue &&
	    deadline < new_deadline) {
		etimer_set_deadline(new_deadline);
	}
}

uint32_t
etimer_queue_migrate(int target_cpu)
{
	(void)target_cpu;
	return 0;
}
