#include <machine/virtio_blk.h>
#include <machine/virtio_mmio.h>
#include <stddef.h>
#include <stdint.h>

#define VIRTIO_ID_BLOCK			2U

#define VIRTIO_MMIO_DRIVER_FEATURES	0x020U
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL	0x024U
#define VIRTIO_MMIO_GUEST_PAGE_SIZE	0x028U
#define VIRTIO_MMIO_QUEUE_SEL		0x030U
#define VIRTIO_MMIO_QUEUE_NUM_MAX	0x034U
#define VIRTIO_MMIO_QUEUE_NUM		0x038U
#define VIRTIO_MMIO_QUEUE_ALIGN		0x03cU
#define VIRTIO_MMIO_QUEUE_PFN		0x040U
#define VIRTIO_MMIO_QUEUE_NOTIFY	0x050U
#define VIRTIO_MMIO_INTERRUPT_STATUS	0x060U
#define VIRTIO_MMIO_INTERRUPT_ACK	0x064U
#define VIRTIO_MMIO_STATUS		0x070U
#define VIRTIO_MMIO_CONFIG		0x100U

#define VIRTIO_STATUS_ACKNOWLEDGE	1U
#define VIRTIO_STATUS_DRIVER		2U
#define VIRTIO_STATUS_DRIVER_OK		4U
#define VIRTIO_STATUS_FEATURES_OK	8U
#define VIRTIO_STATUS_FAILED		128U

#define VIRTQ_DESC_F_NEXT		1U
#define VIRTQ_DESC_F_WRITE		2U

#define VIRTIO_BLK_QNUM		8U
#define VIRTIO_BLK_SECTOR_SIZE	512U
#define VIRTIO_BLK_T_IN		0U

typedef struct virtq_desc {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
} __attribute__((packed)) virtq_desc_t;

typedef struct virtq_avail {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[VIRTIO_BLK_QNUM];
	uint16_t used_event;
} __attribute__((packed)) virtq_avail_t;

typedef struct virtq_used_elem {
	uint32_t id;
	uint32_t len;
} __attribute__((packed)) virtq_used_elem_t;

typedef struct virtq_used {
	uint16_t flags;
	uint16_t idx;
	virtq_used_elem_t ring[VIRTIO_BLK_QNUM];
	uint16_t avail_event;
} __attribute__((packed)) virtq_used_t;

typedef struct virtio_blk_req {
	uint32_t type;
	uint32_t ioprio;
	uint64_t sector;
} __attribute__((packed)) virtio_blk_req_t;

static riscv32_virtio_mmio_device_t *riscv32_virtio_blk_dev;
static uint32_t riscv32_virtio_blk_qnum;
static uint64_t riscv32_virtio_blk_capacity_sectors;
static uint8_t riscv32_virtio_blk_ring[8192] __attribute__((aligned(4096)));
static virtq_desc_t *riscv32_virtio_blk_desc;
static virtq_avail_t *riscv32_virtio_blk_avail;
static virtq_used_t *riscv32_virtio_blk_used;
static virtio_blk_req_t riscv32_virtio_blk_req;
static volatile uint8_t riscv32_virtio_blk_status;

static volatile uint32_t *
blk_reg(uint32_t offset)
{
	return (volatile uint32_t *)(uintptr_t)(riscv32_virtio_blk_dev->base +
	    offset);
}

static uint32_t
blk_read(uint32_t offset)
{
	return *blk_reg(offset);
}

static void
blk_write(uint32_t offset, uint32_t value)
{
	*blk_reg(offset) = value;
}

static uint64_t
blk_read64(uint32_t offset)
{
	uint32_t lo = blk_read(offset);
	uint32_t hi = blk_read(offset + sizeof(uint32_t));

	return ((uint64_t)hi << 32) | lo;
}

static void
blk_barrier(void)
{
	__sync_synchronize();
}

