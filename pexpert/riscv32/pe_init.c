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
#include <mach/boolean.h>
#include <kern/clock.h>
#include <machine/bootinfo.h>
#include <machine/machine_routines.h>
#include <pexpert/boot.h>
#include <pexpert/device_tree.h>
#include <pexpert/pexpert.h>
#include <pexpert/protos.h>

#if defined(RISCV) || defined(RISCV32) || defined(__riscv)
extern void StartIOKit(void *p1, void *p2, void *p3, void *p4);
extern void riscv32_early_puts(const char *s);
extern void riscv32_iokit_stage0_note_real_startiokit(void);
extern void riscv32_iokit_stage0_note_fdt_legacy_skipped(void);
#endif

PE_state_t PE_state;
clock_frequency_info_t gPEClockFrequencyInfo;
void *gPEEFISystemTable;
void *gPEEFIRuntimeServices;

static timebase_callback_func gTimebaseCallback;

static uint32_t
riscv32_pe_read_be32(const void *ptr)
{
	const unsigned char *p = (const unsigned char *)ptr;

	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	    ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static boolean_t
riscv32_pe_is_fdt(const void *ptr)
{
	if (ptr == (const void *)0) {
		return FALSE;
	}
	return riscv32_pe_read_be32(ptr) == 0xd00dfeedU;
}

int
PE_initialize_console(PE_Video *info, int op)
{
	(void)op;

	if (info != (PE_Video *)0) {
		PE_state.video = *info;
	}
	return 0;
}

void
PE_init_iokit(void)
{
#if defined(RISCV) || defined(RISCV32) || defined(__riscv)
	riscv32_early_puts("[iokit-real] PE_init_iokit: StartIOKit enter\n");
#endif
	StartIOKit(PE_state.deviceTreeHead, PE_state.bootArgs,
	    gPEEFIRuntimeServices, (void *)0);
#if defined(RISCV) || defined(RISCV32) || defined(__riscv)
	riscv32_iokit_stage0_note_real_startiokit();
	riscv32_early_puts("[iokit-real] PE_init_iokit: StartIOKit returned\n");
#endif
}

void
PE_init_platform(boolean_t vm_initialized, void *_args)
{
	boot_args *args = (boot_args *)_args;

	if (args == (boot_args *)0) {
		args = riscv32_boot_args();
	}

	if (PE_state.initialized == FALSE) {
		PE_state.initialized = TRUE;
		PE_state.bootArgs = args;
		PE_state.deviceTreeHead =
		    (void *)(uintptr_t)ml_static_ptovirt(args->deviceTreeP);
		if (riscv32_pe_is_fdt(PE_state.deviceTreeHead)) {
			PE_state.deviceTreeHead = (void *)0;
#if defined(RISCV) || defined(RISCV32) || defined(__riscv)
			riscv32_iokit_stage0_note_fdt_legacy_skipped();
#endif
		}
		PE_state.video.v_baseAddr = args->Video.v_baseAddr;
		PE_state.video.v_rowBytes = args->Video.v_rowBytes;
		PE_state.video.v_width = args->Video.v_width;
		PE_state.video.v_height = args->Video.v_height;
		PE_state.video.v_depth = args->Video.v_depth;
		PE_state.video.v_display = args->Video.v_display;
	}

	if (!vm_initialized && PE_state.deviceTreeHead != (void *)0) {
		DTInit(PE_state.deviceTreeHead);
	}
}

void
PE_create_console(void)
{
	(void)PE_initialize_console(&PE_state.video, kPETextMode);
}

int
PE_current_console(PE_Video *info)
{
	if (info != (PE_Video *)0) {
		*info = PE_state.video;
	}
	return 0;
}

void
PE_display_icon(unsigned int flags, const char *name)
{
	(void)flags;
	(void)name;
}

boolean_t
PE_get_hotkey(unsigned char key)
{
	(void)key;
	return FALSE;
}

void
PE_register_timebase_callback(timebase_callback_func callback)
{
	gTimebaseCallback = callback;
	PE_call_timebase_callback();
}

void
PE_call_timebase_callback(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	struct timebase_freq_t timebase_freq;

	timebase_freq.timebase_num = 1;
	timebase_freq.timebase_den = 10000000;
	if (bootinfo != (const riscv32_bootinfo_t *)0 &&
	    bootinfo->timebase_hz != 0) {
		timebase_freq.timebase_den = bootinfo->timebase_hz;
	}
	if (gTimebaseCallback != (timebase_callback_func)0) {
		gTimebaseCallback(&timebase_freq);
	}
}

static int
PE_stub_poll_input(unsigned int options, char *c)
{
	(void)options;
	*c = 0xff;
	return 1;
}

int (*PE_poll_input)(unsigned int options, char *c) = PE_stub_poll_input;

boolean_t
PE_reboot_on_panic(void)
{
	boot_args *args = (boot_args *)PE_state.bootArgs;

	return args != (boot_args *)0 &&
	    (args->flags & kBootArgsFlagRebootOnPanic) != 0;
}
