# Port Status

This page describes the verified Tranche 201 boundary. A source file, symbol,
or log marker does not by itself establish a complete subsystem.

## Status Vocabulary

| State | Meaning |
|---|---|
| Verified | Covered by a preserved semantic gate or focused assertion |
| Partial | Real code executes, but important contracts remain incomplete |
| Experimental | Useful for development, with narrow assumptions or limited stress |
| Not implemented | No supported behavior is claimed |

## Kernel Surface

| Area | State | Tranche 201 evidence or limit |
|---|---|---|
| OpenSBI/CaribeBootX handoff | Verified | S-mode entry, DTB/boot context, reserved firmware range |
| RV32 CPU state | Verified | independent boot and secondary-hart `cpu_data_t`, stacks, CSRs, current thread, AST/timer state |
| Sv32 VM | Verified | split user/kernel address space, per-process pmap, local fence and SBI RFENCE tests |
| Two-hart SMP | Verified | Gates A-F, 10,000 kernel handoffs, no lock overlap or stale TLB observations |
| Scheduler | Verified/partial | kernel and tested userspace work on both CPUs; broad policy and topology support remain partial |
| Tasks and threads | Verified | real task/thread-backed process lifecycle in the tested personality |
| Linux-compatible ABI | Partial | tested ELF32 exec, fork/clone, wait/exit, signals, FDs, pipes, TTY and selected job control |
| IOKit/libkern C++ | Partial | bootstrap and focused service/probe paths link and execute; driver ecosystem is incomplete |
| HFS+ | Experimental | boot-time read path and kernel loading; not a general writable filesystem |
| Drivers | Partial | QEMU UART, interrupt/timer, virtio and boot-focused paths only |
| Security | Not production-ready | no hostile multi-user or network security claim |

## Userland Surface

musl 1.2.5 and GNU Bash 5.3 with official patches 1-5 run as RV32 binaries in
the tested initrd. Interactive commands, pipelines, signals, process stress,
and exact resource return are covered by preserved evidence. This does not
mean that the full Linux syscall ABI, POSIX surface, package ecosystem, dynamic
loader behavior, or Debian userland is supported.

## Evidence

Compact evidence lives in `docs/evidence/tranche-201/`. The preserved release
set records 35/35 validation and 19/19 restore checks. The kernel artifact is
945,004 bytes with SHA-256
`c63fe4bfbaeaf674a4092c1c579cfe472fc433d2e89b4f55307484ed820e595e`.

See `ROADMAP.md` for work that remains and `TESTING.md` for the semantic gates
required before changing a status.
