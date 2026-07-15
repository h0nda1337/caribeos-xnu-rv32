# Tranche 201: Stage 2 Complete

Tranche 201 preserves the first CaribeOS state in which the RV32 XNU process
model, interactive shell path, and two-hart scheduler are validated together.

## Highlights

- Gates 1-15 pass in their supported UP/SMP modes.
- GNU Bash 5.3.5 runs interactively over the blocking UART TTY path.
- Processes are backed by XNU `task_t`, `thread_t`, and independent pmaps.
- Fork/clone, transactional exec, exit/wait, signals, pipes, pipelines, and
  tested job-control paths execute in U-mode.
- Hart 1 runs XNU idle and kernel threads and executes gated userspace work.
- SMP Gate D completes 10,000 bound-kernel-thread handoffs.
- Gate E reports zero stale translations after remote RFENCE.
- Gate F reports zero RV32A lock overlap with both processors online.
- Gate 14 completes 10,000 CPU1 process lifecycles with exact resource return.
- Gate 15 completes its full UP and SMP workloads with zero zombies.
- `machine_stack_handoff()` swaps reserved-stack ownership and exports
  `reserved_stack_handoffs` telemetry.

## Preserved Kernel

- Size: 945,004 bytes
- SHA-256: `c63fe4bfbaeaf674a4092c1c579cfe472fc433d2e89b4f55307484ed820e595e`

## Known Limitations

QEMU `virt` RV32 is the only verified machine. The Linux personality and IOKit
surface are incomplete, storage is not a general writable filesystem, and no
network or production security claim is made.
