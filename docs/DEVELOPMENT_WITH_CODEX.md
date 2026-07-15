# Development With Codex

Codex was used extensively as a development tool during the CaribeOS port. It
helped inspect source, propose and implement changes, write focused tests,
operate the local toolchain, analyze QEMU serial logs, maintain documentation,
and prepare preservation and publication checks.

## Human Direction

The project owner defined the architecture, required boot chain, target ISA,
stage boundaries, safety constraints, semantic gates, publication criteria,
and decisions about which behavior was acceptable. Codex did not choose the
project's legal author, copyright owner, or licensing policy.

## Validation Practice

AI-assisted changes were not accepted merely because they compiled or emitted
a marker. Validation included:

- exact build commands and tool versions;
- UP and SMP serial assertions;
- IPI, timer, scheduler, pmap, and RFENCE gates;
- process lifecycle, signal, pipe, TTY, and Bash tests;
- 10,000-cycle stress runs and resource-baseline checks;
- artifact sizes and SHA-256 hashes;
- Git bundles, restore tests, fsck, secret scans, and license inventories.

Full logs and machine-oriented audit output are preserved outside the public
repositories. Compact evidence and non-sensitive hashes are checked in.

## Review Boundary

Use of Codex does not replace technical, security, or legal review. Generated
or edited code may contain subtle architecture, concurrency, ABI, undefined
behavior, provenance, or licensing errors even when a focused test passes.

The areas needing the strongest additional human review include RV32 trap and
context-switch assembly, pmap/TLB ordering, SMP locks and scheduler invariants,
copyin/copyout and signal-frame validation, ELF/syscall input handling, HFS+
parsing, IOKit ownership, and all public redistribution decisions.

Codex should be described as a tool used in development, not as the legal
author or accountable maintainer. Every future AI-assisted change should carry
the same burden of review and evidence as a human-written change.
