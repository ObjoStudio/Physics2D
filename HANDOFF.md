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
- The current working tree compiles, but is deliberately not green:
  332 of 336 Physics2D tests pass. The four failures and their diagnoses
  are recorded below.
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

   It fails before allocation measurement:
   sensor events published during warm-up: Expected True, Actual False.

   This has a concrete fixture error. The sensor shapes enable sensor
   events, but the visiting mover shapes do not. The engine's documented
   SensorOverlapTest ignores a visitor whose EnableSensorEvents is False,
   and the established Stage 8 sensor tests enable the flag on both sides.
   Set moverDef.EnableSensorEvents = True, rerun, and only then assess its
   allocation result.

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

### Focused benchmark failure

Running the current executable with:

    --only stage12-clear-rebuild

fails immediately with:

    InvalidArgumentException: ChainDefinition.Body must not be Nothing

The fixture creates chainDef, sets points and IsLoop, but never sets its
owning body. The intended owner is almost certainly the already-created
ground; set chainDef.Body = ground and verify against the public chain
construction rules.

### Filter warning

objo test ... --filter Stage12Tests and --filter AllocatesNothing both
selected zero tests even though 336 were discovered. Do not treat an
exit-zero filtered run as evidence. Until the CLI filter syntax is
confirmed, run the full Physics2D.Tests project and inspect its
passed/failed summary.

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

1. Read the Stage 12 plan and inspect the complete working diff. Preserve
   all files and confirm the exact status above.
2. Fix only the two known invalid fixtures first:
   - set the clear/rebuild chain's Body;
   - enable sensor events on the Stage 12 visitor shapes.
3. Rebuild the benchmark app and run each affected scenario/test before a
   full suite. Confirm the CLI's real test-filter semantics or use the full
   Physics2D.Tests project.
4. Diagnose the 3,153 sleep/wake allocations and 101 bullet allocations.
   Decide from evidence whether each is inadequate warm-up/capacity
   coverage or a real engine allocation. Keep zero as the required normal
   path.
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
