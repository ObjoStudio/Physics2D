# Decision 0001 — Upstream Source And Version 1 Scope

- **Status:** Accepted
- **Date:** 2026-08-30

## Context

Physics2D is a native Objo port of Box2D. The implementation needs one pinned
algorithm source so behaviour, tests, and golden fixtures are auditable, and it
needs a stable version 1 feature scope so porting can proceed without repeated
product decisions.

## Decision

1. Box2D tag `v3.1.1`, commit `8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3`, is the
   sole authoritative algorithm source. Forge2D, JBox2D, and the Xojo Physics
   project are comparative references only and are never copied.
2. The port targets Objo `Double` arithmetic. Behaviour follows the upstream
   algorithms rather than promising bit-for-bit equality with C `float`.
3. Version 1 implements the complete Box2D 3.1.1 rigid-body feature set listed
   in `IMPLEMENTATION_PLAN.md` §3.7. Particles, native SIMD kernels, task
   callbacks, custom C allocators, native timers, and C integration hooks are
   excluded.
4. Box2D 3.1.1 defines eight joint types. The seven families named by the plan
   (distance, motor, mouse, prismatic, revolute, weld, wheel) are delivered in
   Stage 9, and the trivial `FilterJoint` is also implemented so every upstream
   public capability maps to an implemented API. This is an addition to the
   plan's seven named families, not a substitution.
5. Collision-only helpers are exposed as public, world-free static helpers
   where the pinned upstream exposes them publicly; solver, broad-phase, and
   narrow-phase internals stay protected.

## Consequences

- Golden fixtures are generated only from the pinned commit.
- Any future upstream re-pin requires a new decision record, regenerated
  fixtures, and an inventory diff (`docs/PORTING.md`).
- Behaviour differences caused by `Double` arithmetic are documented here and
  in `docs/PORTING.md` rather than hidden.
