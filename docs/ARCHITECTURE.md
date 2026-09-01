# Physics2D Architecture

This document explains how Physics2D is structured inside the single
`Physics2D` module: the data layout, the identity rules, and the reasoning
behind non-obvious representations. Public API documentation lives in
`docs/API.md`; upstream provenance and symbol mapping live in
`docs/PORTING.md`.

```text
Physics2D public API (later stages)
├── World / Body / Shape / Chain / Joint façades
├── Definitions, geometry values, query results, and events
└── DebugRenderer
        │
        ▼
Protected indexed core
├── Foundation: constants, deterministic maths, identity, containers   ← this document
├── Collision: hulls, distance, manifolds, casts, time of impact
├── BroadPhase: bounds, dynamic tree, move/pair buffers
├── Dynamics: body/shape/contact/joint stores and solver sets
├── Solver: islands, graph colours, contacts, joints, sleeping, CCD
└── Diagnostics: validation, statistics, debug drawing
```

## Design Rules

1. **Parallel scalar arrays for hot state.** The Stage 2 bake-off (decision
   0004) measured parallel x/y `Double` arrays fastest for body, solver,
   contact, and joint state (185.1 ms median, zero allocations per
   iteration) and node objects fastest for the dynamic tree (321.5 ms).
   Foundation containers follow the same evidence: growable sequences keep
   one backing array plus a logical count, and the pair table keeps two
   parallel scalar arrays. Nothing in the hot path stores one object per
   element. Stage 9 applies the rule to joints: `JointSims` stores one row
   of parallel scalar columns per joint (base columns shared by every
   family plus one column block per implemented family), so solver loops
   index scalars and the per-step scratch lives in one reused
   `DistanceJointScratch` record.
2. **Zero steady-state allocation.** After a capacity warm-up, every
   container operation on this page allocates nothing: growth happens only
   in `Reserve`-style calls, `Clear` resets a logical count or zeroes in
   place, and removal is swap-based. Each container exposes a `Validate`
   method used aggressively by tests and later by debug world validation.
3. **Determinism.** Container traversal and probe order are fixed by
   construction (linear probing with power-of-two capacity, ascending bit
   iteration, LIFO slot reuse). Trigonometry uses the ported upstream
   approximations rather than platform library calls, so results do not
   depend on the host maths library.
4. **Internal names stay teachable.** The foundation classes below are
   implementation infrastructure, not the stable public API; they may move
   behind narrower visibility when the public surface freezes. Their names
   still follow the public naming rules (no `b2` prefixes, full words).

## Foundation Containers

### IntegerList and DoubleList

Dense growable sequences — the Objo counterpart of the upstream typed
dynamic arrays in `src/array.h`. Objo's built-in `Array` cannot reserve
capacity without changing `Count` and has no swap-remove, so engine code
uses these lists for free lists, pair buffers, traversal stacks, and event
queues.

```text
IntegerList
┌───────────────────────────────────────────────┐
│ Items: [ 10 | 20 | 30 |  0 |  0 |  0 | ... ]  │  backing Array, length = capacity
└───────────────────────────────────────────────┘
  Count = 3 (valid prefix)   capacity = Items.Count
```

- `Reserve(capacity)` extends the backing array with zeros; it never
  changes `Count`. Callers warm capacity up front and then append defaults
  before indexed writes, because `Array.Reserve` does not extend `Count`.
- `Append` grows by the upstream factor (half again, minimum two) only when
  the backing array is full.
- `RemoveSwap(index)` moves the last element into the removed slot and
  returns where it came from (`NULL_INDEX` when the last element was
  removed). Order is not preserved; nothing ever shifts.
- `Clear` resets `Count` to zero and keeps capacity.
- Complexity: index read/write O(1); append amortised O(1); swap-remove
  O(1); clear O(1).

Hot engine code reads `Items[i]` directly within `[0, Count)` to avoid
call overhead; `At`/`SetAt` are the bounds-checked cold-path forms.

### IdPool

Port of `src/id_pool.c`. A free-list slot allocator: `AllocId` pops the
most recently freed slot, otherwise issues `nextIndex++`.

