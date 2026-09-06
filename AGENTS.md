# AGENTS.md

## Purpose

This repository contains `Physics2D`, a finished native Objo implementation
of the Box2D 3.1.1 rigid-body physics engine. It is an educational project:
the source, demo, and documentation should stay teaching-quality. Users open
the Objo Studio solution and consume the module with:

```objo
Import Physics2D
```

The shipped `Physics2D` module has no external runtime dependencies. It must
not require a C library, FFI, Dart, Java, Xojo, a managed assembly, a web
service, or generated native code. Objo's core standard library is part of
the language and is allowed. The desktop demo may use Objo's desktop
standard library, but the physics module itself remains project-type
neutral.

The product priorities, in order:

1. Correct and robust physics behaviour.
2. The fastest practical implementation in native Objo.
3. An idiomatic, stable public Objo API rather than a transliterated C API.
4. Teaching-quality source code, examples, and documentation.

Never trade correctness for speed. Never make the public API awkward merely
to resemble upstream Box2D. Optimised internal code is welcome, but its
representation choices and invariants must be explained.

## Sources Of Truth

1. Box2D tag `v3.1.1`, commit
   `8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3`, is the authority for physics
   algorithms and behavioural intent.
2. The current Objo checkout and language specification are the authority
   for what Objo actually supports.
3. The official Objo documentation (the `objo-docs` repository, normally at
   `/Users/garry/Repos/objo-docs`, published at `https://docs.objo.dev`) is
   the authority for the public language, standard-library, and Studio API.

The relevant local Objo sources normally live at `/Users/garry/Repos/Objo`
and `/Users/garry/Repos/objo-docs`. Read and obey the `AGENTS.md` files in
those repositories before changing them. Do not assume those paths exist on
every machine; locate the checkouts when necessary.

The version 1 public API is frozen by decision `docs/decisions/0005`. Any
breaking public API change requires a new decision record and explicit user
approval.

## Repository Shape

```
Physics2D.objosln                 Solution: module + tests + demo
Shared/Sources/                   Canonical Physics2D module source (edit here)
Projects/Physics2D.Tests/         Test project (the solution's active project)
Projects/Physics2D.Demo/          Desktop demo application
testdata/golden/                  Frozen golden fixtures (see its MANIFEST.md)
docs/                             GETTING_STARTED, API, ARCHITECTURE, DEMO, decisions/
```

The golden fixtures in `testdata/golden` are frozen test data generated
  from the pinned upstream during the port. Never hand-edit them.

Every source item is a `.objobasic` file plus a `.source.json` sidecar
carrying its GUID, kind, build scope, and (for module children)
`ParentModuleId`. Give each new source item a fresh generated UUID; never
reuse or hand-pick GUIDs. The test host's working directory is not the
repository root; fixture paths are resolved by `PhysicsAssert.RepoPath`
(`PHYSICS2D_REPO_ROOT` environment variable, then walking up from the CWD,
then `~/Repos/Physics2D`).

## Development Commands

Run the Objo tooling from the in-development Objo checkout, not from an
installed release: the checkout is the source of truth for the engine, and a
published `objo` on PATH may be missing or stale. Resolve the CLI in this
order: the `OBJO` environment variable if set; otherwise
`dotnet run --project /Users/garry/Repos/Objo/src/studio/Objo.Cli -- ...`
(which always rebuilds current engine code); otherwise an installed `objo`.

`dotnet run` rebuilds on every invocation. For repeated commands, build the
CLI once and run the standalone binary it emits, remembering to rebuild it
after any engine change:

```bash
dotnet build /Users/garry/Repos/Objo/src/studio/Objo.Cli
/Users/garry/Repos/Objo/src/studio/Objo.Cli/bin/Debug/net10.0/objo --version
```

From this repository's root:

