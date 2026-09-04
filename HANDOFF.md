# Physics2D Port — Stage 12 Recovery Handoff

This handoff was refreshed on 2026-09-04 after the previous Pi session
stalled in a command_supervisor/auto-compaction loop. It is the
authoritative recovery checkpoint for the current working tree.

Read AGENTS.md and IMPLEMENTATION_PLAN.md first. In particular, read all of
§19 (Stage 12), §20 (Stage 13), §21 (the progress ledger), and §22 (the
final autonomous audit) before changing anything.

## Executive State

- Repository: /Users/garry/Repos/Physics2D.
- Branch: main. The Stage 12 checkpoint commit containing this handoff is
  the branch tip; its clean starting point was dbde82c (Stage 11 complete).
- Stages 0-11 are complete at dbde82c. The native rigid-body feature
  port is functionally complete: contacts, sleeping, CCD, sensors, events,
  queries, every required joint family, debug rendering, the distribution
  pipeline, tests, benchmarks, and the desktop demo all exist.
- Stage 12 has substantial **uncommitted and partly validated** work, but
  its ledger status correctly remains Not started because none of its exit
  criteria has been accepted yet.
- Stage 13 and the final §22 audit have not started.
- The last accepted clean checkpoint at dbde82c had 332 Physics2D tests
  plus 19 benchmark tests passing, a current generated distribution, and a
  passing clean-room distribution check.
- The current working tree compiles, but is deliberately not green.
  On 2026-09-04 the two known invalid benchmark fixtures were fixed
  (chain Body, sensor-event flags) and focused validation passed; see the
  progress log below. The remaining failing Physics2D tests are the
  expected stale-distribution checksum plus the three allocation gates:
  sleep/wake 3,153, bullet 101, and sensor 626 (the sensor test now
  publishes events and fails only on its allocation assertion).
- Do not reset, clean, stash, overwrite, or discard this working tree.
  All current changes belong to the interrupted Stage 12 attempt.
- The user explicitly requested that the recovered Stage 12 work and this
  handoff be committed and pushed together as a resumable checkpoint.

## The Interrupted Agent Is No Longer Running

The Pi process and its supervised child are no longer active. There is no
live benchmark to preserve or wait for.

The last supervisor run was:

- Run ID: 20260902T200612517Z-c9fc2c6c
- Label: suite-stage12-opt1b
- Command:
  ./build/benchmarks/macOS-Apple-Silicon/Physics2D_Benchmarks --name stage12-opt1
- Result: exit 1 at 2026-09-02 20:37:37 UTC
- Immediate failure:
  InvalidArgumentException: ChainDefinition.Body must not be Nothing

After that failure, Pi repeatedly attempted a tool-only supervisor wait.
The supervisor correctly deferred it because the command had already
reported a result and required a user-visible update. Auto-compaction then
repeatedly hit its output-token cap. Entries written after the failed run
are orchestration noise, not useful implementation progress.

The preserved Pi session is:

/Users/garry/.pi/agent/sessions/--Users-garry-Repos-Physics2D--/2026-09-02T14-31-36-833Z_01a06288-5481-76f2-b2c3-3612d06b63d9.jsonl

Session ID: 01a06288-5481-76f2-b2c3-3612d06b63d9

Do not resume that enormous session unless historical detail is genuinely
needed; this handoff, the working diff, and the retained supervisor logs
are a cleaner continuation point.

## Contents Of The Stage 12 Checkpoint

Tracked modifications:

- Projects/Physics2D.Benchmarks/Sources/App-eb01d1a3.objobasic
- Projects/Physics2D.Benchmarks/Sources/BenchmarkRunner-f1787827.objobasic
- Shared/Sources/SolverMethods.objobasic
- Shared/Sources/World.objobasic

New Stage 12 implementation included in the checkpoint:

