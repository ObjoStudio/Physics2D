# Physics2D Native Objo Port — Staged Implementation Plan

## 1. Mission

Build a complete, fast, readable, and dependency-free native Objo port of the
Box2D 3.1.1 rigid-body physics engine. Users add one public module named
`Physics2D` to an Objo Studio project and write:

```objo
Import Physics2D
```

The project must culminate in all of the following:

- the native `Physics2D` module;
- a VCS-friendly Objo Studio solution containing the canonical source;
- a deterministic single-file module distribution for current Studio users;
- a complete automated correctness and regression test suite;
- reproducible Release-mode performance benchmarks;
- a desktop demonstration application;
- a command-line smoke application proving that the module has no desktop
  dependency;
- API, tutorial, architecture, performance, and porting documentation; and
- complete upstream licensing and provenance records.

The implementation source of truth is Box2D tag `v3.1.1`, commit
`8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3`. Box2D 3.1.1 uses C `float`; Objo
uses `Double`. Physics2D therefore preserves algorithms and behaviour rather
than promising bit-for-bit equality with C.

## 2. Definition Of Done

The port is complete only when every stage in this document is complete and
all of these statements are true:

1. A clean Objo Studio installation at the documented minimum version can add
   `dist/Physics2D.objobasic` to a new project, write `Import Physics2D`, compile,
   and simulate a falling dynamic body without any other downloaded component.
2. The editable `.objosln` Shared Code and generated distribution expose the
   same public API and behaviour.
3. The module compiles in a command-line project and in a desktop project. It
   imports no desktop-only type.
4. Every Box2D 3.1.1 public rigid-body capability is mapped in
   `docs/PORTING.md` to an implemented Physics2D API, a protected internal
   implementation, or an explicit v1 exclusion permitted by this plan.
5. Circles, capsules, segments, convex polygons, chains, contacts, sensors,
   materials, filters, sleeping, continuous collision, world queries, shape
   casts, ray casts, body-move events, contact events, sensor events, and all
   seven joint families work and have tests.
6. Fixed-seed end-to-end scenarios match the committed Box2D-derived golden
   results within documented tolerances and repeat with identical Physics2D
   checksums on the same Objo runtime and platform.
7. `World.Step` performs no steady-state allocation after warm-up in the normal
   path. Explicitly enabled public event snapshots and allocating query
   overloads are the only documented exceptions.
8. The benchmark suite has no unexplained regression over the accepted Stage 2
   representation baseline or the most recent accepted stage baseline.
9. All public APIs, units, ownership rules, allocations, and exceptional cases
   are documented and demonstrated.
10. The demo runs fluidly on the baseline development machine, uses a fixed
    physics timestep, and demonstrates the major feature groups interactively.
11. There are no external runtime dependencies, unfinished placeholders,
    silently skipped tests, or undocumented feature gaps.
12. Licensing, third-party notices, upstream mapping, and generated artifact
    checks pass from a clean checkout.

## 3. Locked Product And API Decisions

These decisions let an implementing agent proceed without repeatedly asking
for product direction.

### 3.1 Naming and public shape

- Root module: `Physics2D`.
- Public class names: `World`, `Body`, `Shape`, `Joint`, and descriptive
  specialised names.
- Construction data: `WorldSettings`, `BodyDefinition`, `ShapeDefinition`,
  `ChainDefinition`, and one definition class per joint family.
- Geometry: `Circle`, `Capsule`, `Segment`, `Polygon`, and `Chain`.
- Supporting public data: `Filter`, `SurfaceMaterial`, `Bounds`, `MassData`,
  `RayHit`, `ShapeCastHit`, `ContactEvent`, `SensorEvent`, and `BodyMove`.
- Enums: `BodyType`, `ShapeType`, `JointType`, and other closed sets where a
  typed enum is clearer than flags or magic integers.
- Do not expose `Fixture`. Box2D 3 shapes attach directly to bodies.
- Do not expose C IDs. Public façade objects validate a private index and
  generation against their owning world.
- Do not use `b2`, `Box2D`, `Def`, `AABB`, or other upstream abbreviations in
  ordinary public names. `Bounds` is the public term for an axis-aligned
  bounding box.

### 3.2 Maths

- The actual built-in type is `Vector2`; use it throughout the public API.
- Do not create `Physics2D.Vector2`, `Vector2D`, or a private vector class.
- Use mutable/caller-output `Vector2` operations in hot paths when they win the
  Stage 2 bake-off. Avoid allocating vector operators there.
- Use built-in `Matrix` for appropriate public 2x2 operations. Internal solver
  storage may use scalar components if benchmarks show that is faster.
- Keep Box2D-specific cosine/sine rotations, rigid transforms, sweeps, planes,
  simplex caches, manifolds, and solver matrices protected inside the module
  unless the standard-library audit proves a type is genuinely general.

### 3.3 Units, axes, and stepping

- Length: metres. Mass: kilograms. Time: seconds. Angles: radians.
- No hidden conversion between physics and pixels.
- No hidden axis inversion.
- `New World()` uses zero gravity.
- `New World(gravity As Vector2)` is the common gravity constructor.
- Documentation and the demo use positive Y gravity to match Objo canvas
  coordinates. Document the negative-Y alternative for Y-up applications.
- `World.Step(timeStep As Double)` uses four substeps.
- `World.Step(timeStep As Double, substepCount As Integer)` rejects non-positive
  counts and invalid timesteps.
- Applications own the fixed-timestep accumulator. `World.Step` does not read a
  wall clock.

### 3.4 Lifetime and ownership

- A `World` owns every body, shape, chain, joint, contact, and simulation
  buffer created within it.
- `Body`, `Shape`, `Chain`, and `Joint` are stable façade objects while valid.
- `Destroy()` is explicit and idempotent on user-owned façade objects.
- Destroying a body destroys its shapes, chains, contacts, and attached joints.
- `World.Clear()` invalidates all outstanding façade objects and returns the
  world to an empty reusable state.
- Slot reuse increments a generation so a stale façade can never address a new
  object.
- Invalid façade access raises a clear runtime exception naming the invalid
  object and operation.
- `UserData As Object` is allowed on body, shape, chain, and joint definitions
  and façades. It is never read by the solver.

### 3.5 Events and callbacks

- The default solver path never invokes user code. The advanced Box2D custom
  filter, pre-solve, friction-mixing, and restitution-mixing capabilities are
  supported as explicit opt-ins because they can change physical behaviour.
- Advanced callbacks run while the world is locked, may not mutate the world,
  and carry a clearly documented performance and determinism cost. The disabled
  path must not allocate, dispatch, or perform an extra per-contact closure
  check inside the deepest loop.
- Contact, sensor, and body-move information is collected during the step and
  exposed only after the world is unlocked.
- Contact and sensor records are produced only for shapes that request them.
- Objo events may be raised after the step for idiomatic use, but their event
  records must come from the same post-step buffer and their allocation cost
  must be documented.
- High-volume consumers use reusable event batches/views; they are not forced
  to allocate one object per event.
- Queries provide both convenient allocating methods and `Into` forms that
  reuse caller-provided result arrays.
- Advanced query filtering may use a callback outside `World.Step`, but the
  ordinary closest/all/filter overloads must not require callbacks.

### 3.6 Concurrency

- A `World` is single-threaded and its public API is not concurrently mutable.
- The first release uses a deterministic scalar solver and does not expose
  Box2D's task-scheduler callbacks.
- Independent worlds may be run by independent Objo Workers at the application
  level, subject to normal Worker isolation and data-transfer rules.
- Do not divide one world step across Workers; process isolation and transfer
  overhead do not match Box2D's shared-memory task model.

### 3.7 Version 1 scope

Version 1 includes:

- collision-only geometry helpers;
- convex hull construction and validation;
- distance, overlap, ray-cast, shape-cast, and time-of-impact algorithms;
- the dynamic AABB tree and world broad phase;
- static, kinematic, and dynamic bodies;
- circle, capsule, segment, polygon, and chain shapes;
- density, mass, inertia, centre of mass, friction, restitution, rolling
  resistance, tangent speed, material identity, category/mask/group filtering;
- contact creation, persistence, manifolds, warm starting, speculative
  collision, sleeping, islands, constraint graph, and the Soft Step solver;
- continuous collision for fast bodies;
- sensors and post-step events;
- distance, motor, mouse, prismatic, revolute, weld, and wheel joints;
- world overlap, ray, shape-cast, mover/plane, and explosion-style APIs where
  present in Box2D 3.1.1;
- debug rendering through a renderer interface independent of desktop
  `Graphics`; and
- validation and statistics useful for testing and development.

Version 1 excludes only C/platform integration features that do not describe
physics behaviour: native SIMD kernels, C task callbacks, custom C allocators,
native timers, and native debug-draw callbacks. Particles are not in Box2D
3.1.1 and are excluded.

## 4. Target Repository Layout

Create and maintain this logical layout. Studio-generated GUID suffixes in the
`.objosln` source filenames are expected and must not be normalised by hand.

```text
Physics2D/
├── AGENTS.md
├── IMPLEMENTATION_PLAN.md
├── README.md
├── LICENSE
├── THIRD_PARTY_NOTICES.md
├── Physics2D.objosln
├── Shared/
│   └── Sources/                    # Canonical nested Physics2D module sources
├── Projects/
│   ├── Physics2D.Tests/            # Objo Test project
│   ├── Physics2D.Benchmarks/       # Command-line benchmark application
│   ├── Physics2D.Smoke/            # Minimal command-line consumer
│   └── Physics2D.Demo/             # Desktop interactive demo
├── docs/
│   ├── GETTING_STARTED.md
│   ├── API.md
│   ├── ARCHITECTURE.md
│   ├── PERFORMANCE.md
│   ├── PORTING.md
│   ├── DEMO.md
│   ├── CONTRIBUTING.md
│   └── decisions/
├── benchmarks/
│   ├── README.md
│   ├── scenarios/
│   ├── baselines/
│   └── results/
├── testdata/
│   ├── golden/
│   └── regression/
├── tools/                          # Deterministic packaging/fixture tools
└── dist/                           # Generated release artifacts
```

`Shared/Sources` is authoritative. `dist` is reproducible output. The normal
test suite must detect a generated module that is stale relative to Shared
Code.