```text
IdPool
  nextIndex = 5
  freeIds (stack): [ 3 | 1 ]        ← IntegerList, LIFO
  live slots: { 0, 2, 4 }           ← nextIndex - freeIds.Count
```

- Recycling is LIFO, so the hottest slots stay cache-warm, matching
  upstream behaviour.
- `ValidateFreeId` / `ValidateUsedId` mirror the upstream validation
  entry points; `Validate` additionally rejects duplicate or out-of-range
  free entries.
- `Clear` returns the pool to empty (upstream destroys it; Physics2D adds
  clear-and-reuse to support `World.Clear`).
- Complexity: alloc/free O(1); validation O(free list).

### GenerationalPool

Generational identity over slots — the mechanism behind Box2D 3.1.1's
`b2BodyId`-style handles (index + generation + world), expressed as one
reusable container. It combines an `IdPool` with two parallel integer
stores addressed by slot: the generation counter and a live flag.

```text
slot:        0    1    2    3    4
generations: 3    0    1    2    0     ← 0 means never issued (or cleared)
live:        1    0    1    1    0
             └── free list in IdPool: { 1, 4 }
```

- A slot's generation starts at `INVALID_GENERATION` (0) and increments
  each time the slot becomes live; issued generations are 1, 2, 3, ...
- `FreeSlot` clears the live flag and preserves the generation. The next
  `AllocSlot` on that slot bumps it, so handles issued before a free stop
  validating after reuse.
- The live flag closes the reuse window: a freed handle does not validate
  even before its slot is handed out again.
- `Clear` zeroes every generation and returns the pool to empty, so
  handles from a previous world lifetime never validate.
- `IsValid(slot, generation)` is the runtime handle check: O(1), two array
  reads and comparisons after one bounds test.
- Complexity: alloc/free O(1) amortised; validation O(capacity).

### BitSet

Port of `src/bitset.c`/`bitset.h` with 64-bit words stored as `Integer`.
Used for solver-set membership, enlarged-proxy flags, and awake islands.

```text
BitSet (WordCount = 3 of capacity 8)
  Words: [ 0b...101 | 0 | 0b110 | 0 | 0 | 0 | 0 | 0 ]
             word 0    word 1  word 2   ← words above WordCount stay zero
```

- Bit *b* lives in word `b \ 64` at bit `b Mod 64`; bit 63 is handled with
  two's-complement arithmetic (`1.ShiftLeft(63)` is the sign bit and
  `BitAnd`/`BitOr`/`BitNot` work uniformly).
- `SetCountAndClear(bitCount)` is the step-loop reset; `Grow(wordCount)`
  expands the used range by whole words; both follow the upstream 1.5x
  capacity policy. Unlike upstream, the constructor leaves the reserved
  range immediately usable.
- `InPlaceUnion` ORs word-by-word and requires equal used word counts.
- `CollectSetBits(sink)` appends set indices in ascending order to an
  `IntegerList`, scanning each word with `PhysicsMaths.TrailingZeros` and
  clearing the lowest bit per step (`word BitAnd (word - 1)`).
- Complexity: set/clear/get O(1); union O(words); iteration O(words + set
  bits); `CountSetBits` O(words).

### PairKeySet

Port of `src/table.c` (`b2HashSet`), used by the broad phase to
deduplicate shape pair keys. Objo's built-in `HashSet` is a
general-purpose, allocation-per-operation type that is banned in hot
paths, so the engine carries this fixed-shape integer table instead.

```text
PairKeySet (capacity 16, Count 3)
  slot:     0     1     2     3    ...   15
  keys:     0   4294967298  0  8589934593 ... 0     ← 0 = empty (reserved)
  hashes:   0   337744490   0  ...          0     ← 0 = empty marker
```

- Keys are 64-bit values built by `PairKeySet.PairKey(a, b)`:
  `(min << 32) | max`, which is order-independent and nonzero for distinct
  nonzero ids. Key 0 is reserved and rejected by `Add`.
- The hash is the Murmur3 64-bit finaliser (upstream `b2KeyHash`), computed
  with logical shifts over signed 64-bit arithmetic and reduced to 32 bits.
  A computed hash of 0 maps deterministically to 1 (upstream asserts this
  cannot happen).