- Projects/Physics2D.Benchmarks/Sources/Stage12Support-2aeab36e-86a7-442b-b328-8cf599f8b164.objobasic
- Projects/Physics2D.Benchmarks/Sources/Stage12Support-2aeab36e-86a7-442b-b328-8cf599f8b164.source.json
- Projects/Physics2D.Benchmarks/Sources/Stage12Workloads-c686277b-ac7d-404b-8d54-047f77c1d975.objobasic
- Projects/Physics2D.Benchmarks/Sources/Stage12Workloads-c686277b-ac7d-404b-8d54-047f77c1d975.source.json
- Projects/Physics2D.Benchmarks/Sources/Stage12Scenarios-1ca2dad8-91b2-442c-85bc-37174c2f8998.objobasic
- Projects/Physics2D.Benchmarks/Sources/Stage12Scenarios-1ca2dad8-91b2-442c-85bc-37174c2f8998.source.json
- Projects/Physics2D.Tests/Tests/Stage12Tests-c4baf4b3-a2e5-4c43-a9e7-83eac8786f80.objobasic
- Projects/Physics2D.Tests/Tests/Stage12Tests-c4baf4b3-a2e5-4c43-a9e7-83eac8786f80.source.json
- benchmarks/results/stage12-before-2026-09-02T15-40-37.json
- HANDOFF.md

The three benchmark sidecars are full application-source sidecars with
fresh IDs, empty module parents, and BuildScope: Application. The test
sidecar has the full required field set, a fresh ID, an empty module
parent, and BuildScope: Test. Test discovery sees all four new tests.

Before the checkpoint commit, the existing tracked files had 167 insertions
and 69 deletions, and the new Stage 12 Objo sources added approximately
2,046 lines. git diff --check passed.

## What The Stage 12 Work Implements

### Benchmark coverage

The new workloads and scenario wrappers cover the §19 required matrix:

1. stage12-pyramid-256: exact 40-level/820-body pyramid, 256 frames.
2. stage12-sleep-wake-600: 600-body wall through wake, solve, forced
   sleep, and asleep stepping.
3. stage12-tumbler-create-destroy: motor-driven tumbler with continuous
   creation/destruction.
4. stage12-dense-contacts-240: mixed box/circle/capsule dense contacts.
5. stage12-bullet-ccd-24: recycled bullet stream against a thin wall.
6. stage12-world-queries: closest/all ray, shape cast, shape overlap, and
   bounds overlap throughput.
7. stage12-sensor-field: 100 sensors and 200 moving visitors.
8. stage12-joint-machine: all seven solver joint families together.
9. stage12-clear-rebuild: repeated mixed-world clear/rebuild.
10. stage12-demo-benchmark-pyramid: the demo's heaviest scene without
    drawing.

Existing Stage 5 tree churn/rebuild scenarios and existing Stage 9
per-family joint scenarios supply the remaining required coverage.

The benchmark app now:

- registers all ten Stage 12 scenarios;
- supports --only via BenchmarkRunner.KeepMatching;
- supports --profile <steps>;
- prints one completion line per scenario to stderr, so a failed or
  interrupted long run retains useful progress.

### Profiler

Stage12Profiler accumulates World.Profile() stage timings and
World.Counters() data over selected workloads and measures
System.AllocationCount. The agent subsequently added pre-population for
the tumbler and bullet workloads; the retained profile below predates that
fix, so its tumbler row is not representative.

### New allocation gates

Stage12Tests adds four tests:

- a warm twelve-level pyramid;
- a 200-body sleep/wake migration;
- a sensor field;
- a repeated continuous bullet flight.

Only the pyramid test currently passes. Do not weaken the other assertions;
fix invalid fixtures first, then determine whether remaining allocations
are real engine defects or insufficiently representative warm-up.

### Candidate engine optimisation

The uncommitted engine changes are an initial, evidence-directed
optimisation candidate:

- World.CollideStep hoists contact-store references and skips zero words
  when scanning contact state bits.
- World.CollideOneContact hoists shape/body/list lookups, fills reused
  transforms directly from dense body rows, and avoids repeated property
  chains.
- World.ApplyContactStateChange hoists the live flag list.
- SolverMethods.IntegrateVelocities and IntegratePositions hoist dense
  list references out of body loops.

An initial version incorrectly reused the pre-UpdateContact SimFlags value
and clobbered the refreshed SimTouchingFlag. That made a contact repeatedly
enter an island and caused “Contact already belongs to an island”. The
interrupted agent correctly diagnosed and fixed this by re-reading the
live flag row in the started/stopped-touching branches.