Until source packages ship in the minimum supported Studio version, distribute
`dist/Physics2D.objobasic` as one explicit `Module Physics2D ... End Module`
source. Keep authoring split into navigable nested source items. When Studio
source packages become available, add `.objopackage` as an additional artifact
without removing the single-file form during version 1.

## 5. Internal Architecture

Use an idiomatic façade over a data-oriented core:

```text
Physics2D public API
├── World / Body / Shape / Chain / Joint façades
├── Definitions, geometry values, query results, and events
└── DebugRenderer
        │
        ▼
Protected indexed core
├── Foundation: constants, validation, IDs, pools, bit sets, tables
├── Collision: hulls, distance, manifolds, casts, time of impact
├── BroadPhase: bounds, dynamic tree, move/pair buffers
├── Dynamics: body/shape/contact/joint stores and solver sets
├── Solver: islands, graph colours, contacts, joints, sleeping, CCD
└── Diagnostics: validation, statistics, debug drawing
```

The façade object stores only its owner, slot index, and generation plus
application-facing user data when appropriate. Hot state is held in dense
stores chosen by Stage 2 benchmarks. Keep active body states compact and
separate from sleeping/disabled state so the solver does not scan irrelevant
objects.

Do not transliterate C structs into one Objo class per array element. Do not
transliterate C pointers into a web of Objo object references. Preserve the
algorithm, invariants, and traversal order while choosing a representation that
fits Objo's VM.

## 6. Cross-Cutting Quality Gates

Every stage after Stage 1 must pass these gates before its status becomes
complete.

### 6.1 Correctness gate

- All existing tests pass together in a fresh process.
- New functionality has focused unit tests and at least one integration path.
- Invalid public inputs have explicit tests.
- Internal validation functions pass after every seeded stress scenario.
- Golden comparisons name the upstream function or sample that produced the
  expected result and record the tolerance rationale.

### 6.2 Performance gate

- Run Release mode after a warm-up period.
- Record hardware, OS, Objo/Studio version, git commit, scenario parameters,
  iteration count, median, p95, maximum, and allocation count.
- Compare against the accepted baseline for every affected scenario.
- Investigate any regression above 5%. Accept it only with a written design
  decision demonstrating a correctness or API benefit worth the cost.
- Reject benchmark changes that reduce simulated work, contacts, solver
  iterations, substeps, or checksum coverage.

### 6.3 Documentation gate

- Every new public symbol is documented in source and `docs/API.md`.
- At least one example compiles for each new public feature group.
- Performance-sensitive APIs identify allocating and reuse forms.
- `docs/PORTING.md` maps the new subsystem to pinned upstream files/functions.
- Non-obvious internal representations are explained in
  `docs/ARCHITECTURE.md` or a decision record.

### 6.4 Packaging gate

- The authoring solution compiles.
- The generated single-file module is current.
- The generated module compiles in the clean smoke project, which references
  only the distribution artifact rather than Shared Code.
- No runtime dependency or upstream C source leaks into distribution output.

## 7. Stage 0 — Bootstrap, Provenance, And Feature Inventory

### Goal

Create a reproducible project foundation and eliminate ambiguity about what is
being ported.

### Work

1. Create the VCS-friendly `Physics2D.objosln` solution through current Objo
   Studio or its supported automation, not by guessing GUID metadata.
2. Add a root `Physics2D` module to Shared Code.
3. Add empty-but-compiling `Physics2D.Tests`, `Physics2D.Benchmarks`,
   `Physics2D.Smoke`, and `Physics2D.Demo` projects of the correct Studio types.
4. Make the smoke project import the empty module to prove nested source
   assembly and project visibility.
5. Create `THIRD_PARTY_NOTICES.md` containing the Box2D MIT notice.
6. Create `docs/PORTING.md` and record:
   - upstream repository URL;
   - tag and full commit;
   - source retrieval date;
   - Box2D licence;
   - the decision to use Double;
   - secondary references that must not be copied.
7. Inventory every public declaration in Box2D 3.1.1's public headers. For each
   symbol, record its subsystem, intended Physics2D name, visibility, target
   stage, and any approved exclusion.
8. Inventory every relevant upstream source and test file. Create a subsystem
   mapping that can be updated as functions are ported.
9. Record the current Objo language/API/Studio version and identify the minimum
   version that provides `Vector2`, `Matrix`, nested module source items, test
   projects, and VCS-friendly solutions.
10. Create `docs/decisions/0001-upstream-and-scope.md` and
    `0002-module-distribution.md`.
11. Define a deterministic generated-file header and a packaging design for
    assembling nested Shared Code into one `Physics2D.objobasic` module.

### Exit criteria

- The solution opens and all four projects compile.
- The public feature inventory accounts for every Box2D 3.1.1 public symbol.
- The licence and upstream commit are recorded.
- No implementation has been copied from an unpinned or BSD secondary source.
- The minimum Objo version and current distribution method are documented.

## 8. Stage 1 — Test Oracle, Harnesses, And Packaging Pipeline

### Goal

Make every later porting slice testable, benchmarkable, and distributable before
substantial physics code arrives.

### Work

1. Build reusable Objo test helpers:
   - exact assertions for IDs, counts, enums, and Boolean invariants;
   - absolute-plus-relative `Double` comparison;
   - `Vector2` and matrix comparison;
   - unordered-pair comparison where traversal order is not contractual;
   - deterministic array and world-state checksums;
   - expected-exception helpers.
2. Establish tolerance classes rather than one global epsilon: scalar maths,
   geometry, manifold, transforms, velocities, and long-running scenes.
3. Create a fixed-seed deterministic pseudo-random generator in test code so
   stress inputs do not depend on platform randomness.
4. Build an optional reference-fixture generator against the pinned C Box2D
   checkout. It may be a developer tool, but generated fixture files must be
   plain committed data and ordinary tests must not invoke C.
5. Generate initial golden fixtures for vector/matrix helpers, hulls, distance,
   ray casts, shape casts, manifolds, mass properties, and selected simulation
   scenes. Add a manifest with upstream commit and generation command.
6. Create the benchmark runner with:
   - warm-up iterations;
   - fixed measured iteration counts;
   - median, p95, and maximum calculation;
   - a final checksum;
   - scenario metadata and machine metadata;
   - machine-readable and human-readable output.
7. Add a test-only allocation measurement route. Prefer existing Objo profiler
   support. If it cannot report allocations, add narrowly scoped profiler/test
   instrumentation to the Objo runtime following that repository's rules.
8. Implement the deterministic module assembler. It must:
   - read the canonical Shared Code source graph;
   - preserve a stable topological declaration order;
   - emit exactly one `Physics2D` module;
   - include licence/provenance and generated-file warnings;
   - normalise line endings and final newline;
   - reject a public declaration outside the root module;
   - fail on duplicate names or unresolved parents;
   - produce byte-identical output from the same checkout.
9. Add an automated stale-distribution check.
10. Make the smoke project compile against a copied/generated distribution
    source rather than the canonical Shared Code source.

### Exit criteria

- A deliberately wrong golden value causes a clear focused test failure.
- Re-running the fixture generator produces no diff.
- Re-running the module assembler produces byte-identical output.
- The benchmark runner rejects a scenario without a checksum.
- Allocation measurement distinguishes an allocating vector expression from a
  mutating vector operation.
- The smoke project imports the generated skeleton module successfully.

## 9. Stage 2 — Objo Maths Audit And Representation Bake-Off

### Goal

Choose the fastest teachable Objo storage strategy before porting an
architecture that would be expensive to replace.

### 9.1 Standard-library audit

Audit the built-in `Vector2`, `Matrix`, `Double`, `Maths`, and `Array` APIs
against Box2D's maths needs. Start from the fact that current `Vector2` already
provides mutable arithmetic, `AddScaled`, normalisation, rotation, dot product,
all three 2D cross-product forms, distance, interpolation, min/max/clamp, and
`ArrayOf`. Current `Matrix` is already a mutable 2x2 matrix with allocation-free
transform/multiply output overloads.

Evaluate, with standalone usefulness and benchmarks, these likely additions:

- `Vector2.LeftPerpendicular()` and `RightPerpendicular()`, including
  allocation-free output or mutating forms;
- allocation-free scalar/vector cross-product overloads;
- component-wise absolute-value output if it is repeatedly useful;
- `Matrix.Inverse()` / `Inverse(out)` / an in-place inverse with defined
  singular behaviour;
- `Matrix.Solve(vector)` / `Solve(vector, out)` for a 2x2 linear system;
- a concise finite-number helper only if repeated `IsNaN`/`IsInfinity` checks
  materially harm clarity or speed.

Do not add `Rotation`, `Transform`, `Sweep`, `Plane`, or `Bounds` to the standard
library merely because Box2D has such structs. First demonstrate at least one
non-physics use, coherent general semantics, and a benefit over existing
`Matrix`/`Vector2` composition.

If an addition passes the gate:

1. Create/follow the required Objo feature issue.
2. Implement runtime registration and compile-time metadata.
3. Add engine tests, autocomplete/introspection tests, and user documentation.
4. Run the Objo repository's required validation.
5. Update Physics2D's minimum Objo API version.
6. Use the released/accepted standard-library API in Physics2D; do not carry a
   duplicate compatibility implementation indefinitely.

If no addition is justified, record the evidence in
`docs/decisions/0003-stdlib-maths-audit.md` and proceed.

### 9.2 Representation candidates

Build equivalent representative kernels using:

1. one class object per body/contact/tree node;
2. arrays of mutable `Vector2` plus scalar property arrays;
3. parallel X/Y `Double` arrays;
4. flattened numeric arrays with documented field offsets;
5. compact state objects stored densely;
6. indexed tree nodes versus linked node objects; and
7. `MemoryBlock` only as an evidence-gathering candidate.

The kernels must include:

- integration of at least 10,000 active body states;
- rotation/transform and inverse-transform operations;
- dot, cross, perpendicular, normalisation, and 2x2 solve workloads;
- dynamic-tree insert, move, query, and rebuild workloads;
- pair generation and deduplication;
- a simplified contact-constraint iteration with warm-start data;
- allocation and destruction/reuse pressure; and
- sequential and pseudo-random access patterns.

Run every candidate in Release mode with identical work and checksums. Record
source readability and invariant complexity alongside timing and allocation.

### Decision rules

