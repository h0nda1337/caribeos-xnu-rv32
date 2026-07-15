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
