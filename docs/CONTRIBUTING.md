# Contributing To Physics2D

Thank you for improving Physics2D. This guide covers the day-to-day work:
building and testing, the benchmark discipline, Objo source style,
documentation standards, generated artifacts, and provenance rules. The
authoritative rulebook is [AGENTS.md](../AGENTS.md) — read it fully before
your first change; this document is the practical companion.

## Setting up

Physics2D is developed against the in-development Objo checkout, not an
installed release. Resolve the `objo` CLI in this order:

1. the `OBJO` environment variable, if set;
2. `dotnet run --project /Users/garry/Repos/Objo/src/studio/Objo.Cli --`
   (always rebuilds current engine code); or
3. an installed `objo` as a last resort.

For repeated commands, build the CLI once and run the standalone binary:

```bash
dotnet build /Users/garry/Repos/Objo/src/studio/Objo.Cli
/Users/garry/Repos/Objo/src/studio/Objo.Cli/bin/Debug/net10.0/objo --version
```

If you change anything in the Objo checkout, rebuild the CLI **and both**
`Objo.TestHost` configurations — the test host locator prefers `bin/Debug`,
so a fresh Release build does not rescue a stale Debug binary:

```bash
dotnet build /Users/garry/Repos/Objo/src/studio/Objo.TestHost -c Debug
dotnet build /Users/garry/Repos/Objo/src/studio/Objo.TestHost -c Release
```

## Repository layout

- `Shared/Sources/` — the canonical `Physics2D` module: one `.objobasic`
  source item plus a `.source.json` sidecar (GUID, kind, build scope,
  parent module) per item.
- `Projects/` — `Physics2D.Tests` (Objo test project),
  `Physics2D.Benchmarks` (command-line benchmark app),
  `Physics2D.Smoke` (command-line consumer), `Physics2D.Demo` (desktop demo).
- `dist/Physics2D.objobasic` — the generated single-file distribution. Never
  edit it by hand.
- `docs/` — user documentation, porting notes, and numbered decisions.
- `testdata/golden/` — fixtures generated from the pinned Box2D checkout.
- `benchmarks/results/` — committed raw benchmark results.
- `tools/` — deterministic packaging, audit, and fixture tooling.

## Everyday checks

From the repository root (shown with the built CLI):

```bash
OBJO=/Users/garry/Repos/Objo/src/studio/Objo.Cli/bin/Debug/net10.0/objo

$OBJO check Physics2D.objosln                 # compile-check the active project
$OBJO test  Physics2D.objosln --all-projects  # every test project
python3 tools/assemble_module.py --check      # distribution staleness
python3 tools/audit_api.py                    # inventory vs distribution
python3 tools/generate_api_docs.py            # regenerate docs/API.md
python3 tools/check_doc_samples.py            # doc code samples compile
tools/check_distribution.sh                   # clean-room consumer check
```

Release-audit tools (run before tagging a release; see
[docs/RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md)):

```bash
python3 tools/inspect_distribution.py         # dist content inspection
python3 tools/clean_consumer_check.sh         # dist-only consumers build and run
```

Notes that save time:

- `check` covers only the active project; use `test --all-projects` and
  `build` for full-source coverage.
- Test methods must start with `Test` (case-insensitive) or discovery
  silently skips them; test classes need `Inherits TestCase` and a
  full-field `.source.json` sidecar with `"BuildScope": "Test"`.
- `--filter` is an anchored glob over `QualifiedClass.TestMethod`: filter
  `Stage13Tests.*`, not `Stage13Tests` (a zero-selection run proves nothing).
- The default per-test timeout is 30 s (`--timeout 120s` exists for focused
  runs); design tests to fit.
- Golden fixtures regenerate with `tools/fixture_gen/build.sh` followed by
  the per-fixture commands in `testdata/golden/MANIFEST.md`; committed
  fixtures must never depend on the C tool at test time.

## Writing tests

Every algorithmic change ships with tests in the same commit. Use the
level of evidence that matches the change:

- focused unit tests for maths, containers, geometry, and lifecycle rules;
- golden comparisons against the Box2D-derived fixtures;
- invariant checks (`Validate`) after seeded stress scenarios;
- deterministic seeded stress tests with a fixed-seed generator;
- end-to-end scenes with pinned fold checksums;
- compile tests for public API surface (`ApiExamplesTests`);
- zero-allocation gates for hot paths (`System.AllocationCount`).

Rules: never weaken an assertion or loosen a tolerance to make a gate pass —
record the reason for any unusually loose tolerance instead. Tests must pass
together in a fresh process, not merely in isolation.

## Benchmarks

Performance claims require Release-mode evidence. The benchmark app builds
with:

```bash
$OBJO build Physics2D.objosln --project Physics2D.Benchmarks --output build/benchmarks
build/benchmarks/macOS-Apple-Silicon/Physics2D_Benchmarks --output benchmarks/results --name my-run
build/benchmarks/macOS-Apple-Silicon/Physics2D_Benchmarks --only <scenario> --output /tmp/p2d --name focused
```

