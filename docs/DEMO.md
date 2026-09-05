# The Physics2D Demo

The desktop demo (`Projects/Physics2D.Demo`) is the interactive companion
to the user documentation: every scene teaches one part of the engine using
only documented public APIs, and every control maps to a documented world
operation. Build it from the repository root:

```bash
objo build Physics2D.objosln --project Physics2D.Demo --output build/demo
open "build/demo/macOS-Apple-Silicon/Physics2D Demo.app"
```

Run `Physics2D_Demo --soak [minutes]` for the automated soak gate: the demo
cycles every scene automatically, prints nothing, and exits when the gate
completes. The default is 30 minutes of simulated physics time (108,000
fixed steps), counted per executed step so the gate terminates regardless
of timer throttling. While soaking the loop runs flat out, executing a
full catch-up budget of steps per tick, so the gate completes in a few
wall minutes instead of thirty. Scene logic advances on simulated time
while soaking, so scene timers — the sensor drops, the CCD bullets, the
conveyor — fire within each scene window exactly as they do
interactively. Any runtime error is logged with its stack to
`/tmp/physics2d-demo-errors.log` and makes the gate exit non-zero. A
freeze watchdog appends scene switches, a heartbeat every 120 ticks, and
any step or paint slower than 0.25 s to `/tmp/demo-trace.log`, so a
stalled demo records its last completed phase. `--start N` opens on
scene N (1–9).

## Where each feature lives

| File | Responsibility |
|---|---|
| `Sources/App-*.objobasic` | Application entry: parses `--soak`, creates and shows the window |
| `Sources/DemoWindow.objobasic` | Window, fixed-timestep loop, input, HUD, scene switching |
| `Sources/DemoScene.objobasic` | Scene base class: Build, Update, PaintExtras, HandleKey, Substeps, MaximumStepsPerTick, EventLines |
| `Sources/WelcomeScene.objobasic` | Playground: one body per shape family |
| `Sources/MaterialsScene.objobasic` | Friction, restitution, rolling resistance, conveyor |
| `Sources/SensorFilterScene.objobasic` | Sensor events, category/mask filters, group index |
| `Sources/JointsScene.objobasic` | All seven joint families in one world |
| `Sources/ContinuousScene.objobasic` | Bullets versus thin walls with the CCD toggle |
| `Sources/QueryScene.objobasic` | Cursor ray cast and overlap region |
| `Sources/ChainsScene.objobasic` | Chain terrain, a loop, and ghost collision |
| `Sources/MoverScene.objobasic` | Character capsule driven by CollideMover planes, Mover.SolvePlanes, and CastMover |
| `Sources/BenchmarkScene.objobasic` | Forty level pyramid, one step per frame |
| `Sources/CanvasDebugRenderer.objobasic` | Desktop adapter: world metres to canvas points |

Scenes build their geometry for the window's gravity preset and never set
`World.Gravity` themselves, so the `G` key re-tars every scene with the
chosen preset. The one exception is the continuous scene, which pins zero
gravity because its bullet corridor is authored for weightless motion.

Every scene draws a scene menu in the canvas's top-right corner listing
each scene's digit key and title, with the active scene marked by a
chevron, so keyboard scene switching is discoverable without the help
line.

## The fixed-timestep loop

Physics advances in fixed 1/60 s steps regardless of frame rate. The
window's timer fires at 120 Hz; each tick measures real elapsed time with
the monotonic `System.Ticks` clock, feeds an accumulator, and runs whole
fixed steps until the accumulator is drained:

```objo
Var now As Double = System.Ticks
Var frame As Double = Maths.Min(now - mLastTicks, 0.25)
mLastTicks = now
mAccumulator = mAccumulator + frame
While mAccumulator >= 1.0 / 60.0
mWorld.StepWorld(1.0 / 60.0)      # Default four substeps
mAccumulator = mAccumulator - 1.0 / 60.0
Wend
```

The 0.25 s clamp bounds a single frame's measured time, and each scene
bounds the burst itself: `MaximumStepsPerTick` defaults to four fixed
steps. When the cap bites, the window drops the leftover accumulator so
a scene heavier than real time runs in slow motion instead of wedging
the interface. Rendering reads the post-step state directly (no
interpolation); the visual lag is under one step and the demo stays
simple. The benchmark scene runs one substep and one fixed step per
frame, so the forty level pyramid stays responsive; measurement happens
in the Benchmarks project, never here.

