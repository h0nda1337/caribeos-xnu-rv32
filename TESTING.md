# Testing

Tests are orchestrated by the companion runtime's `testproject.ps1`. Every
mode rebuilds the latest XNU RV32 kernel before launching QEMU.

## Release Validation

```powershell
.\testproject.ps1 -Mode up -Seconds 45
.\testproject.ps1 -Mode gates -Seconds 50
.\testproject.ps1 -Mode interactive -Seconds 120
.\testproject.ps1 -Mode process-stability -Seconds 300
.\testproject.ps1 -Mode process-stress -Seconds 300 -StressRounds 10000
```

`gates` performs six separate boots for SMP Gates A-F. `process-stability`
runs Gate 15 once in UP and once in SMP. The 10,000-cycle stress mode is kept
separate because it validates long process lifecycle/resource accounting.

## Required Semantics

A zero process exit code alone is insufficient. Release evidence must show:

- `QEMU smoke test OK` and all required gate `PASS` markers
- `stale=0`, `overlap=0`, and zero gate errors
- `zombies=0` where reported
- exact resource return to the captured baseline
- no panic, unexpected trap, or semantic `FAIL`
- no orphaned QEMU or build process
- the expected kernel size and SHA-256

Do not run `-Mode all` merely to duplicate already completed suites. Preserve
the first failing serial log before diagnosis.
