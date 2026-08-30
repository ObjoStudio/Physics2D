# Decision 0004 — Body-State and Broad-Phase Tree Representation Bake-Off

- **Status:** Accepted
- **Date:** 2026-08-30

## Context

Stage 2 requires choosing the runtime representation for body state and the
broad-phase dynamic tree before the Stage 3 port begins, because every later
kernel is written against that choice. Objo is an interpreted, ARC-managed
language, so C intuition (objects are expensive, flat arrays are fastest)
does not transfer. Six body-state representations and two dynamic-tree
storage schemes were implemented as benchmark candidates running identical
kernels in identical order, with folded checksums proving the candidates
performed exactly the same work.

The candidates:

- `stage2-object-nodes` — one class instance per body with `Vector2` child
  objects for position, velocity, and force.
- `stage2-vector-arrays` — parallel `Vector2` arrays plus scalar arrays.
- `stage2-scalar-arrays` — parallel x/y `Double` arrays.
- `stage2-flat-block` — one flattened `Double` array, stride 11.
- `stage2-dense-nodes` — one compact scalar-only node object per body.
- `stage2-memory-block` — one `MemoryBlock` addressed by byte offsets.
- `stage2-tree-indexed` — dynamic tree with parallel arrays and a free list.
- `stage2-tree-linked` — dynamic tree with one allocated node object per
  node, references dropped on removal.

Each body candidate ran integration (4 substeps), transform/inverse
transform, vector operations (dot, cross, perpendicular-normalise, 2x2
solve), sequential and pseudo-random velocity access, grid pair generation
with deduplication, a 1500-contact velocity iteration (8 iterations per
substep with warm-start impulses), and an allocation-pressure kernel
(3000 slots, 1500 frees and replacements) over 10,000 bodies. Each tree
candidate built 2000 proxies, ran 800 moves, 1000 overlap queries, 200
destroy/reinsert churn operations, and a full rebuild. Warm-up 5, measured
20. The tree algorithm ports upstream `b2FindBestSibling` (cost-based greedy
descent including the centroid-distance tie-break), `b2RotateNodes`,
`b2InsertLeaf`, and `b2RemoveLeaf` from Box2D 3.1.1; the port splits the
four-case rotation into a helper method to stay within project method
complexity limits, and uses `Double` where upstream uses `float`, consistent
with Objo's lack of a single-precision type.

## Decision

Adopt the following representations for the Stage 3 port, measured on the
reference Mac (`benchmarks/results/stage2-bake-off-2026-08-30T19-31-33.json`,
Objo 26.9.1, medians):

1. **Body state: parallel x/y `Double` arrays (`stage2-scalar-arrays`).**
   Fastest overall at 185.1 ms median, allocating nothing per iteration.
   Solver, contact, and joint working state follows the same representation
   (parallel scalar arrays inside pool/storage classes), because the frozen
   contact kernel and the pair kernel exercise exactly that access pattern;
   public and cold-boundary APIs may still use mutable `Vector2` where it
   costs nothing measurable. Compact node objects (`stage2-dense-nodes`,
   188.3 ms) are statistically close but allocate 90,000 objects per run
   through the pressure kernel and add ARC retention traffic; scalar arrays
   keep the port closest to upstream SoA layouts. `Vector2` arrays
   (228.8 ms) lose because every kernel pays property-get costs on the
   aggregate; object-per-body (244.6 ms) loses on both time and allocations
   (360,000 per run); `MemoryBlock` (392.3 ms) is slowest because every
   access is a native call with offset arithmetic. Per-byte flat arrays
   (233.2 ms) lose to parallel arrays because Objo bounds-checks each
   computed offset.
2. **Dynamic tree: node objects (`stage2-tree-linked`).** Faster in the Objo
   VM (321.5 ms) than the parallel-array pool (354.4 ms) despite allocating
   183,960 nodes per run: each array access in the indexed tree goes through
   a bounds-checked Array native call, which costs more than object field
   access plus ARC retain/release. This inverts the C expectation and is a
   VM-cost artefact, so decision records for later stages must re-validate
   if the VM gains array-access fast paths. The linked form also exercises
   cycle collection, since parent and child references form reference
   cycles that Objo's ARC collects.
3. **All six body candidates produced checksum `5870864046903980835`; both
   tree candidates produced checksum `1585456338644426542`**, proving the
   kernel formulas are representation-independent. The scalar-array kernels
   allocate zero objects per measured iteration (covered by a standing test
   gate).
4. **Source readability and invariant complexity**, recorded alongside the
   measurements as the plan requires: compact node objects are the most
   readable and carry the simplest invariants; parallel scalar arrays are
   more verbose at call sites but need only array-length invariants;
   `Vector2` arrays read well but hide per-access aggregate overhead; the
   flattened block requires offset constants that invite silent misuse; the
   `MemoryBlock` candidate is the least readable (manual byte offsets, no
   bound checking beyond the native call); the linked tree is considerably
   easier to verify than the indexed tree, whose free-list and proxy-map
   bookkeeping produced the only representation bug found during the bake-off
   (a stale pool across repeated executions). Readability did not override
   the measured winner for body state because the scalar-array kernels are
   mechanical translations of upstream component maths.

## Consequences

- Stage 3 ports body state, islands, and contacts against parallel scalar
  arrays and ports `b2DynamicTree` with per-node objects; both follow the
  kernel formulas frozen by the bake-off sources.
- `Vector2.LeftPerpendicular`/`RightPerpendicular` and `Matrix.Solve`
  (decision 0003) are load-bearing in the frozen vector-operations kernel;
  the checksum pinning means any future engine change that alters their
  results will be caught by the bake-off tests.
- The `MemoryBlock` result rules out byte-addressed storage for the engine
  port; it remains available for serialisation boundaries only.
- The benchmark harness, candidates, and checksum contract are retained as
  regression fixtures; rerunning the bake-off after VM performance work
  should reproduce the same checksums with different medians.
