#ifndef _RISCV32_VIRTIO_BLK_H_
#define _RISCV32_VIRTIO_BLK_H_

#include <stdint.h>

int riscv32_virtio_blk_init(void);
uint64_t riscv32_virtio_blk_capacity(void);
int riscv32_virtio_blk_read_blocks(uint64_t lba, uint32_t count, void *buf);

#endif /* _RISCV32_VIRTIO_BLK_H_ */