- Open addressing with linear probing; `index = hash BitAnd (capacity - 1)`
  with capacity a power of two, minimum 16, doubling on `2 * Count >=
  capacity`.
- Removal backshifts later probe-chain entries (the cyclic-interval test
  from upstream) so no tombstones are needed and every stored key still
  probes to its own slot.
- `Validate` recomputes every stored key's probe path and stored hash.
- Complexity: add/remove/contains O(1) expected; grow O(count); clear
  O(capacity); validation O(capacity).

Determinism note: unlike the built-in `HashSet`, iteration and probe order
here are fully determined by the keys and capacity, never by object
hashing or insertion history beyond what the algorithm prescribes.

## Collision Geometry (Stage 4)

The Stage 4 sources port `hull.c`, `geometry.c`, `shape.c` (mass/AABB
methods), `distance.c`, `manifold.c`, and `mover.c` on top of the
deterministic maths from Stage 3. Three rules shape the layout:

1. **Cold/warm form pairs.** Every entry point that produces a result has an
   allocating convenience form and a reuse form that fills a caller-owned
   output object: `Distance.ShapeDistance`/`ShapeDistanceTo`,
   `Casts.RayCast*`/`RayCast*To`, `Casts.ShapeCast*`/`ShapeCast*To`,
   `TimeOfImpact.ComputeTOI`/`ComputeTo`, `Collide.Collide*`/`Collide*To`,
   and `Mover.SolvePlanes`/`SolvePlanesTo`. The cold form constructs the
   output, runs the warm form, and returns it; all algorithmic logic lives
   in the warm form.
2. **Scratch bundles hide warm-up allocation.** Each output object lazily
   creates a scratch bundle (`DistanceOutput.Scratch()`, `CastOutput.Scratch()`,
   `TOIOutput.Scratch()`, `Manifold.Scratch()`) holding the growable lists,
   simplex workspace, and reused value objects the algorithm needs. Once
   warm, a repeated query on the same output allocates nothing — the steady
   state the zero-alloc benchmark gate enforces.
3. **Index-based shapes.** `ShapeProxy` stores vertices, normals, and radius;
   collision functions address shapes by index pairs, mirroring the upstream
   index-based clip points and manifold ids so the golden fixtures compare
   exactly.

Chain segments get their own family (`CollideChainSegmentAnd*`,
`ChainSegment`, `ChainNormalType`) because upstream treats one-sided chains
with ghost collisions differently from ordinary segments, including
normal-flip bookkeeping during SAT selection and clipping.

## Dynamic Tree and Broad Phase (Stage 5)

Stage 5 ports `dynamic_tree.c` and `broad_phase.c` on top of the Stage 3
containers. Four rules shape the layout:

1. **Node objects with a free list.** Each `TreeNode` owns its `AABB Bounds`
   (decision 0004): node arrays hold references, and `AllocateNode`/`FreeNode`
   thread the free list through `Parent`. Every mutator keeps the tree valid
   incrementally, and `Validate`/`ValidateNoEnlarged` re-derive heights,
   bounds, category unions, and the enlarged-flag invariant from scratch.
2. **One scratch bundle per tree.** Traversal stacks, ray/shape-cast input
   copies, rebuild workspace (`LeafIndices`, leaf centres, explicit rebuild
   stack), and the greedy sibling-search costs live in a lazily created
   `TreeScratch`. Queries and mutations are allocation-free once warm but not
   re-entrant; each tree serialises its own traversals.
3. **Callbacks are abstract classes.** `TreeQueryCallback`, `TreeRayCastCallback`,
   and `TreeShapeCastCallback` implement the upstream return-value protocol
   (0 terminates, -1 skips, a fraction narrows the cast). Cold/warm pairs
   (`Query`/`QueryTo`, `RayCast`/`RayCastTo`, `ShapeCast`/`ShapeCastTo`,
   `GetAABB`/`GetAABBTo`) keep the allocating forms out of warm code.
