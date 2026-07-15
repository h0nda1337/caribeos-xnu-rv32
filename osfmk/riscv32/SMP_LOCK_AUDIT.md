# RV32 SMP lock audit

Scope: first SMP phase for XNU-CaribeOS RV32IMACSU. This audit covers lock
operations that can be crossed by hart 0 and hart 1 through Gates C-F.

## Implementations reviewed

- `hw_lock_*`: ownership is acquired with `__sync_bool_compare_and_swap` and
  released with `__sync_lock_release`.
- `usimple_lock_*`: delegates to `hw_lock_*`; XNU processor-set, wait-queue,
  thread and scheduler interlocks therefore use the same RV32A primitive.
- `lck_spin_*`: CAS acquisition and release-store unlock, with the current
  thread pointer recorded as owner.
- `lck_mtx_*`: currently a spin mutex layered on the same atomic interlock.
- `lck_rw_*`: the interlock and reader/writer counters use atomic RV32A
  operations; writer exclusion and reader count transitions were reviewed.
- `hw_atomic_*`, bit locks and atomic queues: use GCC `__sync` builtins, which
  provide full compiler and hardware ordering around the operation.

## Cross-hart paths exercised

- Processor-set and run-queue transitions while bringing processor 1 online.
- Wait-queue and thread locks while starting and parking bound kernel threads.
- Per-pmap active CPU masks and SBI RFENCE target selection.
- Gate D: 10,000 ordered request/completion handshakes with no lost events.
- Gate E: 1,024 shared mapping replacements with local `sfence.vma` and remote
  RFENCE, with no stale translation observed.
- Gate F: two concurrently active XNU kernel threads execute 50,000 critical
  sections each through one `hw_lock`; the protected total must be 100,000 and
  overlap, lost updates and ownership errors must remain zero.
- Gate F also queues a priority-95 thread behind a running priority-81 thread
  on CPU1 and requires an AST signal, SSIP acknowledgement and interrupt-return
  preemption before the lower-priority thread is released.

## Ordering contract

Writers publish shared data before release. Readers acquire the lock or observe
an atomic sequence value before consuming that data. Pmap mutation uses a full
barrier before local `sfence.vma`, synchronous SBI RFENCE for active remote
harts, and a final barrier before returning to the caller.

The generated RV32 objects were inspected with `riscv64-unknown-elf-objdump`;
the audited paths contain `amoadd.w.aqrl` and `amoswap.w.aq` instructions rather
than non-atomic load/store substitutions.

## Known limitations

- `lck_mtx_*` spins instead of blocking and has no priority inheritance.
- RW locks do not yet provide writer-starvation prevention.
- Lock ownership diagnostics and timeout panic detail are minimal.
- Gate F validates mutual exclusion and ordering on two harts, not fairness or
  scalability beyond two CPUs.

These limitations do not invalidate the phase-1 bound-kernel-thread workload,
but they remain blockers for production SMP and unrestricted userspace SMP.
