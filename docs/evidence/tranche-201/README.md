# Tranche 201 Compact Evidence

The companion runtime produced and asserted the Tranche 201 evidence on
2026-07-14. Full run and serial logs remain in the timestamped local backup.

Verified suites:

- UP: PASS, reported 49.3 s
- SMP Gates A-F across six boots: PASS, reported 310.6 s
- interactive GNU Bash 5.3.5: PASS, reported 15.5 s
- Gate 14 with 10,000 CPU1 lifecycles: PASS, reported 28.8 s
- Gate 15 UP and SMP: PASS, reported 90.6 s

Kernel SHA-256:

```text
c63fe4bfbaeaf674a4092c1c579cfe472fc433d2e89b4f55307484ed820e595e
```

See `summary.txt`, `commands.txt`, and the runtime evidence directory for the
individual process-gate summaries. Commit and tag IDs are recorded externally
after the Git objects exist.
