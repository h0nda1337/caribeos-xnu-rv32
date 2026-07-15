# Tranche 201 Evidence

Evidence was collected from the runtime runner and serial logs on 2026-07-14.
Full logs are intentionally kept in the timestamped local preservation set;
this repository keeps only compact summaries under `docs/evidence/tranche-201`.

| Suite | Result | Reported duration |
|---|---|---:|
| UP boot and userland | PASS | 49.3 s |
| SMP Gates A-F, six boots | PASS | 310.6 s |
| Interactive Bash | PASS | 15.5 s |
| Gate 14, 10,000 CPU1 lifecycles | PASS | 28.8 s |
| Gate 15 UP and SMP | PASS | 90.6 s |

The semantic logs report `stale=0`, `overlap=0`, `zombies=0`, zero gate errors,
and exact resource-baseline return for the long stress run. The runner also
asserts required markers and rejects panic/unexpected-trap patterns.

The preserved tag is `tranche-201-stage2-complete`. Commit IDs and archive
hashes are recorded in the external `TRANCHE_201_MANIFEST.json` generated after
the commits and tags exist; this avoids embedding a circular self-hash in the
commit that contains the evidence.