- Normal `World.Step` must reach zero steady-state allocation after warm-up.
- Prefer the fastest candidate when the difference is repeatable and material.
- Treat less than 5% as noise unless repeated across multiple kernels.
- A somewhat slower representation may be selected only when it materially
  reduces correctness risk and still meets the performance budget; document
  that trade-off.
- It is acceptable and likely to choose a hybrid: for example, mutable
  `Vector2` at public/cold boundaries and scalar or vector arrays in solver
  state.

### Exit criteria

- Raw results and machine metadata are committed.
- A decision record selects body, solver, contact, joint, and tree
  representations.
- The record lists rejected candidates and why.
- Required standard-library work is complete and validated, or explicitly
  rejected with evidence.
- A frozen Stage 2 performance baseline exists.

## 10. Stage 3 — Foundation Containers And Identity

### Goal

Implement the private infrastructure required by every simulation subsystem.

### Work

1. Port and adapt constants, numeric validation, clamping, angle unwinding, and
   deterministic trigonometric helpers required by Box2D 3.1.1.
2. Implement generation-based identity and slot allocation.
3. Implement dense stores with reserve, grow, swap-remove, and clear operations.
4. Implement reusable scratch/arena semantics using Objo arrays and logical
   counts; no object creation during a warmed step.
5. Implement bit sets, including set, clear, union, intersection, iteration,
   growth, and validation.
6. Implement the integer hash/pair table with deterministic probing and
   resizing.
7. Define private null-index and invalid-generation rules. Avoid ambiguous
   zero/sentinel use.
8. Implement validation methods for every container and run them aggressively
   in tests.
9. Add seeded stress tests for allocation, free, stale access, resize, clear,
   collision chains, and repeated world reuse.
10. Document each structure with diagrams and complexity notes in
    `docs/ARCHITECTURE.md`.

### Exit criteria

- Millions of seeded allocate/free/reuse operations preserve every invariant.
- Stale generations never validate after slot reuse or clear.
- Hash-table and bit-set results match simple reference implementations.
- Warm container operations allocate nothing after reserve.
- Foundation benchmarks remain within the accepted Stage 2 representation
  envelope.

## 11. Stage 4 — Geometry, Distance, Casts, And Manifolds

### Goal

Complete the collision library independently of world simulation.

### Work order

1. Protected rotation, transform, plane, sweep, and internal bounds helpers.
2. Public reusable geometry classes with validated constructors: `Circle`,
   `Capsule`, `Segment`, and `Polygon`. A body shape copies the supplied geometry
   data so later edits to the standalone geometry object do not mutate the
   world implicitly.
3. Convex hull computation, validation, winding, centroid, normals, rounded
   polygon radius, box factories, and transformed geometry.
4. Mass and rotational inertia computation.
5. Point tests and local bounds.
6. Distance proxy, simplex, simplex cache, GJK distance, and witness points.
7. Ray casts for every primitive.
8. Shape casts and time-of-impact support data.
9. Manifold generation for every supported primitive pairing.
10. Chain-segment one-sided collision and ghost-vertex handling.
11. Plane solver and character-mover geometry functions present in the pinned
    upstream release.
12. Public collision-only helpers that are useful without a `World`, expressed
    with Objo names and allocating plus caller-output forms where appropriate.

For every function family, port the upstream tests first or alongside the
implementation. Add degeneracy cases: zero-length segments, nearly collinear
points, duplicate hull points, maximum polygon vertices, tangent hits, initial
overlap, parallel casts, very small radii, and invalid finite values.

### Exit criteria

- Every primitive pairing returns the expected manifold type, point count,
  normal, separation, anchors, and stable feature identity.
- Golden distance, hull, ray-cast, shape-cast, mass, and manifold cases pass.
- Geometry methods reject invalid input with documented exceptions.
- Collision-only APIs work without constructing a world.
- Reused-output geometry loops allocate nothing after warm-up.

## 12. Stage 5 — Dynamic Tree And Broad Phase

### Goal

Build and validate the spatial index and candidate-pair pipeline before adding
contact solving.

### Work

1. Implement protected bounds operations: union, containment, overlap,
   perimeter, enlargement, centre, extents, ray input, and shape-cast input.
2. Implement the dynamic tree with indexed nodes, free list, insertion,
   removal, proxy movement, category bits, queries, ray casts, shape casts,
   rebuild, height/area metrics, and validation.
3. Implement proxy types for static, kinematic, and dynamic shapes.
4. Implement move buffering and deterministic pair generation.
5. Implement pair deduplication using the Stage 3 table.
6. Keep query traversal buffers reusable and non-recursive where practical.
7. Provide a protected or advanced public `DynamicTree` only if the Stage 0 API
   inventory and API review show a clear Box2D-compatible consumer need. World
   queries must not require users to understand the tree.
8. Add adversarial tests: sorted insertion, clustered movement, huge bounds,
   repeated rebuild, category changes, proxy churn, and queries that terminate
   early.

### Exit criteria

- Tree validation passes after every mutation in seeded stress tests.
- Query/ray/cast results match brute-force bounds tests.
- Broad-phase candidate pairs match brute-force overlap pairs after filtering.
- Pair output is deterministic for identical creation and movement order.
- Warm queries and pair generation allocate nothing.
- Tree quality and timing baselines are recorded.

## 13. Stage 6 — World, Bodies, Shapes, Chains, And Queries

### Goal

Deliver the first usable, non-solving public world API and validate lifecycle
semantics before contacts add complexity.

### Work

1. Implement `WorldSettings` and the three `World` constructors.
2. Implement façade validation and the world/index/generation contract.
3. Implement `BodyDefinition`, body creation, body type changes, transforms,
   velocities, damping, gravity scale, fixed rotation, bullet state, enabled
   state, awake state, sleep threshold, and user data.
4. Implement `ShapeDefinition`, `Filter`, `SurfaceMaterial`, shape creation,
   destruction, density/material/filter changes, and user data.
5. Implement chain creation, lifecycle, material arrays, loops, and one-sided
   segments.
6. Maintain mass, centre of mass, and inertia as shapes or density change.
7. Synchronise proxies when transforms or shape geometry changes.
8. Implement body local/world point and vector conversion.
9. Implement overlap, closest ray cast, all-hit ray cast, shape cast, and bounds
   queries, including category filtering and caller-result reuse.
10. Implement world origin/length-scale configuration only if present and
    semantically required by the pinned upstream.
11. Implement `World.Clear`, object destruction cascades, stale façade errors,
    and capacity reuse.
12. Build a minimal public API smoke scene with one static ground shape and one
    dynamic body, even though solving is not yet enabled.

### Exit criteria

- Creation/destruction and slot reuse tests pass for every façade type.
- Body mass data matches upstream golden fixtures.
- World and local transform helpers round-trip within tolerance.
- Queries match brute-force geometry results.
- Public collection/result ownership is documented and tested.
- `World.Clear` retains reusable capacity and allocates nothing on an equivalent
  second build after warm-up, except façade objects explicitly recreated by the
  application.

## 14. Stage 7 — Contacts, Islands, Constraint Graph, And Soft Step

### Goal

Implement the discrete rigid-body solver and reach the first physically complete
stacking simulation.

### Work order

1. Contact identity, creation/destruction, broad-phase pair consumption, and
   shape filtering.
2. Default category/mask/group filtering plus the opt-in custom contact-filter
   hook, with a zero-overhead disabled path.
3. Contact manifold update, persisted point matching, sensor separation, and
   warm-start impulse storage.
4. Opt-in pre-solve manifold editing and contact disabling, including locked
   world rules and exception handling.
5. Default material mixing plus optional friction and restitution mixer hooks,
   again with a zero-overhead disabled path.
6. Awake, sleeping, and disabled solver sets with dense state migration.
7. Island graph construction, linking/unlinking, splitting, merging, and
   validation.
8. Constraint graph colours and overflow colour.
9. Contact constraint preparation.
10. Soft Step substep integration and bias/softness calculation.
11. Warm starting.
12. Normal and friction impulse solving, including restitution, rolling
   resistance, and tangent speed.
13. Position/velocity finalisation, transform/proxy synchronisation, and force
    clearing.
14. Sleeping decisions and deterministic set transfer.
15. World locking so mutation during `Step` and advanced callbacks is rejected
    predictably.
16. Public force, torque, impulse, wake, and sleep methods.

Port scalar kernels first. Preserve Box2D's algorithmic stage order. Do not
imitate SIMD lane structures when Objo cannot exploit them; use the same maths
over the Stage 2 winning representation.

### Required scenes

- one falling body;
- mixed circle/capsule/polygon collisions;
- a 40-level pyramid;
- a tall sleeping stack;
- friction and restitution grids;
- kinematic platforms;
- rolling-resistance tests;
- changing density/filter/material at runtime;
- world clear and rebuild; and
- deterministic repeated runs.

### Exit criteria

- Required scenes remain stable and match accepted golden checkpoints.
- Resting stacks sleep and remain asleep without visible drift.
- Warm starting measurably improves convergence and can be regression-tested.
- The world refuses unsafe mutation while locked and allows documented
  post-step mutation.
- Custom filter, pre-solve, and material mixer hooks match golden cases, reject
  world mutation, and add no measurable work when they are not configured.
- A warmed `World.Step` with events disabled allocates zero objects.
- The 40-level pyramid benchmark establishes the first full-solver baseline.

## 15. Stage 8 — Continuous Collision, Sensors, And Events

### Goal

Complete high-speed collision handling and the safe post-step observation model.

### Work

1. Port time-of-impact and continuous-collision candidate selection.
2. Implement bullet and fast-body handling, speculative contacts, clipping,
   and proxy enlargement according to Box2D 3.1.1.
3. Add tests for tunnelling through thin segments, fast rotating shapes,
   initial overlap, bullet-versus-dynamic, and continuous enable/disable.
4. Implement sensor overlap begin/end tracking and destruction edge cases.
5. Implement internal primitive event buffers for:
   - contact begin/end/hit;
   - sensor begin/end; and
   - moved bodies that did not fall asleep.
6. Expose reusable event-batch views with a documented validity window ending
   at the next `Step`, `Clear`, or destructive world operation.
7. Add idiomatic Objo events raised after the world unlocks. Materialise stable
   public event records only when the user requests that convenience path.
