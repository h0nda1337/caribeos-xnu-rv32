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
#include <mach/clock_types.h>
#include <mach/mach_time.h>
#include <mach/mach_types.h>
#include <kern/clock.h>
#include <kern/etimer.h>
#include <machine/bootinfo.h>
#include <machine/cpu_data.h>
#include <machine/machine_cpu.h>
#include <machine/machine_routines.h>
#include <machine/timer.h>
#include <riscv32/sbi.h>
#include <stdint.h>

#define RISCV32_DEFAULT_TIMEBASE_HZ	10000000ULL
#define RISCV32_NSEC_PER_SEC		1000000000ULL
#define RISCV32_SIE_STIE		0x00000020U
#define RISCV32_SSTATUS_SPP		0x00000100U

extern int rtclock_config(void);
extern int rtclock_init(void);

kern_return_t rtclock_gettime(mach_timespec_t *cur_time);
kern_return_t rtclock_getattr(clock_flavor_t flavor, clock_attr_t attr,
    mach_msg_type_number_t *count);
kern_return_t calend_gettime(mach_timespec_t *cur_time);
kern_return_t calend_getattr(clock_flavor_t flavor, clock_attr_t attr,
    mach_msg_type_number_t *count);

static uint64_t riscv32_timebase_hz = RISCV32_DEFAULT_TIMEBASE_HZ;

__attribute__((weak)) struct clock_ops sysclk_ops = {
	rtclock_config,
	rtclock_init,
	rtclock_gettime,
	rtclock_getattr
};

__attribute__((weak)) struct clock_ops calend_ops = {
	0,
	0,
	calend_gettime,
	calend_getattr
};

struct clock clock_list[] = {
	{ &sysclk_ops, 0, 0 },
	{ &calend_ops, 0, 0 }
};

int clock_count = sizeof(clock_list) / sizeof(clock_list[0]);

static void
riscv32_timer_configure_timebase(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();

	if (bootinfo != (const riscv32_bootinfo_t *)0 &&
	    bootinfo->timebase_hz != 0) {
		riscv32_timebase_hz = bootinfo->timebase_hz;
	}
}

static uint64_t
riscv32_ticks_to_abstime(uint64_t ticks)
{
	uint64_t secs = ticks / riscv32_timebase_hz;
	uint64_t rem = ticks % riscv32_timebase_hz;

	return secs * RISCV32_NSEC_PER_SEC +
	    (rem * RISCV32_NSEC_PER_SEC) / riscv32_timebase_hz;
}

static uint64_t
riscv32_abstime_to_ticks(uint64_t abstime)
{
	uint64_t secs = abstime / RISCV32_NSEC_PER_SEC;
	uint64_t rem = abstime % RISCV32_NSEC_PER_SEC;

	return secs * riscv32_timebase_hz +
	    (rem * riscv32_timebase_hz) / RISCV32_NSEC_PER_SEC;
}

static void
riscv32_abstime_to_timespec(uint64_t abstime, clock_sec_t *secs,
    clock_nsec_t *nanosecs)
{
	*secs = (clock_sec_t)(abstime / RISCV32_NSEC_PER_SEC);
	*nanosecs = (clock_nsec_t)(abstime % RISCV32_NSEC_PER_SEC);
}

uint64_t
riscv32_timer_read(void)
{
	return riscv32_sbi_rdtime();
}

void
riscv32_timer_enable(void)
{
	uint32_t mask = RISCV32_SIE_STIE;

	__asm__ volatile("csrs sie, %0" :: "r"(mask) : "memory");
}

int
rtclock_config(void)
{
	return 1;
}

int
rtclock_init(void)
{
	cpu_init();
	riscv32_timer_configure_timebase();
	clock_timebase_init();
	ml_init_lock_timeout();
	ml_init_delay_spin_threshold(10);
	riscv32_timer_enable();
	etimer_resync_deadlines();
	return 1;
}

void
clock_get_system_microtime(clock_sec_t *secs, clock_usec_t *microsecs)
{
	uint64_t now = mach_absolute_time();
	clock_nsec_t nsecs;

	riscv32_abstime_to_timespec(now, secs, &nsecs);
	*microsecs = nsecs / 1000U;
}

void
clock_get_system_nanotime(clock_sec_t *secs, clock_nsec_t *nanosecs)
{
	riscv32_abstime_to_timespec(mach_absolute_time(), secs, nanosecs);
}