All pre-Stage-12 behavioural tests now pass, and the existing
stage7-pyramid-40 checksum remains 161419885332893877. The candidate
nevertheless is not accepted until the complete Stage 12 correctness and
performance gates pass.

## Validation Performed On 2026-09-04

Commands used the in-development CLI:

/Users/garry/Repos/Objo/src/studio/Objo.Cli/bin/Debug/net10.0/objo

### Passed

- git diff --check
- objo check Physics2D.objosln
  - Result: Check succeeded for Physics2D.Tests.
- Benchmark application build:
  objo build Physics2D.objosln --project Physics2D.Benchmarks --output build/handoff-assess
- Benchmark-project tests:
  - 19 passed, 0 failed, 0 errors, 0 skipped in 17.51 s.
- API inventory audit:
  - 329 inventory rows;
  - 241 member references checked;
  - every row maps to a declared generated-distribution member.
- Physics test discovery:
  - 336 tests, proving the new Stage 12 sidecar is valid.
- Full Physics2D.Tests result:
  - 332 passed, 4 failed, 0 errors, 0 skipped in 87.88 s.
- New TestPyramidWarmStepAllocatesNothing passes.
- Every pre-existing behavioural, scene, CCD, sensor, joint, API, query,
  geometry, broad-phase, and foundation test passes. The only pre-existing
  failure is the expected stale-distribution checksum after shared-source
  edits.

### Four current Physics2D.Tests failures

1. DistributionTests.TestDistributionInputsMatchSharedSources

   dist/Physics2D.objobasic is stale because World.objobasic and
   SolverMethods.objobasic changed. This is expected while the candidate
   optimisation is unaccepted. Once the candidate is accepted (or
   reverted), regenerate the distribution and run the clean-room gate.

2. Stage12Tests.TestSensorOverlapAllocatesNothing

   RESOLVED FIXTURE, OPEN ALLOCATION DIAGNOSIS. The fixture error is
   fixed (2026-09-04): the mover shapes now set
   moverDef.EnableSensorEvents = True, matching the engine rule that
   SensorOverlapTest ignores a visitor whose EnableSensorEvents is
   False. The test now publishes sensor events during warm-up and
   reaches the allocation gate, failing with 626 allocations instead of
   the warm-up assertion. Diagnose those 626 allocations (begin/end
   buffer growth, event-bit traffic, or genuine engine allocation)
   before touching engine code. Do not weaken the zero assertion.

3. Stage12Tests.TestSleepWakeMigrationAllocatesNothing

   Expected 0 allocations; actual 3,153.

   This remains unresolved. A likely test-design issue is that all bodies
   are created asleep before contacts/islands exist. Waking only bodies[0]
   in the first cycle may not exercise or reserve capacity for the final
   merged 200-body island that the measured cycle encounters. Verify
   island/contact counts across warm-up and measured cycles before changing
   engine code. If both cycles are structurally equivalent, trace the
   allocations as an engine defect.

4. Stage12Tests.TestContinuousBulletStepAllocatesNothing

   Expected 0 allocations; actual 101.

   This remains unresolved. Confirm that the first flight warms the same
   maximum contact/TOI/pair capacities used by the reset second flight.
   Do not merely increase the warm-up until the source of the 101
   allocations is understood.

### Focused benchmark failure — RESOLVED

The clear/rebuild InvalidArgumentException is fixed (2026-09-04):
ClearRebuildWorkload.Rebuild now sets chainDef.Body = ground before
CreateChain, matching the documented one-body chain rule. Focused run:

    Physics2D_Benchmarks --only stage12-clear-rebuild \
      --output /tmp/physics2d-stage12 --name clear-rebuild-fix-check
    exit 0; median 88.355 ms, p95 183.945 ms,
    checksum -4364940994124906251

### Test-filter semantics — CONFIRMED

objo test --filter takes a glob matched with both ends anchored against
QualifiedClass.TestMethod (Objo.Studio.Core TestSelection). Plain
"Stage12Tests" therefore selects zero tests; the correct form is
"Stage12Tests.*" (repeatable, case-insensitive, * and ? wildcards).
Focused filtered runs are now trustworthy evidence; a zero-selection
filtered run is still not evidence of anything passing.

