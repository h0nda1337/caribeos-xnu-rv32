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
 * port of Apple XNU 2050.48.11 on 2026-10-12.
 */

#include <kern/simple_lock.h>
#include <machine/bootinfo.h>
#include <machine/copyio.h>
#include <machine/cpu_data.h>
#include <machine/early_print.h>
#include <machine/graphics.h>
#include <machine/virtio_mmio.h>
#include <stdint.h>
#include <string.h>

#define GFX_W 800U
#define GFX_H 600U
#define QNUM 64U
#define INPUT_MAX 2U
#define EVENT_MAX 256U

struct qdesc {
  uint64_t addr;
  uint32_t len;
  uint16_t flags, next;
};
struct qused_entry {
  uint32_t id, len;
};
struct queue {
  uint8_t memory[8192] __attribute__((aligned(4096)));
  uint16_t seen;
};
struct input_event {
  uint16_t type, code;
  int32_t value;
};
struct input_device {
  const riscv32_virtio_mmio_device_t *dev;
  struct queue queue;
  struct input_event events[QNUM];
};
struct gpu_header {
  uint32_t type, flags;
  uint64_t fence;
  uint32_t context;
  uint8_t ring, padding[3];
};
static const riscv32_virtio_mmio_device_t *gpu;
static struct queue control;
static struct input_device inputs[INPUT_MAX];
static uint32_t input_count;
static uint32_t framebuffer[GFX_W * GFX_H] __attribute__((aligned(4096)));
static uint32_t staging[GFX_W * GFX_H] __attribute__((aligned(4096)));
static uint32_t command[32] __attribute__((aligned(16)));
static uint32_t response[128] __attribute__((aligned(16)));
static struct caribe_gfx_event pending[EVENT_MAX];
static uint32_t event_head, event_count;
static struct caribe_gfx_stats stats;
static int initialized, ready;
static volatile uint32_t lock_state;
static simple_lock_data_t graphics_lock;

static uint32_t reg_read(const riscv32_virtio_mmio_device_t *dev,
                         uint32_t off) {
  return *(volatile uint32_t *)(uintptr_t)(dev->base + off);
}
static void reg_write(const riscv32_virtio_mmio_device_t *dev, uint32_t off,
                      uint32_t value) {
  *(volatile uint32_t *)(uintptr_t)(dev->base + off) = value;
}
static void barrier(void) { __asm__ volatile("fence iorw, iorw" ::: "memory"); }
static struct qdesc *descriptors(struct queue *q) { return (void *)q->memory; }
static volatile uint16_t *available(struct queue *q) {
  return (void *)(q->memory + 1024);
}
static volatile uint16_t *used(struct queue *q) {
  return (void *)(q->memory + 4096);
}
static volatile struct qused_entry *used_entries(struct queue *q) {
  return (void *)(q->memory + 4100);
}