__attribute__((weak)) kern_return_t
rtclock_gettime(mach_timespec_t *cur_time)
{
	clock_sec_t secs;
	clock_nsec_t nanosecs;

	clock_get_system_nanotime(&secs, &nanosecs);
	cur_time->tv_sec = (unsigned int)secs;
	cur_time->tv_nsec = nanosecs;
	return KERN_SUCCESS;
}

__attribute__((weak)) kern_return_t
calend_gettime(mach_timespec_t *cur_time)
{
	return rtclock_gettime(cur_time);
}

__attribute__((weak)) kern_return_t
rtclock_getattr(clock_flavor_t flavor, clock_attr_t attr,
    mach_msg_type_number_t *count)
{
	if (*count != 1) {
		return KERN_FAILURE;
	}

	switch (flavor) {
	case CLOCK_GET_TIME_RES:
	case CLOCK_ALARM_CURRES:
	case CLOCK_ALARM_MINRES:
	case CLOCK_ALARM_MAXRES:
		*(clock_res_t *)attr = RISCV32_NSEC_PER_SEC / 100U;
		return KERN_SUCCESS;
	default:
		return KERN_INVALID_VALUE;
	}
}

__attribute__((weak)) kern_return_t
calend_getattr(clock_flavor_t flavor, clock_attr_t attr,
    mach_msg_type_number_t *count)
{
	if (flavor == CLOCK_GET_TIME_RES) {
		return rtclock_getattr(flavor, attr, count);
	}
	if (*count != 1) {
		return KERN_FAILURE;
	}
	switch (flavor) {
	case CLOCK_ALARM_CURRES:
	case CLOCK_ALARM_MINRES:
	case CLOCK_ALARM_MAXRES:
		*(clock_res_t *)attr = 0;
		return KERN_SUCCESS;
	default:
		return KERN_INVALID_VALUE;
	}
}

void
clock_gettimeofday_set_commpage(uint64_t abstime, uint64_t epoch,
    uint64_t offset, clock_sec_t *secs, clock_usec_t *microsecs)
{
	clock_nsec_t nsecs;

	riscv32_abstime_to_timespec(abstime + offset, secs, &nsecs);
	*secs += (clock_sec_t)epoch;
	*microsecs = nsecs / 1000U;
}

void
clock_timebase_info(mach_timebase_info_t info)
{
	info->numer = 1;
	info->denom = 1;
}

void
rtclock_intr(riscv_saved_state_t *state)
{
	int user_mode = (state->csr.sstatus & RISCV32_SSTATUS_SPP) == 0;

	etimer_intr(user_mode, state->thread.pc);
}

int
setPop(uint64_t time)
{
	uint64_t now = mach_absolute_time();
	uint64_t delta = 0;

	if (time == 0 || time == EndOfAllTime) {
		current_cpu_datap()->rtclock_timer.deadline = EndOfAllTime;
		riscv32_sbi_set_timer(UINT64_MAX);
		return 0;
	}

	current_cpu_datap()->rtclock_timer.deadline = time;
	riscv32_sbi_set_timer(riscv32_abstime_to_ticks(time));
	if (time > now) {
		delta = time - now;
	}
	if (delta > INT32_MAX) {
		return INT32_MAX;
	}
	return (int)delta;
}

uint64_t
mach_absolute_time(void)
{
	return riscv32_ticks_to_abstime(riscv32_timer_read());
}

void
clock_interval_to_absolutetime_interval(uint32_t interval,
    uint32_t scale_factor, uint64_t *result)
{
	*result = (uint64_t)interval * scale_factor;
}

void
absolutetime_to_microtime(uint64_t abstime, clock_sec_t *secs,
    clock_usec_t *microsecs)
{
	clock_nsec_t nsecs;

	riscv32_abstime_to_timespec(abstime, secs, &nsecs);
	*microsecs = nsecs / 1000U;
}

void
absolutetime_to_nanotime(uint64_t abstime, clock_sec_t *secs,
    clock_nsec_t *nanosecs)
{
	riscv32_abstime_to_timespec(abstime, secs, nanosecs);
}

void
nanotime_to_absolutetime(clock_sec_t secs, clock_nsec_t nanosecs,
    uint64_t *result)
{
	*result = ((uint64_t)secs * RISCV32_NSEC_PER_SEC) + nanosecs;
}

void
absolutetime_to_nanoseconds(uint64_t abstime, uint64_t *result)
{
	*result = abstime;
}

void
nanoseconds_to_absolutetime(uint64_t nanoseconds, uint64_t *result)
{
	*result = nanoseconds;
}