static void
blk_bzero(void *ptr, uint32_t len)
{
	uint8_t *p = (uint8_t *)ptr;
	uint32_t i;

	for (i = 0; i < len; i++) {
		p[i] = 0;
	}
}

static uint32_t
blk_align_up(uint32_t value, uint32_t align)
{
	return (value + align - 1U) & ~(align - 1U);
}

static uint64_t
blk_phys(const volatile void *ptr)
{
	return (uint64_t)(uintptr_t)ptr;
}

static void
blk_fail(void)
{
	if (riscv32_virtio_blk_dev !=
	    (riscv32_virtio_mmio_device_t *)0) {
		blk_write(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
	}
	riscv32_virtio_blk_dev = (riscv32_virtio_mmio_device_t *)0;
}

int
riscv32_virtio_blk_init(void)
{
	uint32_t qmax;
	uint32_t desc_sz;
	uint32_t avail_sz;
	uint32_t avail_off;
	uint32_t used_off;
	uint32_t status;

	riscv32_virtio_blk_dev =
	    riscv32_virtio_mmio_find_device(VIRTIO_ID_BLOCK);
	if (riscv32_virtio_blk_dev ==
	    (riscv32_virtio_mmio_device_t *)0) {
		return -1;
	}

	blk_write(VIRTIO_MMIO_STATUS, 0);
	status = VIRTIO_STATUS_ACKNOWLEDGE;
	blk_write(VIRTIO_MMIO_STATUS, status);
	status |= VIRTIO_STATUS_DRIVER;
	blk_write(VIRTIO_MMIO_STATUS, status);

	blk_write(VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
	blk_write(VIRTIO_MMIO_DRIVER_FEATURES, 0);
	blk_write(VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
	blk_write(VIRTIO_MMIO_DRIVER_FEATURES, 0);
	blk_write(VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);

	status |= VIRTIO_STATUS_FEATURES_OK;
	blk_write(VIRTIO_MMIO_STATUS, status);
	if ((blk_read(VIRTIO_MMIO_STATUS) & VIRTIO_STATUS_FEATURES_OK) == 0) {
		blk_fail();
		return -1;
	}
	riscv32_virtio_blk_capacity_sectors =
	    blk_read64(VIRTIO_MMIO_CONFIG);

	blk_write(VIRTIO_MMIO_QUEUE_SEL, 0);
	qmax = blk_read(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (qmax < 3U) {
		blk_fail();
		return -1;
	}
	riscv32_virtio_blk_qnum = qmax < VIRTIO_BLK_QNUM ?
	    qmax : VIRTIO_BLK_QNUM;
	blk_write(VIRTIO_MMIO_QUEUE_NUM, riscv32_virtio_blk_qnum);

	desc_sz = sizeof(virtq_desc_t) * VIRTIO_BLK_QNUM;
	avail_off = desc_sz;
	avail_sz = sizeof(uint16_t) * 2U +
	    sizeof(uint16_t) * VIRTIO_BLK_QNUM + sizeof(uint16_t);
	used_off = blk_align_up(avail_off + avail_sz, 4096U);

	blk_bzero(riscv32_virtio_blk_ring,
	    (uint32_t)sizeof(riscv32_virtio_blk_ring));
	riscv32_virtio_blk_desc =
	    (virtq_desc_t *)(riscv32_virtio_blk_ring + 0);
	riscv32_virtio_blk_avail =
	    (virtq_avail_t *)(riscv32_virtio_blk_ring + avail_off);
	riscv32_virtio_blk_used =
	    (virtq_used_t *)(riscv32_virtio_blk_ring + used_off);

	blk_write(VIRTIO_MMIO_GUEST_PAGE_SIZE, 4096U);
	blk_write(VIRTIO_MMIO_QUEUE_ALIGN, 4096U);
	blk_write(VIRTIO_MMIO_QUEUE_PFN,
	    (uint32_t)(blk_phys(riscv32_virtio_blk_ring) >> 12));

	status |= VIRTIO_STATUS_DRIVER_OK;
	blk_write(VIRTIO_MMIO_STATUS, status);
	return 0;
}

uint64_t
riscv32_virtio_blk_capacity(void)
{
	return riscv32_virtio_blk_capacity_sectors;
}

int
riscv32_virtio_blk_read_blocks(uint64_t lba, uint32_t count, void *buf)
{
	uint16_t avail_idx;
	uint16_t want_used;
	uint32_t isr;
	uint32_t spins;

	if (count == 0) {
		return 0;
	}
	if (riscv32_virtio_blk_dev ==
	    (riscv32_virtio_mmio_device_t *)0 ||
	    riscv32_virtio_blk_desc == (virtq_desc_t *)0) {
		return -1;
	}
	if (count > UINT32_MAX / VIRTIO_BLK_SECTOR_SIZE) {
		return -1;
	}
	if (riscv32_virtio_blk_capacity_sectors != 0 &&
	    (lba >= riscv32_virtio_blk_capacity_sectors ||
	    count > riscv32_virtio_blk_capacity_sectors - lba)) {
		return -1;
	}

	riscv32_virtio_blk_req.type = VIRTIO_BLK_T_IN;
	riscv32_virtio_blk_req.ioprio = 0;
	riscv32_virtio_blk_req.sector = lba;
	riscv32_virtio_blk_status = 0xffU;

	isr = blk_read(VIRTIO_MMIO_INTERRUPT_STATUS);
	if (isr != 0) {
		blk_write(VIRTIO_MMIO_INTERRUPT_ACK, isr);
	}

	want_used = (uint16_t)(riscv32_virtio_blk_used->idx + 1U);

	riscv32_virtio_blk_desc[0].addr = blk_phys(&riscv32_virtio_blk_req);
	riscv32_virtio_blk_desc[0].len = sizeof(riscv32_virtio_blk_req);
	riscv32_virtio_blk_desc[0].flags = VIRTQ_DESC_F_NEXT;
	riscv32_virtio_blk_desc[0].next = 1;

	riscv32_virtio_blk_desc[1].addr = blk_phys(buf);
	riscv32_virtio_blk_desc[1].len = count * VIRTIO_BLK_SECTOR_SIZE;
	riscv32_virtio_blk_desc[1].flags =
	    VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;
	riscv32_virtio_blk_desc[1].next = 2;

	riscv32_virtio_blk_desc[2].addr = blk_phys(&riscv32_virtio_blk_status);
	riscv32_virtio_blk_desc[2].len = sizeof(riscv32_virtio_blk_status);
	riscv32_virtio_blk_desc[2].flags = VIRTQ_DESC_F_WRITE;
	riscv32_virtio_blk_desc[2].next = 0;

	avail_idx = riscv32_virtio_blk_avail->idx;
	riscv32_virtio_blk_avail->ring[avail_idx % riscv32_virtio_blk_qnum] =
	    0;
	blk_barrier();
	riscv32_virtio_blk_avail->idx = (uint16_t)(avail_idx + 1U);
	blk_barrier();

	blk_write(VIRTIO_MMIO_QUEUE_SEL, 0);
	blk_barrier();
	blk_write(VIRTIO_MMIO_QUEUE_NOTIFY, 0);
	blk_barrier();

	spins = 0;
	while (riscv32_virtio_blk_used->idx != want_used &&
	    spins++ < 100000000U) {
		blk_barrier();
	}
	if (riscv32_virtio_blk_used->idx != want_used) {
		return -1;
	}

	isr = blk_read(VIRTIO_MMIO_INTERRUPT_STATUS);
	if (isr != 0) {
		blk_write(VIRTIO_MMIO_INTERRUPT_ACK, isr);
	}

	return riscv32_virtio_blk_status == 0 ? 0 : -1;
}