```bash
OBJO="dotnet run --project /Users/garry/Repos/Objo/src/studio/Objo.Cli --"  # or export a working objo

$OBJO check  Physics2D.objosln                                        # checks the active project (Tests)
$OBJO test   Physics2D.objosln [--filter <pattern>]                   # run the test suite
$OBJO build  Physics2D.objosln --project Physics2D.Demo --output build/demo
```

`check` only checks the solution's active project (`Physics2D.Tests`). It
does not cover Demo app sources; rely on `test` for the suite and `build`
for the Demo.

There is no `run` command: build, then execute the binary under
`build/<output>/macOS-Apple-Silicon/`.

After changing engine code in the Objo checkout, also rebuild the test host
or `objo test` keeps using a stale binary and reports missing
standard-library members such as `System.AllocationCount`. Rebuild BOTH
configurations: the test host locator tries `bin/Debug` before `bin/Release`
(it walks the CLI's own configuration name first), so a fresh Release build
does not rescue a stale Debug build — a stale Debug binary silently wins:

```bash
dotnet build /Users/garry/Repos/Objo/src/studio/Objo.TestHost -c Release
dotnet build /Users/garry/Repos/Objo/src/studio/Objo.TestHost -c Debug
```

If a member that definitely exists in the Objo checkout still reports
"Undefined property" at runtime, suspect a stale test host or CLI binary;
rebuild `Objo.TestHost` (both configurations) and `Objo.Cli`.

## Objo Language Notes

Practical rules learned while building this project. The language
specification in the Objo checkout remains authoritative.

- Loops end with `Next` (optionally `Next i`), not `End For`.
- Lexing is fully case-insensitive, including identifiers: a variable named
  `iF` lexes as the `If` keyword and produces confusing parse cascades. Avoid
  any name that collides with a keyword when case is ignored (`iF`, `var`,
  `set`, `step`, `in`, `mod`, ...). `Set` is reserved for property setters, so
  a method cannot be named `Set` (see `CosSin.SetPair`).
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
- There is no fixed-size array declaration (`Var a(n) As T` is a parse error).
  Build arrays with `New Array(Of T)` and `Append`, or reserve capacity by
  appending filler values in a warm-up loop.
- Module children (Shared Code classes) require `Import Physics2D` in the
  consuming source.
- The backslash is a string escape character (`"\\"` is one backslash). Use
  `Chr(34)` for JSON quotes.
- `WriteAllText` emits a UTF-8 BOM; machine consumers of generated JSON should
  read UTF-8 with BOM tolerance.
- `FileSystemItem.Child()` rejects path separators; walk segments with
  `ResolveChild`-style helpers.
- `System.Platform` returns an `Integer` (0 Windows, 1 macOS, 2 Linux).
- `System.AllocationCount` reports lifetime VM object allocations and reading
  it allocates nothing.
- `Exit` (optionally `Exit While`, `Exit For`, `Exit Do`) exits the innermost
  loop; use a boolean flag when the intent is to leave nested loops. `Break`
  does NOT exit loops: Objo commit `de96e932` ("Implement Break keyword as
  programmatic breakpoint") made `Break` pause execution whenever a debugger
  is attached and a no-op otherwise. Never use `Break` for loop control in
  module, demo, or test sources.
- A property and a method cannot share a name in one class. The reuse pattern
  is a private-name property plus a public accessor, as in `Manifold`'s
  `mScratch` property and `Scratch()` method.
- A local variable may not shadow a type name while calling that type's
  shared members: `Var sweep As New Sweep` makes `Sweep.GetTransformTo(...)`
  a compile error. Name the local after its role (`motion`, `input`).
- `System.Print` does not exist. Tests surface diagnostic values with
  `Assert.Fail("...")`.
- `While` loops end with `Wend`, not `End While`. `While True` triggers an
  "always true" warning; use a boolean flag instead.
- `Static` is a keyword; name body-type members `StaticBody`/`KinematicBody`/
  `DynamicBody` rather than `Static`/`Kinematic`/`Dynamic`.
- Arrays remove by value with `Remove(item)` and by index with `RemoveAt(i)`;
  `Remove(slot)` with an index silently fails to compile against a list of
  the wrong element type.
- `IntegerList.SetAt(i, v)` requires `i` to be inside the current `Count`; it
  does not grow the list. Grow with `Clear` plus `Append`, reserving capacity
  first in zero-alloc loops.
- `String` does not support `<`/`>` comparison. Encode sort keys as Integers
  (for example `a * 1000 + b`) instead of sorting strings.
- Enumerated values convert to `Integer` with `Integer(BodyType.Dynamic)`,
  but an `Integer` does not convert back to the enum; pass enum values
  directly to enum-typed parameters.
- `AABB` has no `Set` method; write `box.LowerBound.Set(x, y)` and
  `box.UpperBound.Set(x, y)` separately.
- `AABB`, `Vector2`, and result objects are heap classes: every `New`
  allocates. Hot callbacks and loops must reuse receiver objects
  (`GetAABBTo(proxyId, field)`), pre-build inputs outside the loop, and
  prefer scalar slab math over constructing intermediate geometry.
- Classes owned by a module follow ordinary inheritance and may call
  `Super.Constructor()` (Objo issue #1315).
- A Window (or other native-desktop) subclass constructor MUST call
  `Super.Constructor()` before configuring controls. Without it the native
  backing object never registers: the window shows as an empty shell,
  `Opening`/`Opened` never fire, and timers wired in `Opened` never tick —
  with no error at build or run time.
- Event handlers run with `Me` bound to the event source (the canvas or
  timer that raised the event) and `Self` bound to the instance that owns
  the handler method. Use `Self.field` inside handlers to reach the
  owning object's state.
- Declared object properties default to a live empty instance, not
  `Nothing` (`Property mQuad As Array(Of Vector2)` starts as an empty
  array), so `x = Nothing` lazy-initialisation never fires. Construct
  state explicitly in the constructor.
- The virtual machine dispatches same-arity constructors by arity alone: a
  second constructor with the same arity is silently unreachable. Expose the
  alternatives as shared factory functions (`World.WithGravity`) instead.
- A parameter name may not collide case-insensitively with a property of the
  same class (`gravity` vs `Gravity` is an error). Rename the parameter.
- Array literals are supported and are the way to build small fixed data:
  `Var points() As Vector2 = [New Vector2(0.0, 1.0)]`.
- `ElseIf` is one keyword; `Else If` does not parse. `Continue For` is
  supported. `Double.Infinity` is a shared property.
- A test sidecar must carry the full field set (`Version`, `Name`, `Id`,
  `Kind`, `BuildScope`, `Namespace`, `Folder`, `ParentModule`,
  `ParentModuleId`, `AvailableInDesigner`, `CodeFile`, `LayoutFile`, `Notes`,
  `InspectorBehaviour`); a minimal sidecar makes the tests compile but
  silently drops them from discovery.
- Project-source sidecars (`Projects/*/Sources`) carry an empty
  `ParentModule`; only `Shared/Sources` items name `Physics2D` as their
  parent.

Forge2D, JBox2D, and the older Xojo Physics project are secondary references
only. Do not copy implementation code from them into Physics2D. A single
pinned MIT upstream keeps the port auditable.

## Non-Negotiable Product Decisions

- The public root is exactly one module named `Physics2D`.
- Public names have no `b2` or `Box2D` prefix.
- Use Objo's built-in `Vector2` type; do not introduce a competing vector.
- Use Objo's built-in `Matrix` where a public or cold-path 2x2 matrix is
  useful.
- Public objects such as `Body`, `Shape`, and `Joint` are friendly façade
  objects. Authoritative hot simulation state lives in private indexed
  stores.
- The authoritative algorithm version is Box2D 3.1.1. Do not silently mix in
  algorithms from another Box2D version.
- Physics uses metres, kilograms, seconds, and radians.
- Physics2D does not perform a hidden Y-axis inversion. Examples for Objo's
  screen coordinates use positive Y gravity; Y-up applications may use
  negative Y gravity.
- `World()` has zero gravity. Gravity is explicit in normal examples.
- `World.Step(timeStep)` defaults to four substeps. An overload accepts an
  explicit positive substep count.
- Particles, native SIMD, Box2D's task callback API, custom C allocators,
  and C integration hooks are not part of the native Objo module.
- The desktop demo application is a required deliverable, not optional
  polish.

## Objo Source Style

Follow the canonical Objo style guide. In particular, Objo source files
contain no leading whitespace; Studio supplies visual indentation. Do not
indent stored `.objobasic` source with spaces or tabs.

- Types, methods, properties, and enum members use `PascalCase`.
- Locals and parameters use `camelCase`.
- Constants use `UPPER_SNAKE_CASE`.
- Use full words in the public API: `BodyDefinition`, not `BodyDef`.
- Use one blank line between declarations.
- Use `#` followed by one space for comments.
- Prefer properties for state and methods for actions.
- Use overloads for common construction paths because Objo does not have
  named arguments.
- Keep public collection and ownership behaviour explicit.
- Avoid exposing an internal integer index, generation, pool, graph colour,
  or solver-set concept unless it is independently useful to an application.

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
non-obvious performance choices. Do not narrate obvious assignments. Where a
ported algorithm has a non-obvious upstream origin, mention the upstream
function by name, but use Physics2D terminology in prose.

Each user-facing feature needs at least one compact example. The demo must
use the same recommended API patterns as the documentation; do not let
examples become a showcase for private shortcuts. Keep `docs/API.md` in
step with the module source.

## Performance And Hot-Path Rules

After capacity warm-up, `World.Step` allocates no objects in the normal
path; allocations explicitly requested by public event snapshots or
allocating query overloads are documented exceptions. The zero-allocation
gates in the test suite enforce this.

- Pre-size and reuse arrays and scratch buffers.
- Use dense indexed storage and generation-checked handles.
- Prefer swap removal or free lists over shifting array contents.
- Do not use `Array.RemoveAt`, general-purpose dictionaries, closures,
  iterators, string construction, reflection, or polymorphic dispatch inside
  solver, broad-phase, or narrow-phase loops. The only user callbacks allowed
  during `World.Step` are the explicitly configured advanced custom-filter,
  pre-solve, friction-mixing, and restitution-mixing hooks; their disabled
  default path has no callback overhead.
- Avoid value-style `Vector2` and `Matrix` operators in hot loops when they
  create temporaries. Prefer mutating or caller-output operations, or scalar
  components.
- Do not use `MemoryBlock` merely because it resembles C memory.
- Keep deterministic object creation order and stable traversal order
  wherever the algorithm permits it.

Before accepting a hot-path refactor, run the focused correctness tests and
keep the representation decisions recorded in `docs/ARCHITECTURE.md` and
`docs/decisions/` unless a new measurement justifies changing them.

## Testing Rules

Every algorithmic change must include tests in the same change. Use several
levels of evidence:

- focused unit tests for maths, containers, geometry, and lifecycle rules;
- golden results in `testdata/golden` generated from the pinned Box2D source;
- invariant tests for trees, pools, graphs, contacts, islands, and mass data;
- deterministic seeded stress tests;
- end-to-end scenes with stable transform/contact checksums;
- compile tests for the public API;
- lifecycle tests for stale handles, slot reuse, destruction, and world
  clear.

Use tolerances derived from Objo's `Double` arithmetic. Do not blindly copy
float tolerances from C and do not weaken a tolerance just to make a failure
go away. Record the reason for every unusually loose tolerance.

A change is incomplete while tests are skipped, flaky, order-dependent, or
only pass in isolation. Run `objo test` (and the Demo build after Demo
changes) before finishing.

## Destructive Actions

Preserve unrelated user changes. Never reset, clean, overwrite, or delete a
broad path to recover a working tree. Inspect targets before deleting
generated artifacts. Publishing a release or pushing commits requires
explicit user direction.
