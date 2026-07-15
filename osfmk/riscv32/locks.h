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
#ifndef _RISCV32_LOCKS_H_
#define _RISCV32_LOCKS_H_

#include <sys/appleapiopts.h>
#include <kern/kern_types.h>
#include <stdint.h>

#ifdef MACH_KERNEL_PRIVATE
extern unsigned int LcksOpts;

#define enaLkDeb		0x00000001
#define enaLkStat		0x00000002
#endif

typedef struct {
	volatile uintptr_t	interlock;
} lck_spin_t;

#define LCK_SPIN_TAG_DESTROYED	0x00002007

struct _lck_mtx_ext_;

typedef struct _lck_mtx_ {
	volatile uintptr_t	lck_mtx_owner;
	volatile uint16_t	lck_mtx_waiters;
	volatile uint8_t	lck_mtx_pri;
	volatile uint8_t	lck_mtx_promoted;
	volatile uint8_t	lck_mtx_is_ext;
	volatile uint8_t	lck_mtx_pad[3];
	struct _lck_mtx_ext_	*lck_mtx_ptr;
	uint32_t		lck_mtx_tag;
	uint32_t		lck_mtx_state;
} lck_mtx_t;

#define LCK_MTX_TAG_INDIRECT	0x07ff1007
#define LCK_MTX_TAG_DESTROYED	0x07fe2007

typedef struct {
	unsigned int	type;
	vm_offset_t	pc;
	vm_offset_t	thread;
} lck_mtx_deb_t;

typedef struct {
	unsigned int	lck_mtx_stat_data;
} lck_mtx_stat_t;

typedef struct _lck_mtx_ext_ {
	lck_mtx_t		lck_mtx;
	struct _lck_grp_	*lck_mtx_grp;
	unsigned int		lck_mtx_attr;
	lck_mtx_deb_t		lck_mtx_deb;
	uint64_t		lck_mtx_stat;
} lck_mtx_ext_t;

#define MUTEX_TAG		0x4d4d
#define LCK_MTX_ATTR_DEBUG	0x1
#define LCK_MTX_ATTR_DEBUGb	0
#define LCK_MTX_ATTR_STAT	0x2
#define LCK_MTX_ATTR_STATb	1

typedef struct _lck_rw_t_internal_ {
	volatile uint16_t	lck_rw_shared_count;
	volatile uint8_t	lck_rw_interlock;
	volatile uint8_t	lck_rw_priv_excl:1,
				lck_rw_want_upgrade:1,
				lck_rw_want_write:1,
				lck_r_waiting:1,
				lck_w_waiting:1,
				lck_rw_can_sleep:1,
				lck_rw_padb6:2;
	uint32_t		lck_rw_tag;
	uint32_t		lck_rw_pad8;
} lck_rw_t;

#define LCK_RW_ATTR_DEBUG	0x1
#define LCK_RW_ATTR_DEBUGb	0
#define LCK_RW_ATTR_STAT	0x2
#define LCK_RW_ATTR_STATb	1
#define LCK_RW_ATTR_READ_PRI	0x3
#define LCK_RW_ATTR_READ_PRIb	2
#define LCK_RW_ATTR_DIS_THREAD	0x40000000
#define LCK_RW_ATTR_DIS_THREADb	30
#define LCK_RW_ATTR_DIS_MYLOCK	0x10000000
#define LCK_RW_ATTR_DIS_MYLOCKb	28

#define LCK_RW_TAG_DESTROYED	0x00002007

#endif /* _RISCV32_LOCKS_H_ */
