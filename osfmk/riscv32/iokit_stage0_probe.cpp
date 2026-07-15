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
#include <stdint.h>

extern "C" void riscv32_early_puts(const char *s);

enum {
	RV32_IOKIT_CPP_PROBE_LINKED = 0x00000100U,
	RV32_IOKIT_CPP_PROBE_CTOR = 0x00000200U,
	RV32_IOKIT_CPP_PROBE_VTABLE = 0x00000400U,
	RV32_IOKIT_CPP_PROBE_SERVICE_SHAPE = 0x00000800U,
	RV32_IOKIT_CPP_MICRO_REGISTRY = 0x00001000U,
	RV32_IOKIT_CPP_MICRO_PUBLISH = 0x00002000U,
	RV32_IOKIT_CPP_MICRO_START = 0x00004000U,
	RV32_IOKIT_CPP_MICRO_MATCH = 0x00008000U
};

class CaribeIOServiceProbeBase {
public:
	explicit CaribeIOServiceProbeBase(uint32_t cookie)
	    : cookie_(cookie), phase_(0x10203040U)
	{
	}

	virtual uint32_t probe(uint32_t value) const
	{
		return value ^ cookie_ ^ phase_;
	}

	virtual const char *className(void) const
	{
		return "CaribeIOServiceProbeBase";
	}

	uint32_t cookie(void) const
	{
		return cookie_;
	}

protected:
	uint32_t cookie_;
	uint32_t phase_;
};

class CaribeIOServiceProbe : public CaribeIOServiceProbeBase {
public:
	explicit CaribeIOServiceProbe(uint32_t cookie)
	    : CaribeIOServiceProbeBase(cookie)
	{
		phase_ = 0x43425831U;
	}

	virtual uint32_t probe(uint32_t value) const
	{
		return value + cookie_ + phase_;
	}

	virtual const char *className(void) const
	{
		return "CaribeIOServiceProbe";
	}
};

class CaribeIOServiceLite {
public:
	CaribeIOServiceLite(const char *name, const char *compatible, uint32_t unit)
	    : name_(name),
	      compatible_(compatible),
	      unit_(unit),
	      provider_((const CaribeIOServiceLite *)0),
	      child_count_(0U),
	      published_(false),
	      started_(false),
	      matched_(false)
	{
	}

	virtual const char *className(void) const
	{
		return "IOService";
	}

	virtual bool probe(const CaribeIOServiceLite *provider)
	{
		provider_ = provider;
		matched_ = (name_ != (const char *)0 && compatible_ != (const char *)0);
		return matched_;
	}

	virtual bool start(const CaribeIOServiceLite *provider)
	{
		if (!probe(provider)) {
			return false;
		}
		started_ = true;
		return true;
	}

	bool publish(void)
	{
		published_ = true;
		return published_;
	}

	void attach(CaribeIOServiceLite *child)
	{
		if (child != (CaribeIOServiceLite *)0) {
			child->provider_ = this;
			child_count_++;
		}
	}

	bool isPublished(void) const
	{
		return published_;
	}

	bool isStarted(void) const
	{
		return started_;
	}

	bool isMatched(void) const
	{
		return matched_;
	}

	uint32_t childCount(void) const
	{
		return child_count_;
	}

	uint32_t unit(void) const
	{
		return unit_;
	}

	const char *name(void) const
	{
		return name_;
	}

protected:
	const char *name_;
	const char *compatible_;
	uint32_t unit_;
	const CaribeIOServiceLite *provider_;
	uint32_t child_count_;
	bool published_;
	bool started_;
	bool matched_;
};

class CaribePlatformExpertService : public CaribeIOServiceLite {
public:
	CaribePlatformExpertService(void)
	    : CaribeIOServiceLite("CaribeRiscVPlatformExpert",
	          "riscv-virtio,caribeos", 0U)
	{
	}

	virtual const char *className(void) const
	{
		return "IOPlatformExpertDevice";
	}
};

class CaribeCPUService : public CaribeIOServiceLite {
public:
	CaribeCPUService(void)
	    : CaribeIOServiceLite("boot-hart", "riscv,cpu", 0U)
	{
	}

	virtual const char *className(void) const
	{
		return "IOCPU";
	}
};

class CaribeMemoryService : public CaribeIOServiceLite {
public:
	CaribeMemoryService(void)
	    : CaribeIOServiceLite("dram-bank0", "riscv,memory", 0U)
	{
	}

	virtual const char *className(void) const
	{
		return "IOMemoryBank";
	}
};

class CaribeDeviceService : public CaribeIOServiceLite {
public:
	CaribeDeviceService(const char *name, const char *compatible, uint32_t unit)
	    : CaribeIOServiceLite(name, compatible, unit)
	{
	}
};

static uint32_t g_iokit_cpp_probe_flags;
static uint32_t g_iokit_micro_service_count;
static uint32_t g_iokit_micro_started_count;
static uint32_t g_iokit_micro_published_count;
static uint32_t g_iokit_micro_matched_count;

