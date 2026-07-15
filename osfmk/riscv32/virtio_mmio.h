#ifndef _RISCV32_VIRTIO_MMIO_H_
#define _RISCV32_VIRTIO_MMIO_H_

#include <stdint.h>

#define RISCV32_VIRTIO_MMIO_MAX_DEVICES	8U

typedef struct riscv32_virtio_mmio_device {
	uint32_t base;
	uint32_t size;
	uint32_t irq;
	uint32_t version;
	uint32_t device_id;
	uint32_t vendor_id;
	uint32_t device_features_lo;
	uint32_t device_features_hi;
	uint32_t status;
	uint32_t interrupt_status;
} riscv32_virtio_mmio_device_t;

void riscv32_virtio_mmio_init(void);
uint32_t riscv32_virtio_mmio_count(void);
const riscv32_virtio_mmio_device_t *riscv32_virtio_mmio_device(
    uint32_t index);
riscv32_virtio_mmio_device_t *riscv32_virtio_mmio_find_device(
    uint32_t device_id);

#endif /* _RISCV32_VIRTIO_MMIO_H_ */
