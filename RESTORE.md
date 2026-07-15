# Restoring Tranche 201

The preservation set has three independent layers:

1. Raw pre-edit working-tree archives, including untracked/generated files.
2. Self-contained Git bundles for XNU and runtime.
3. A compact QEMU developer-preview ZIP with hashes and a manifest.

## Preferred Git Restore

```powershell
git bundle verify <backup>\caribeos-xnu-tranche201.bundle
git clone <backup>\caribeos-xnu-tranche201.bundle <new-empty-path>
git -C <new-empty-path> fsck --full
git -C <new-empty-path> tag --verify tranche-201-stage2-complete
```

Restore the runtime bundle beside XNU using the same procedure. Never restore
over an existing project tree.

## Raw Snapshot Restore

Use the runtime repository's `tools/restore-caribeos-backup.ps1`. It requires a
new destination and refuses to overwrite an existing path. Verify
`RAW_SNAPSHOT_SHA256SUMS.txt` before extraction.

The initial XNU checkout was a linked partial worktree whose administrative
directory lived outside F:. Its pointer and common Git directory are preserved
in separate raw archives. The Tranche 201 bundle is the preferred autonomous
Git recovery path.
