# Native Syllable Foundations

Development branch: `develop/syllable-native-v0.2`. The target is the authentic
Syllable AppServer, libsyllable, Desktop, Dock and FileBrowser on XNU RV32,
without Wayland/X11 dependencies. These components are not all built yet.

`native_ipc.c` supplies process-owned message ports through `/dev/caribe-ipc`:
bounded FIFO queues, generation IDs, publication, AppServer registration,
metadata and XNU scheduler waits with deadlines and signal interruption.
Its independent-exec probe verifies 10,000 bidirectional exchanges, bounds,
permissions, EFAULT no-loss behavior and exec/exit reclamation.

`native_area.c` adds real shared physical objects and independently protected
Sv32 views. Fork registers references before eager pmap cloning and returns
the existing shared PPN only for registered area pages; private pages retain
their old private-clone path. View teardown removes mappings before backing
reuse. A successful exec revokes old-image views only after commit; failed
exec preserves them. Exit and failed fork reclaim registered references.

Areas are a bounded wired prototype: 32 objects, 64 views, 1024 total pages,
512 pages per area. READ/RW and ANY/EXACT addressing are supported. Shared
EXEC, physical remap and demand paging are not. The reserved user VA range
`0x68000000..0x70000000` is protected from generic Linux mapping mutations.
Known area IDs permit cloning; no production capability/security isolation
claim is made for this initial local GUI transport.

Both native subsystems use RV32A simple locks. Area lock order is area -> pmap;
the fork clone callback is invoked without the source/destination pmap lock.
No native blocking port wait holds its global queue lock across thread_block.
pmap removes retain the existing local/RFENCE ordering before zero/reuse.
GUI probe processes remain CPU0-bound. The separate SMP A-F gates continue
to exercise both kernel processors, but area/IPC cross-hart userspace stress
and native multi-threaded AppServer execution are not claimed yet.

Shared-area gates verify alias writes, read-only copyout, permission checks,
full resource exhaustion, failed create copyout/fork rollback, 256 reuse
cycles, fork inheritance, exec/exit lifetime and 10,000 shared-memory exchanges
between independently exec'd processes. A child-rendered shared framebuffer
is presented by its parent to the actual virtio GPU; QMP pixels are checked.

Kernel SHA-256 `bef8795f430f4a27cb6e76052b2c8e703690d4d089ea384f195422bcd53aac1f`
passed area A-E, native IPC A-D, existing graphics/input A-E, private VM
isolation and interactive Bash in OpenSBI UP, OpenSBI SMP2 without F/D/H and
SecureSBI SMP2. It also passed process gates 1-15, Gate 15 SMP stability and
17 CaribeSecurity QEMU regressions. Forced recompilation reproduced the SHA.
The stable Tranche 201 tag and its evidence are unchanged.

See [runtime APIs, exact limits and runners](https://github.com/h0nda1337/caribeos-runtime-rv32/blob/develop/syllable-native-v0.2/docs/SYLLABLE_NATIVE.md)
and [recorded evidence](https://github.com/h0nda1337/caribeos-runtime-rv32/tree/develop/syllable-native-v0.2/docs/evidence/syllable-areas-v0.3).
Full C++/Message/Looper/thread/bitmap/font/configuration/attribute support
remains required before an authentic complete Syllable Desktop can boot.
