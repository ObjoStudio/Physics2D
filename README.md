# Physics2D

Physics2D is a native Objo implementation of the Box2D 3.1.1 rigid-body
physics engine, delivered as one dependency-free module that you add to an
Objo Studio project and import with:

```objo
Import Physics2D
```

There is no C library, no FFI, no managed assembly, and no web service behind
it: the engine is Objo source code, compiled and run by Objo's own toolchain.
It preserves Box2D 3.1.1's algorithms and behaviour (upstream tag `v3.1.1`,
commit `8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3`) behind an idiomatic Objo
API that uses Objo's built-in `Vector2` and `Matrix` maths types.

## Capabilities

- **Bodies and shapes.** Static, kinematic, and dynamic bodies carrying
  circles, capsules, segments, convex polygons, and one-sided chains, with
  density, mass properties, friction, restitution, rolling resistance,
  tangent speed, and category/mask/group collision filtering.
- **Simulation.** Speculative contacts, warm starting, sleeping islands, the
  constraint graph, and Box2D 3.1.1's Soft Step solver with a deterministic
  scalar implementation of all four substeps.
- **Continuous collision.** Bullet bodies, speculative contacts, and
  time-of-impact advancement for fast movers.
- **Joints.** All seven solver families — distance, mouse, motor, revolute,
  prismatic, weld, and wheel — plus the filter joint, each with limits,
  motors, springs, and runtime retuning.
- **Sensors and events.** Sensor begin/end tracking, plus batched contact,
  hit, sensor, and body-move event views read after the step.
- **Queries.** Bounds and shape overlap, closest and all-hit ray casts, shape
  casts, capsule movers (`CastMover`/`CollideMover`), radial explosions, and
  reusable `Into` overloads for allocation-free frame loops.
- **Debug drawing.** A project-type-neutral `DebugRenderer` interface with
  shape, joint, bounds, contact, graph-colour, mass, name, and island layers.
- **Determinism.** Same construction order, same inputs, same results — the
  test suite pins scenario checksums bit for bit.

## One-screen example

A dynamic box falling onto static ground, stepped by a fixed 60 Hz
accumulator loop (see [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) for
the full walkthrough):

```objo
Import Physics2D

Class App Inherits CommandLineApplication

Event Run(args() As String) As Integer
Pragma Unused args

Var world As World = World.WithGravity(New Vector2(0.0, -10.0))

Var groundDef As New BodyDefinition()
groundDef.Type = BodyType.StaticBody
Var ground As Body = world.CreateBody(groundDef)
ground.CreateSegment(New ShapeDefinition(), New Segment(-10.0, 0.0, 10.0, 0.0))

Var boxDef As New BodyDefinition()
boxDef.Type = BodyType.DynamicBody
boxDef.Position = New Vector2(0.0, 8.0)
Var box As Body = world.CreateBody(boxDef)
box.CreatePolygon(New ShapeDefinition(), Polygon.MakeBox(0.5, 0.5))

For i As Integer = 1 To 120
world.StepWorld(1.0 / 60.0)
Next

Print("Box settled at y = " + box.GetPosition().Y.ToString())
Return 0
End Event

End Class
```

Units are metres, kilograms, seconds, and radians. There is no hidden
Y-axis inversion: Objo's screen coordinates grow downward, so screen demos
use positive-Y gravity (`New Vector2(0.0, 10.0)`) and Y-up applications use
negative-Y gravity as above.

## Installation

1. Copy `dist/Physics2D.objobasic` — one generated, self-contained
   `Module Physics2D ... End Module` source — into your project as a Shared
   Code module source item (in Objo Studio: add the file to the solution's
   Shared Code and keep its `Kind` as Module).
2. Add `Import Physics2D` to the sources that use it.
3. Build and run. That is the whole install: the module has no other
   component to download.

`dist/Physics2D.objobasic` is generated output — never edit it by hand. The
canonical, navigable source lives in `Shared/Sources` inside this repository.

**Compatibility:** Objo Studio 26.8.6 or newer, including Objo issue #1302's
standard-library additions (`Vector2.LeftPerpendicular`/`RightPerpendicular`,
`Matrix.Inverse`/`InvertSelf`/`Solve`, `Double.IsFinite`) and the issue #1315
constructor-inheritance fix. The module reports its version as
`Physics2D.VERSION` (SemVer; currently `1.0.0-rc.1`), and the generated file's
header repeats the compatibility record. See
[docs/PORTING.md](docs/PORTING.md) for the full version table.

## Documentation map

| Document | Contents |
|---|---|
| [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) | Install the module, first world, fixed-step loop, bodies and shapes, events, destruction, common mistakes |
| [docs/API.md](docs/API.md) | Generated reference for every public type and member, with units, defaults, ownership, and allocation notes |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Data layout, identity rules, broad phase, solver stages, CCD, and events |
| [docs/PERFORMANCE.md](docs/PERFORMANCE.md) | Benchmark method and results, zero-allocation patterns, capacity planning, profiling guidance |
| [docs/DEMO.md](docs/DEMO.md) | The interactive desktop demo: scenes, controls, and the teaching code behind them |
| [docs/PORTING.md](docs/PORTING.md) | Upstream provenance, symbol-by-symbol Box2D mapping, and deliberate differences |
| [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) | Tests, benchmarks, source style, generated artifacts, provenance rules |
| [docs/decisions/](docs/decisions/) | Numbered design decisions, including the frozen version 1 API |
| [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) | The staged porting plan, progress ledger, and exit criteria |
| [AGENTS.md](AGENTS.md) | Repository rules for automated and human contributors |

## Demo and smoke applications

- **Physics2D.Demo** — the interactive desktop teaching demo: ten scenes
  (playground, pyramid, materials, sensors/filters, all joints, CCD,
  queries, chains, character mover, benchmark pyramid), body dragging,
  spawning, debug-draw layer toggles, and a `--soak` automated gate. Build
  with `objo build Physics2D.objosln --project Physics2D.Demo --output
  build/demo`; [docs/DEMO.md](docs/DEMO.md) is the guided tour.
- **Physics2D.Smoke** — a command-line consumer that exercises the joint
  families end to end, proving the module compiles and runs without any
  desktop dependency.
- **Physics2D.Tests** and **Physics2D.Benchmarks** — the correctness suite
  (including Box2D-derived golden fixtures and zero-allocation gates) and the
  Release-mode benchmark runner.

## Performance statement

Physics2D targets the fastest practical implementation in native Objo. After
a short warm-up, `World.StepWorld` allocates nothing on the normal path
(enforced by tests), dense indexed stores and generation-checked handles keep
the solver cache-friendly, and every optimisation decision is backed by a
committed Release-mode benchmark with a deterministic checksum. As one
reference point on an Apple Silicon MacBook Pro: a 40-level pyramid (820
bodies) steps in tens of milliseconds per frame with the default four
substeps. The Objo VM is substantially slower than compiled C — absolute
timings, the full results table, the drift band, and reproducible commands
live in [docs/PERFORMANCE.md](docs/PERFORMANCE.md). Never trade correctness
for a benchmark result.

## Licence

Physics2D is MIT licensed ([LICENSE](LICENSE)). It is a port of Box2D,
which is also MIT licensed; the upstream notice and the pinned commit are
preserved in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and
[docs/PORTING.md](docs/PORTING.md). The distribution is generated from
[Box2D](https://github.com/erincatto/box2d) tag `v3.1.1` algorithms; no code
from other physics ports (Forge2D, JBox2D, Xojo Physics) is included.
