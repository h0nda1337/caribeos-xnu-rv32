#ifndef _MACHINE_VIRTIO_BLK_H_
#define _MACHINE_VIRTIO_BLK_H_

#if (defined(__riscv) && (__riscv_xlen == 32)) || defined(__RISCV32__)
#include "riscv32/virtio_blk.h"
#else
#error architecture not supported
#endif

#endif /* _MACHINE_VIRTIO_BLK_H_ */
