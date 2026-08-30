# AGENTS.md

## Purpose

This repository contains `Physics2D`, a native Objo implementation of the
Box2D 3.1.1 rigid-body physics engine. The finished library must be Objo source
code that users can add to an Objo Studio project and consume with:

```objo
Import Physics2D
```

The shipped `Physics2D` module must have no external runtime dependencies. It
must not require a C library, FFI, Dart, Java, Xojo, a managed assembly, a web
service, or generated native code. Objo's core standard library is part of the
language and is allowed. The desktop demo may use Objo's desktop standard
library, but the physics module itself must remain project-type neutral.

The project has four product priorities, in this order:

1. Correct and robust physics behaviour.
2. The fastest practical implementation in native Objo.
3. An idiomatic, stable public Objo API rather than a transliterated C API.
4. Teaching-quality source code, examples, and documentation.

Never trade correctness for a benchmark result. Never make the public API
awkward merely to resemble upstream Box2D. Optimised internal code is welcome,
but its representation choices and invariants must be explained.

## Sources Of Truth

Use these sources in descending order of authority:

1. `IMPLEMENTATION_PLAN.md` for scope, stage order, architecture, and completion
   gates.
2. Box2D tag `v3.1.1`, commit
   `8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3`, for physics algorithms and
   behavioural intent.
3. The current Objo checkout and language specification for what Objo actually
   supports.
4. The official Objo documentation for the public language, standard-library,
   and Studio API. It lives in the `objo-docs` repository (normally at
   `/Users/garry/Repos/objo-docs`) and is published at
   `https://docs.objo.dev`. Treat both as authoritative; the checkout is the
   editable source of the published site.

The relevant local Objo sources normally live at:

- `/Users/garry/Repos/Objo`
- `/Users/garry/Repos/objo-docs`

When a standard-library behaviour matters to this port, verify it against the
official documentation (checkout or `docs.objo.dev`) and, where the two could
diverge, against the Objo checkout itself. Update `objo-docs` for any
user-visible behaviour change, following its `AGENTS.md`.

Read and obey the `AGENTS.md` files in those repositories before changing them.
Do not assume those paths exist on every machine; locate the checkouts when
necessary.

## Development Commands And Machine Notes

Run the Objo tooling from the in-development Objo checkout, not from an
installed release: the checkout is the source of truth for the engine, and a
published `objo` on PATH may be missing or stale. Resolve the CLI in this
order: the `OBJO` environment variable if set; otherwise
`dotnet run --project /Users/garry/Repos/Objo/src/studio/Objo.Cli -- ...`
(which always rebuilds current engine code); otherwise an installed `objo`.

`dotnet run` rebuilds on every invocation. For repeated commands, build the CLI
once and run the standalone binary it emits, remembering to rebuild it after
any engine change:

```bash
dotnet build /Users/garry/Repos/Objo/src/studio/Objo.Cli
/Users/garry/Repos/Objo/src/studio/Objo.Cli/bin/Debug/net10.0/objo --version
```

From this repository's root:

```bash
OBJO="dotnet run --project /Users/garry/Repos/Objo/src/studio/Objo.Cli --"  # or export a working objo

$OBJO check /Users/garry/Repos/Physics2D/Physics2D.objosln
$OBJO test  /Users/garry/Repos/Physics2D/Physics2D.objosln [--project <name> | --all-projects] [--filter <pattern>]
$OBJO build /Users/garry/Repos/Physics2D/Physics2D.objosln --project Physics2D.Benchmarks --output build/benchmarks
```

There is no `run` command: build, then execute the binary under
`build/<output>/macOS-Apple-Silicon/`.

After changing engine code in the Objo checkout, also rebuild the test host or
`objo test` keeps using a stale binary and reports missing standard-library
members such as `System.AllocationCount`. Rebuild BOTH configurations: the
test host locator tries `bin/Debug` before `bin/Release` (it walks the CLI's
own configuration name first), so a fresh Release build does not rescue a
stale Debug build — a stale Debug binary silently wins:

```bash
dotnet build /Users/garry/Repos/Objo/src/studio/Objo.TestHost -c Release
dotnet build /Users/garry/Repos/Objo/src/studio/Objo.TestHost -c Debug
```