## Benchmark Evidence And Caveats

### The “before” file is incomplete

benchmarks/results/stage12-before-2026-09-02T15-40-37.json is valid JSON
and records Objo 26.9.1 on the 16-inch MacBook Pro, but it contains only
the 30 pre-existing benchmark scenarios. It contains **none** of the ten
new Stage 12 scenarios. Therefore it is not an accepted Stage 12 baseline
and must not be described or committed as one.

### Directional pre-optimisation profile

A 30-step profile taken before the candidate optimisation reported:

| Workload | ms/step | Main stages | Profile allocations |
|---|---:|---|---:|
| Pyramid, 1 substep | 211.40 | collide 51%, solve 37%, pairs 11% | 5,552 |
| Sleep/wake, awake | 210.42 | collide 60%, solve 29%, pairs 11% | 11,840 |
| Dense contacts | 32.57 | collide 53%, solve 24%, pairs 23% | 3,408 |
| Bullet CCD | 3.03 | solve 67%, pairs 28%, collide 5% | 188 |
| Sensor field | 121.78 | solve 57%, collide 24%, pairs 16% | 0 |
| Joint machine | 3.91 | solve 51%, collide 35%, pairs 14% | 1,603 |
| Demo pyramid, 4 substeps | 366.89 | solve 59%, collide 29%, pairs 12% | 0 |

This profile is useful for ranking broad hotspots only. It used five warm-up
steps, which is demonstrably insufficient for several allocation readings,
and the tumbler/bullet pre-population edits happened afterward.

### Partial full-suite run

The final attempted full run completed the existing scenarios and these
Stage 12 scenarios before failing at clear/rebuild:

| Scenario | Total runner time |
|---|---:|
| stage12-pyramid-256 | 81.81 s |
| stage12-sleep-wake-600 | 48.19 s |
| stage12-tumbler-create-destroy | 3.91 s |
| stage12-dense-contacts-240 | 4.29 s |
| stage12-bullet-ccd-24 | 3.18 s |
| stage12-world-queries | 1,617.58 s |
| stage12-sensor-field | 35.26 s |
| stage12-joint-machine | 2.67 s |

These are elapsed per-scenario runner totals from stderr, not medians, and
no JSON was written because the run failed. The world-query scenario took
about 27 minutes by itself and must be scaled to a representative but
practical gate before running the entire suite again.

A focused pre-optimisation stage12-pyramid-256 run exists only under
/tmp/stage12-bisect/:

- two samples: 50,146.23 ms and 21,664.92 ms;
- checksum: 92265315502742034;
- reported allocations: 261,313.

The enormous variation reflects an evolving long-horizon world and only
two samples. Review the scenario methodology before accepting it.

### Promising but unaccepted optimisation signal

For existing stage7-pyramid-40:

- before candidate: median 940.59 ms;
- after the flag fix with the candidate: median 796.24 ms;
- apparent improvement: about 15.3%;
- checksum unchanged: 161419885332893877;
- reported benchmark allocations unchanged: 24,121.

This is promising evidence for the contact/list-hoisting candidate, not an
accepted result. Re-run in a controlled before/after comparison after the
benchmark fixtures and methodology are stable.

Because the new benchmark sources and candidate engine optimisation are
mixed in one working tree, do not mislabel current results as “before”.
Preserve the candidate diff and obtain a HEAD-equivalent engine baseline
with the same benchmark sources in an isolated worktree or otherwise
carefully separated comparison. Do not reset or destructively juggle the
current tree.

## Documentation And Distribution State

Documentation already present from completed stages:

- docs/API.md — substantial generated API reference (1,678 lines).
- docs/ARCHITECTURE.md — substantial architecture document (454 lines).
- docs/DEMO.md — demo and teaching guide (192 lines).
- docs/PORTING.md — upstream mapping and deliberate differences (845
  lines).

Still missing or stale:

- README.md is a 20-line planning placeholder that still says the port
  “will” be implemented.
- docs/PERFORMANCE.md is missing and is a Stage 12 deliverable.
- docs/GETTING_STARTED.md is missing.
- docs/CONTRIBUTING.md is missing.
- Existing API, architecture, demo, and porting documents need the final
  Stage 13 consistency and teaching review.
