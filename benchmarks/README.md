# Physics2D Benchmarks

Micro-benchmarks for the Physics2D module, built on a deterministic,
checksum-verified harness.

## Purpose

- Track performance across ports and optimisation stages.
- Enforce allocation budgets: every scenario records object allocations from
  `System.AllocationCount` around each measured iteration, so zero-allocation
  hot paths can be gated in tests. This property is unreleased at the time of
  writing (Objo issue #1299); the harness requires an Objo build that includes
  it.
- Guard correctness: every scenario must report a deterministic checksum;
  a changed checksum fails the run. `Checksum()` is abstract, so a scenario
  without a checksum does not compile.

## Layout

- `Projects/Physics2D.Benchmarks/Sources/` — harness (`BenchmarkScenario`,
  `BenchmarkResult`, `BenchmarkRunner`) and scenarios.
- `Projects/Physics2D.Benchmarks/Tests/` — harness contract tests, including
  the allocation-gate self-tests.
- `results/` — committed JSON records of notable runs, one per milestone.

## Running

```bash
objo build Physics2D.objosln --project Physics2D.Benchmarks --output build/benchmarks
build/benchmarks/macOS-Apple-Silicon/Physics2D_Benchmarks --output benchmarks/results --name <run-name>
```

Options: `--output <dir>` (default `benchmarks/results`), `--name <run-name>`
(default `local`), `--only <substr>` to run a subset of scenarios.

Each run writes `results/<run-name>-<timestamp>.json` containing the platform,
computer, Objo version, per-scenario median/p95/max/mean timings, raw samples,
allocation counts, and checksums.

## Adding a scenario

Subclass `BenchmarkScenario` (Application scope) and implement `Run()`,
`Checksum()`, and optionally `Prepare()` and `Parameters()`. The runner
rejects scenarios without a name or with non-positive iteration counts.
Keep scenarios deterministic: same input, same checksum, every run.
