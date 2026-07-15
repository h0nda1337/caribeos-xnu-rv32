# RV32 Stage0 MIG Shims

The normal XNU build generates these client sources from MIG definitions. The
portable RV32 stage0 build does not yet carry a native MIG compiler, so
`riscv32-gen-mig-stubs.ps1` copies these reviewed compatibility sources into
the ignored build tree.

`exc_user.c` and `mach_exc_user.c` report failure for unimplemented exception
delivery. `clock_reply_user.c` and `mach_notify_user.c` preserve the behavior
of the Tranche 201 stage0 link. In particular, notification sends are inert
success paths used only to keep early IPC objects linkable; they do not prove
working MIG transport or notification delivery.

The other generated stage0 MIG sources are empty translation-unit placeholders
because their symbols are not traversed by the verified boot path. Replacing
all placeholders with generated interfaces and testing real IPC delivery
remains explicit port work, not a completed feature.
