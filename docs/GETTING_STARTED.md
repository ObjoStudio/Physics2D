# Getting Started With Physics2D

This walkthrough takes a new Objo Studio project from empty to a running
physics scene: install the module, create a world, drive it with a fixed
timestep, build bodies and shapes, read events, and destroy objects safely.
Every snippet is a complete fragment that compiles against the shipped
module. Units are metres, kilograms, seconds, and radians throughout.

## 1. Install the module

1. Copy `dist/Physics2D.objobasic` into your project as one Shared Code
   module source item. In Objo Studio, add the file to the solution and keep
   its item kind as **Module** — the file already wraps everything in
   `Module Physics2D ... End Module`.
2. In every source that uses physics, add `Import Physics2D` at the top.
3. Build. The module has no other component: no downloads, no native code,
   no configuration.

The module requires Objo Studio 26.8.6 or newer (with Objo issue #1302's
standard-library additions and the issue #1315 constructor-inheritance fix).
You can print the installed version at any time:

```objo
Print("Physics2D " + Physics2D.VERSION)
```

## 2. Create a world

A `World` owns every body, shape, chain, contact, and joint created inside
it. `New World()` starts with zero gravity; gravity is explicit, and the
sign follows your coordinate convention:

```objo
# Screen-style coordinates (Y grows downward, like Objo's canvas):
Var screenWorld As World = World.WithGravity(New Vector2(0.0, 10.0))

# Y-up coordinates (classical physics convention):
Var uprightWorld As World = World.WithGravity(New Vector2(0.0, -10.0))
```

For tuned worlds, build a `WorldSettings` and construct from it. Every field
has a documented default matching upstream Box2D 3.1.1:

```objo
Var settings As New WorldSettings
settings.Gravity = New Vector2(0.0, 10.0)
settings.EnableSleep = True
settings.EnableContinuous = True
Var world As New World(settings)
```

A `World` is single-threaded. Do not call its API from another thread while a
step is running; independent worlds may run on independent Workers at the
application level.

## 3. Drive it with a fixed timestep

Physics advances in fixed-size steps. `StepWorld(1.0 / 60.0)` advances
exactly 1/60 second (the default four substeps divide that step internally;
the two-argument overload accepts an explicit positive substep count). Build
an accumulator so a slow frame runs several whole steps and a fast frame
runs none, and clamp the catch-up so a backgrounded window cannot spiral:

```objo
Var accumulator As Double = 0.0
Var lastTicks As Double = System.Ticks

# Once per frame (a timer tick, a canvas paint, or your game loop):
Var now As Double = System.Ticks
Var frame As Double = Maths.Min(now - lastTicks, 0.25)
lastTicks = now
accumulator = accumulator + frame
While accumulator >= 1.0 / 60.0
world.StepWorld(1.0 / 60.0)
accumulator = accumulator - 1.0 / 60.0
Wend
```

`StepWorld` never reads a wall clock, so the same inputs always produce the
same results — this is what makes scenarios testable and replays exact.

> **Application entry points.** Objo runs the class named `App` (a subclass
> of `CommandLineApplication` for command-line programs, `DesktopApplication`
> for desktop programs) as the program's entry point. A class with another
> name compiles but never runs — this is the silent-exit trap to know about.

## 4. Bodies and shapes

Every body is created from a `BodyDefinition`, and every shape attaches
through a `ShapeDefinition` plus geometry. The definition copies into the
world: later edits to your geometry objects do not leak into the simulation.

```objo
# Static ground: a horizontal segment from (-10, 0) to (10, 0).
Var groundDef As New BodyDefinition()
groundDef.Type = BodyType.StaticBody
Var ground As Body = world.CreateBody(groundDef)
ground.CreateSegment(New ShapeDefinition(), New Segment(-10.0, 0.0, 10.0, 0.0))

# A dynamic box one metre square, dropped from four metres up.
Var boxDef As New BodyDefinition()
boxDef.Type = BodyType.DynamicBody
boxDef.Position = New Vector2(0.0, 4.0)
Var box As Body = world.CreateBody(boxDef)
box.CreatePolygon(New ShapeDefinition(), Polygon.MakeBox(0.5, 0.5))
```

Geometry factories cover the common cases; constructors cover the rest:

```objo
Var shapeDef As New ShapeDefinition
shapeDef.Density = 1.0
shapeDef.Material.Friction = 0.4
shapeDef.Material.Restitution = 0.3

# A box half a metre by a quarter metre, offset and rotated 45 degrees.
Var rot45 As Rot = Rot.Make(0.7853981633974483)
Var slab As Polygon = Polygon.MakeOffsetBox(0.5, 0.25, New Vector2(0.0, 0.25), rot45)
box.CreatePolygon(shapeDef, slab)
```

The shape families are `Circle` (centre + radius), `Capsule` (two endpoints
+ radius), `Segment` (an edge with optional ghost vertices for one-sided
collision), `Polygon` (3–8 vertices, convex, counter-clockwise), and `Chain`
(a connected run of segments for terrain; `ChainDefinition.IsLoop = True`
closes it). Bodies expose `GetPosition()`, `GetAngle()`, `GetLinearVelocity()`,
`SetLinearVelocity(...)`, `ApplyForce(...)`, `ApplyLinearImpulse(...)`,
`SetAwake(...)`, and the full surface in [docs/API.md](API.md).

Transform positions, not velocities, in the step loop; and prefer forces and
impulses over teleporting dynamic bodies.

## 5. Rendering scale

Physics lives in metres; your canvas lives in points or pixels. The adapter
owns the conversion — the module never scales for you, and there is no
hidden Y-axis flip:

```objo
# One-world-metre = 40 canvas points; the world origin sits at the
# horizontal centre, three quarters down the canvas.
Var ppm As Double = 40.0
Var origin As Vector2 = New Vector2(-0.5 * width / ppm, -0.75 * height / ppm)
canvasX = (worldX - origin.X) * ppm
canvasY = (worldY - origin.Y) * ppm
```

Screen-oriented scenes put gravity along +Y (downward). Y-up applications
use negative-Y gravity and flip the origin mapping themselves. For debugging,
`World.DrawDebug(renderer, options)` draws shapes, joints, bounds, contacts,
and more through a project-type-neutral `DebugRenderer` you implement; the
desktop demo's `CanvasDebugRenderer` is a complete example.

## 6. Reading events

Enable events per shape (or per body with the body-level enablers), step,
then read the batched view. The view is reusable and stays valid until the
next step — reading it never allocates:

```objo
Var events As WorldEvents = world.Events

Var i As Integer = 0
While i < events.SensorBeginCount()
Var entered As Shape = events.SensorBeginVisitorShape(i)
If entered <> Nothing And entered.GetBody() <> Nothing Then
# A visitor entered a sensor this step; react here.
Pragma Unused entered
End If
i = i + 1
Wend
```

Contact begin/end records name the two shapes; hit records add the approach
speed and point; body-move records report transforms and whether the body
fell asleep. Never mutate the world from inside `StepWorld` — the world is
locked while it steps, and mutation attempts throw. React after the step, or
queue changes and apply them once the step returns.

## 7. Destruction and lifetime

Destroy objects explicitly; destroying a body destroys its shapes, chains,
contacts, and attached joints. Facade objects survive destruction as
handles: `IsValid()` reports whether they still reference a live object, and
using a stale handle raises a clear runtime error naming the object and
operation.

```objo
# Prune anything that fell far below the play field.
If drop.GetPosition().Y < -12.0 Then
drop.Destroy()
End If
```

`World.Clear()` returns the world to an empty, reusable state and invalidates
every outstanding facade. Slot reuse increments a generation, so a stale
facade can never accidentally address a newly created object.

## 8. Queries in a frame loop

Queries come in two forms: convenient methods that allocate a fresh result,
and `Into` forms that fill caller-owned lists for allocation-free frame
loops. In a per-frame path, reuse one list:

```objo
# mCursorHits is one ShapeHitList reused across frames.
mCursorHits.Clear()
world.OverlapBoundsInto(cursorBounds, New QueryFilter, mCursorHits)
```

The callback forms allocate a fresh hit per visited shape so your callback
may retain point and normal references; they are cold-path conveniences.
Each call also returns a lightweight `TreeStats` traversal record — ignore
it if you do not need it.

## 9. Common mistakes

- **Pixel-scale physics.** A 500-pixel box is a 500-metre box to the solver.
  Work in metres and scale only in the renderer.
- **Variable timesteps.** Passing the raw frame delta to `StepWorld` changes
  behaviour frame to frame. Use the fixed-step accumulator above.
- **Mutating during the step.** World mutation inside `StepWorld` (or inside
  the opt-in pre-solve/filter callbacks) throws. Collect what you need and
  apply changes after the step returns.
- **Holding geometry, not handles.** `Vector2`, manifold, and hit results may
  be reused views that the next call overwrites; copy values you keep.
  Facades (`Body`, `Shape`, `Joint`) are stable handles — check `IsValid()`
  after destruction instead of caching "liveness".
- **Expecting an implicit Y-flip.** Physics2D never inverts axes. Screen
  demos use positive-Y gravity; Y-up apps use negative-Y.
- **Enabling events and never reading them.** Sensor and contact event
  production has a small per-step cost for enabled shapes; leave the
  enablers off for shapes whose events nobody reads.
- **Retuning joints without waking.** Runtime setters follow upstream
  semantics and do not wake sleeping bodies; call `SetAwake(True)` when a
  retarget should move a settled body now.

## Where to next

- [docs/API.md](API.md) — the full generated reference.
- [docs/DEMO.md](DEMO.md) — the interactive demo as a guided tour, with
  a scene for each feature group.
- [docs/PERFORMANCE.md](PERFORMANCE.md) — zero-allocation patterns,
  capacity planning, and profiling guidance.
- [docs/ARCHITECTURE.md](ARCHITECTURE.md) — how the engine works inside.