4. **The broad phase mirrors upstream move and pair bookkeeping.** Three
   trees (static, kinematic, dynamic) sit behind proxy keys packing the tree
   proxy id and body type (`(proxyId << 2) | type`). Moves buffer into a
   `PairKeySet` (key + 1, 0 reserved) plus an ordered array; pair generation
   queries the kinematic, static, and dynamic trees per moved proxy,
   de-duplicates through the move set and the pair set (Stage 3 table), and
   threads per-move candidate lists through parallel `IntegerList` pools that
   are consumed LIFO in deterministic move order. Pair reporting goes through
   a `BroadPhasePairSink`: `ShouldCollide` filters at record time and
   `AcceptPair` receives survivors, letting the Stage 6 world register
   contacts without exposing the broad phase.

Rebuild always uses the upstream median-split configuration
(`B2_TREE_HEURISTIC 0`); the binned SAH partitioning is deliberately not
ported. A partial rebuild dissolves only the enlarged path and treats clean
sibling subtrees as atomic leaves, so its returned leaf count is bounded by
but not equal to the proxy count.

## World Facade (Stage 6)

Stage 6 ports the world and lifecycle family of `box2d.h` plus `world.c`'s
non-solving half: body, shape, and chain records; mass maintenance; proxy
synchronisation; transforms; and the overlap/ray/shape-cast queries. Four
rules shape the layout:

1. **Façade objects over flat state.** `Body`, `Shape`, and `Chain` are
   per-slot façade objects holding scalars plus identity (`Owner`, `Id`,
   `Live`, `Generation`); heavy geometry lives in World-owned records
   (`BodySims`/`BodyStates` per solver set, `ShapeExtent` per shape). Every
   façade member calls `Validate`, which throws on a destroyed slot, so a
   stale façade can never read another object's state.
2. **Per-slot generations.** `BodyGenerations`/`ShapeGenerations`/
   `ChainGenerations` (parallel `IntegerList`s) carry the current generation
   per slot; freeing bumps the slot and the stale façade, creation adopts the
   slot value, so a reused slot never repeats a generation.
3. **Upstream-faithful bookkeeping.** Shape creation runs the conditional
   mass update on all four geometry paths; `SetType`, `SetTransform`,
   density, and material changes re-derive mass data and proxies exactly
   where upstream does. Solver sets 0/1/2 come from the solver-set id pool at
   world creation so per-island sleeping sets start at 3. Open chains attach
   segment *i* to material *i + 1* (the upstream leading-point rule). The
   world raises `RuntimeException` when locked instead of upstream's silent
   early returns; `StepWorld` validates the time step, locks the world, and
   drives the Stage 7 pipeline described below.
4. **Queries own nothing the caller did not pass.** The four `Into` forms
   reset only the caller's hit list, fill `ShapeHit` façades owned by the
   list, and apply category filtering through the broad-phase trees. The
   closest ray-cast form takes the nearest hit inside the traversal, and
   all-hit forms never clip across trees — both match upstream callback
   protocols exactly.

The virtual machine dispatches same-arity constructors by arity alone, so
`World` exposes `New World()`, `New World(settings)`, and the shared factory
`World.WithGravity(gravity)` instead of a second one-argument constructor.
Measured allocation budgets (test-pinned): a warm shape creation allocates
17 objects, all façade-owned state recreated by design; `World.Clear` plus
rebuilding eight one-shape bodies allocates 136 objects — exactly the eight
façade groups — because tree, sim, proxy, and id-pool capacity all survive
`Clear`.

## Soft Step Solver (Stage 7)

Stage 7 completes the simulation loop: `World.StepWorld` now runs pair
discovery, the narrow phase, island maintenance, the Soft Step constraint
solver, body finalisation, broad-phase refit, and island sleeping in
upstream's exact stage order.

1. **Contacts and islands.** Contacts live in per-set `ContactSims` stores;
   touching contacts additionally occupy a constraint-graph colour row.
   Island records link bodies, contacts, and their solver sets so sleeping
   moves a whole island at once. `IslandMethods` ports linking, splitting
   (DFS with `DfsVisitBody`), merging, and sleeping; `GraphMethods` ports
   colour choice, insertion, and removal.
2. **Constraint graph.** Twelve colours: 0-10 hold touching contacts whose
   body sets stay disjoint (BitSet rows over body ids), colour 11 is the
   serial overflow colour. Contacts carry `ColorIndex` plus their
   set/local location; validation cross-checks both.