## Metres to pixels

Physics uses metres. The renderer owns the conversion:
`CanvasDebugRenderer.PixelsPerMetre` (40 in the demo) scales lengths, and
`Origin` is the world position of the canvas top-left corner. The demo puts
the world origin at the horizontal centre, three quarters down:

```objo
Var ppm As Double = 40.0
Var origin As Vector2 = New Vector2(-0.5 * width / ppm, -0.75 * height / ppm)
canvasX = (worldX - origin.X) * ppm
canvasY = (worldY - origin.Y) * ppm
```

There is no Y-axis flip: Objo screen coordinates grow downward and
Physics2D never inverts the axis, so screen-oriented demos use positive-Y
gravity. Y-up applications set negative gravity and flip the origin
themselves.

## Body and shape construction

Every body comes from a definition and every shape from a shape
definition; nothing is mutated after creation except through documented
runtime setters:

```objo
Var def As New BodyDefinition
def.Type = BodyType.DynamicBody
def.Position = New Vector2(0.0, 3.0)
Var body As Body = world.CreateBody(def)

Var shapeDef As New ShapeDefinition
shapeDef.Density = 1.0
shapeDef.Material.Friction = 0.4
body.CreatePolygon(shapeDef, Polygon.MakeBox(0.5, 0.5))
```

Scene construction runs once per scene switch (`DemoWindow.SelectScene`
builds a fresh world first), so scenes never mutate static geometry after
the fact.

## Safe event handling

Sensor scenes read the batched event view after the step and never inside
it:

```objo
Var events As WorldEvents = world.Events
Var i As Integer = 0
While i < events.SensorBeginCount()
Var entered As Shape = events.SensorBeginVisitorShape(i)
If entered <> Nothing And entered.GetBody() <> Nothing Then
# ... react here: read names, counts, positions ...
Pragma Unused entered
End If
i = i + 1
Wend
```

The view is valid until the next step; event shapes can be stale if their
body was destroyed, and the facade chain reports `Nothing` instead of
throwing. The demo keeps a short log array and trims it, so the HUD shows
recent events without growing.

## Destruction and lifetime

Scenes that spawn continuously (sensor drops, CCD bullets) prune objects
that leave the play field:

```objo
If drop.GetPosition().Y < -12.0 Then
drop.Destroy()
End If
```

`Destroy` detaches shapes, contacts, and joints and wakes the island.
Dropped facades report `IsValid() = False`; the demo checks validity before
touching pruned bodies because a body may already be gone.

## Query result reuse

The cursor query reuses one `ShapeHitList` across frames instead of
allocating per frame:

```objo
mCursorHits.Clear()
world.OverlapBoundsInto(bounds, New QueryFilter, mCursorHits)
```

Ray casts allocate one `ShapeHit` per call in the closest form; the query
scene draws it immediately and never retains it.

## Why the demo avoids per-frame allocation

The world's fixed step is zero-alloc after warm-up, and the demo keeps its
own per-frame paths allocation-light so the HUD step time measures physics,
not garbage. Rules the demo follows:

- One `ShapeHitList`, one `DebugDrawOptions`, and one `CanvasDebugRenderer`
  live for the whole session; layers are toggled in place.
- Scene `Update` hooks mutate lists owned by the scene, pruning them in
  place with `RemoveAt`.
- The renderer writes into pre-built vertex storage where the module
  offers it (the debug-draw quad array); polygon vertices for shapes are
  the polygon's own arrays, borrowed for the duration of the draw call.
- Strings for the HUD are built per frame on purpose: they are short, the
  cost is trivial next to rendering, and clarity beats micro-optimising
  five label lines.

## Controls

| Input | Action |
|---|---|
| `1`–`9` | Select scene 1–9 |
| `P` or Space | Pause or resume |
| `S` | Single fixed step while paused |
| `R` | Rebuild the current scene |
| `G` | Cycle gravity: down, zero, up |
| `C` | Toggle continuous collision |
| `W` | Toggle sleeping |
| `F1`–`F8` | Toggle shape, joint, bounds, contact, graph-colour, mass, name, and island layers |
| Left drag | Mouse joint on the body under the cursor |
| Right click | Spawn a box |
| Middle click | Spawn a circle |
| Arrows / Up | Drive the character mover scene |