- Every scenario folds a deterministic checksum; if it changes, the run is a
  correctness failure, not a faster engine.
- Identical code drifts roughly ±5% between runs (up to 10% on short
  scenarios). Only paired comparisons with identical checksums are evidence.
- Preserve raw results under `benchmarks/results/`; never report a single
  best run.
- Before accepting a hot-path refactor, run the focused correctness tests and
  the affected microbenchmark. Revert optimisations that do not beat noise or
  that make invariants harder to explain.
- After capacity warm-up, `World.StepWorld` must allocate nothing in the
  normal path; the zero-allocation tests pin this.

## Objo source style

Stored `.objobasic` files contain **no leading whitespace** — Studio supplies
visual indentation. Beyond that:

- Types, methods, properties, and enum members use `PascalCase`; locals and
  parameters use `camelCase`; constants use `UPPER_SNAKE_CASE`.
- Put a `##` doc comment above each member — one per declaration, since a
  comment attaches only to the declaration that immediately follows.
- Avoid any identifier that collides with a keyword when case is ignored
  (`iF`, `var`, `set`, `step`, `in`, `mod`). The lexer is fully
  case-insensitive, so `iF` lexes as `If`.
- Compare references (including `Nothing`) with `=` and `<>`; there is no
  `Is` operator.
- Use overloads for common construction paths (Objo has no named arguments)
  and shared factory functions when two constructors share an arity — the VM
  dispatches same-arity constructors by position, and the second is silently
  unreachable.
- Declare helper classes as separate source items; classes cannot nest inside
  classes. A class in a script must be declared before the top-level code
  that uses it.
- Loops end with `Next` (repeating the same loop variable for `For Each`),
  `While` loops end with `Wend`, and `ElseIf` is one keyword.
- String concatenation is `+`; the backslash is the string escape character.
- Hot-path rules (no allocating operators, no dictionaries, closures, or
  shifting removals inside solver loops, pre-sized and reused buffers) are
  listed in [AGENTS.md](../AGENTS.md) and enforced by gates — see
  [docs/PERFORMANCE.md](PERFORMANCE.md) for the patterns that satisfy them.

Each new source item needs a fresh generated UUID in its `.source.json`
sidecar — never reuse or hand-pick GUIDs. Project test sidecars carry the
full field set; module children name `Physics2D` as their parent, while
project sources leave `ParentModule` empty.

## Documentation

Every public type and member documents: what it represents, units and
coordinate conventions, valid ranges and exceptional cases, ownership and
lifetime rules, whether returned vectors/arrays are copies or reused views,
allocation behaviour where it matters in a frame loop, and whether the world
may be mutated while the operation is active. Internal comments explain
algorithms, invariants, data layout, and non-obvious performance choices —
and retain a nearby reference to the corresponding upstream function for
substantial ported algorithms.

`docs/API.md` is generated from the verified distribution declarations by
`tools/generate_api_docs.py`; do not edit its declaration sections by hand.
Every user-facing feature keeps at least one compact example, and every
`objo` code sample in the docs must compile — `tools/check_doc_samples.py`
enforces it in aggregate. Update the relevant document in the same commit as
the feature: README, GETTING_STARTED, API (regenerated), ARCHITECTURE,
PERFORMANCE, PORTING, or DEMO.

## Generated artifacts

- `dist/Physics2D.objobasic` is build output of `tools/assemble_module.py`,
  which is deterministic: regenerating from an unchanged checkout is
  byte-identical. Commit regenerated output when `Shared/Sources` changes;
  `assemble_module.py --check` fails in CI-style runs when the two diverge.
- The distribution must compile in a clean consumer project containing only
  that file (`tools/check_distribution.sh` proves it) with no external
  imports, no desktop-only types, no debug output, and no TODO markers.
- `docs/decisions/` records every durable design decision; the version 1
  public API is frozen (decision 0005), so breaking changes need a decision
  record and maintainer approval first.

## Provenance and licensing

The pinned upstream is Box2D tag `v3.1.1`, commit
`8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3`. Algorithms come from that source
alone; Forge2D, JBox2D, and the Xojo Physics project are consulted as
comparative references and never copied. If a change ever requires copying
from a secondary port, stop and add the applicable notice and provenance
record to `THIRD_PARTY_NOTICES.md` first. Keep the subsystem/function mapping
in `docs/PORTING.md` current as you port.

## Commit expectations

A coherent commit contains the change, its tests, its documentation, and —
when `Shared/Sources` changed — the regenerated distribution and refreshed
generated docs. Run at least the focused tests and the packaging checks
before committing; stage gates require the full suite plus Release
benchmarks. Never reset, clean, or overwrite a broad path to recover a
working tree; inspect targets before deleting generated artifacts.
