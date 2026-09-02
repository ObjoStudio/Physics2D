# Decision 0005 — Version 1 Public API Freeze

- **Status:** Accepted
- **Date:** 2026-09-02

## Context

Stage 10 closes the useful Objo surface and freezes the version 1 API. The
freeze review ran over the complete module, not a summary of it:

1. **Inventory reconciliation.** `tools/audit_api.py` (new) parses every row
   of the 429-symbol PORTING inventory and verifies each `Class.Member`
   mapping against the generated `dist/Physics2D.objobasic`, reporting
   unknown types, missing members, and rows without a mapping. The audit
   run at freeze time checked 241 member references across 329 mapped rows
   with zero errors. The audit found and fixed two stale mapping rows: the
   friction and restitution callbacks map to `World.MaterialMixer` (the
   implemented `CustomMaterialMixer` surface), not the never-built
   `World.FrictionMixer`/`World.RestitutionMixer` names.
2. **Length-scale rows resolved.** The two "only if pinned upstream
   requires it" rows (`b2GetLengthUnitsPerMeter`,
   `b2SetLengthUnitsPerMeter`) are resolved as approved version 1
   exclusions: Physics2D pins the upstream default of one metre per length
   unit, and screen-scale conversion belongs to the render adapter (the
   demo's `CanvasDebugRenderer` shows the pattern). No public member
   remains conditional.
3. **Internal surface guidance.** The dist exposes engine bookkeeping the
   compiler requires to be public (solver-set indices, edge keys, event
   buffers, pools). Their doc comments now say "Internal solver
   bookkeeping" or "Internal event bookkeeping" and point at the intended
   surface (`World.Events`, `IsValid`), and the generated reference omits
   them. The user-facing reference is not polluted with solver internals.
4. **Compile coverage.** `ApiExamplesTests` (new) constructs and calls
   every public overload family: the three world construction paths, both
   step overloads, every shape family, all eight joint families with their
   definition and runtime setters, every query overload family (callback
   and reusable-list forms, closest forms, movers), the debug-draw option
   combinations, and the event/lifetime rules. Six tests, all passing.
5. **Reference document.** `docs/API.md` is generated from the verified
   declarations in the dist by `tools/generate_api_docs.py` with curated
   categories; it contains no aspirational members by construction, and
   regeneration fails loudly if a curated name disappears.

## Decision

The public API of the `Physics2D` module is **frozen for version 1** as of
this decision, on the surfaces documented in `docs/API.md`:

- the `World` façade and its construction, stepping, tuning, query, mover,
  explosion, counters, profile, event, and debug-draw entries;
- the `Body`, `Shape`, and `Chain` façades with their definitions and
  geometry value types;
- all eight joint families with their definitions and runtime accessors;
- the reusable records and callbacks (`WorldCounters`, `WorldProfile`,
  `WorldEvents`, `ShapeHitList`, the callback abstract classes);
- the debug-drawing surface (`DebugDrawOptions`, `DebugRenderer`,
  `DebugColors`, `World.DrawDebug`).

Classes listed under "Internal infrastructure" in `docs/API.md` are engine
implementation, not frozen API; they may change between versions.

**Breaking changes after this point** — removing or renaming a public
member, changing a public signature, changing documented defaults, units,
ownership, or allocation promises — require a new decision record in this
directory and explicit user approval before implementation.

Additive changes (new overloads, new read-only accessors, new joint or
shape features ported from upstream) remain allowed within the existing
naming and documentation standards.

## Consequences

- The version 1 release documents exactly `docs/API.md`; user-facing docs
  and examples must not teach internal classes.
- `tools/audit_api.py` becomes part of the release audit: rerun it after
  any inventory or source change and before tagging a release.
- Engine-internal classes may be refactored freely (subject to the
  performance rules) without a decision record, because no application
  code is expected to depend on them.
- The minimum supported Objo version is governed by `docs/PORTING.md`
  (minimum Objo version section) and changes there require the documented
  compatibility test.
