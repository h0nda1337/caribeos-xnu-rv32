# RISC-V 32-bit bring-up

This directory is the first local scaffold for an RV32IMAC target running with
supervisor and user modes under QEMU `virt`.

The current files only define the minimal type, endian, and CPU-number surface
needed by the early configuration path.  The real port still needs trap entry,
CSR handling, timer/IPI, pmap/Sv32, context switch, copyio, and Platform Expert
device-tree plumbing.
