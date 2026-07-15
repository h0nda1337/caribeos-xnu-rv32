# Roadmap

This roadmap describes direction, not a release promise. Work is accepted only
when it preserves the last verified gate and adds evidence for the new surface.

## Publication Baseline

- finish public documentation, static CI, license review, and clean-clone tests;
- provide a reproducible public Developer Preview and corresponding source;
- keep the technical Tranche 201 tag immutable and use a separate public tag.

## Kernel Stabilization

- remove remaining stage0 compatibility stubs with real subsystem behavior;
- harden Sv32 validation, copyin/copyout, fault paths, and pmap accounting;
- extend scheduler and userspace SMP stress beyond the current two-hart model;
- improve trap diagnostics without exposing host build paths;
- replace QEMU-specific assumptions with explicit platform interfaces.

## Runtime and I/O

- expand IOKit service matching and lifecycle coverage;
- define a maintainable storage/VFS path beyond read-mostly boot media;
- improve interactive TTY semantics and process-group behavior;
- broaden Linux-compatible syscalls only with process and resource tests;
- add device support incrementally, with virtio contracts and negative tests.

## Security and Quality

- introduce hostile-input tests for ELF, initrd, FDT, HFS+, and syscall paths;
- make builds location-independent and more reproducible;
- reduce generated-header and compatibility-shim debt;
- add static analysis suitable for freestanding RV32 code;
- document a security boundary before any non-laboratory use.

## Explicit Non-Goals

CaribeOS does not aim to run macOS binaries, reproduce proprietary Apple
platforms, embed a Linux kernel, or claim production readiness. Networking,
broad hardware support, and a writable general filesystem are not Tranche 201
capabilities.