If a member that definitely exists in the Objo checkout still reports
"Undefined property" at runtime, suspect a stale test host or CLI binary;
rebuild `Objo.TestHost` (both configurations) and `Objo.Cli`.
`tools/check_distribution.sh` resolves the CLI itself
(`$OBJO` override, then the Objo checkout, then `objo` on PATH).

Regenerating the distribution requires only Python 3:

```bash
python3 tools/assemble_module.py [--check]
tools/check_distribution.sh
```

Golden fixtures regenerate with `tools/fixture_gen/build.sh` followed by the
per-fixture commands in `testdata/golden/MANIFEST.md`.

Every source item is a `.objobasic` file plus a `.source.json` sidecar carrying
its GUID, kind, build scope, and (for module children) `ParentModuleId`. Give
each new source item a fresh generated UUID; never reuse or hand-pick GUIDs.
The test host's working directory is not the repository root; fixture paths are
resolved by `PhysicsAssert.RepoPath` (`PHYSICS2D_REPO_ROOT` environment
variable, then walking up from the CWD, then `~/Repos/Physics2D`).

## Objo Language Notes

Practical rules learned while building the harness. The language specification
in the Objo checkout remains authoritative.

- Loops end with `Next` (optionally `Next i`), not `End For`.
- Lexing is fully case-insensitive, including identifiers: a variable named
  `iF` lexes as the `If` keyword and produces confusing parse cascades. Avoid
  any name that collides with a keyword when case is ignored (`iF`, `var`,
  `step`, `in`, `mod`, ...).
- There is no `Is`/`IsNot` operator. Compare references — including against
  `Nothing` — with `=` and `<>` (`If node <> Nothing Then`).
- Classes cannot nest inside classes. Declare helper classes as separate
  source items.
- In a script (test program or app source), a class must be declared before
  the top-level code that uses it.
- Put a `##` description comment above each member; the compiler warns when a
  public member lacks one.
- Default parameter values need the `Optional` keyword; overrides need the
  `Override` keyword; abstract members are supported in `Abstract Class`.
- `Integer` is a signed 64-bit type. An FNV-1a offset basis overflows the
  commonly published 64-bit constant; use `-3750763034362895579`.
- `Mod` is an operator (`i Mod 7`), not a method. There is no `Array.Copy()`;
  use `New Array(Of T)` plus `AppendAll`.
- The backslash is a string escape character (`"\\"` is one backslash). Use
  `Chr(34)` for JSON quotes.
- `WriteAllText` emits a UTF-8 BOM; machine consumers of generated JSON should
  read UTF-8 with BOM tolerance.
- `FileSystemItem.Child()` rejects path separators; walk segments with
  `ResolveChild`-style helpers.