static int device_start(const riscv32_virtio_mmio_device_t *dev) {
  uint32_t hi;
  if (dev->version != 1U && dev->version != 2U)
    return -1;
  reg_write(dev, 0x070, 0);
  reg_write(dev, 0x070, 3);
  reg_write(dev, 0x014, 1);
  hi = reg_read(dev, 0x010);
  reg_write(dev, 0x024, 0);
  reg_write(dev, 0x020, 0);
  reg_write(dev, 0x024, 1);
  if (dev->version == 2U && (hi & 1U) == 0U)
    return -1;
  reg_write(dev, 0x020, dev->version == 2U ? 1U : 0U);
  reg_write(dev, 0x024, 0);
  reg_write(dev, 0x070, 11);
  return (reg_read(dev, 0x070) & 8U) ? 0 : -1;
}
static int queue_start(const riscv32_virtio_mmio_device_t *dev,
                       struct queue *q) {
  uint32_t addr = (uint32_t)(uintptr_t)q->memory;
  memset(q, 0, sizeof(*q));
  reg_write(dev, 0x030, 0);
  if (reg_read(dev, 0x034) < QNUM)
    return -1;
  reg_write(dev, 0x038, QNUM);
  if (dev->version == 1U) {
    reg_write(dev, 0x028, 4096);
    reg_write(dev, 0x03c, 4096);
    reg_write(dev, 0x040, addr >> 12);
  } else {
    reg_write(dev, 0x080, addr);
    reg_write(dev, 0x084, 0);
    reg_write(dev, 0x090, addr + 1024);
    reg_write(dev, 0x094, 0);
    reg_write(dev, 0x0a0, addr + 4096);
    reg_write(dev, 0x0a4, 0);
    reg_write(dev, 0x044, 1);
  }
  return 0;
}
static void publish(const riscv32_virtio_mmio_device_t *dev, struct queue *q,
                    uint16_t id) {
  volatile uint16_t *a = available(q);
  uint16_t index = a[1];
  a[2U + index % QNUM] = id;
  barrier();
  a[1] = (uint16_t)(index + 1U);
  barrier();
  reg_write(dev, 0x050, 0);
}
static int gpu_request(uint32_t bytes, uint32_t expected) {
  struct qdesc *d = descriptors(&control);
  uint32_t spins;
  volatile uint16_t *u = used(&control);
  memset(response, 0, sizeof(response));
  d[0] = (struct qdesc){(uint32_t)(uintptr_t)command, bytes, 1, 1};
  d[1] = (struct qdesc){(uint32_t)(uintptr_t)response, sizeof(response), 2, 0};
  publish(gpu, &control, 0);
  for (spins = 0; spins < 10000000U; spins++) {
    barrier();
    if (u[1] != control.seen)
      break;
  }
  if (spins == 10000000U || (uint16_t)(u[1] - control.seen) != 1U ||
      used_entries(&control)[control.seen % QNUM].id != 0U ||
      used_entries(&control)[control.seen % QNUM].len <
          sizeof(struct gpu_header)) {
    ready = 0;
    reg_write(gpu, 0x070, 128);
    return -1;
  }
  control.seen++;
  barrier();
  reg_write(gpu, 0x064, reg_read(gpu, 0x060));
  return response[0] == expected ? 0 : -1;
}
static void gpu_command(uint32_t type) {
  memset(command, 0, sizeof(command));
  command[0] = type;
}
static int present(void) {
  gpu_command(0x0105);
  command[8] = GFX_W;
  command[9] = GFX_H;
  command[12] = 1;
  if (gpu_request(56, 0x1100) != 0)
    return -1;
  gpu_command(0x0104);
  command[8] = GFX_W;
  command[9] = GFX_H;
  command[10] = 1;
  if (gpu_request(48, 0x1100) != 0)
    return -1;
  stats.presents++;
  return 0;
}
static void init_inputs(void) {
  uint32_t i, j;
  for (i = 0; i < riscv32_virtio_mmio_count() && input_count < INPUT_MAX; i++) {
    const riscv32_virtio_mmio_device_t *dev = riscv32_virtio_mmio_device(i);
    struct input_device *in = &inputs[input_count];
    if (dev->device_id != 18U)
      continue;
    if (device_start(dev) != 0 || queue_start(dev, &in->queue) != 0) {
      reg_write(dev, 0x070, 128);
      continue;
    }
    in->dev = dev;
    for (j = 0; j < QNUM; j++) {
      descriptors(&in->queue)[j] =
          (struct qdesc){(uint32_t)(uintptr_t)&in->events[j],
                         sizeof(struct input_event), 2, 0};
      publish(dev, &in->queue, (uint16_t)j);
    }
    reg_write(dev, 0x070, 15);
    reg_write(dev, 0x050, 0);
    input_count++;
  }
}
static int graphics_init(void) {
  if (initialized)
    return ready;
  initialized = 1;
  gpu = riscv32_virtio_mmio_find_device(16);
  if (!gpu)
    return 0;
  if (device_start(gpu) != 0 || queue_start(gpu, &control) != 0)
    goto fail;
  reg_write(gpu, 0x070, 15);
  gpu_command(0x0100);
  if (gpu_request(24, 0x1101) != 0 || response[10] == 0U)
    goto fail;
  gpu_command(0x0101);
  command[6] = 1;
  command[7] = 2;
  command[8] = GFX_W;
  command[9] = GFX_H;
  if (gpu_request(40, 0x1100) != 0)
    goto fail;
  gpu_command(0x0106);
  command[6] = 1;
  command[7] = 1;
  command[8] = (uint32_t)(uintptr_t)framebuffer;
  command[10] = sizeof(framebuffer);
  if (gpu_request(48, 0x1100) != 0)
    goto fail;
  gpu_command(0x0103);
  command[8] = GFX_W;
  command[9] = GFX_H;
  command[10] = 0;
  command[11] = 1;
  if (gpu_request(48, 0x1100) != 0)
    goto fail;
  init_inputs();
  ready = 1;
  riscv32_early_puts(
      "[graphics-gate-A] XNU virtio-gpu 800x600 XRGB8888 ready inputs=");
  riscv32_early_putdec(input_count);
  riscv32_early_puts(" PASS\n");
  return 1;
fail:
  reg_write(gpu, 0x070, 128);
  ready = 0;
  riscv32_early_puts("[graphics-gate-A] virtio-gpu initialization FAIL\n");
  return 0;
}
int riscv32_graphics_available(void) {
  if (get_cpu_number() != 0)
    return 0;
  /* Initialization and all callers are confined to the existing CPU0 user ABI.
   */
  if (__sync_bool_compare_and_swap(&lock_state, 0U, 1U)) {
    simple_lock_init(&graphics_lock, 0);
    __sync_synchronize();
    lock_state = 2U;
  }
  while (__sync_fetch_and_add(&lock_state, 0U) != 2U)
    __asm__ volatile("nop");
  simple_lock(&graphics_lock);
  graphics_init();
  simple_unlock(&graphics_lock);
  return ready;
}
static void collect_input(void) {
  uint32_t i;
  for (i = 0; i < input_count; i++) {
    struct input_device *in = &inputs[i];
    volatile uint16_t *u = used(&in->queue);
    uint32_t drained = 0;
    if (!in->dev)
      continue;
    barrier();
    while (in->queue.seen != u[1] && drained++ < QNUM) {
      struct qused_entry entry =
          used_entries(&in->queue)[in->queue.seen % QNUM];
      struct caribe_gfx_event event;
      if (entry.id >= QNUM || entry.len != sizeof(struct input_event)) {
        reg_write(in->dev, 0x070, 128);
        in->dev = 0;
        break;
      }
      barrier();
      event.device = i;
      event.type = in->events[entry.id].type;
      event.code = in->events[entry.id].code;
      event.value = in->events[entry.id].value;
      event.sequence = ++stats.events;
      if (event_count < EVENT_MAX) {
        pending[(event_head + event_count++) % EVENT_MAX] = event;
      } else
        stats.dropped++;
      in->queue.seen++;
      publish(in->dev, &in->queue, (uint16_t)entry.id);
      barrier();
    }
    if (in->dev)
      reg_write(in->dev, 0x064, reg_read(in->dev, 0x060));
  }
}
int riscv32_graphics_ioctl(uint32_t request, uint32_t user_arg) {
  int error = 0;
  if (!riscv32_graphics_available())
    return 19;
  simple_lock(&graphics_lock);
  if (request == CARIBE_GFX_GET_MODE) {
    struct caribe_gfx_mode mode = {CARIBE_GFX_ABI,
                                   GFX_W,
                                   GFX_H,
                                   GFX_W * 4,
                                   CARIBE_GFX_FORMAT_XRGB8888,
                                   input_count ? 1U : 0U};
    error = riscv32_copyout(&mode, user_arg, sizeof(mode)) ? 14 : 0;
  } else if (request == CARIBE_GFX_BLIT) {
    struct caribe_gfx_blit b;
    uint32_t row, span;
    if (riscv32_copyin(user_arg, &b, sizeof(b))) {
      error = 14;
      goto out;
    }
    if (b.abi != CARIBE_GFX_ABI || b.reserved || !b.width || !b.height ||
        b.x >= GFX_W || b.y >= GFX_H || b.width > GFX_W - b.x ||
        b.height > GFX_H - b.y || b.stride < b.width * 4 ||
        b.stride > sizeof(staging) || b.height > sizeof(staging) / b.stride) {
      error = 22;
      goto out;
    }
    span = (b.height - 1U) * b.stride + b.width * 4U;
    if (!riscv32_user_range_valid(b.pixels, span)) {
      error = 14;
      goto out;
    }
    /* Validate/copy the complete submission before changing the scanout
     * backing. */
    for (row = 0; row < b.height; row++) {
      if (riscv32_copyin(b.pixels + row * b.stride, &staging[row * b.width],
                         b.width * 4U)) {
        error = 14;
        goto out;
      }
    }
    for (row = 0; row < b.height; row++)
      memcpy(&framebuffer[(b.y + row) * GFX_W + b.x], &staging[row * b.width],
             b.width * 4U);
    stats.blits++;
  } else if (request == CARIBE_GFX_PRESENT) {
    error = present() ? 5 : 0;
  } else if (request == CARIBE_GFX_GET_EVENT) {
    collect_input();
    if (!event_count) {
      error = 11;
      goto out;
    }
    error = riscv32_copyout(&pending[event_head], user_arg,
                            sizeof(struct caribe_gfx_event))
                ? 14
                : 0;
    if (!error) {
      event_head = (event_head + 1U) % EVENT_MAX;
      event_count--;
    }
  } else if (request == CARIBE_GFX_GET_STATS) {
    error = riscv32_copyout(&stats, user_arg, sizeof(stats)) ? 14 : 0;
  } else
    error = 25;
out:
  simple_unlock(&graphics_lock);
  return error;
}