- Semantic version, final compatibility record, release-candidate
  checklist, clean consumer proof, and artifact hashes remain undone.
- dist/Physics2D.objobasic is stale relative to the two modified shared
  sources. python3 tools/assemble_module.py --check currently fails with:
  STALE: distribution checksum does not match Shared Code.
- The API audit still passes because the candidate changes are internal and
  do not alter the frozen public surface.

Do not begin Stage 13 or mark Stage 12 complete until every §19 exit
criterion passes.

## Recommended Resume Order

1. [DONE 2026-09-04] Stage 12 plan read; checkpoint commit 956a73e
   inspected; working tree confirmed clean at that commit.
2. [DONE 2026-09-04] Both invalid fixtures fixed: chainDef.Body = ground
   in ClearRebuildWorkload.Rebuild; moverDef.EnableSensorEvents = True in
   SensorFieldWorkload and Stage12Tests.TestSensorOverlapAllocatesNothing.
3. [DONE 2026-09-04] Benchmark app rebuilt; stage12-clear-rebuild passes
   (exit 0, median 88.355 ms, checksum -4364940994124906251); filtered
   Stage12Tests run with the confirmed "Stage12Tests.*" glob: pyramid
   passes, sensor reaches its allocation gate (626), bullet (101) and
   sleep/wake (3,153) unchanged; 19 benchmark-project tests pass.
4. Diagnose the 3,153 sleep/wake allocations, the 101 bullet allocations,
   and the 626 sensor allocations. Decide from evidence whether each is
   inadequate warm-up/capacity coverage or a real engine allocation.
   Keep zero as the required normal path.
5. Right-size stage12-world-queries so it remains representative but takes
   seconds or low minutes, not 27 minutes. Review the two-sample,
   evolving-world pyramid methodology as well.
6. Stabilise all ten scenario definitions and checksums before treating
   any file as the Stage 12 baseline.
7. Capture a true pre-candidate baseline using the same scenarios and a
   HEAD-equivalent engine without discarding the current optimisation.
8. Re-run the candidate optimisation, focused correctness tests, and
   affected benchmarks. Accept it only if the improvement beats noise and
   invariants remain clear; otherwise revert only that candidate.
9. Continue hotspot-by-hotspot profiling and optimisation with raw
   before/after files. Do not optimise from source appearance.
10. Once engine changes are accepted, regenerate dist/Physics2D.objobasic,
    run the full correctness/determinism gates, benchmark suite, API audit,
    and clean-room distribution check.
11. Write docs/PERFORMANCE.md, satisfy every Stage 12 exit criterion, then
    update the ledger and commit the coherent Stage 12 change.
12. Proceed to Stage 13 and finally the independent §22 audit.

## Progress Log

### 2026-09-04 (night) — resume step 5 complete: world-queries right-sized and
### the world query path made allocation-free at the leaf level

- stage12-world-queries took 27 minutes alone because its 2500-shape
  static grid was inserted in column order (degenerate static tree) and it
  ran 45,000 queries. Fixed: one World.RebuildStaticTree() after grid
  construction and a 2,250-query mix per iteration (rays=1000,
  rayAll=250, shapeCast=250, shapeOverlap=250, boundsOverlap=500).
- Diagnosed ~276 allocations per query in the engine's leaf paths:
  RayCastShape/ShapeCastShape allocated a CastOutput per leaf visit,
  GetShapeTransform allocated a Transform per leaf, hit-tail
  TransformPoint/RotateVector allocated Vector2 temporaries, the
  shape-cast path rebuilt its local proxy with a fresh Vector2 per point
  and ran the allocating Casts.ShapeCastViaProxy chain per leaf, and the
  overlap context allocated New Transform + GetBodyTransformById per
  leaf. All replaced with world-owned scratch (mQueryCastOutput,
  mQueryTransform, mCastShapeProxy, mPolygonCastProxy,
  mShapeCastPairInput, mQueryRayInput, mQueryShapeCastInput), reused
  proxy point records, Casts.ShapeCastTo into the reusable output, and a
  context-owned LeafTransform filled by GetBodyTransformByIdTo. Callback
  forms keep allocating outputs (user callbacks may retain point/normal
  references); Into and closest forms copy values out immediately.