3. **Solver kernels.** `ContactSolver` ports `b2PrepareContactsTask`,
   warm start, the Soft Step velocity solve (bias, speculative separation,
   mass scale, impulse scale), friction with conveyor-belt `TangentSpeed`,
   rolling resistance against accumulated normal impulse, restitution, and
   impulse storage. `Softness.Make` ports `b2MakeSoft`; `StepContext` is
   world-owned scratch so warm steps allocate nothing.
4. **Step pipeline.** `CollideStep` gathers colour and awake-set sims,
   refreshes mass terms, recomputes manifolds, and collects touching-state
   transitions in a contact-id BitSet consumed in ascending order.
   `SolveStep` merges and splits islands, prepares colours then overflow,
   and runs the substep loop (integrate velocities, warm start, bias solve,
   integrate positions, relax), restitution, and impulse storage.
   `FinalizeBodies` applies deltas, tracks sleep and split candidates,
   updates shape AABBs; `RefitBroadPhase` enlarges grown proxies;
   `SleepIslands` reverse-iterates awake islands into sleeping sets.
5. **Hooks.** `CustomContactFilter`, `CustomPreSolve` (manifold editing, opt
   in per shape via `EnablePreSolveEvents`), and `CustomMaterialMixer` run
   only when assigned, matching upstream's zero-overhead disabled path.

Determinism rests on ascending-id serial stages, the fixed colour order,
and the deterministic `PhysicsMaths` routines. The `stage7-pyramid-40`
benchmark (820 bodies, four 60 Hz steps per iteration) is the first
full-solver baseline and doubles as a checksum-pinned regression.

## Continuous Collision, Sensors, And Events (Stage 8)

Stage 8 adds high-speed collision handling and the post-step observation
model without changing the solver.

1. **Continuous collision.** `FinalizeBodies` flags dynamic bodies whose
   `maxVelocity * dt` exceeds half their minimum extent (`ShapeExtent` via
   `Shape.ComputeExtentTo`). Non-bullet fast bodies resolve
   `World.SolveContinuous` immediately: a swept-TOI query over the three
   broad-phase trees (static, kinematic, and dynamic for bullets only)
   advances the body to the exact impact transform, refreshes shape AABBs,
   and updates its moved-body event. The query callback mirrors upstream's
   `b2ContinuousQueryCallback`, including the chain-junction clipping rule
   and the small-circle TOI fallback. Bullet bodies are collected into
   `StepContext.BulletBodies` and resolved after the broad-phase refit,
   whose fast-bullet branch queues proxy moves for determinism.
2. **Sensors.** `Sensor` records live in `World.Sensors`, one per sensor
   shape, each holding double-buffered overlap lists of shape slot plus
   generation. After the solve, `OverlapSensors` swaps buffers, re-queries
   the three trees with the sensor's filter, resolves exact overlaps with
   a shape-distance test, sorts, and diffs against the previous buffer to
   publish begin/end events. Disabled sensor bodies and destroyed sensors
   publish end events, mirroring `b2DestroySensor`. Sensor shapes never
   create contacts: the world's broad-phase sink rejects sensor pairs.
3. **Events.** Flat world-owned columns back four event groups: contact
   begin (with pooled-manifold snapshots copied via `Manifold.AssignFrom`),
   contact end (also on touching-contact destruction), contact hit
   (approach speed and point from the stored manifold), and moved bodies
   (transform plus a fell-asleep flag patched by island sleeping).
   `World.Events` exposes a reusable `WorldEvents` view whose accessors
   return the existing stable Shape/Body façades or `Nothing` for stale
   references. Begin/hit/move buffers clear at step start and end-event
   buffers double-swap at step end, so every event is readable until the
   next step or destructive world operation, and user code never runs
   while the world is locked.

The pile golden in `SceneTests` was recomputed for Stage 8 because
continuous collision now advances fast bodies to their exact impact
positions.

## Deterministic Maths

`PhysicsMaths` ports the upstream trigonometric approximations so
simulation results do not depend on the platform maths library:

| Member | Upstream | Notes |
|---|---|---|
| `UnwindAngle(radians)` | `b2UnwindAngle` | Wraps to [-pi, pi] with half-to-even quotient rounding, like C `remainder`. |
| `Atan2(y, x)` | `b2Atan2` | Minimax polynomial tuned for 32-bit floats; kept verbatim in Double. Maximum deviation from the platform atan2 is about 2.8e-5 radians. |
| `ComputeCosSin(radians, out)` | `b2ComputeCosSin` | Bhaskara-style approximation, normalised to the unit circle; caller-owned output object avoids allocation. Deviation from platform sin/cos is about 1.7e-3, intrinsic to the upstream formula. |
| `TrailingZeros(value)` | `ctz.h` | Comparison ladder; returns 64 for zero. |
| `IsPowerOfTwo`, `BoundingPowerOfTwo`, `RoundUpPowerOfTwo` | `ctz.h` | Capacity helpers; zero is rejected as a power of two (upstream accepts it). |
| `HashKey(key)` | `b2KeyHash` | Murmur3 64-bit finaliser with logical shifts. |

`CosSin` mirrors the upstream `b2CosSin` struct as a small reusable object.

## Constants and Identity Sentinels

`PhysicsConstants` ports `src/constants.h` in metre units (upstream scales
some lengths by a user-settable `b2_lengthUnitsPerMeter`; Physics2D fixes
one metre per unit) and adds the identity sentinels from `src/core.h`:

- `NULL_INDEX = -1` — "no slot". Never interchangeable with slot 0.
- `INVALID_GENERATION = 0` — a generation that can never validate; issued
  generations start at 1.

`B2_MAX_WORKERS` and `B2_MAX_WORLDS` are excluded: Physics2D has no task
system and worlds are ordinary Objo objects.

## Foundation Performance Envelope

Release results on the Stage 2 reference machine (Mac, `objo` 26.9.1, see
`benchmarks/results/stage3-foundation-2026-08-30T21-41-14.json`) keep every
foundation container inside the Stage 2 representation envelope — per-element
costs at or below the accepted scalar-array kernel (~182 ms for the full
10,000-body kernel set) and zero allocations in every measured scenario:

| Scenario | Work per iteration | Median | Allocations |
|---|---|---|---|
| `stage3-slot-pool` | 4096 allocs + 1365 frees + 1365 reuses + fold | 10.8 ms | 0 |
| `stage3-bit-set` | 16384 sets + 3277 clears + union + full iteration | 22.2 ms | 0 |
| `stage3-pair-key-set` | 2000 adds + 500 removes + 1000 probes + 1000 re-adds | 6.5 ms | 0 |
| `stage3-scratch-lists` | 20000 appends + 3000 swap removals + 200 pops | 7.1 ms | 0 |

Growth and `Clear` costs are warm-up costs by design: capacity persists
across `Clear`, so a warmed engine step never re-grows.

Stage 4 adds the collision geometry envelope (same machine, see
`benchmarks/results/` for the latest run). Every scenario is the steady-state
reuse path after one warm-up query:

| Scenario | Work per iteration | Median | Allocations |
|---|---|---|---|
| `stage4-distance` | 12 shape-distance queries on warmed outputs | 0.25 ms | 0 |
| `stage4-manifold` | 8 polygon and capsule collision manifolds | 0.26 ms | 0 |
| `stage4-cast` | 8 ray casts and 8 shape casts | 0.17 ms | 0 |
| `stage4-time-of-impact` | 8 swept-capsule time-of-impact solves | 0.47 ms | 0 |
| `stage4-plane-solver` | 8 mover solves against 3 planes | 0.13 ms | 0 |

Stage 5 adds the spatial index and broad-phase envelope (same machine, see
`benchmarks/results/` for the latest run). Tree mutation costs are dominated
by the interpreted VM's scalar arithmetic; allocation gates still read zero:

| Scenario | Work per iteration | Median | Allocations |
|---|---|---|---|
| `stage5-tree-churn` | 64 moves + 16 destroy/create pairs in a 2048-leaf tree | 11.1 ms | 0 |
| `stage5-tree-query` | 4 queries, 4 rays, 1 shape cast on a 4096-leaf tree | 2.0 ms | 0 |
| `stage5-tree-rebuild` | 32 enlarges + partial rebuild of a 2048-leaf tree | 12.2 ms | 0 |
| `stage5-broadphase-pairs` | 32 of 512 dynamic proxies moved; pairs reported | 4.2 ms | 0 |