8. Confirm handlers may safely queue or perform documented post-step changes.
9. Add tests for event order, duplicate suppression, destroyed objects, sleeping
   transitions, retained event records, and zero-subscriber behaviour.

### Exit criteria

- Bullet and continuous-collision golden scenes do not tunnel.
- Sensor/contact event sequences match upstream intent for create, persist,
  separate, destroy, disable, and filter-change cases.
- No user code runs while the solver is locked.
- Event buffers do not allocate after warm-up.
- With all public event production disabled, Stage 7 step allocation and timing
  remain unchanged within measurement noise.

## 16. Stage 9 — Joints

### Goal

Implement all Box2D 3.1.1 joint families without destabilising the contact
solver.

### Common joint slice

For each joint family, complete all of the following before moving to the next:

1. Definition class with documented defaults and validation.
2. Public joint façade with typed properties and inherited common lifecycle.
3. Protected dense joint state.
4. Creation/destruction and collision-connected behaviour.
5. Solver preparation, warm start, velocity/position solution, and reaction
   force/torque.
6. Runtime property changes, wake-up semantics, and limit/motor state changes.
7. Golden tests, invariant tests, destruction tests, stress scene, and focused
   benchmark.
8. Public example and API documentation.

### Implementation order

1. Distance joint.
2. Mouse joint.
3. Motor joint.
4. Revolute joint.
5. Prismatic joint.
6. Weld joint.
7. Wheel joint.

This order starts with simpler scalar constraints, provides the demo's dragging
interaction early, and leaves the more coupled 2x2 solve cases until the joint
infrastructure is proven.

### Exit criteria

- All seven families satisfy their common slice.
- Limits, motors, springs, damping, softness, and runtime toggles work where
  supported.
- Joint destruction during normal post-step application code leaves no stale
  graph edges or contacts.
- Mixed joint/contact stress scenes remain stable and deterministic.
- Joint benchmarks have no unexplained solver regression.

## 17. Stage 10 — API Completion, Advanced Operations, And Debug Rendering

### Goal

Finish the useful Objo surface, remove C leakage, and freeze the version 1 API.

### Work

1. Reconcile the Stage 0 public symbol inventory against the implementation.
2. Complete world tuning, enabling/disabling, sleeping controls, explosions,
   movers/planes, counters/statistics, and remaining pinned upstream operations.
3. Complete closest/all/reusable query overloads without making callbacks the
   default API.
4. Define `DebugRenderer` as a project-type-neutral interface using physics
   primitives and colours/data that do not require desktop `Graphics`.
5. Implement `World.DrawDebug(renderer, options)` or the final idiomatic
   equivalent with flags for shapes, joints, bounds, contacts, normals, mass
   centres, graph colours, and text/statistics where supported.
6. Add the desktop adapter in the demo project, not in the core module.
7. Audit every public name, overload, default, exception, ownership rule, and
   allocation promise.
8. Replace C-flavoured names and magic flags before freezing the API.
9. Add compile-only API examples covering every public overload family.
10. Generate the first complete `docs/API.md` from verified declarations or a
    maintained audited source; do not publish aspirational members.
11. Mark the public API frozen for version 1. After this point, breaking changes
    require an explicit decision record and user approval.

### Exit criteria

- The public inventory has no unmapped item.
- No public symbol exposes a C prefix, pointer concept, allocator, task callback,
  internal generation, or solver representation.
- Core sources compile under command-line and desktop profiles.
- Debug drawing works through a fake recording renderer and the desktop adapter.
- Every public declaration is documented and has a compile test.
- The version 1 API review is recorded in a decision document.

## 18. Stage 11 — Interactive Demo And Teaching Examples

### Goal

Create an attractive, self-explanatory desktop application that both validates
the engine and teaches users how to use it correctly.

### Demo architecture

- Use an Objo Desktop project and `GameCanvas` or the current recommended canvas
  control.
- Keep all Physics2D source in Shared Code or use the exact distribution module;
  never duplicate engine code in the demo.
- Use a fixed physics timestep and accumulator, interpolate rendering if useful,
  and clamp extreme frame catch-up.
- Use one explicit pixels-per-metre value in the rendering adapter.
- Keep camera/render transforms separate from physics coordinates.
- Draw with standard Objo graphics primitives so the demo needs no downloaded
  assets.
- Show frame time, physics step time, body/contact/joint counts, sleeping count,
  and substeps.

### Required interactive scenes

1. **Welcome playground:** ground, boxes, circles, capsules, and polygons with
   click/touch spawning.
2. **Pyramid and stack:** sleeping, wake-up, stability, and stress.
3. **Materials:** friction, restitution, rolling resistance, and tangent speed.
4. **Sensors and filters:** visible begin/end events and category/mask/group
   filtering.
5. **Joints:** an interactive gallery containing all seven families.
6. **Continuous collision:** bullets crossing thin obstacles with CCD toggle.
7. **Queries:** cursor ray cast, overlap region, and shape cast visualisation.
8. **Chains:** one-sided terrain, loops, and ghost-collision behaviour.
9. **Character mover/planes:** only if present in the completed public API.
10. **Benchmark view:** a visually throttled 40-level pyramid or equivalent
    stress scene separate from benchmark measurement.

### Required controls

- pause/resume;
- single step;
- reset scene;
- scene selection;
- gravity toggle or direction control;
- debug-draw layer toggles;
- body dragging using a mouse joint;
- spawn shape controls;
- CCD/sleeping toggles where safe; and
- visible explanatory text for the current scene.

### Teaching material

Create `docs/DEMO.md` explaining:

- where each scene's code lives;
- the fixed-step loop;
- metre-to-pixel conversion;
- body and shape construction;
- safe event handling;
- destruction/lifetime;
- query result reuse; and
- why the demo avoids per-frame allocation.

Extract small focused examples into `examples` sections of
`docs/GETTING_STARTED.md` and `docs/API.md`; do not force readers to reverse
engineer the full demo.

### Exit criteria

- Every required scene and control works.
- The demo can run for at least 30 minutes of automated scene cycling without an
  exception, unbounded growth, invalid tree, or solver assertion.
- Rendering code never mutates the world during a locked step.
- The demo uses only documented public APIs.
- A new user can follow `docs/DEMO.md` to locate and understand each feature.

## 19. Stage 12 — Full Optimisation And Allocation Audit

### Goal

Profile the complete port and make it as fast as practical in native Objo while
preserving correctness and teachability.

### Required benchmark scenarios

- 40-level pyramid for 256 frames;
- large sleeping and waking stacks;
- tumbler with continuous creation/destruction;
- mixed dense contacts;
- bullet/CCD stress;
- dynamic-tree churn and rebuild;
- ray-cast, shape-cast, and overlap query throughput;
- sensor-heavy scene;
- each joint family and a mixed-joint machine;
- world clear/rebuild loops; and
- the demo's heaviest representative scene without drawing.

### Work

1. Profile total time, per-stage time, call counts, and allocations.
2. Rank hotspots by total cost; do not optimise based on source appearance.
3. For each material hotspot, test focused changes such as:
   - scalarising temporary vector maths;
   - hoisting validation out of inner loops while retaining boundary checks;
   - reducing method dispatch;
   - improving dense store traversal;
   - reserving better capacities;
   - reducing pair-table probes;
   - separating active/cold fields;
   - replacing shifting operations;
   - caching repeated trigonometric/transform values; and
   - simplifying graph-colour overflow processing.
4. Keep a before/after benchmark and correctness result for every accepted
   optimisation.
5. Revert changes that do not beat noise or make invariants substantially harder
   to explain.
6. Verify zero steady-state allocations for the ordinary step, broad phase,
   collision, solver, sleeping, and disabled-event paths.
7. Measure allocating convenience APIs separately so their cost is transparent.
8. Document capacity planning, no-allocation usage, substep trade-offs, and
   query/event reuse in `docs/PERFORMANCE.md`.

### Exit criteria

- All required scenarios have accepted baseline files and raw results.
- No normal-step subsystem allocates after warm-up.
- No unexplained regression exceeds 5% from the best accepted equivalent
  implementation.
- The profiler shows no obviously dominant avoidable allocation or dispatch
  path.
- All correctness and determinism tests still pass after the final optimisation.
- Performance documentation includes honest limitations and reproducible
  commands.

## 20. Stage 13 — Documentation, Distribution, And Release Candidate

### Goal

Make the finished engine consumable by a user who has never read Box2D or this
implementation plan.

### Documentation deliverables

1. `README.md`: purpose, capabilities, one-screen example, installation, demo,
   documentation map, compatibility, performance statement, and licence.
2. `docs/GETTING_STARTED.md`: install the module, create a world, fixed-step
   loop, bodies/shapes, rendering scale, events, destruction, and common
   mistakes.
3. `docs/API.md`: every public type/member, defaults, units, ownership,
   exceptions, allocation, and examples.
4. `docs/ARCHITECTURE.md`: façade/core split, IDs, stores, broad phase,
   contacts, islands, solver stages, sleeping, CCD, events, and data-flow
   diagrams.
5. `docs/PERFORMANCE.md`: benchmark method/results, no-allocation patterns,
   capacity planning, substeps, event/query choices, and profiling guidance.
6. `docs/PORTING.md`: complete upstream source/function mapping, deliberate
   Objo differences, Double precision, and version-update procedure.
7. `docs/DEMO.md`: scene and code guide.
8. `docs/CONTRIBUTING.md`: tests, benchmarks, source style, documentation,
   generated artifacts, and provenance rules.

### Distribution work

1. Regenerate `dist/Physics2D.objobasic` from a clean checkout.
2. Add a semantic version and minimum Objo language/API/Studio compatibility
   record without making the source depend on a runtime package manager.
3. Verify the generated file includes the Physics2D and Box2D MIT notices.
4. Create a clean temporary Studio solution and add only the distribution
   module. Build and run the documented falling-box example.
5. Build and run `Physics2D.Smoke` using the distribution source.
6. Build and manually/automatically smoke `Physics2D.Demo`.
7. Run all tests and Release benchmarks from a clean checkout.
8. Inspect the distribution for external imports, desktop-only types, TODOs,
   debug output, stale generated headers, and non-public implementation leakage.
9. If the minimum supported Studio version now implements source packages,
   produce and validate a deterministic `physics2d-<version>.objopackage` as an
   additional artifact. Otherwise document the single-source installation.
10. Create a release-candidate checklist and record exact artifact hashes.

