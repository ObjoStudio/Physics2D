# Decision 0003 — Objo Standard-Library Maths Audit

- **Status:** Accepted
- **Date:** 2026-08-30

## Context

Stage 2 of the port audits the Objo standard library against every maths
operation Box2D 3.1.1 needs, because hot-path solver code must not allocate
and must not fall back to component-wise scalar workarounds that obscure the
port. The audit walked the pinned upstream sources
(`math_functions.h`, `b2_math.c`, `b2_dynamic_tree.c`, joint and contact
sources) and classified each operation as covered by the existing library,
requiring an addition, or deliberately rejected.

Three gaps mattered for the solver and broad-phase work:

1. Upstream applies `b2CrossSV`/`b2CrossVS` (scalar cross with a vector) at
   more than thirty solver and joint sites. The existing `Vector2` offers
   `Cross(Vector2)` and `Multiply(scalar)` but no perpendicular, so each site
   would have to hand-roll `(-y, x)` or `(y, -x)` with two `Set` calls.
2. `b2GetInverse22` and `b2Solve22` (2x2 matrix inverse and solve used by the
   revolute, prismatic, and weld joint blocks) have no `Matrix` equivalent;
   the existing `Matrix` covers rotation, transpose, transform, and 3x3 work.
3. Broad-phase and body code test finiteness (`b2IsFinite`) when accepting
   proxies and validating simulation state; no `Double` or `Maths` member
   exposed that check.

Component-wise `Vector2.Abs` was considered and rejected: upstream applies
`b2Abs` to a `b2Vec2` at only three sites (AABB centre computation and two
dynamic-tree helpers), all initialisation-phase code where allocation-free
scalar handling is already natural.

## Decision

1. Adopt four standard-library additions, implemented and tracked as Objo
   issue #1302:
   - `Vector2.LeftPerpendicular()` and `Vector2.RightPerpendicular()` —
     mutating, returning `this`; `(-y, x)` and `(y, -x)` respectively. With
     `Multiply(scalar)` they express both `b2CrossSV` and `b2CrossVS`
     allocation-free: `v.RightPerpendicular().Multiply(s)` computes
     `s * cross(v)`-style terms in the upstream grouping.
   - `Matrix.Inverse()` (new matrix), `Matrix.Inverse(out)` (into a caller
     matrix), and `Matrix.InvertSelf()` (in place). A singular matrix yields
     the zero matrix, matching upstream `b2GetInverse22`, which writes a zero
     matrix when the determinant is zero.
   - `Matrix.Solve(b)` and `Matrix.Solve(b, out)` — 2x2 linear solve using
     Cramer's rule with the same zero-result-on-singular behaviour as
     upstream `b2Solve22`.
   - `Double.IsFinite()` — matches `b2IsFinite` semantics for simulation
     validation. The shared `Maths.IsFinite(value)` form was rejected as
     redundant with the instance form.
2. Reject a dedicated scalar-cross pair of overloads
   (`Cross(scalar)`/`CrossOf(scalar, v)`): the perpendicular-plus-multiply
   composition covers every upstream call site without adding API surface,
   and keeps the vector API close to the language's existing in-place style.
3. Reject component-wise `Vector2.Abs` for the three upstream sites; scalar
   `Maths.Abs` at those call sites is clearer and allocation-free.

## Consequences

- Solver and joint ports use one consistent allocation-free idiom for
  scalar-cross terms instead of per-site hand-rolled component maths.
- The bake-off kernels (decision 0004) exercise all four additions, and their
  bit-identical checksums across candidate representations depend on the new
  members computing exactly the scalar formulas they replace.
- Physics2D's minimum Objo version (see `docs/PORTING.md`) now includes these
  members; they ship with Objo issue #1302 and are covered by engine tests
  and standard-library documentation.
