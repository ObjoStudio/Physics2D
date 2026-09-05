# Decision 0007 — Loop Exit Uses `Exit`, Never `Break`

- **Status:** Accepted
- **Date:** 2026-09-04

## Context

Running the desktop demo under the Objo Studio debugger paused execution
inside `DynamicTree.FindBestSibling` at a `Break` statement before the demo
window appeared. Engine commit `de96e932b2b9f6a1ebe1344d2f9cd3fc26c73083`
("Implement Break keyword as programmatic breakpoint", 2026-02-10) defines
Objo's `Break` statement as a programmatic debugger breakpoint: when a
debugger is attached, execution pauses at the `Break` line; without a
debugger it is a no-op. It has never been a loop-exit statement in this
engine lineage: `Exit` (optionally `Exit While`, `Exit For`, `Exit Do`)
exits the innermost loop and has existed since the original recursive
descent parser (`d919565f`, 2026-02-08). The repository's AGENTS.md
language note previously claimed `Break` exits the innermost loop — a
carried-over Xojo convention, where `Break` pauses the debugger and `Exit`
leaves a loop.

Because the module was authored against that stale note, six loop-early-out
sites used `Break` instead of `Exit`: three in `DynamicTree` (the
`FindBestSibling` descent stop and both `EnlargeProxy` ascent early-outs),
one in `BroadPhase` (move-buffer removal scan), one in the demo's
single-step accumulator loop, and one in a `BroadPhaseTests` helper. All
six were silent no-ops at runtime: the intended early exits never fired,
loops ran on to their ordinary termination conditions, and every debug run
paused the app at the first reached `Break` — during demo scene
construction, before the window appeared.

## Decision

1. **`Exit` is the only loop-exit statement in this repository.** Module,
   demo, benchmark, and test sources use `Exit`, `Exit While`, `Exit For`,
   or `Exit Do`. Nested loops that must both unwind use a boolean flag per
   the existing style rules.
2. **`Break` is reserved for deliberate debugger pauses and appears nowhere
   in shipped or test code.** It compiles to `OpCode.Breakpoint` and pauses
   any debug run that reaches it.
3. **The stale AGENTS.md language note is corrected** so future slices do
   not reintroduce the bug.
4. **No compatibility bump.** `Exit` predates the documented minimum Objo
   version (26.8.6) by months; `docs/PORTING.md` is unchanged.

## Consequences

- Debug runs no longer pause inside physics internals; the demo manual
  interaction pass works under the Studio debugger.
- The intended hot-path early exits actually execute again. The full suite
  (337 Physics2D tests plus 19 benchmark harness tests) passes identically
  before and after the repair, and all golden fixtures are unchanged, which
  confirms the dead early exits did not alter observable results within
  test coverage; the practical losses were wasted descent/ascent/scan work
  and the debugger pauses.
- Contributors arriving from Xojo or classic BASIC must map `Break` to
  `Exit` for loop control; `Break` keeps its debugger meaning exactly as
  documented in the Objo debugging guide.
