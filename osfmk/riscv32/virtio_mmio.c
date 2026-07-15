#include <machine/bootinfo.h>
#include <machine/plic.h>
#include <machine/virtio_mmio.h>
#include <stddef.h>
#include <stdint.h>

#define VIRTIO_MMIO_MAGIC_VALUE		0x000U
#define VIRTIO_MMIO_VERSION		0x004U
#define VIRTIO_MMIO_DEVICE_ID		0x008U
#define VIRTIO_MMIO_VENDOR_ID		0x00cU
#define VIRTIO_MMIO_DEVICE_FEATURES	0x010U
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL	0x014U
#define VIRTIO_MMIO_INTERRUPT_STATUS	0x060U
#define VIRTIO_MMIO_INTERRUPT_ACK	0x064U
#define VIRTIO_MMIO_STATUS		0x070U

#define VIRTIO_MMIO_MAGIC		0x74726976U

static riscv32_virtio_mmio_device_t
    riscv32_virtio_devices[RISCV32_VIRTIO_MMIO_MAX_DEVICES];
static uint32_t riscv32_virtio_device_count;

static volatile uint32_t *
virtio_reg(const riscv32_virtio_mmio_device_t *dev, uint32_t offset)
{
	return (volatile uint32_t *)(uintptr_t)(dev->base + offset);
}

static uint32_t
virtio_read(const riscv32_virtio_mmio_device_t *dev, uint32_t offset)
{
	return *virtio_reg(dev, offset);
}

static void
virtio_write(const riscv32_virtio_mmio_device_t *dev, uint32_t offset,
    uint32_t value)
{
	*virtio_reg(dev, offset) = value;
}

static void
virtio_interrupt(void *target, void *refCon, void *nub, int source)
{
	riscv32_virtio_mmio_device_t *dev =
	    (riscv32_virtio_mmio_device_t *)refCon;
	uint32_t status;

	(void)target;
	(void)nub;
	(void)source;

	if (dev == (riscv32_virtio_mmio_device_t *)0) {
		return;
	}

	status = virtio_read(dev, VIRTIO_MMIO_INTERRUPT_STATUS);
	dev->interrupt_status |= status;
	if (status != 0) {
		virtio_write(dev, VIRTIO_MMIO_INTERRUPT_ACK, status);
	}
}

static int
virtio_probe_one(const riscv32_fdt_mmio_t *mmio,
    riscv32_virtio_mmio_device_t *dev)
{
	if (mmio->base == 0 || mmio->size < 0x100U) {
		return -1;
	}

	dev->base = mmio->base;
	dev->size = mmio->size;
	dev->irq = mmio->irq;

	if (virtio_read(dev, VIRTIO_MMIO_MAGIC_VALUE) != VIRTIO_MMIO_MAGIC) {
		return -1;
	}

	dev->version = virtio_read(dev, VIRTIO_MMIO_VERSION);
	dev->device_id = virtio_read(dev, VIRTIO_MMIO_DEVICE_ID);
	dev->vendor_id = virtio_read(dev, VIRTIO_MMIO_VENDOR_ID);
	dev->status = virtio_read(dev, VIRTIO_MMIO_STATUS);

	if (dev->version == 0 || dev->device_id == 0) {
		return -1;
	}

	virtio_write(dev, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 0);
	dev->device_features_lo = virtio_read(dev, VIRTIO_MMIO_DEVICE_FEATURES);
	virtio_write(dev, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1);
	dev->device_features_hi = virtio_read(dev, VIRTIO_MMIO_DEVICE_FEATURES);
	virtio_write(dev, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 0);

	if (dev->irq != 0) {
		riscv32_plic_register_irq((int)dev->irq, NULL, NULL,
		    virtio_interrupt, dev);
	}

	return 0;
}

void
riscv32_virtio_mmio_init(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	uint32_t i;

	riscv32_virtio_device_count = 0;
	if (bootinfo == (const riscv32_bootinfo_t *)0) {
		return;
	}

	for (i = 0; i < bootinfo->virtio_count &&
	    riscv32_virtio_device_count < RISCV32_VIRTIO_MMIO_MAX_DEVICES;
	    i++) {
		riscv32_virtio_mmio_device_t *dev =
		    &riscv32_virtio_devices[riscv32_virtio_device_count];

		if (virtio_probe_one(&bootinfo->virtio[i], dev) == 0) {
			riscv32_virtio_device_count++;
		}
	}
}

uint32_t
riscv32_virtio_mmio_count(void)
{
	return riscv32_virtio_device_count;
}

const riscv32_virtio_mmio_device_t *
riscv32_virtio_mmio_device(uint32_t index)
{
	if (index >= riscv32_virtio_device_count) {
		return (const riscv32_virtio_mmio_device_t *)0;
	}
	return &riscv32_virtio_devices[index];
}

riscv32_virtio_mmio_device_t *
riscv32_virtio_mmio_find_device(uint32_t device_id)
{
	uint32_t i;

	for (i = 0; i < riscv32_virtio_device_count; i++) {
		if (riscv32_virtio_devices[i].device_id == device_id) {
			return &riscv32_virtio_devices[i];
		}
	}
	return (riscv32_virtio_mmio_device_t *)0;
}