- stage12-world-queries: median 5,900 ms → 1,189 ms per iteration,
  allocations 3,105,164 → 23,000 (remaining: grid Prepare plus the
  documented fresh TreeStats per public query call), checksum unchanged
  at -7148840557192300803 (bit-identical results).
- Validation: full Physics2D suite 335 passed / 1 expected stale-dist
  failure; 19 benchmark tests pass.

Next: resume step 6 (stabilise the ten scenario definitions), then the
isolated-worktree baseline (step 7), candidate accept/revert (step 8),
dist regeneration and full gates (step 10), PERFORMANCE.md and the ledger
(step 11).

### 2026-09-04 (evening) — resume step 4 complete: all four allocation gates pass

Root causes diagnosed with per-stage allocation probes (temporary engine
instrumentation, since removed), then fixed in the engine:

1. Per-store manifold/cache pools scattered recycled slots away from the
   store that needed them next (churn migrates rows between 12 colour
   stores, the awake set, disabled set, and sleeping sets), so nearly every
   contact begin grew a pool (~15 VM objects each). Fixed by world-shared
   pools: World.ContactManifoldPool/ContactCachePool plus shared free
   lists, wired by reference into every SolverSet and GraphColor
   ContactSims; ContactSims.ClearAll no longer recycles shared slots and
   World.Clear rebuilds the shared free lists.
2. Manifold collision scratch was created lazily on first narrow-phase
   use (~20 objects per fresh pool object). Now created eagerly in the
   Manifold constructor so pool growth is the only allocation moment.
3. IslandMethods.SplitIsland allocated fresh Array(Of Integer) scratch per
   split (splits run every step at rest). Now uses world-owned
   SplitBodyIdsScratch/SplitStackScratch, converted to IntegerList (Clear
   retains capacity); DfsVisitBody/DfsVisitJoints take the IntegerList.
4. First transfer into a fresh sleeping solver set grew 36 column lists
   (sensor sleep: ~608 allocations). Added Reserve(rows) to BodySims (28
   columns), BodyStates (8), ContactSims (16); TrySleepIsland pre-sizes
   from island.BodyCount/island.ContactCount, WakeSolverSet and
   MergeSolverSets pre-size their targets from incoming counts.
5. Per-colour constraint pools grew their fill by exactly the needed
   count, so the wake transient's creeping overflow peak allocated one
   ContactConstraint per new peak element every cycle. PrepareColorContacts
   now doubles the fill on growth (amortised; sub-doubling transients are
   allocation-free).
6. World.ApplyContinuousHit allocated Rot.NLerp and Vector2.Lerp
   interpolation objects per continuous hit (2 per hit step in the bullet
   gate). Scalarised with bit-identical operation order (NLerp:
   omt*q1+t*q2 normalised; Lerp: a+(b-a)*t).

Fixture warm-ups made representative (zero assertions unchanged):
- Sleep/wake gate: wall rescaled 20x10 to 12x10 (120 bodies) to fit the
  30s per-test timeout; four wake/36-step/forced-sleep warm-up cycles so
  the per-colour constraint pools and transfer stores reach the high-water
  marks the measured window observes.
- Sensor gate: after the 180-step crossing warm-up, a forced
  sleep/wake cycle plus 150 settle steps so the first sleeping-set creation
  happens during warm-up and the recycled set serves later sleeps.
- Bullet gate: three reset flights prime the swept-query scratch, TOI
  buffers, and per-colour constraint high-water marks.

Evidence trail (per-step stage attribution):
- Sleep/wake: 3,153 → 153 after the shared pool → 147 after split-scratch
  and reserves → 0 with representative warm-up + fill-doubling.
- Bullet: 101 → 26 (shared pool) → 4 (scalarised hit) → 0 (three flights).
- Sensor: 626 → 608 (reserves did not move it; the cost was first-use
  store capacity inside SleepIslands) → 0 with the forced cycle warm-up.

Validation after probe removal:
- objo test --project Physics2D.Tests: 335 passed, 1 failed — the only
  failure is the expected stale-distribution checksum
  (DistributionTests.TestDistributionInputsMatchSharedSources); dist
  regenerates once the optimisation work is accepted.
