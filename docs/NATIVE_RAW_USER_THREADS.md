# Native Raw User Threads

The `develop/syllable-native-v0.2` checkpoint adds an opt-in shared-task
RV32 U-mode thread backend. It is not pthread, threaded musl/GNU TLS or
complete Syllable AppServer support. Workers remain bound to CPU0.

`osfmk/riscv32/native_thread.c` calls actual XNU `thread_create` on the
owning task, initializes independent saved registers/user stack/raw TP,
checks the active shared pmap, and enters U-mode. The existing scheduler
owns each kernel stack and wait state. START, CANCEL, JOIN and EXIT use
generation-tagged owner-checked records. JOIN waits for the generic reaper's
final thread deallocation notification before permitting slot/stack reuse.
Process exit reaps extra threads before tearing down shared descriptors/VM.

Native semaphore worker waits do not transition the main thread's Linux
process state; holder identity distinguishes the actual worker. The
user-return path takes timer preemption only for this opted-in profile,
and a raw worker's fault does not fabricate a main-thread signal frame.
No thread runs on the secondary user CPU by implication of kernel SMP.

The `native-threads` boot option is mandatory for CREATE. Without it,
CREATE returns ENOTSUP without allocating. Raw workers intentionally use
native ECALLs and no libc/C++ thread runtime. Ordinary worker Linux syscalls
return ENOTSUP, except thread EXIT. Main fork/exec/VM mutations return EBUSY
while raw-thread records exist; normal single-thread Linux paths remain
unchanged. A callback must explicitly EXIT; returning through an unset RA
is a user fault, not a successful exit trampoline.

The runtime retains progressive A-D and default-off gates: 309 actual
create/reaper pairs, 256 reuse cycles, 10,000 same-pmap semaphore ping-pongs,
timer cancellation and 16 owner-exit cycles. Complete strict-ISA evidence
requires separate UP/SMP/OpenSBI/SecureSBI results, existing dependency/Bash/
graphics gates, process regression and byte-identical recompilation of the
eight changed kernel C units. These results do not establish libc or GNU
per-thread TLS. See the companion runtime's
[thread contract and runners](https://github.com/h0nda1337/caribeos-runtime-rv32/blob/develop/syllable-native-v0.2/docs/SYLLABLE_NATIVE_THREADS.md).

Kernel additions/modifications remain APSL-2.0. Existing XNU/third-party
notices and the technical `tranche-201-stage2-complete` tag are preserved.
No Linux kernel, RV64 substitute, Wayland or X11 server is introduced.

## Private Futex Follow-Up

An independent `native-futex` boot option now enables real private WAIT/WAKE
for syscall 98/time32 and 422/time64, including raw workers. Default worker
syscall denial is unchanged without that option. A pmap/address-keyed queue
arms XNU waits while holding the same RV32A lock as bounded WAKE. Relative
timeouts, cancellation and owner teardown remove actual wait records; owner
exit asserts zero queued references before releasing VM. Shared/PI/bitset/
requeue futex operations remain ENOSYS, not success stubs. This exception to
the raw syscall boundary does not enable pthread, clear-tid or threaded TLS.
See the companion [private futex gates](https://github.com/h0nda1337/caribeos-runtime-rv32/blob/develop/syllable-native-v0.2/docs/SYLLABLE_PRIVATE_FUTEX.md).