### Exit criteria

- A clean consumer project succeeds using only the distributed module.
- All documentation code samples compile.
- All repository links and API names are current.
- Distribution regeneration produces no diff.
- Licence/provenance and artifact hashes are complete.
- The release candidate satisfies the global Definition of Done.
- No publishing or remote release action occurs without explicit user approval.

## 21. Progress Ledger

Agents must update this table only after the corresponding exit criteria pass.
Use `Not started`, `In progress`, `Blocked`, or `Complete`. When blocked, link a
short note below the table containing evidence and the precise condition needed
to resume.

| Stage | Status | Evidence |
|---|---|---|
| 0. Bootstrap, provenance, inventory | Complete | `Physics2D.objosln` compiles (all four projects, `objo check` 26.8.6); 422-symbol inventory in `docs/PORTING.md`; provenance in `THIRD_PARTY_NOTICES.md`; decisions 0001/0002 |
| 1. Test oracle, harnesses, packaging | Complete | 10 golden fixtures + `MANIFEST.md` (regeneration byte-identical); wrong-golden gate demonstrated; `Tolerances`/`PhysicsAssert`/`TestRandom` helpers (16 tests); `tools/assemble_module.py` byte-identical with stale-dist test; `tools/check_distribution.sh` clean-room smoke; benchmark runner with checksums, metadata, JSON + human output; allocation gate via `System.AllocationCount` (Objo #1299, commit 485c4fab) proven by allocating-vs-mutating tests (9 tests); abstract `Checksum` rejects checksum-less scenarios at compile time |
| 2. Maths audit and representation bake-off | Complete | Stdlib additions via Objo issue #1302 (commit `5fc54453`: `Vector2.Left/RightPerpendicular`, `Matrix.Inverse`/`InvertSelf`/`Solve`, `Double.IsFinite`; engine suite 4183 passed, docs updated); 8 bake-off candidates with bit-identical checksums (`5870864046903980835` bodies, `1585456338644426542` trees); Release results in `benchmarks/results/stage2-bake-off-2026-08-30T19-31-33.json`; decisions 0003/0004 select parallel scalar arrays for body/solver/contact/joint state and node objects for the dynamic tree; `docs/PORTING.md` minimum version updated; bake-off tests gate zero-allocation scalar kernels and checksum equality |
| 3. Foundation containers and identity | Complete | Foundation in `Shared/Sources`: `PhysicsConstants`, `PhysicsMaths` (deterministic `Atan2`/`ComputeCosSin`/`UnwindAngle`/`HashKey`/bit helpers), `CosSin`, `IdPool`, `GenerationalPool`, `IntegerList`, `DoubleList`, `BitSet`, `PairKeySet`; upstream `test_math`/`test_bitset`/`test_table` sweeps ported; two 1,000,000-op seeded stress tests with periodic `Validate`; reference-implementation comparisons for hash set (scan list) and bit set (Boolean array); stale-generation and clear-invalidation gates pass; zero-allocation gates on all containers and workloads; 81 tests pass (`objo test --all-projects`: 62 + 19); dist regenerated and clean-room smoke passed (`tools/check_distribution.sh`); Release results `benchmarks/results/stage3-foundation-2026-08-30T21-41-14.json` (all four scenarios 0 allocations, Stage 2 checksums unchanged); `docs/ARCHITECTURE.md` written with diagrams, complexity, identity/sentinel rules, and envelope table; `docs/PORTING.md` mappings updated plus deliberate differences 10-11 |
| 4. Geometry, distance, casts, manifolds | Complete | Stage 4 sources in `Shared/Sources`: `Hull` (gift-wrap, `WeldPoint`/`MergeCollinear`), shape values `Circle`/`Capsule`/`Segment`/`Polygon` with mass and AABB methods plus `MassData`/`ShapeProxy`/`AABB`, `Distance` GJK with `Simplex`/`SimplexVertex`/`SimplexCache` and segment distance, `Casts` (ray + shape, circle/capsule/segment/polygon), `TimeOfImpact` with `SeparationFunction`/`Sweep`, `Collide` manifold family with chain segments (`ChainSegment`/`ChainNormalType`/`ChainParams`) and `ClipPolygons`/`ClipSegments`, `Mover` plane solver with `Plane`/`PlaneResult`/`CollisionPlane`/`PlaneSolverResult`; cold/warm `To`-form pairs throughout with lazily created scratch bundles (`DistanceOutput`/`CastOutput`/`TOIOutput`/`Manifold` `.Scratch()`); six golden fixtures (hull 15, mass 9, distance 6, raycast 20, shapecast 12, manifold 60 = 122 records) plus TOI unit tests; cold-form-equals-warm-form and reuse-across-queries tests for every family; 100 tests pass (`objo test --all-projects`); dist regenerated, `objo check` clean; Release benchmarks `benchmarks/results/local-2026-08-31T08-03-00.json`: all five stage4 scenarios 0 allocations (distance 0.25 ms, manifold 0.26 ms, cast 0.17 ms, TOI 0.47 ms, plane solver 0.13 ms medians); `docs/ARCHITECTURE.md` collision-geometry section and envelope table; `docs/PORTING.md` mappings, test mapping, and deliberate differences 12-14 |
| 5. Dynamic tree and broad phase | Complete | Stage 5 sources in `Shared/Sources`: `DynamicTree` (indexed node pool with free list, scratch-based greedy sibling search, rotations, partial/full rebuild with median partitioning, category bits, queries/ray casts/shape casts with cold/warm forms and callback classes, Validate/ValidateNoEnlarged), `TreeScratch`, `TreeStats`, `TreeNode`, `TreeQueryCallback`/`TreeRayCastCallback`/`TreeShapeCastCallback`, `BodyType`, and `BroadPhase` (three-tree proxy keys, move buffer + move set, pair set de-duplication, per-move LIFO pair pool over parallel IntegerLists, `BroadPhasePairSink`, `BroadPhaseQueryContext`, TestOverlap/GetShapeIndex/RebuildTrees/Validate); adversarial tests in `DynamicTreeTests` (18) and `BroadPhaseTests` (11): brute-force query/ray/cast oracles, sorted insertion, clusters, category changes, huge/invalid bounds, destroy accounting, enlarge + rebuild, termination/skip, determinism, sink filtering, error throws, and zero-alloc warm traversals; 126 tests pass; dist regenerated, `objo check` clean; Release benchmarks `benchmarks/results/stage5-tree-broadphase-2026-08-31T12-21-13.json`: all four stage5 scenarios 0 allocations (churn 11.1 ms, query 2.0 ms, rebuild 12.2 ms, broad-phase pairs 4.2 ms medians); `AABB` gained allocation-free `UnionPerimeter` so tree mutations allocate nothing; `docs/ARCHITECTURE.md` Stage 5 section and envelope table; `docs/PORTING.md` broad-phase mappings, test mapping, exclusions update, deliberate differences 15-16 |
| 6. World, bodies, shapes, chains, queries | Complete | Stage 6 sources in `Shared/Sources` (23, all clean-named): `World` façade (constructors plus `WithGravity` factory, InitCommon, body/shape/chain creation and destruction with cascades, mass maintenance `UpdateBodyMassData`/`ApplyMassFromShapes`, type/transform/velocity/damping/gravity-scale/fixed-rotation/bullet/enabled/awake/sleep-threshold setters, proxy sync, local/world point-vector conversion, overlap bounds/shape and ray closest/all-hits and shape-cast queries with caller-owned `Into` forms and category filtering, `Clear`/`Validate`, locked-world guards, `StepWorld` refusing until Stage 7), `WorldSettings`, `Body`, `BodyDefinition`, `BodySims`, `BodyStates`, `Chain`, `ChainDefinition`, `Shape`, `ShapeDefinition`, `ShapeExtent`, `ShapeType`, `ShapeHit`, `ShapeHitList`, `Filter`, `QueryFilter`, `SurfaceMaterial`, `SolverSet`, and the four world callback/context classes; per-slot generation lists (`BodyGenerations`/`ShapeGenerations`/`ChainGenerations`) so reused slots never repeat a generation; solver sets 0/2/1 allocated from the solver-set pool at world creation so sleeping sets start at 3; conditional mass updates on all four shape-create paths; open chains attach segment i to material i+1 (upstream leading-point rule); zero-alloc create path with World-owned scratch and `To`-form helpers (warm shape create = 17 façade-owned allocations, `Clear` + eight-body rebuild = 136, both test-pinned); tests in `WorldTests` (9), `BodyTests` (25), `QueryTests` (11): construct/validation errors, locked-world throws, slot reuse with generations and stale-façade throws, destroy cascades for shapes and chains, chain loops/open chains/ghost points/leading-point materials, transform round-trips with rotation-aware expectations, mass and inertia against upstream formulas, AABB updates, fixed rotation, sleeping placement in per-island sets, brute-force overlap and ray/cast oracles, callback protocols, hit-list reuse, and the warm-rebuild allocation budget; 171 tests pass (`objo test --all-projects`); dist regenerated, `objo check` clean; `docs/ARCHITECTURE.md` World-facade section; `docs/PORTING.md` body/shape stage columns fixed to 6, world mappings updated, deliberate differences 17-20; `AGENTS.md` language notes for same-arity constructor dispatch, identifier collisions, array literals, `ElseIf`, and full-field test sidecars |
| 7. Contacts, islands, graph, Soft Step | Complete | Stage 7 sources in `Shared/Sources`: `Contact`/`ContactMethods` (pair-sink creation, primary ordering, filtering with default category/mask/group plus `CustomContactFilter`, destroy cascades, `UpdateContact` with persisted-point matching, anchor shifting, speculative collapse, `CustomPreSolve` and `CustomMaterialMixer` hooks with zero-overhead disabled paths), `ContactFlags`, `ContactSims` (dense per-set store with pooled manifolds/caches, `MoveRowTo`), `Island`/`IslandMethods` (link/unlink, DFS split via `DfsVisitBody`, merge, `TrySleepIsland`), `GraphColor`/`ConstraintGraph`/`GraphMethods` (11 bitset colours plus overflow colour 11, `ChooseContactColor`), `Softness` (+`Make` = `b2MakeSoft`), `ContactConstraint` (flat scalar record), `ContactSolver` (prepare, warm start, Soft Step bias/speculative solve, friction with `TangentSpeed`, rolling resistance, restitution, impulse store), `StepContext`, `SolverMethods` (colour loops, integrate velocities/positions), and `World.StepWorld` pipeline (`CollideStep` with ascending-id state BitSet, `SolveStep` merge/split + overflow-first substep loop, `FinalizeBodies` sleep/split accounting + AABB refresh, `RefitBroadPhase`, `SleepIslands`); sleeping migration fixed to skip by `ColorIndex` (scene tests caught a wrong-store move) and `AddContactToGraph` now retargets waking contacts to the awake set; tests in `ContactTests` (31) and `WorldTests` (11): island lifecycle, graph round-trips and overflow, kernel stopping/restitution, step rests/sleep/wake, pre-solve opt-in and disable, mixer override, `SceneTests` (10) required scenes with pinned fold checksums and deterministic repeats; `stage7-pyramid-40` benchmark (820 bodies, median 1.38 s per 4-step iteration, checksum 161419885332893877) in `benchmarks/results/stage7-pyramid-baseline-2026-08-31T23-00-23.json`; contact sim migration no longer aliases pooled manifolds across solver sets (`MoveRowTo` vacates the source slot; scene tests caught boxes tunneling and hovering after an island slept); zero-alloc warm step (`Softness.MakeTo`, scratch transforms, reused colour constraint arrays); 215 tests pass in `Physics2D.Tests` plus 19 benchmark harness tests (234 total, `objo test --all-projects`), dist regenerated, `objo check` clean; `docs/ARCHITECTURE.md` solver section; `docs/PORTING.md` note 20 replaced plus deliberate differences 21-23; `AGENTS.md` language notes for doc-comment attachment, `+` concat, bare self-calls, Count property/method, BitSet ctor, Next-variable matching, and project sidecar parents |
| 8. Continuous collision, sensors, events | Complete | Stage 8 sources in `Shared/Sources`: `Sensor` (double-buffered shape-ref overlap lists), `SensorQueryCallback`, `ContinuousContext`/`ContinuousQueryCallback`, and `WorldEvents` (reusable batch view with documented validity window; façade accessors resolve generation-checked Shape/Body references and return Nothing when stale); World additions: sensor registration/destroy (`DestroySensor` with end events and swap-remove fixup), `OverlapSensors` after the solve (buffer swap, three-tree filtered queries with exact shape-distance overlap tests, insertion sort by id then generation, diff-based begin/end publication), `SolveContinuous` (swept-TOI queries over the three trees with chain-junction clipping and small-circle fallback, hit branch advancing the body to the exact impact transform and refreshing AABBs, no-hit branch advancing rotation0/center0), fast/bullet branch in `FinalizeBodies` (`Shape.ComputeExtentTo` ports `b2ComputeShapeExtent`; fast non-bullets solve immediately, bullets queue in `StepContext.BulletBodies`), `CollectHitEvents`, contact begin events with pooled-manifold snapshots (`Manifold.AssignFrom`/`ManifoldPoint.AssignFrom`), end events on state change and touching-contact destruction, moved-body events with fell-asleep patching in `TrySleepIsland`, fast-bullet refit branch plus post-refit bullet stage, sensor-pair rejection in the broad-phase sink, and end-event buffer double-swap at step end (including the paused zero-dt path); fixed a latent port bug where `Distance.WriteOverlapOutput` left distance/normal stale on reused scratch outputs (upstream zero-initialises the struct) which surfaced as a `SeparationFunction` throw on continuous sweeps; tests in `Stage8Tests` (14): thin-wall tunnelling with continuous on and off, bullet-vs-wall, fast spinner, initial overlap, sensor begin/end/persist/destruction/disable/suppression, contact begin with manifold snapshot, end on destroy, hit event with approach speed, move events with sleep flag, stale-reference safety; pile scene golden recomputed for Stage 8 TOI advances (4006121332606765641); 230 tests pass in `Physics2D.Tests` plus 19 benchmark harness tests (249 total, `objo test --all-projects`), dist regenerated, `objo check` clean; `docs/ARCHITECTURE.md` Stage 8 section; `docs/PORTING.md` world/solver rows and deliberate differences 24-26 |
| 9. Joints | Complete | The common joint infrastructure and distance family exist, including lifecycle and solver-set migration, island/graph integration, distance solving, 249 passing Physics2D tests plus 19 benchmark tests, and `stage9-joint-chain-60`. The 2026-09-01 audit continuation gate is complete: cross-world joint creation throws (repair 1 with ordered coincident-anchor regression for repair 2), idiomatic constructors restored on the Objo #1315 engine fix (repair 3), `JointSims` rewritten to the decision-0004 scalar layout with one reused `DistanceJointScratch` (repair 4), public `UserData` is `Object` across body/shape/chain/joint definitions and façades (repair 5), duplicate initialization and indentation removed (repair 6), the visibility audit moved 43 internal members to Private and deleted 4 dead members (repair 7), `World.StepWorld(timeStep[, subStepCount])` defaults to four substeps and rejects non-positive counts (repair 8), and repair 9 landed the `joint_distance` golden fixture generated from the pinned upstream source, the warmed `TestWarmJointSleepWakeAllocatesNothing` gate (0 allocations across 90 steps with wakes), and Release benchmarks: joint chain median 243.7 ms → 71.3 ms and recorded allocations 44,868 → 17,357 (remaining count is Release-VM boxing in the benchmark's own checksum fold plus its 60 per-iteration `GetBodyTransform` snapshots; warmed steady-state stepping is allocation-free in the test host and the checksum `4283633503840920984` is unchanged). Stage 8 CCD and SceneTests checksums were re-pinned for the four-substep default and verified against upstream C probes.

  The mouse joint family (2026-09-01) completes common-slice items 1-8: `MouseJointDefinition` with upstream defaults (4 Hz, damping 1, 1 N max force) and boundary validation, the `MouseJoint` façade (`SetTarget` vector plus allocation-free scalar overload, spring hertz/damping/max-force setters, documented no-wake retarget semantics, `GetAnchorB` addition), mouse-family `JointSims` columns reusing the base `IndexB`/`AnchorB`/`DeltaCenter` columns and a three-scalar symmetric linear mass, `World.CreateMouseJoint` with both anchors at the target point, and the prepare/warm-start/solve kernels ported from `b2PrepareMouseJoint`/`b2WarmStartMouseJoint`/`b2SolveMouseJoint` including the angular-damping block and the impulse-magnitude clamp with `b2Normalize`'s degenerate branch; a non-awake body B stores `NULL_INDEX` and the kernels no-op instead of upstream's unguarded state indexing (PORTING difference 31). Tests: eight new Stage 9 tests (drag-to-target with settle-retarget-re-sleep, saturated 1 N clamp, runtime setters with validation and no-wake sleeping semantics, lifecycle with slot reuse, cross-world rejection, static-body-B no-op, bit-for-bit determinism, `TestWarmMouseJointAllocatesNothing` gate with a mid-window scalar retarget), the `joint_mouse` golden fixture from the pinned upstream source (drag with mid-scene retarget+wake, weak clamp, stiff retarget; MANIFEST SHA-256 `87312b76…`) passing within scene/joint-force tolerances, and the `stage9-mouse-drag-30` Release benchmark (30 per-frame retargeted drags, median 23.5 ms, 720 allocations from the benchmark's own transform fold, checksum `4590636256588353705`) recorded in `benchmarks/results/local-2026-09-01T16-33-51.json`; joint-chain allocations and checksum unchanged. 258 Physics2D tests plus 19 benchmark tests pass (`objo test --all-projects`), dist regenerated (`affa757b…`), clean-room `check_distribution.sh` passes, and the Smoke app gained the compact mouse-drag example. Continue with the motor joint in the Stage 9 order. |

  The motor joint family (2026-09-01) completes common-slice items 1-8: `MotorJointDefinition` with upstream defaults (zero offsets, 1 N max force, 1 N-m max torque, 0.3 correction factor) and boundary validation, the `MotorJoint` façade (linear/angular offset, force, torque, and correction-factor setters with upstream clamping — negatives clamp to zero and the correction factor clamps into [0,1] — plus non-finite throws and the allocation-free scalar `SetLinearOffset(x, y)` overload, documented no-wake retarget semantics), motor-family `JointSims` columns (offsets, clamps, impulses, the three-scalar symmetric linear mass, angular mass, and `MotorDeltaAngle`; no softness columns because upstream's solve body never reads the base constraint softness) reusing the base two-body columns, `World.CreateMotorJoint` with zero local anchors and correction-factor clamping, and the prepare/warm-start/solve kernels ported from `b2PrepareMotorJoint`/`b2WarmStartMotorJoint`/`b2SolveMotorJoint` including the unwound correction-factor angular bias, the accumulated-clamp linear and angular limits with `b2Normalize`'s degenerate zero branch, and scalar-component relative angles (no intermediate `Rot` allocations in the kernels); non-awake bodies store `NULL_INDEX` state rows with identity dummies (PORTING difference 33). Tests: ten new Stage 9 tests (pose drive with settle-asleep equilibrium, zero-torque linear-only drive plus saturated 1 N clamp fall at 6 m/s², dynamic-carrier relative-pose convergence, runtime setters with clamping and no-wake sleeping semantics, lifecycle with slot reuse, cross-world rejection, bit-for-bit determinism, `TestWarmMotorJointAllocatesNothing` gate with a mid-window scalar retarget and wake), the `joint_motor` golden fixture from the pinned upstream source (default-factor pose drive, retarget via setters with explicit wake, 3 N clamp transit; MANIFEST SHA-256 `ac82f90a…`) passing within scene/joint-force tolerances, and the `stage9-motor-drive-30` Release benchmark (30 per-frame retargeted drives, median 17.7 ms, 827 allocations = the benchmark's 720 fold allocations plus sporadic island-merge graph growth; a 30-box probe shows zero allocations after the graph settles and an isolated spacing probe attributes the remainder to the shared island-merge path, not the motor kernels; checksum `-3109180529058850647`) recorded in `benchmarks/results/local-2026-09-01T20-25-08.json`; joint-chain, mouse, and pyramid checksums unchanged. 267 Physics2D tests plus 19 benchmark tests pass (`objo test --all-projects`), dist regenerated (`22ac8c83…`), clean-room `check_distribution.sh` passes, and the Smoke app gained the compact motor-drive example. Continue with the revolute joint in the Stage 9 order. |

  The revolute joint family (2026-09-01) completes common-slice items 1-8: `RevoluteJointDefinition` with upstream defaults (coincident anchors, zero angles, no spring, limits, or motor) and boundary validation (limits throw outside [-0.99 pi, 0.99 pi] or inverted, non-finite tuning throws, reference and target angles clamp into [-pi, pi] like upstream), the `RevoluteJoint` façade (spring/limit/motor toggles that clear their accumulated impulses on change exactly like upstream, plain runtime tuning setters per the distance-family convention, `GetAngle` as the unwound live relative angle minus the reference, and `GetMotorTorque`), revolute-family `JointSims` columns (impulses, spring and motor tuning, reference and limits, axial mass, `RevoluteDeltaAngle`, and prepared spring softness) reusing the base two-body columns and the base `EnableSpring`/`EnableLimit`/`EnableMotor` flag columns, `World.CreateRevoluteJoint` with definition anchors and clamping, and the prepare/warm-start/solve kernels ported from `b2PrepareRevoluteJoint`/`b2WarmStartRevoluteJoint`/`b2SolveRevoluteJoint` including the axial bundle of spring, motor, and limit impulses, the speculative and soft-step limit bias branches, the sign-flipped upper-limit block, the fixed-rotation guard, and the inline symmetric 2x2 point-to-point solve with the guarded inverse determinant (the combined matrix changes every iteration with the delta rotations, so no prepare-time mass is stored); non-awake bodies store `NULL_INDEX` state rows with identity dummies (PORTING difference 34). Tests: eleven new Stage 9 tests (hinge holds the anchor and weight through a kicked swing, spring drive with gravity sag and no-wake retarget, torque-clamped motor brake plus constant-speed tracking, lower-limit hold with widened-limits release, runtime setters and toggle impulse clearing, golden fixture recreation, lifecycle with slot reuse, invalid-definition rejection including the limit range, two-dynamic-body convergence, bit-for-bit determinism, and the `TestWarmRevoluteJointAllocatesNothing` gate with a mid-window retarget), the `joint_revolute` golden fixture from the pinned upstream source (hinge, spring, brake, and lower-limit scenes, all settling asleep; MANIFEST SHA-256 `e74b23d9…`) passing within scene/joint-force tolerances, and the `stage9-revolute-swing-30` Release benchmark (30 motor-driven hinges, median 7.5 ms, 811 allocations = the benchmark's 720 fold allocations plus the same sporadic graph-growth class documented for the motor workload, checksum `-7562213319518099237`) recorded in `benchmarks/results/local-2026-09-01T21-14-34.json`; joint-chain, mouse, motor, and pyramid checksums unchanged. dist regenerated (`4fdb08ef…`), clean-room `check_distribution.sh` passes, and the Smoke app gained the compact revolute-hinge example. Continue with the prismatic joint in the Stage 9 order. |

  The prismatic joint family (2026-09-01) completes common-slice items 1-8: `PrismaticJointDefinition` with upstream defaults (coincident anchors, world X axis, zero reference and target, all features disabled) and boundary validation (inverted translation limits, non-finite axis or tuning, and negative hertz or motor force throw; the axis normalizes at creation through `b2Normalize` including the degenerate zero branch), the `PrismaticJoint` façade (spring/limit/motor toggles that clear their accumulated impulses exactly like upstream, plain runtime tuning setters, `GetTranslation` measuring between the live anchor points on the live axis, `GetSpeed` porting the anchor-offset velocity projection, `GetMotorForce`, and `GetLocalAxisA`/`GetReferenceAngle` overrides), prismatic-family `JointSims` columns (local and prepared world axis, impulses, spring and motor tuning, reference and translation limits, axial mass, unwound `PrismaticDeltaAngle`, and prepared spring softness) reusing the base two-body and flag columns, `World.CreatePrismaticJoint` with axis normalization, and the prepare/warm-start/solve kernels ported from `b2PreparePrismaticJoint`/`b2WarmStartPrismaticJoint`/`b2SolvePrismaticJoint` including the axial spring-motor-limit bundle applied through the anchor-line torque arms, the speculative and soft-step limit bias branches, the sign-flipped upper-limit block, the fixed-rotation `k22 = 1` substitution, and the perpendicular-plus-angular block solve; non-awake bodies store `NULL_INDEX` state rows with identity dummies (PORTING difference 35). A façade bug caught by the tilted-rail test before commit: `GetTranslation` originally measured from the body origin instead of the anchor point, reporting a translation off by the anchor offset; it now ports the upstream anchor-point projection. Tests: twelve new Stage 9 tests (free slide with translation and speed queries and perpendicular weight carrying, upper-limit stop-and-hold, tilted rail held against gravity by the lower limit, spring drive with no-wake retarget, force-clamped motor brake plus constant-speed tracking, runtime setters and toggle impulse clearing, golden fixture recreation, lifecycle with slot reuse, invalid-definition rejection, two-dynamic-body convergence, bit-for-bit determinism, and the `TestWarmPrismaticJointAllocatesNothing` gate with a mid-window retarget), the `joint_prismatic` golden fixture from the pinned upstream source (limits, tilted limits, spring, and brake scenes, all settling asleep; MANIFEST SHA-256 `9a7687a0…`) passing within scene/joint-force tolerances, and the `stage9-prismatic-drive-30` Release benchmark (30 motor-driven sliders with per-frame sine speed retargets, median 5.8 ms, 720 allocations = the benchmark's own transform fold exactly, checksum `6270998793905162839`) recorded in `benchmarks/results/local-2026-09-01T21-45-59.json`; joint-chain, mouse, motor, revolute, and pyramid checksums unchanged. dist regenerated (`43be2907…`), clean-room `check_distribution.sh` passes, and the Smoke app gained the compact prismatic-rail example. Continue with the weld joint in the Stage 9 order. |

  The weld joint family (2026-09-01) completes common-slice items 1-8: `WeldJointDefinition` with upstream defaults (coincident anchors, zero reference, zero hertz for maximum stiffness, zero damping) and boundary validation (negative or non-finite hertz and damping throw where upstream asserts), the `WeldJoint` façade (linear and angular hertz/damping setters, `GetReferenceAngle` override, no-wake retuning), weld-family `JointSims` columns (reference, tuning, impulses, axial mass, unwound `WeldDeltaAngle`, and linear plus angular prepared softness with the zero-hertz fallback copying the base constraint softness exactly like upstream) reusing the base two-body columns, `World.CreateWeldJoint`, and the prepare/warm-start/solve kernels ported from `b2PrepareWeldJoint`/`b2WarmStartWeldJoint`/`b2SolveWeldJoint` including the angular block whose soft bias applies whenever the solver asks for bias or the weld is softened (the upstream `useBias || hertz > 0` condition) and the inline symmetric 2x2 linear block; non-awake bodies store `NULL_INDEX` state rows with identity dummies (PORTING difference 36). Tests: seven new Stage 9 tests (rigid weld drops, topples, and rests with the relative pose preserved plus soft weld flexing and holding a bent pose, runtime setters with validation and no-wake retuning, golden fixture recreation, lifecycle with slot reuse, invalid-definition rejection, bit-for-bit determinism, and the `TestWarmWeldJointAllocatesNothing` gate with a mid-window retune), the `joint_weld` golden fixture from the pinned upstream source (rigid and soft dropped-pair scenes, both settling asleep; MANIFEST SHA-256 `eee16c6a…`) passing within scene tolerances — the rigid case's settled force uses a documented 0.06 N band because it is a frozen last-solved impulse whose equilibrium depends on the millimetre-scale rest pose — and the `stage9-weld-flex-30` Release benchmark (30 soft-welded pairs rocking under oscillating forces through a reusable To-form force vector, median 33.2 ms, 1158 allocations = the benchmark's 720 fold allocations plus island and wake churn from the per-frame force wakes, checksum `7329460962250788856`) recorded in `benchmarks/results/local-2026-09-01T22-07-53.json`; joint-chain, mouse, motor, revolute, prismatic, and pyramid checksums unchanged. dist regenerated (`e572d184…`), clean-room `check_distribution.sh` passes, and the Smoke app gained the compact soft-weld example. Continue with the wheel joint in the Stage 9 order. |

  The wheel joint family (2026-09-01) completes common-slice items 1-8 and closes the seven-family Stage 9 order: `WheelJointDefinition` with upstream defaults (coincident anchors, world Y axis, spring/limits/motor disabled) and boundary validation (inverted translation limits, non-finite axis or tuning, and negative hertz, damping, or motor torque throw; the axis normalizes at creation through `b2Normalize` including the degenerate zero branch), the `WheelJoint` façade (spring/limit/motor toggles that clear their accumulated impulses exactly like upstream, plain runtime tuning setters, the documented `GetTranslation`/`GetSpeed` additions porting the prismatic anchor-point projections — upstream exposes no wheel translation getter — `GetMotorTorque`, and the `GetLocalAxisA` override), wheel-family `JointSims` columns (local and prepared world axis, perp/spring/motor/limit impulses, tuning, limits, three effective masses, and prepared spring softness) reusing the base two-body and flag columns, `World.CreateWheelJoint` with axis normalization, and the prepare/warm-start/solve kernels ported from `b2PrepareWheelJoint`/`b2WarmStartWheelJoint`/`b2SolveWheelJoint` including the torque-clamped motor on the free spin, the suspension spring driving the slide toward zero translation (the upstream struct has no target field), the speculative and soft-step limit bias branches with the sign-flipped upper-limit block, and the perpendicular point-to-line block; non-awake bodies store `NULL_INDEX` state rows with identity dummies (PORTING difference 37). Tests: nine new Stage 9 tests (rigid suspension lands with the wheel resting and the slide free, soft suspension sags holding the chassis weight, torque-clamped brake stops a kicked wheel and the car settles on its tail, constant-speed drive tracking with reversal, runtime setters and toggle impulse clearing, golden fixture recreation, lifecycle with slot reuse, invalid-definition rejection, bit-for-bit determinism, and the `TestWarmWheelJointAllocatesNothing` gate with a mid-window retune), the `joint_wheel` golden fixture from the pinned upstream source (stiff, soft, and brake suspension scenes, all settling asleep; MANIFEST SHA-256 `9ac2ef5a…`) passing within scene tolerances — the brake case's settled force uses the documented 0.06 N frozen-impulse band — and the `stage9-wheel-drive-30` Release benchmark (30 motorized cars with suspension springs, median 25.7 ms, 1472 allocations = the benchmark's 1440 fold allocations plus 32 sporadic, checksum `896695741854586753`) recorded in `benchmarks/results/local-2026-09-01T22-30-51.json`; joint-chain, mouse, motor, revolute, prismatic, weld, and pyramid checksums unchanged. dist regenerated (`1d115a26…`), clean-room `check_distribution.sh` passes, and the Smoke app gained the compact driving-car example. All seven joint families of the Stage 9 order are now complete. |

  The filter joint family (2026-09-01) completes the last `b2CreateFilterJoint` inventory row: `FilterJointDefinition` (bodies and user data only) and the `FilterJoint` façade with no tuning or solver behaviour, `World.CreateFilterJoint` with deliberately no contact destruction at creation, and no solver dispatch (the filter joint joins the constraint graph like every family and every kernel case returns zero, matching upstream's no-op prepare/solve cases), so its only effect is the connected-joint collision rule while it lives (PORTING difference 38). Upstream behaviour verified by a C probe: a contact that already exists when the joint is created persists until it ends naturally, and only newly discovered broad-phase pairs are suppressed. Tests: three new Stage 9 tests (two boxes pushed through each other's column while airborne form no contact and report zero force and torque, destroying the joint restores collision on the same pushed pair, and cross-world/missing-body rejection). No golden fixture and no focused benchmark: the filter joint has no solver behaviour and no dynamical state to record; its suppression rule is covered by the connected-joint walk tests. dist regenerated (`034ee3b4…`), clean-room `check_distribution.sh` passes. |

  Stage 9 completion gates: every joint family completed the eight-item common slice (definition, façade, dense state, lifecycle, solver, runtime changes, tests with golden fixture and zero-alloc gate, docs and example), plus the filter joint inventory row. Limits, motors, springs, damping, softness, and runtime toggles work per family with the documented upstream-matching clamping, validation, and no-wake semantics; lifecycle tests destroy joints and destroy attached joints with bodies, leaving no stale edges; a dedicated mixed joint/contact stress scene (welded pair, hinged pendulum, motor-braked slider, and a free box sharing ground contacts) settles fully asleep and repeats bit for bit; and the joint-chain benchmark checksum stayed `4283633503840920984` across all nine parts with the mouse, motor, revolute, prismatic, weld, wheel, and pyramid checksums likewise unchanged, so no solver regression is unexplained. Eight golden joint fixtures regenerate byte-identically from the pinned upstream source, the Release benchmarks cover all seven solver families plus the joint chain, and the distribution clean-room check passes. 310 Physics2D tests plus 19 benchmark tests pass (`objo test --all-projects`). |
| 10. API completion and debug rendering | In progress | Inventory reconciliation (2026-09-01): scripted every `Class.Member` mapping in the PORTING inventory against the generated dist and resolved all mismatches — renamed mappings to the implemented idiomatic surface (`World.ContinuousEnabled`/`SpeculativeEnabled`/`WarmStartingEnabled` settable properties, `World.EnableSleeping` read, `World.Events` batch view, `World.ContactHertz`/`ContactDampingRatio`/`MaxContactPushSpeed` as the contact-tuning surface, `Joint.GetJointType`, `Joint.GetConstraintHertz`/`GetConstraintDampingRatio`, `DynamicTree` constructor and `ProxyCount` property), implemented the genuinely missing members (`World.RebuildStaticTree` + `BroadPhase.RebuildStaticTree` full static-tree quality rebuild, and the body-level event enablers `Body.EnableContactEvents`/`EnableHitEvents` writing through to every attached shape with the documented `AreContactEventsEnabled`/`AreHitEventsEnabled` getters — upstream has only the setters), verified the sleeping-body semantics of moving static geometry against a pinned-source C probe (a sleeping box stays asleep; upstream behaves identically), and added Stage10Tests covering the rebuild and event enablers. Movers and planes (2026-09-01): `World.CastMover` (capsule sweep over all three broad-phase trees with can-encroach semantics, overlap ignoring, and filter rejection, ported from `b2World_CastMover` through the new engine-internal `WorldMoverCastContext`) and `World.CollideMover` (per-shape collision planes forwarded to a `WorldMoverPlaneCallback` through `WorldMoverCollideContext`, ported from `b2World_CollideMover`) close the world-query mover rows; the per-shape collide ported the four upstream `b2CollideMoverAnd*` geometry functions into `Mover.CollideCircle/Capsule/Polygon/Segment` with `Shape.CollideMover` dispatching and rotating the plane normal into world space, and `PhysicsMaths.IsNormalized` ports the upstream unit-vector tolerance. Numbers verified against a pinned-source C probe: the wall-contact sweep fraction 0.326250, the floor-contact sweep fraction 0.701250, and the sunk-mover plane offset 0.05 all match. Three new Stage 10 tests cover the cast (wall, open side, filter rejection, encroachment, floor), the plane registration, and the early-stop callback. Explosions (2026-09-01): `World.Explode` ported from `b2World_Explode` through the engine-internal `ExplosionContext` tree query — per dynamic shape within the blast range it wakes the body and applies the facing-perimeter impulse at the closest point (deep overlaps use the shape centroid), with the linear falloff scale and the local-space projected-perimeter helpers ported from `b2GetShapeProjectedPerimeter`/`b2GetShapeCentroid`; `ExplosionDefinition` carries the upstream defaults. Impulse velocities verified against a pinned-source C probe (inner box (4.327869, 0.393443), falloff box (2.567863, 0.073368), out-of-range zero). The explosion test also caught a real port bug: `World.SetBodyAwake(False)` performed a raw solver-set transfer that left island bookkeeping disagreeing with the body's set (validation threw "Body solver set disagrees with island"); it now ports the upstream island-aware path — split a pending-split island, then `TrySleepIsland` — and the full suite passes without sleep/wake regressions. Two new Stage 10 tests cover the impulse pattern and the implosion direction. Remaining Stage 10 deliverables: `World.DrawDebug`, `World.Counters`/`Profile`, the API audit, compile-only examples, and `docs/API.md`. 316 Physics2D tests plus 19 benchmark tests pass; dist regenerated (`b7bb9fb7…`), clean-room check passes. |
| 11. Interactive demo and teaching examples | Not started | |
| 12. Optimisation and allocation audit | Not started | |
| 13. Documentation and release candidate | Not started | |

### Stage 9 continuation gate (audit 2026-09-01)

The distance-joint slice is working code, but it is not yet an accepted common
slice. Complete every item below before starting mouse, motor, revolute,
prismatic, weld, or wheel joints. These are required repairs to the current
slice, not debt that may be deferred to Stage 10 or the final audit:

1. Reject bodies from another `World` in every joint creation path. Validation
   must prove both liveness and ownership before interpreting an internal id.
2. Clear the reusable distance-joint axis before the coincident-anchor branch,
   and add an ordered regression that would expose axis state leaking from a
   previously solved joint.
3. Restore idiomatic constructors now that Objo #1315 is fixed. `Joint` owns
   its lifecycle sentinels, `JointSim` owns its base state and
   `ConstraintSoftness`, and `DistanceJointSim` calls `Super.Constructor()` and
   owns `DistanceSoftness`. `NewJointSimOfType` may select a concrete type but
   must not initialize class-owned state.
4. Replace the object-per-row `JointSims` hot representation with the
   decision-0004 scalar layout. A different representation is allowed only
   after a representative bake-off, a zero-allocation gate, and a new accepted
   decision demonstrate that it is preferable in Objo.
5. Make public `UserData` consistently `Object` on body, shape, chain, and
   joint definitions and façades, copy definition user data during creation,
   and test non-integer object identity and lifecycle behaviour.
6. Remove duplicate joint-store initialization in `World.InitCommon` and all
   leading indentation from canonical Shared source files.
7. Audit the visibility of every Stage 9 type and member now. Joint façades and
   definitions form the public API; ids, generations, solver-set/local indices,
   graph colours and edges, dense stores, simulation records, and scratch state
   are implementation details and must use the narrowest supported visibility.
   Do not let another joint family expand the accidental public surface.
8. Conform the locked stepping API: `World.Step(timeStep)` uses four substeps,
   the explicit overload accepts a positive substep count, and non-positive
   counts are rejected. Keep `World.WithGravity`; same-arity constructor
   dispatch is a separate, still-open VM limitation.
9. Add a pinned Box2D 3.1.1-derived distance-joint golden, focused lifecycle
   and migration invariants, a zero-allocation warmed joint-step/migration
   gate, a compact public example, joint API documentation, port mappings, and
   the minimum-Objo requirement for issue #1315. Regenerate the distribution
   and run all Stage 9 correctness, clean-room, and Release benchmark gates.

After the gate passes, continue the remaining joint families in the order and
vertical-slice workflow specified in Stage 9. Update this ledger only from
fresh recorded evidence.

## 22. Final Autonomous Audit

Before declaring the port complete, perform one final audit independent of the
stage checklists:

1. Diff the complete public API against the Stage 0 inventory.
2. Search for `b2`, `Box2D`, `TODO`, `FIXME`, `stub`, `temporary`, silent catch
   blocks, empty implementations, hard-coded test results, and disabled tests.
3. Search hot paths for allocating vector/matrix operators, array removal,
   closures, callbacks, dictionaries, string interpolation, and accidental
   object construction.
4. Validate every pool, store, tree, graph, island, contact, and joint after the
   longest seeded stress run.
5. Run the entire test suite twice in different test order if supported.
6. Run deterministic scenarios in fresh processes and compare checksums.
7. Run the complete Release benchmark suite and compare raw results.
8. Regenerate golden fixtures with the pinned Box2D checkout and confirm no
   unexplained diff.
9. Regenerate distribution artifacts twice and compare hashes.
10. Build a new command-line and desktop consumer from the documentation alone.
11. Run automated demo scene cycling and then perform a manual interaction pass.
12. Review every public comment and documentation example as teaching material,
    not merely an API inventory.
13. Verify that normal consumers require only `Import Physics2D` plus the
    distributed source and standard Objo installation.

Only after this audit and every prior stage gate passes is the native Objo
Physics2D port complete.