- `System.Platform` returns an `Integer` (0 Windows, 1 macOS, 2 Linux).
- `System.AllocationCount` reports lifetime VM object allocations and reading
  it allocates nothing. It comes from the Objo checkout at commit `485c4fab`
  (issue #1299) and is not yet in a released Studio; see the Minimum Objo
  Version section of `docs/PORTING.md`.

Forge2D, JBox2D, and the older Xojo Physics project are secondary references
only. Do not copy implementation code from them into Physics2D. A single pinned
MIT upstream keeps the port easier to audit and update.

## Non-Negotiable Product Decisions

- The public root is exactly one module named `Physics2D`.
- Public names have no `b2` or `Box2D` prefix.
- Use Objo's built-in `Vector2` type. There is no `Vector2D` type in the current
  standard library and this project must not introduce a competing vector.
- Use Objo's built-in `Matrix` where a public or cold-path 2x2 matrix is useful.
- Public objects such as `Body`, `Shape`, and `Joint` are friendly façade
  objects. Authoritative hot simulation state lives in private indexed stores.
- The initial authoritative algorithm version is Box2D 3.1.1. Do not silently
  mix in algorithms from another Box2D version.
- Physics uses metres, kilograms, seconds, and radians.
- Physics2D does not perform a hidden Y-axis inversion. Examples for Objo's
  screen coordinates use positive Y gravity; Y-up applications may use negative
  Y gravity.
- `World()` has zero gravity. Gravity is explicit in normal examples.
- `World.Step(timeStep)` defaults to four substeps. An overload accepts an
  explicit positive substep count.
- The first complete release includes the Box2D 3.1.1 rigid-body feature set,
  including continuous collision, sleeping, sensors, queries, events, and all
  seven Box2D 3.1.1 joint families.
- Particles, native SIMD, Box2D's task callback API, custom C allocators, and C
  integration hooks are not part of the native Objo module.
- A desktop demo application and a command-line smoke application are required
  deliverables, not optional polish.

## Repository Shape

The target repository layout is described in `IMPLEMENTATION_PLAN.md`. Keep the
authoring form as a VCS-friendly Objo Studio `.objosln` solution with the
`Physics2D` module in Shared Code and tests, benchmarks, and demo applications
as separate projects.

Keep these generated and authored forms distinct:

- The `.objosln` Shared Code tree is the canonical editable source.
- `dist/Physics2D.objobasic` is a generated, self-contained, single-module
  distribution for users to add to other Studio projects.
- Never edit the generated distribution file by hand.
- Do not invent or hand-author an `.objopackage` format. Add that distribution
  only after the minimum supported Objo Studio version ships source packages.

The generated module and all example solutions must compile without any source
outside the repository.

## Objo Source Style

Follow the canonical Objo style guide. In particular, Objo source files contain
no leading whitespace; Studio supplies visual indentation. Do not indent stored
`.objobasic` source with spaces or tabs.

- Types, methods, properties, and enum members use `PascalCase`.
- Locals and parameters use `camelCase`.
- Constants use `UPPER_SNAKE_CASE`.
- Use full words in the public API: `BodyDefinition`, not `BodyDef`.
- Use one blank line between declarations.
- Use `#` followed by one space for comments.
- Prefer properties for state and methods for actions.
- Use overloads for common construction paths because Objo does not have named
  arguments.
- Keep public collection and ownership behaviour explicit.
- Avoid exposing an internal integer index, generation, pool, graph colour, or
  solver-set concept unless it is independently useful to an application.

## Documentation Standard

This repository teaches users both physics programming and good Objo. Every
public type and public member must document:

- what it represents or does;
- units and coordinate conventions;
- valid ranges and exceptional cases;
- ownership and lifetime rules;
- whether a returned `Vector2`, array, or event record is a copy, a reusable
  view, or caller-owned output;
- whether the operation allocates when that matters in a frame loop; and
- whether the world may be mutated while the operation or event is active.

Internal comments should explain algorithms, invariants, data layout, and
non-obvious performance choices. Do not narrate obvious assignments. Retain a
nearby reference to the corresponding upstream file/function for substantial
ported algorithms, but use Physics2D terminology in prose.

Each user-facing feature needs at least one compact example. The demo must use
the same recommended API patterns as the documentation; do not let examples
become a showcase for private shortcuts.

## Performance Rules

Performance claims require a Release-mode benchmark on a recorded machine and
Objo version. Preserve raw results under `benchmarks/results/`; do not report a
single best run.

Hot-path rules:

- After capacity warm-up, `World.Step` must allocate no objects in the normal
  path. Allocations explicitly requested by public event snapshots or allocating
  query overloads are exceptions and must be documented.
- Pre-size and reuse arrays and scratch buffers.
- Use dense indexed storage and generation-checked handles.
- Prefer swap removal or free lists over shifting array contents.
- Do not use `Array.RemoveAt`, general-purpose dictionaries, closures,
  iterators, string construction, reflection, or polymorphic dispatch inside
  solver, broad-phase, or narrow-phase loops. The only user callbacks allowed
  during `World.Step` are the explicitly configured advanced custom-filter,
  pre-solve, friction-mixing, and restitution-mixing hooks required by the
  Box2D feature set. Their disabled default path must have no callback overhead.
- Avoid value-style `Vector2` and `Matrix` operators in hot loops when they
  create temporaries. Prefer mutating or caller-output operations, or scalar
  components when benchmarks prove them faster.
- Do not assume a structure-of-arrays layout is fastest in Objo. Preserve the
  winning representation from the Stage 2 bake-off until a new benchmark
  justifies changing it.
- Do not use `MemoryBlock` merely because it resembles C memory. It needs to win
  representative benchmarks by a meaningful margin and remain explainable.
- Do not optimise away assertions in tests. Runtime validation at public
  boundaries and internal debug validation are separate concerns.
- Keep deterministic object creation order and stable traversal order wherever
  the algorithm permits it.

Before accepting a hot-path refactor, run the focused correctness tests and the
affected microbenchmark. Before completing a stage, run the stage's full
correctness and benchmark gates.

## Standard-Library Changes

The user has authorised scoped improvements to Objo's general-purpose maths
standard library when Physics2D demonstrates a real need. Such changes are not
shortcuts for moving the physics engine into native C#.

A proposed standard-library addition must satisfy all of these conditions:

1. It is broadly useful outside physics.
2. Its semantics belong naturally on an existing general maths type, or justify
   a genuinely general new type.
3. Physics2D uses it in more than an incidental cold path, or it substantially
   improves clarity for all Objo users.
4. A benchmark or API analysis demonstrates the benefit.
5. Runtime registration, compile-time metadata, tests, user documentation, and
   compatibility/versioning are completed together in the Objo repositories.

Expected audit candidates include allocation-free perpendicular/cross outputs
on `Vector2`, and inverse/linear-solve operations on `Matrix`. They are
candidates, not pre-approved implementations. Physics-specific rotation pairs,
sweeps, manifolds, transforms, and broad-phase bounds normally remain protected
inside `Physics2D`.

When Physics2D begins using a newly added standard-library member, update the
minimum supported Objo API version in the compatibility documentation and test
that an older version fails with an actionable message or documented compiler
requirement.

## Testing Rules

Every algorithmic slice must include tests in the same change. Use several
levels of evidence:

- focused unit tests for maths, containers, geometry, and lifecycle rules;
- golden results generated from the pinned Box2D source;
- invariant tests for trees, pools, graphs, contacts, islands, and mass data;
- deterministic seeded stress tests;
- end-to-end scenes with stable transform/contact checksums;
- compile tests for the public API and generated distribution module;
- lifecycle tests for stale handles, slot reuse, destruction, and world clear;
- benchmark regression scenarios.

Golden fixture generation may use a separately built Box2D reference tool, but
the committed fixtures, normal build, tests, module, and demo must not depend on
that tool or on a native Box2D binary.

Use tolerances derived from Objo's `Double` arithmetic. Do not blindly copy
float tolerances from C and do not weaken a tolerance just to make a failure go
away. Record the reason for every unusually loose tolerance.

A stage is incomplete while tests are skipped, flaky, order-dependent, or only
pass in isolation.

## Autonomous Stage Workflow

Work through `IMPLEMENTATION_PLAN.md` in order. For each stage:

1. Read the stage, its prerequisites, and its exit criteria completely.
2. Inspect current repository status and the progress ledger.
3. Complete the smallest coherent vertical slice, including tests and docs.
4. Run focused validation immediately.
5. Run the full stage gate in Release mode.
6. Record benchmark data and design decisions where required.
7. Update the progress ledger only after every exit criterion passes.
8. Inspect the diff for generated-file drift, undocumented API, stale TODOs,
   copied C idioms, and unrelated changes.
9. Continue to the next stage without waiting for confirmation when the plan
   already specifies the decision.

Stop and request direction only when proceeding would require changing the
pinned upstream version, adding a runtime dependency, removing an agreed v1
feature, making a breaking public API change after the API freeze, or making a
standard-library change whose general semantics cannot be resolved from the
existing Objo conventions. A failed test or difficult optimisation is not by
itself a reason to stop; diagnose it and continue.

Do not mark a stage complete because most code exists. Its exit criteria are
the definition of complete. Do not leave placeholder methods, commented-out
algorithms, silent fallbacks, or untracked exclusions for later stages.

## Provenance And Licensing

The repository is MIT licensed. Preserve the Box2D MIT copyright and licence
notice in `THIRD_PARTY_NOTICES.md`, identify the exact upstream tag and commit
in `docs/PORTING.md`, and keep a subsystem/function mapping there.

Do not paste code from Forge2D, JBox2D, or the Xojo port. If a future task
requires copying rather than merely consulting one of those projects, stop and
add the appropriate BSD notice and provenance record before doing so.

Do not include upstream build products, vendored package caches, or a C Box2D
binary in release artifacts.

## Destructive And Release Actions

Preserve unrelated user changes. Never reset, clean, overwrite, or delete a
broad path to recover a working tree. Inspect targets before deleting generated
artifacts.

Building release artifacts locally is part of the plan. Publishing a release,
pushing commits, changing public registry state, or modifying an external
project beyond the authorised standard-library work requires explicit user
direction.
