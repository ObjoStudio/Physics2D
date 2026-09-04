# Physics2D Performance

This document records how Physics2D is benchmarked, what the measured
results are, how the engine stays allocation-free in steady state, and how
to profile your own scenes. Every claim below is backed by a raw results
file under `benchmarks/results/` recorded on the machine listed in the
Method section.

## Method

- **Machine:** 16-inch MacBook Pro (Apple Silicon), recorded in each
  results file's metadata together with the exact Objo version.
- **Runner:** the `Physics2D.Benchmarks` command-line application. Each
  scenario builds its world, runs a warm-up phase, then measures a fixed
  number of iterations. Every scenario folds a deterministic checksum of
  its observable physics so a timing improvement can never hide a
  behaviour change; a results file whose checksums differ from the
  accepted record is treated as a correctness failure, not a faster run.
- **Raw results:** committed under `benchmarks/results/`. Median, p95, and
  max per-scenario iteration times plus `System.AllocationCount` deltas
  are recorded; no single best run is reported.
- **Cross-run drift:** repeated full-suite runs on identical sources vary
  by roughly ±5% per scenario (observed up to 10% on short scenarios),
  driven by machine state rather than code. A change is only considered
  real when it exceeds that band in a paired comparison and keeps
  checksums identical.
- **Allocations:** `System.AllocationCount` (Objo issue #1299) reports
  lifetime VM object allocations and reading it allocates nothing. It is
  the gate behind every zero-allocation test described below.

Reproduce the results from the repository root (resolve `objo` per the
repository's development notes; the in-development CLI is preferred):

```
objo build Physics2D.objosln --project Physics2D.Benchmarks --output build/benchmarks
build/benchmarks/macOS-Apple-Silicon/Physics2D_Benchmarks --output benchmarks/results --name my-run
build/benchmarks/macOS-Apple-Silicon/Physics2D_Benchmarks --only stage12-pyramid-256 --output /tmp/p2d --name focused
build/benchmarks/macOS-Apple-Silicon/Physics2D_Benchmarks --profile 30
objo test Physics2D.objosln --project Physics2D.Tests --filter "Stage12Tests.*"
```

A full-suite run records every scenario into one JSON file under
`--output`; a focused `--only` run is the right tool when iterating on a
single scenario. The zero-allocation gates run as part of the ordinary
test suite and need no special invocation.

## Stage 12 results

The Stage 12 optimisation audit compared three engine states on identical
scenario definitions:

| Configuration | Sources |
|---|---|
| `stage12-before-candidate` | Stage 11 engine (`dbde82c`), recorded in an isolated git worktree |
| `stage12-candidate-only` | plus the contact/solver list-hoisting candidate (`5958d31`) |
| `stage12-final` | plus the accepted allocation fixes, with the candidate reverted (accepted) |

All three runs produced identical checksums for every scenario.

### Accepted changes and their measured effect

Accepted — engine allocation fixes (before-candidate → final, same run
conditions, identical checksums):

| Scenario | Median | Change | Allocations | Change |
|---|---:|---|---:|---|
| `stage7-pyramid-40` | 962 → 943 ms | −2% | 24,121 → 20,009 | −17% |
| `stage12-pyramid-256` | 24,546 → 22,587 ms | −8% | 261,313 → 47,130 | −82% |
| `stage12-sleep-wake-600` | 1,022 → 727 ms | −29% | 15,227 → 7,738 | −49% |
| `stage12-clear-rebuild` | 639 → 541 ms | −15% | 66,696 → 37,310 | −44% |
| `stage12-world-queries` | 95,274 → 1,496 ms | −98% | 3,105,164 → 23,000 | −99% |
| `stage12-demo-benchmark-pyramid` | 3,511 → 3,226 ms | −8% | — | — |
| `stage9-joint-chain-60` | 115 → 58 ms | −50% | 17,357 → 16,887 | −3% |
| `stage9-motor-drive-30` p95 | 111 → 22 ms | −80% | 827 → 720 | −13% |

The joint-chain median and the motor/weld/wheel p95 improvements are the
clearest evidence of why allocation work matters in a long-running
process: the old code's per-step allocations triggered garbage-collection
spikes inside measured windows, which is exactly the failure mode the
zero-allocation gates exist to prevent.

Accepted — world-query leaf-path fixes. The query leaf paths previously
allocated a fresh cast output, transform, and vector temporaries per
visited leaf; the world now runs leaf visits through world-owned scratch
(reusable cast output, transform, shape-side proxies, pair input, and
query inputs) with the same arithmetic. `stage12-world-queries` kept
checksum `-7148840557192300803` across every measurement.

Reverted — the contact/solver list-hoisting candidate. Its before/after
comparison showed mixed ±2–5% deltas on the contact- and solver-heavy
scenarios while scenarios with byte-identical code drifted by the same
amount between runs. It never beat the noise band, so it was reverted per
the Stage 12 rule; the five touched functions are byte-identical to the
Stage 11 engine again. The lesson is recorded here deliberately: hoisting
dense-list references out of loops is plausible in the abstract but did
not measure as a win in Objo's VM, and the earlier isolated +15%
observation did not survive a controlled comparison.

### Full-suite picture (accepted record, `stage12-final`)

Selected medians per iteration, recorded on the reference machine:

| Scenario | Median | Allocations |
|---|---:|---:|
| `stage12-pyramid-256` (820 bodies, 256 frames) | 22.6 s | 47,130 |
| `stage12-sleep-wake-600` (600-body wall) | 0.73 s | 7,738 |
| `stage12-dense-contacts-240` | 142 ms | 3,545 |
| `stage12-bullet-ccd-24` | 378 ms | 13,063 |
| `stage12-world-queries` (2,250 queries) | 1.50 s | 23,000 |
| `stage12-sensor-field` (100 sensors, 200 movers, 30 frames) | 5.63 s | 74,400 |
| `stage12-joint-machine` (all seven families) | 105 ms | 5,291 |
| `stage12-clear-rebuild` | 541 ms | 37,310 |
| `stage12-demo-benchmark-pyramid` | 3.23 s | 7,380 |

Scenario-level allocation counts include world construction and warm-up;
the steady-state per-step cost is covered by the zero-allocation gates in
`Stage12Tests` (see below).

## Zero-allocation guarantees

The normal `World.Step` path allocates nothing after warm-up. This is
enforced by tests that build real scenes, run a representative warm-up,
and require a zero `System.AllocationCount` delta across measured
windows:

- `WorldTests.TestWarmStepAllocatesNothing` — small mixed scene.
- `Stage12Tests.TestPyramidWarmStepAllocatesNothing` — 78-body pyramid
  through settling, contact churn, and natural island sleeping.
- `Stage12Tests.TestSleepWakeMigrationAllocatesNothing` — 120-body wall
  through wake, solve, island-aware forced sleep, and asleep stepping
  (solver-set transfers, graph retargeting, contact store moves).
- `Stage12Tests.TestSensorOverlapAllocatesNothing` — 16 sensors and 24
  movers with sensor events published every step (double-buffered overlap
  lists and event publication).
- `Stage12Tests.TestContinuousBulletStepAllocatesNothing` — eight
  recycled bullets against a wall with continuous collision resolving a
  time of impact every step.
- `Stage12Tests` also gates the warm twelve-level pyramid; the
  foundation, geometry, and broad-phase suites gate their own kernels.

The mechanisms that make this hold, in the order they usually matter:

1. **One shared contact object pool.** Manifold and simplex-cache objects
   live in world-shared pools (`World.ContactManifoldPool` and
   `World.ContactCachePool`) with world-shared free lists. Every solver
   set and graph colour references the same pool, so a slot freed by any
   store is immediately reusable by any other. Per-store pools were
   measured to scatter recycled slots away from the store that needed
   them next and re-allocate on most contact begins.
2. **Rows carry their slots through transfers.** Wake, sleep, island
   merge, and graph migration move row slots between stores without
   copying objects; transfers pre-size the target store with
   `Reserve(rows)` from known row counts, so a first transfer into a
   fresh sleeping set allocates nothing.
3. **Amortised pool growth.** Pools grow in geometric strides (contact
   object pools adopt new objects only when the shared free list is
   empty; per-colour constraint pools double their fill on growth), so
   one-time warm-up dominates and sub-stride transients never allocate.
4. **World-owned scratch everywhere.** Narrow-phase transforms, sweep
   records, split DFS stacks, query inputs, cast outputs, and proxy
   records are world-owned and reused. Collision scratch is created
   eagerly when a manifold enters the pool so first use never allocates.
5. **Stable event buffers.** Contact, sensor, and body-event lists are
   world-owned and cleared per step; their capacity equals the
   scene's event high-water mark after warm-up.

The deliberately allocating APIs (documented in `docs/API.md`): world
construction, body/shape/joint creation and destruction, `Clear`, the
query forms that return fresh records (`CastRayClosest`,
`OverlapBounds`, and every callback-form query, because user callbacks
may retain point and normal references), and the per-call `TreeStats`
record returned by every query. Allocation-gated tests pin the budget of
the warm create/destroy paths (for example, a warm shape create is 17
façade-owned allocations and `Clear` plus an eight-body rebuild is 136,
both test-pinned).

## Capacity planning

The engine warms by first encounter; a scene that wakes a larger island
or touches more sensors than any previous frame may allocate once while
pools grow. To pre-warm deterministically:

- Run the scene through its full structural envelope once before the
  measured loop: maximum expected contacts awake at once, one wake
  cycle from a settled state, at least one island sleep (a recycled
  sleeping solver set then serves later sleeps allocation-free), and
  one pass of every query you will issue.
- The Stage 12 gates show the pattern: the sleep/wake gate runs four
  wake/sleep cycles, the sensor gate forces one sleep/wake cycle during
  warm-up, and the bullet gate flies three reset flights before
  measuring.
- `World.Counters()` reports body, shape, contact, joint, island, and
  per-graph-colour counts, plus tree heights, so a scene can assert its
  own expected envelope in tests.
- Islands allocate island records from an id pool; a world that holds
  more simultaneous islands than ever before grows its island array
  once.

## Substeps

`World.StepWorld(timeStep)` defaults to four substeps; the explicit overload
accepts a positive count. Substeps divide the solver's integration and
iteration work, so per-frame cost scales roughly linearly with the
substep count:

- More substeps: better stacking behaviour, smaller bias velocities,
  more stable tall stacks, proportionally more solver time.
- Fewer substeps: cheaper frames; fine for sparse scenes, kinematic
  drivers, and games that do not stack deeply.
- The contact softness parameters are derived from the substep length,
  so behaviour stays consistent across counts; SceneTests and the golden
  joint fixtures pin the four-substep default.

The Stage 12 scenarios pin both ends: `stage12-demo-benchmark-pyramid`
runs the demo's forty-level pyramid at one substep (the demo's own
setting) and `stage12-pyramid-256` runs a taller one at the default
four. Measure your own scene with both before choosing.

## Query and event choices

- Prefer the `Into`/reusable forms in per-frame loops:
  `CastRayClosestTo`, `CastRayInto`, `CastShapeInto`,
  `OverlapShapeInto`, and `OverlapBoundsInto` take caller-owned records
  and lists. Each call still returns a fresh lightweight `TreeStats`
  record; ignore it if you do not need traversal statistics.
- The callback forms (`CastRay` with a callback, `OverlapShape` with a
  callback) allocate a fresh output per visited shape so your callback
  may retain points; they are cold-path conveniences.
- Build query filters once and reuse them; `QueryFilter` values are
  cheap but the reused forms avoid re-validating.
- Sensor events publish through double-buffered overlap lists diffed
  per step; enabling `EnableSensorEvents` on a shape costs one tree
  query per sensor per step plus event-buffer traffic for actual
  begin/end transitions. Scenes that never read sensor events should
  leave the flag off (the default path publishes nothing).
- Contact events copy a manifold snapshot per begin event; enable
  `EnableContactEvents` on shapes only when the application reads them.
- Hit events are computed for approaching contacts of shapes with
  `EnableHitEvents` set; the cost is one relative-velocity evaluation
  per touching contact per step for enabled shapes.

## Profiling your own scenes

- `World.Profile()` returns per-stage wall-clock milliseconds for the
  most recent step: pairs (broad-phase pair discovery), collide
  (narrow phase plus state transitions), solve (integration, colour
  iterations, finalisation, refit, bullets, sleeping), and sensors.
  Measure across a representative window; single frames jitter.
- `World.Counters()` returns structural counts (bodies, shapes,
  contacts, joints, islands, per-colour constraint counts, tree
  heights) — the first thing to read when a scene behaves oddly or
  costs more than expected.
- `System.AllocationCount` around any code region gives its exact
  allocation cost; the test suite uses it for the gates above, and the
  same pattern works in application code.
- The Stage 12 diagnosis workflow, which found the issues described
  here, was: record `System.AllocationCount` per step, attribute deltas
  to engine stages with temporary instrumentation, and only then change
  code — never optimise from source appearance.

## Honest limitations

- **Objo VM throughput.** Physics2D is a native Objo implementation; the
  VM is substantially slower than compiled C. Absolute numbers above are
  meaningful only relative to other Objo code on the reference machine.
  The eight-hundred-body pyramid steps in tens of milliseconds on
  desktop-class Apple Silicon — usable for many games, far from C
  Box2D's microsecond budgets.
- **Warm-up dependence.** Pools grow by first encounter. A scene whose
  structural envelope changes over time (a world that keeps growing)
  will allocate occasionally by design. The gates prove the steady
  state, not unbounded growth.
- **Solver scale.** The constraint graph colours solve in parallel
  arrays with an overflow colour solved serially; scenes whose bodies
  saturate all eleven colours route more contacts through the serial
  overflow path. `World.Counters().ColorCounts` exposes the
  distribution.
- **Continuous collision.** Bullet bodies run swept queries every step
  they stay fast; a scene with many simultaneous fast bodies pays per
  body. Non-bullet fast movers solve immediately instead.
- **Single-threaded.** Box2D 3.1.1's task system is out of scope by
  design (see `docs/PORTING.md`); there is no parallel solver.
- **Benchmark coverage.** Raw results are recorded per machine and Objo
  version. Regenerate them (see the scenario list in
  `Physics2D.Benchmarks`) when either changes; do not compare across
  machines.