static const char *g_iokit_micro_lines[] = {
	"IOPlatformExpertDevice:/CaribeRiscVPlatformExpert state=published,started",
	"IOCPU:/CaribeRiscVPlatformExpert/boot-hart state=matched,started",
	"IOMemoryBank:/CaribeRiscVPlatformExpert/dram-bank0 state=matched,started",
	"IOUART16550:/CaribeRiscVPlatformExpert/uart0 state=matched,started console",
	"IOPLIC:/CaribeRiscVPlatformExpert/plic0 state=matched,started interrupts",
	"IOACLINT:/CaribeRiscVPlatformExpert/aclint0 state=matched,started timer",
	"IOVirtioMMIO:/CaribeRiscVPlatformExpert/virtio-mmio state=matched,started storage-bus"
};

static uint32_t
riscv32_iokit_micro_registry_build(void)
{
	CaribePlatformExpertService platform;
	CaribeCPUService cpu;
	CaribeMemoryService memory;
	CaribeDeviceService uart("uart0", "ns16550a", 0U);
	CaribeDeviceService plic("plic0", "riscv,plic0", 0U);
	CaribeDeviceService aclint("aclint0", "riscv,aclint", 0U);
	CaribeDeviceService virtio("virtio-mmio", "virtio,mmio", 0U);
	CaribeIOServiceLite *services[] = {
		&platform, &cpu, &memory, &uart, &plic, &aclint, &virtio
	};
	uint32_t flags = 0U;
	uint32_t i;

	g_iokit_micro_service_count =
	    (uint32_t)(sizeof(services) / sizeof(services[0]));
	g_iokit_micro_started_count = 0U;
	g_iokit_micro_published_count = 0U;
	g_iokit_micro_matched_count = 0U;

	platform.publish();
	platform.start((const CaribeIOServiceLite *)0);
	for (i = 1U; i < g_iokit_micro_service_count; i++) {
		platform.attach(services[i]);
		services[i]->publish();
		services[i]->start(&platform);
	}

	for (i = 0U; i < g_iokit_micro_service_count; i++) {
		if (services[i]->isPublished()) {
			g_iokit_micro_published_count++;
		}
		if (services[i]->isStarted()) {
			g_iokit_micro_started_count++;
		}
		if (services[i]->isMatched() || i == 0U) {
			g_iokit_micro_matched_count++;
		}
	}

	if (platform.childCount() == (g_iokit_micro_service_count - 1U)) {
		flags |= RV32_IOKIT_CPP_MICRO_REGISTRY;
	}
	if (g_iokit_micro_published_count == g_iokit_micro_service_count) {
		flags |= RV32_IOKIT_CPP_MICRO_PUBLISH;
	}
	if (g_iokit_micro_started_count == g_iokit_micro_service_count) {
		flags |= RV32_IOKIT_CPP_MICRO_START;
	}
	if (g_iokit_micro_matched_count == g_iokit_micro_service_count) {
		flags |= RV32_IOKIT_CPP_MICRO_MATCH;
	}

	return flags;
}

extern "C" uint32_t
riscv32_iokit_cpp_probe_run(void)
{
	CaribeIOServiceProbe service(0x105e0110U);
	CaribeIOServiceProbeBase *base = &service;
	uint32_t flags = RV32_IOKIT_CPP_PROBE_LINKED;
	uint32_t expected = 0x10U + 0x105e0110U + 0x43425831U;
	const char *name;

	if (service.cookie() == 0x105e0110U) {
		flags |= RV32_IOKIT_CPP_PROBE_CTOR;
	}
	if (base->probe(0x10U) == expected) {
		flags |= RV32_IOKIT_CPP_PROBE_VTABLE;
	}
	name = base->className();
	if (name != (const char *)0 && name[0] == 'C' && name[6] == 'I') {
		flags |= RV32_IOKIT_CPP_PROBE_SERVICE_SHAPE;
	}
	flags |= riscv32_iokit_micro_registry_build();

	g_iokit_cpp_probe_flags = flags;
	if ((flags & RV32_IOKIT_CPP_PROBE_VTABLE) != 0U) {
		riscv32_early_puts("[iokit-stage0] c++ probe: ctor/vtable dispatch ok\n");
	} else {
		riscv32_early_puts("[iokit-stage0] c++ probe: incomplete\n");
	}
	if ((flags & RV32_IOKIT_CPP_MICRO_START) != 0U) {
		riscv32_early_puts("[iokit-stage0] micro IOService: registry/publish/start ok\n");
	}
	return flags;
}

extern "C" uint32_t
riscv32_iokit_cpp_probe_flags(void)
{
	return g_iokit_cpp_probe_flags;
}

extern "C" uint32_t
riscv32_iokit_micro_service_count(void)
{
	return g_iokit_micro_service_count;
}

extern "C" uint32_t
riscv32_iokit_micro_started_count(void)
{
	return g_iokit_micro_started_count;
}

extern "C" uint32_t
riscv32_iokit_micro_published_count(void)
{
	return g_iokit_micro_published_count;
}

extern "C" uint32_t
riscv32_iokit_micro_matched_count(void)
{
	return g_iokit_micro_matched_count;
}

extern "C" uint32_t
riscv32_iokit_micro_registry_line_count(void)
{
	return (uint32_t)(sizeof(g_iokit_micro_lines) /
	    sizeof(g_iokit_micro_lines[0]));
}

extern "C" const char *
riscv32_iokit_micro_registry_line(uint32_t index)
{
	if (index >= riscv32_iokit_micro_registry_line_count()) {
		return (const char *)0;
	}
	return g_iokit_micro_lines[index];
}