- objo test --project Physics2D.Benchmarks: 19 passed, 0 failed.
- All four Stage 12 allocation gates pass in the default 30s timeout
  (sleep/wake 25.8s).
- Temporary probe code and the Stage12DiagTests file were removed; the
  working tree contains only the engine fixes and gate warm-up changes.

Next: resume step 5 (right-size stage12-world-queries), then step 6
(stabilise the ten scenario definitions), then the isolated-worktree
baseline (step 7), candidate accept/revert (step 8), dist regeneration and
full gates (step 10), PERFORMANCE.md and the ledger (step 11).

### 2026-09-04 — resume steps 1-3 complete

- Working tree confirmed clean at 956a73e (checkpoint commit is HEAD).
- Fixture fixes applied (3 edits, +10/-1 lines, git diff --check clean):
  - Stage12Workloads ClearRebuildWorkload.Rebuild: chainDef.Body = ground.
  - Stage12Workloads SensorFieldWorkload.Prepare: moverDef.EnableSensorEvents = True.
  - Stage12Tests TestSensorOverlapAllocatesNothing: moverDef.EnableSensorEvents = True.
- Build: objo build Physics2D.objosln --project Physics2D.Benchmarks
  --output build/benchmarks — success in 5 s.
- Focused scenario: --only stage12-clear-rebuild → exit 0, median
  88.355 ms, checksum -4364940994124906251, JSON in /tmp/physics2d-stage12.
- Focused tests: objo test --project Physics2D.Tests --filter
  "Stage12Tests.*" → 1 passed, 3 failed in 27.54 s. TestPyramidWarmStep-
  AllocatesNothing passes. Sensor test now publishes warm-up events and
  fails only on allocations (626). Bullet 101 and sleep/wake 3,153 match
  the pre-fix counts, so the fixture fixes changed nothing else.
- Benchmark project tests: 19 passed, 0 failed in 18.37 s.
- CLI filter semantics confirmed from Objo.Studio.Core TestSelection:
  anchored glob over QualifiedClass.TestMethod; use "Stage12Tests.*".
- Next: resume step 4 (allocation diagnosis). No long benchmark runs have
  been started yet; no baseline files have been re-recorded.

## Useful Commands

    OBJO=/Users/garry/Repos/Objo/src/studio/Objo.Cli/bin/Debug/net10.0/objo

    $OBJO check /Users/garry/Repos/Physics2D/Physics2D.objosln
    $OBJO test /Users/garry/Repos/Physics2D/Physics2D.objosln --project Physics2D.Tests
    $OBJO test /Users/garry/Repos/Physics2D/Physics2D.objosln --project Physics2D.Benchmarks
    $OBJO build /Users/garry/Repos/Physics2D/Physics2D.objosln --project Physics2D.Benchmarks --output build/benchmarks

    build/benchmarks/macOS-Apple-Silicon/Physics2D_Benchmarks --only stage12-clear-rebuild --output /tmp/physics2d-stage12 --name clear-rebuild
    build/benchmarks/macOS-Apple-Silicon/Physics2D_Benchmarks --only stage12-pyramid-256 --output /tmp/physics2d-stage12 --name pyramid-256
    build/benchmarks/macOS-Apple-Silicon/Physics2D_Benchmarks --profile 30

    python3 tools/audit_api.py
    python3 tools/assemble_module.py --check
    tools/check_distribution.sh

After any Objo engine change, rebuild the CLI and both TestHost
configurations exactly as required by AGENTS.md.

## Non-Negotiable Cautions

- Do not update the Stage 12 ledger row until every §19 exit criterion
  passes.
- Do not call the incomplete stage12-before JSON an accepted baseline.
- Keep HANDOFF.md with the checkpoint until Stage 12 is complete; update or
  remove it deliberately in the eventual coherent Stage 12 commit.
- Do not edit the generated distribution by hand.
- Do not weaken allocation assertions or correctness tolerances merely to
  make a gate pass.
- Do not change the frozen public API without a decision record and user
  approval.
- Do not push, publish, or create a release without explicit user
  direction.
- Preserve the pinned Box2D 3.1.1 provenance and do not copy secondary-port
  implementation code.
