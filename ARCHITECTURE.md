# Architecture

## Privilege and Boot

OpenSBI remains resident in M-mode. CaribeBootX and XNU execute in S-mode.
CaribeBootX passes the OpenSBI DTB pointer and a validated boot context into
the RV32 XNU entry path. XNU reserves the firmware range
`0x80000000-0x80200000` and discovers RAM/platform data from the handoff.

## Virtual Memory

XNU owns `satp` and Sv32 after entry. The lower 2 GiB form the process user
space; the upper 2 GiB hold the shared kernel mapping. Each process has an
independent user pmap and shares kernel root entries. Per-pmap active CPU masks,
local `sfence.vma`, memory ordering, and SBI RFENCE implement the tested remote
TLB invalidation path.

## SMP

The tested configuration has two harts. OpenSBI HSM starts hart 1. Each hart
has independent `cpu_data_t`, hart/slot identity, kernel and interrupt stack,
trap CSRs, current thread, active pmap, timer state, AST state, and pending
interrupt state. Gates A-F cover per-hart entry, bidirectional IPI, idle/timer,
bound kernel work, TLB coherence, RV32A locking, and dual-processor scheduling.

## Processes and Linux Personality

The Linux personality maps RV32 Linux-compatible syscalls onto XNU tasks,
threads, pmaps, descriptors, and scheduler state. Tranche 201 exercises real
fork/exec/exit/wait lifecycles, signal delivery/return, pipes, blocking TTY,
pipelines, and selected job-control behavior. It is not a Linux kernel and does
not promise a complete Linux ABI.

## IOKit

IOKit and libkern C++ bootstrap portions are linked and probed. The current
device surface is deliberately small and QEMU-specific. This is not a complete
Darwin driver environment.
