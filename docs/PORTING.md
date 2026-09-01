# Porting Notes — Box2D 3.1.1 to Physics2D

This document is the provenance and mapping record for the Physics2D port. It
identifies the authoritative upstream source, records every deliberate
behavioural difference, and maps every public Box2D 3.1.1 symbol to its
Physics2D counterpart.

## Upstream Source

| Field | Value |
|---|---|
| Repository | https://github.com/erincatto/box2d |
| Tag | `v3.1.1` |
| Full commit | `8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3` |
| Retrieved | 2026-08-30 (source tarball for that commit) |
| Licence | MIT (see `THIRD_PARTY_NOTICES.md`) |
| Upstream language | C, 32-bit `float` |

This is the sole authoritative algorithm source. Box2D 3.1.1 computes with C
`float`; Physics2D computes with Objo's `Double`. Physics2D therefore preserves
algorithms, iteration orders, and behavioural intent rather than promising
bit-for-bit equality with C output. Test tolerances are chosen from Objo's
`Double` arithmetic and are documented per fixture.

## Secondary References (Never Copied)

Forge2D (Dart), JBox2D (Java), and the Xojo Physics project are consulted as
comparative references only. Code is never copied from them. If a future task
requires copying rather than consulting, the applicable BSD/Apache notice and a
provenance record must be added to `THIRD_PARTY_NOTICES.md` first.

## Deliberate Differences From Upstream

1. **Language and arithmetic.** Native Objo source, `Double` arithmetic, no C
   compiler, no FFI, and no native SIMD kernels.
2. **Public naming.** No `b2`/`Box2D` prefixes, no C-style abbreviations. The
   façade classes are `World`, `Body`, `Shape`, `Chain`, and the joint
   families. Definitions use full words: `BodyDefinition`, not `BodyDef`.
3. **Ownership.** Box2D IDs become generation-checked façade objects owned by
   their `World`. There are no C pointers, no manual world destruction, and
   `World.Clear()` returns a world to a reusable empty state.
4. **Fixtures are gone.** Box2D 3.1.1 attaches shapes directly to bodies, and
   Physics2D mirrors that: there is no fixture type in the public API.
5. **Units and axes.** Metres, kilograms, seconds, radians. No hidden axis
   inversion; Objo canvas examples use positive-Y gravity.
6. **Threading.** The deterministic scalar solver replaces Box2D's task-based
   SIMD solver. The constraint graph and Soft Step algorithms are preserved;
   the SIMD lane structure is not.
7. **Callbacks.** The custom filter, pre-solve, friction mixer, and restitution
   mixer hooks are explicit opt-ins on `World`; the default step path invokes no
   user code. Contact/sensor/body-move observation happens after the step from
   reusable batch views, plus optional idiomatic post-step Objo events.
8. **Allocation.** The step path pre-allocates and reuses buffers so a warmed
   `World.Step` allocates nothing in the normal path. Allocating conveniences
   are documented where they exist.
9. **Precision-sensitive constants.** Constants such as epsilon thresholds keep
   upstream values; where `float`/`double` differences would change behaviour,
   the change is recorded in a decision document under `docs/decisions/`.
10. **Deterministic hash sentinel.** Upstream `b2AddKey` asserts the computed
   32-bit hash is never zero; Physics2D deterministically maps a zero hash to
   one instead, so release behaviour stays correct without debug assertions.
   Key 0 remains reserved exactly as upstream.
11. **Bit set construction.** Upstream creates an empty `b2BitSet` and calls
    `b2SetBitCountAndClear` before first use; the Physics2D `BitSet`
    constructor makes its reserved range immediately usable, with
    `SetCountAndClear` remaining the step-loop reset path.
12. **Assertions become validation.** Upstream `b2Assert` calls become
    documented `InvalidArgumentException` throws for invalid construction
    input and `RuntimeException` throws for degenerate runtime state.
    `RayCastInput.IsValid`/`ShapeCastInput.IsValid` remain user-side checks;
    the cast entry points assume valid input exactly like upstream release
    builds.
13. **No debug simplex capture.** Upstream `b2ShapeDistance` accepts a caller
    buffer to record every simplex for debugging; Physics2D always passes
    capacity 0, so `DistanceOutput.SimplexCount` stays 0.
14. **Degenerate polygon factories.** Upstream `b2MakePolygon` falls back to
    `b2MakeSquare` when hull construction fails; `Polygon.FromHull` and the
    other polygon factories throw on degenerate input instead and expose
    explicit `MakeSquare`/`MakeBox` factories.
15. **Tree rebuild partitioning.** Physics2D compiles the upstream
    `B2_TREE_HEURISTIC 0` configuration, so `DynamicTree.Rebuild` always
    partitions by the splitting axis median (`PartitionMid`) and the binned
    SAH path of `b2PartitionSAH` is not ported.
16. **Broad-phase pair pool.** Upstream threads candidate pairs through a
    pair array with per-move linked lists consumed by the world's contact
    factory; Physics2D keeps the same per-move LIFO consumption over parallel
    `IntegerList` pools and reports pairs through a `BroadPhasePairSink`.
17. **World constructor surface.** The virtual machine dispatches
    same-arity constructors by arity alone, so the upstream gravity
    constructor becomes the shared factory `World.WithGravity(gravity)`
    alongside `New World()` and `New World(settings)`.
18. **Locked-world mutation errors.** Upstream world mutators silently
    return when the world is locked during stepping; Physics2D throws
    `RuntimeException` from `CheckNotLocked` so misuse is visible.
19. **Solver-set id pool.** Upstream `b2CreateWorld` allocates solver-set ids
    0-2 through the id pool; Physics2D mirrors this so the first per-island
    sleeping set starts at 3.
20. **Step availability.** `World.StepWorld` arrived with the Stage 7
    solver; every other Stage 6 API was already complete.
21. **Serial solver stages.** Upstream fans each solver stage out through a
    task scheduler and SIMD lanes; Physics2D runs the same stage order as
    sequential loops over the graph colours, overflow colour first, with
    `StepContext` and the colour constraint arrays reused across steps.
22. **Per-step validation.** Upstream revalidates solver sets inside every
    step; Physics2D validates through the explicit `World.Validate` API
    (tests call it after steps) so release steps stay lean.
23. **Hook signatures.** The pre-solve hook receives `Shape` façades and the
    live `Manifold` instead of shape IDs; the material mixer receives user
    material IDs alongside each value, matching the upstream callback
    signature, and routes through `World.MixFrictionWithMaterials` /
    `World.MixRestitutionWithMaterials`.
24. **Polled events.** Upstream collects primitive event buffers that user
    code reads through `b2World_Get*Events` structs; Physics2D exposes the
    same validity windows through the reusable `WorldEvents` view over flat
    world-owned columns, with begin/hit/move events clearing at step start
    and end-event buffers double-swapping at step end. There are no
    registered event callbacks; façade accessors resolve generation-checked
    references and return `Nothing` for stale ids.
25. **Body-level collision filter.** Upstream `b2ShouldBodiesCollide` walks
    connected joints to enforce `collideConnected`; the joint walk arrives
    with Stage 9, so Stage 8 implements the dynamic-body type rule only.
26. **Distance overlap reporting.** Upstream zero-initialises
    `b2DistanceOutput` on the stack, so its simplex-overlap early returns
    report zero distance and a zero normal; the Physics2D `Distance`
    module now writes the same zeros explicitly in `WriteOverlapOutput`
    because reused scratch outputs made the stale fields observable.
27. **Scalar joint store.** Upstream keeps one `b2JointSim` object per joint
    per solver set; Physics2D keeps parallel scalar columns in `JointSims`
    (base columns shared by every joint family plus one column block per
    implemented family), matching the Stage 2 representation decision. The
    step's distance-joint scratch lives in one reused `DistanceJointScratch`
    record on `StepContext` instead of a per-sim struct. The mouse joint
    reuses the base `IndexB`, `AnchorB`, `DeltaCenter`, `InvMassB`, and
    `InvIB` columns for its single moving body and stores its symmetric
    2x2 linear mass as three scalars.
28. **Cross-world joints are rejected.** Upstream `b2CreateJoint` asserts
    when the two bodies come from different worlds (and asserts on null
    bodies); Physics2D throws `InvalidArgumentException` from
    `CheckJointBody` so misuse is visible instead of silently released in
    distribution builds.
29. **Solver-set recycling.** Upstream frees solver sets back to the heap
    when islands merge or the world is destroyed; Physics2D keeps destroyed
    sets in a `World` pool and reinitialises them in place, so sleeping-set
    churn allocates nothing after capacity warm-up.
30. **Joint user data.** Upstream stores `void*` user data on joints;
    Physics2D `Joint.UserData` holds any Objo object reference and is
    never read by the solver.
31. **Mouse joint non-awake body B.** Upstream `b2PrepareMouseJoint`
    asserts body B is awake and `b2WarmStartMouseJoint`/`b2SolveMouseJoint`
    index the body-state array through `joint->indexB` unguarded, so a
    mouse joint whose body B is not awake (for example a static body B in
    the awake graph) is undefined behaviour in upstream release builds.
    Physics2D stores `NULL_INDEX` for a non-awake body B and the warm-start
    and solve kernels return without applying impulses; a zero-mass body B
    therefore receives nothing, matching the singular inverse-mass result
    upstream produces for dynamic body A/static body B.
32. **Mouse joint drag ergonomics.** Upstream `b2MouseJoint_SetTarget`
    takes `b2Vec2` by value; Physics2D adds the scalar overload
    `MouseJoint.SetTarget(x, y)` because constructing a `Vector2` argument
    allocates, and a per-frame drag loop must stay allocation-free. The
    `MouseJointDefinition` tuning setters throw `InvalidArgumentException`
    where upstream only asserts. `MouseJoint.GetAnchorB` returns the
    dragged body's world anchor point as a documented Physics2D addition
    for drag rendering.
33. **Motor joint clamping and ergonomics.** Upstream clamps motor joint
    tuning at the setter: `b2MotorJoint_SetMaxForce` and
    `b2MotorJoint_SetMaxTorque` clamp negative inputs to zero through
    `b2MaxFloat`, and `b2MotorJoint_SetCorrectionFactor` and
    `b2CreateMotorJoint` clamp the correction factor into [0, 1] through
    `b2ClampFloat`. Physics2D matches that clamping and additionally throws
    `InvalidArgumentException` for non-finite inputs and for negative
    definition force or torque values, where upstream only asserts.
    `MotorJoint.SetLinearOffset` adds the scalar overload
    `SetLinearOffset(x, y)` for allocation-free per-frame retargeting. The
    linear impulse clamp ports `b2Normalize`'s degenerate branch: below the
    float epsilon the upstream normalization returns zero rather than a
    unit direction, so a clamped impulse collapses to zero instead of
    scaling. Non-awake bodies store a `NULL_INDEX` state row and receive no
    impulses (upstream asserts at least one awake body and otherwise reads
    the state array unguarded), and the solve body never reads the base
    constraint softness, so the motor family stores no softness columns.
34. **Revolute joint validation and plain setters.** Upstream asserts the
    revolute limit range at creation and in `b2RevoluteJoint_SetLimits`
    (lower not above upper, both within [-0.99 pi, 0.99 pi]); Physics2D
    throws `InvalidArgumentException` instead, matching the port's
    validation policy. Upstream clamps the reference and target angles into
    [-pi, pi] at creation; Physics2D matches the clamping and throws on
    non-finite definition input. Runtime tuning setters that upstream
    stores plainly (`SetSpringHertz`, `SetSpringDampingRatio`,
    `SetTargetAngle`, `SetMotorSpeed`, `SetMaxMotorTorque`) stay plain, so
    non-finite values cannot enter through the definition but follow the
    distance-family convention at runtime. Toggling the spring, limits, or
    motor clears the corresponding accumulated impulses exactly like
    upstream, and tuning changes do not wake the bodies. The hinge reuses
    the base `EnableSpring`/`EnableLimit`/`EnableMotor` flag columns and
    stores its symmetric 2x2 point-to-point solve inline because the
    combined matrix changes every iteration with the delta rotations.
35. **Prismatic joint axis and defaults.** The upstream prismatic default
    axis is the world X axis (`b2DefaultPrismaticJointDef`), and
    `b2CreatePrismaticJoint` normalizes `localAxisA` through `b2Normalize`,
    whose degenerate branch leaves a zero axis zero; Physics2D matches
    both. `b2PrismaticJoint_SetLimits` asserts only lower not above upper
    (translations have no pi range) and Physics2D throws instead; the
    remaining validation and plain-setter policy matches difference 34. The
    block solve keeps the upstream fixed-rotation substitution
    (`k22 = 1` when both rotational inertias vanish) and the axial
    effective mass is prepared once per step from the anchor-line torque
    arms.
36. **Weld joint softness fallback.** Upstream's `b2PrepareWeldJoint`
    copies the base constraint softness into the weld's linear or angular
    softness when the corresponding hertz is zero, so a rigid weld
    inherits the solver's constraint tuning; Physics2D matches this by
    copying the prepared base softness columns into the weld softness
    columns at prepare time. The runtime tuning setters throw
    `InvalidArgumentException` where upstream asserts finite non-negative
    input, and the angular and linear bias blocks apply whenever the
    solver asks for bias or the weld is softened, matching the upstream
    `useBias || hertz > 0` condition.
37. **Wheel joint surface additions.** Upstream exposes no wheel
    translation or speed getter; Physics2D adds the documented
    `WheelJoint.GetTranslation` and `WheelJoint.GetSpeed` conveniences
    porting the prismatic anchor-point projections, because a suspension's
    ride travel is the wheel joint's most-queried quantity. Upstream's
    wheel spring drives the slide toward zero translation (the struct has
    no target field) and the motor is a torque-clamped velocity drive on
    the free spin; Physics2D matches both. The axis normalizes at creation
    like the prismatic joint and the default axis is the world Y axis.

## Public Symbol Inventory

Every public declaration in the Box2D 3.1.1 public headers
(`include/box2d/base.h`, `collision.h`, `id.h`, `math_functions.h`, `types.h`,
`box2d.h`) is inventoried below with its intended Physics2D mapping and the
stage that delivers it. Count: 422 unique functions. Structs, enums, and
callback typedefs map to the Physics2D types named by the functions that use
them and are covered in `docs/API.md`.

Mapping conventions:

- `b2Xxx_Yyy` becomes `Xxx.Yyy` on the matching façade class.
- `b2DefaultXxxDef` becomes a `XxxDefinition` class whose constructor sets the
  same defaults.
- Internal-only machinery (`id.h` internals, arena allocators, task system)
  becomes protected implementation inside the module.
- Stage numbers refer to `IMPLEMENTATION_PLAN.md`.

### World and lifecycle (61 symbols)

| Upstream symbol | Physics2D mapping | Stage |
|---|---|---|
| `b2CreateBody` | World.CreateBody(definition) | 6 |
| `b2CreateCapsuleShape` | Body.CreateCapsule(definition) | 6 |
| `b2CreateChain` | World.CreateChain(definition) | 6 |
| `b2CreateCircleShape` | Body.CreateCircle(definition) | 6 |
| `b2CreateDistanceJoint` | World.CreateDistanceJoint(definition) | 9 |
| `b2CreateFilterJoint` | World.CreateFilterJoint(definition) | 9 |
| `b2CreateMotorJoint` | World.CreateMotorJoint(definition) | 9 |
| `b2CreateMouseJoint` | World.CreateMouseJoint(definition) | 9 |
| `b2CreatePolygonShape` | Body.CreatePolygon(definition) | 6 |
| `b2CreatePrismaticJoint` | World.CreatePrismaticJoint(definition) | 9 |
| `b2CreateRevoluteJoint` | World.CreateRevoluteJoint(definition) | 9 |
| `b2CreateSegmentShape` | Body.CreateSegment(definition) | 6 |
| `b2CreateWeldJoint` | World.CreateWeldJoint(definition) | 9 |
| `b2CreateWheelJoint` | World.CreateWheelJoint(definition) | 9 |
| `b2CreateWorld` | New World(settings) / New World() / World.WithGravity(gravity) | 6 |
| `b2DestroyBody` | Body.Destroy() | 6 |
| `b2DestroyChain` | Chain.Destroy() | 6 |
| `b2DestroyJoint` | Joint.Destroy() | 9 |
| `b2DestroyShape` | Shape.Destroy() | 6 |
| `b2DestroyWorld` | Not required: worlds are garbage-collected Objo objects; World.Clear() empties a reusable world | 6 |
| `b2World_CastMover` | World.CastMover | 10 |
| `b2World_CastRay` | World.CastRay and World.CastRayInto | 6 |
| `b2World_CastRayClosest` | World.CastRayClosest | 6 |
| `b2World_CastShape` | World.CastShape and World.CastShapeInto | 6 |
| `b2World_CollideMover` | World.CollideMover into a reusable plane buffer | 10 |
| `b2World_Draw` | World.DrawDebug(renderer, options) | 10 |
| `b2World_DumpMemoryStats` | World.Counters plus documented capacity planning; no text dump | 10 |
| `b2World_EnableContinuous` | World.EnableContinuous | 6 |
| `b2World_EnableSleeping` | World.EnableSleeping | 6 |
| `b2World_EnableSpeculative` | World.EnableSpeculative | 6 |
| `b2World_EnableWarmStarting` | World.EnableWarmStarting | 6 |
| `b2World_Explode` | World.Explode | 10 |
| `b2World_GetAwakeBodyCount` | World.AwakeBodyCount | 6 |
| `b2World_GetBodyEvents` | World.GetBodyEvents batch view plus post-step BodyMoved events | 8 |
| `b2World_GetContactEvents` | World.Events contact begin/end/hit views | 8 |
| `b2World_GetCounters` | World.Counters statistics record | 10 |
| `b2World_GetGravity` | World.Gravity | 6 |
| `b2World_GetHitEventThreshold` | World.HitEventThreshold | 6 |
| `b2World_GetMaximumLinearSpeed` | World.MaximumLinearSpeed | 6 |
| `b2World_GetProfile` | World.Profile statistics record | 10 |
| `b2World_GetRestitutionThreshold` | World.RestitutionThreshold | 6 |
| `b2World_GetSensorEvents` | World.Events sensor begin/end views | 8 |
| `b2World_GetUserData` | World.UserData | 6 |
| `b2World_IsContinuousEnabled` | World.ContinuousEnabled | 6 |
| `b2World_IsSleepingEnabled` | World.SleepingEnabled | 6 |
| `b2World_IsValid` | Protected internal validation; façade objects validate their own state | 6 |
| `b2World_IsWarmStartingEnabled` | World.WarmStartingEnabled | 6 |
| `b2World_OverlapAABB` | World.OverlapBounds and World.OverlapBoundsInto | 6 |
| `b2World_OverlapShape` | World.OverlapShape and World.OverlapShapeInto | 6 |
| `b2World_RebuildStaticTree` | World.RebuildStaticTree | 6 |
| `b2World_SetContactTuning` | World.ContactTuning | 6 |
| `b2World_SetCustomFilterCallback` | World.CustomFilter hook (explicit opt-in) | 7 |
| `b2World_SetFrictionCallback` | World.FrictionMixer hook (explicit opt-in) | 7 |
| `b2World_SetGravity` | World.Gravity | 6 |
| `b2World_SetHitEventThreshold` | World.HitEventThreshold | 6 |
| `b2World_SetMaximumLinearSpeed` | World.MaximumLinearSpeed | 6 |
| `b2World_SetPreSolveCallback` | World.PreSolve hook (explicit opt-in) | 7 |
| `b2World_SetRestitutionCallback` | World.RestitutionMixer hook (explicit opt-in) | 7 |
| `b2World_SetRestitutionThreshold` | World.RestitutionThreshold | 6 |
| `b2World_SetUserData` | World.UserData | 6 |
| `b2World_Step` | World.StepWorld(timeStep) / World.StepWorld(timeStep, substepCount) | 6 |

### Bodies, shapes, and chains (120 symbols)

| Upstream symbol | Physics2D mapping | Stage |
|---|---|---|
| `b2Body_ApplyAngularImpulse` | Body.ApplyAngularImpulse | 3 |
| `b2Body_ApplyForce` | Body.ApplyForce | 3 |
| `b2Body_ApplyForceToCenter` | Body.ApplyForceToCenter | 3 |
| `b2Body_ApplyLinearImpulse` | Body.ApplyLinearImpulse | 3 |
| `b2Body_ApplyLinearImpulseToCenter` | Body.ApplyLinearImpulseToCenter | 3 |
| `b2Body_ApplyMassFromShapes` | Body.ApplyMassFromShapes | 3 |
| `b2Body_ApplyTorque` | Body.ApplyTorque | 3 |
| `b2Body_ComputeAABB` | Body.ComputeAABB | 3 |
| `b2Body_Disable` | Body.Disable | 3 |
| `b2Body_Enable` | Body.Enable | 3 |
| `b2Body_EnableContactEvents` | Body.EnableContactEvents | 3 |
| `b2Body_EnableHitEvents` | Body.EnableHitEvents | 3 |
| `b2Body_EnableSleep` | Body.EnableSleep | 3 |
| `b2Body_GetAngularDamping` | Body.GetAngularDamping || 6 |
| `b2Body_GetAngularVelocity` | Body.GetAngularVelocity || 6 |
| `b2Body_GetContactCapacity` | Body.GetContactCapacity || 7 |
| `b2Body_GetContactData` | Body.GetContactData || 7 |
| `b2Body_GetGravityScale` | Body.GetGravityScale || 6 |
| `b2Body_GetJointCount` | Body.GetJointCount | 9 |
| `b2Body_GetJoints` | Body.GetJoints | 9 |
| `b2Body_GetLinearDamping` | Body.GetLinearDamping || 6 |
| `b2Body_GetLinearVelocity` | Body.GetLinearVelocity || 6 |
| `b2Body_GetLocalCenterOfMass` | Body.GetLocalCenterOfMass || 6 |
| `b2Body_GetLocalPoint` | Body.GetLocalPoint || 6 |
| `b2Body_GetLocalPointVelocity` | Body.GetLocalPointVelocity || 6 |
| `b2Body_GetLocalVector` | Body.GetLocalVector || 6 |
| `b2Body_GetMass` | Body.GetMass || 6 |
| `b2Body_GetMassData` | Body.GetMassData || 6 |
| `b2Body_GetName` | Body.GetName || 6 |
| `b2Body_GetPosition` | Body.GetPosition || 6 |
| `b2Body_GetRotation` | Body.GetRotation || 6 |
| `b2Body_GetRotationalInertia` | Body.GetRotationalInertia || 6 |
| `b2Body_GetShapeCount` | Body.GetShapeCount || 6 |
| `b2Body_GetShapes` | Body.GetShapes || 6 |
| `b2Body_GetSleepThreshold` | Body.GetSleepThreshold || 6 |
| `b2Body_GetTransform` | Body.GetTransform || 6 |
| `b2Body_GetType` | Body.GetType || 6 |
| `b2Body_GetUserData` | Body.GetUserData || 6 |
| `b2Body_GetWorld` | Body.GetWorld || 6 |
| `b2Body_GetWorldCenterOfMass` | Body.GetWorldCenterOfMass || 6 |
| `b2Body_GetWorldPoint` | Body.GetWorldPoint || 6 |
| `b2Body_GetWorldPointVelocity` | Body.GetWorldPointVelocity || 6 |
| `b2Body_GetWorldVector` | Body.GetWorldVector || 6 |
| `b2Body_IsAwake` | Body.IsAwake || 6 |
| `b2Body_IsBullet` | Body.IsBullet || 6 |
| `b2Body_IsEnabled` | Body.IsEnabled || 6 |
| `b2Body_IsFixedRotation` | Body.IsFixedRotation || 6 |
| `b2Body_IsSleepEnabled` | Body.IsSleepEnabled || 6 |
| `b2Body_IsValid` | Body.IsValid || 6 |
| `b2Body_SetAngularDamping` | Body.SetAngularDamping || 6 |
| `b2Body_SetAngularVelocity` | Body.SetAngularVelocity || 6 |
| `b2Body_SetAwake` | Body.SetAwake || 6 |
| `b2Body_SetBullet` | Body.SetBullet || 6 |
| `b2Body_SetFixedRotation` | Body.SetFixedRotation || 6 |
| `b2Body_SetGravityScale` | Body.SetGravityScale || 6 |
| `b2Body_SetLinearDamping` | Body.SetLinearDamping || 6 |
| `b2Body_SetLinearVelocity` | Body.SetLinearVelocity || 6 |
| `b2Body_SetMassData` | Body.SetMassData || 6 |
| `b2Body_SetName` | Body.SetName || 6 |
| `b2Body_SetSleepThreshold` | Body.SetSleepThreshold || 6 |
| `b2Body_SetTargetTransform` | Body.SetTargetTransform || 6 |
| `b2Body_SetTransform` | Body.SetTransform || 6 |
| `b2Body_SetType` | Body.SetType || 6 |
| `b2Body_SetUserData` | Body.SetUserData || 6 |
| `b2Chain_GetFriction` | Chain.GetFriction || 6 |
| `b2Chain_GetMaterial` | Chain.GetMaterial || 6 |
| `b2Chain_GetRestitution` | Chain.GetRestitution || 6 |
| `b2Chain_GetSegmentCount` | Chain.GetSegmentCount || 6 |
| `b2Chain_GetSegments` | Chain.GetSegments || 6 |
| `b2Chain_GetWorld` | Chain.GetWorld || 6 |
| `b2Chain_IsValid` | Chain.IsValid || 6 |
| `b2Chain_SetFriction` | Chain.SetFriction || 6 |
| `b2Chain_SetMaterial` | Chain.SetMaterial || 6 |
| `b2Chain_SetRestitution` | Chain.SetRestitution || 6 |
| `b2Shape_AreContactEventsEnabled` | Shape.AreContactEventsEnabled || 6 |
| `b2Shape_AreHitEventsEnabled` | Shape.AreHitEventsEnabled || 6 |
| `b2Shape_ArePreSolveEventsEnabled` | Shape.ArePreSolveEventsEnabled || 6 |
| `b2Shape_AreSensorEventsEnabled` | Shape.AreSensorEventsEnabled || 6 |
| `b2Shape_EnableContactEvents` | Shape.EnableContactEvents || 6 |
| `b2Shape_EnableHitEvents` | Shape.EnableHitEvents || 6 |
| `b2Shape_EnablePreSolveEvents` | Shape.EnablePreSolveEvents || 6 |
| `b2Shape_EnableSensorEvents` | Shape.EnableSensorEvents || 6 |
| `b2Shape_GetAABB` | Shape.GetAABB || 6 |
| `b2Shape_GetBody` | Shape.GetBody || 6 |
| `b2Shape_GetCapsule` | Shape.GetCapsule || 6 |
| `b2Shape_GetChainSegment` | Shape.GetChainSegment || 6 |
| `b2Shape_GetCircle` | Shape.GetCircle || 6 |
| `b2Shape_GetClosestPoint` | Shape.GetClosestPoint || 6 |
| `b2Shape_GetContactCapacity` | Shape.GetContactCapacity || 7 |
| `b2Shape_GetContactData` | Shape.GetContactData || 7 |
| `b2Shape_GetDensity` | Shape.GetDensity || 6 |
| `b2Shape_GetFilter` | Shape.GetFilter || 6 |
| `b2Shape_GetFriction` | Shape.GetFriction || 6 |
| `b2Shape_GetMassData` | Shape.GetMassData || 6 |
| `b2Shape_GetMaterial` | Shape.GetMaterial || 6 |
| `b2Shape_GetParentChain` | Shape.GetParentChain || 6 |
| `b2Shape_GetPolygon` | Shape.GetPolygon || 6 |
| `b2Shape_GetRestitution` | Shape.GetRestitution || 6 |
| `b2Shape_GetSegment` | Shape.GetSegment || 6 |
| `b2Shape_GetSensorCapacity` | Shape.GetSensorCapacity || 6 |
| `b2Shape_GetSensorOverlaps` | Shape.GetSensorOverlaps || 6 |
| `b2Shape_GetSurfaceMaterial` | Shape.GetSurfaceMaterial || 6 |
| `b2Shape_GetType` | Shape.GetType || 6 |
| `b2Shape_GetUserData` | Shape.GetUserData || 6 |
| `b2Shape_GetWorld` | Shape.GetWorld || 6 |
| `b2Shape_IsSensor` | Shape.IsSensor || 6 |
| `b2Shape_IsValid` | Shape.IsValid || 6 |
| `b2Shape_RayCast` | Shape.RayCast || 6 |
| `b2Shape_SetCapsule` | Shape.SetCapsule || 6 |
| `b2Shape_SetCircle` | Shape.SetCircle || 6 |
| `b2Shape_SetDensity` | Shape.SetDensity || 6 |
| `b2Shape_SetFilter` | Shape.SetFilter || 6 |
| `b2Shape_SetFriction` | Shape.SetFriction || 6 |
| `b2Shape_SetMaterial` | Shape.SetMaterial || 6 |
| `b2Shape_SetPolygon` | Shape.SetPolygon || 6 |
| `b2Shape_SetRestitution` | Shape.SetRestitution || 6 |
| `b2Shape_SetSegment` | Shape.SetSegment || 6 |
| `b2Shape_SetSurfaceMaterial` | Shape.SetSurfaceMaterial || 6 |
| `b2Shape_SetUserData` | Shape.SetUserData || 6 |
| `b2Shape_TestPoint` | Shape.TestPoint || 6 |


### Joints (132 symbols)

| Upstream symbol | Physics2D mapping | Stage |
|---|---|---|
| `b2DistanceJoint_EnableLimit` | DistanceJoint.EnableLimit | 9 |
| `b2DistanceJoint_EnableMotor` | DistanceJoint.EnableMotor | 9 |
| `b2DistanceJoint_EnableSpring` | DistanceJoint.EnableSpring | 9 |
| `b2DistanceJoint_GetCurrentLength` | DistanceJoint.GetCurrentLength | 9 |
| `b2DistanceJoint_GetLength` | DistanceJoint.GetLength | 9 |
| `b2DistanceJoint_GetMaxLength` | DistanceJoint.GetMaxLength | 9 |
| `b2DistanceJoint_GetMaxMotorForce` | DistanceJoint.GetMaxMotorForce | 9 |
| `b2DistanceJoint_GetMinLength` | DistanceJoint.GetMinLength | 9 |
| `b2DistanceJoint_GetMotorForce` | DistanceJoint.GetMotorForce | 9 |
| `b2DistanceJoint_GetMotorSpeed` | DistanceJoint.GetMotorSpeed | 9 |
| `b2DistanceJoint_GetSpringDampingRatio` | DistanceJoint.GetSpringDampingRatio | 9 |
| `b2DistanceJoint_GetSpringHertz` | DistanceJoint.GetSpringHertz | 9 |
| `b2DistanceJoint_IsLimitEnabled` | DistanceJoint.IsLimitEnabled | 9 |
| `b2DistanceJoint_IsMotorEnabled` | DistanceJoint.IsMotorEnabled | 9 |
| `b2DistanceJoint_IsSpringEnabled` | DistanceJoint.IsSpringEnabled | 9 |
| `b2DistanceJoint_SetLength` | DistanceJoint.SetLength | 9 |
| `b2DistanceJoint_SetLengthRange` | DistanceJoint.SetLengthRange | 9 |
| `b2DistanceJoint_SetMaxMotorForce` | DistanceJoint.SetMaxMotorForce | 9 |
| `b2DistanceJoint_SetMotorSpeed` | DistanceJoint.SetMotorSpeed | 9 |
| `b2DistanceJoint_SetSpringDampingRatio` | DistanceJoint.SetSpringDampingRatio | 9 |
| `b2DistanceJoint_SetSpringHertz` | DistanceJoint.SetSpringHertz | 9 |
| `b2Joint_GetAngularSeparation` | Joint.GetAngularSeparation | 9 |
| `b2Joint_GetBodyA` | Joint.GetBodyA | 9 |
| `b2Joint_GetBodyB` | Joint.GetBodyB | 9 |
| `b2Joint_GetCollideConnected` | Joint.GetCollideConnected | 9 |
| `b2Joint_GetConstraintForce` | Joint.GetConstraintForce | 9 |
| `b2Joint_GetConstraintTorque` | Joint.GetConstraintTorque | 9 |
| `b2Joint_GetConstraintTuning` | Joint.GetConstraintTuning | 9 |
| `b2Joint_GetLinearSeparation` | Joint.GetLinearSeparation | 9 |
| `b2Joint_GetLocalAnchorA` | Joint.GetLocalAnchorA | 9 |
| `b2Joint_GetLocalAnchorB` | Joint.GetLocalAnchorB | 9 |
| `b2Joint_GetLocalAxisA` | Joint.GetLocalAxisA | 9 |
| `b2Joint_GetReferenceAngle` | Joint.GetReferenceAngle | 9 |
| `b2Joint_GetType` | Joint.GetType | 9 |
| `b2Joint_GetUserData` | Joint.GetUserData | 9 |
| `b2Joint_GetWorld` | Joint.GetWorld | 9 |
| `b2Joint_IsValid` | Joint.IsValid | 9 |
| `b2Joint_SetCollideConnected` | Joint.SetCollideConnected | 9 |
| `b2Joint_SetConstraintTuning` | Joint.SetConstraintTuning | 9 |
| `b2Joint_SetLocalAnchorA` | Joint.SetLocalAnchorA | 9 |
| `b2Joint_SetLocalAnchorB` | Joint.SetLocalAnchorB | 9 |
| `b2Joint_SetLocalAxisA` | Joint.SetLocalAxisA | 9 |
| `b2Joint_SetReferenceAngle` | Joint.SetReferenceAngle | 9 |
| `b2Joint_SetUserData` | Joint.SetUserData | 9 |
| `b2Joint_WakeBodies` | Joint.WakeBodies | 9 |
| `b2MotorJoint_GetAngularOffset` | MotorJoint.GetAngularOffset | 9 |
| `b2MotorJoint_GetCorrectionFactor` | MotorJoint.GetCorrectionFactor | 9 |
| `b2MotorJoint_GetLinearOffset` | MotorJoint.GetLinearOffset | 9 |
| `b2MotorJoint_GetMaxForce` | MotorJoint.GetMaxForce | 9 |
| `b2MotorJoint_GetMaxTorque` | MotorJoint.GetMaxTorque | 9 |
| `b2MotorJoint_SetAngularOffset` | MotorJoint.SetAngularOffset | 9 |
| `b2MotorJoint_SetCorrectionFactor` | MotorJoint.SetCorrectionFactor | 9 |
| `b2MotorJoint_SetLinearOffset` | MotorJoint.SetLinearOffset | 9 |
| `b2MotorJoint_SetMaxForce` | MotorJoint.SetMaxForce | 9 |
| `b2MotorJoint_SetMaxTorque` | MotorJoint.SetMaxTorque | 9 |
| `b2MouseJoint_GetMaxForce` | MouseJoint.GetMaxForce | 9 |
| `b2MouseJoint_GetSpringDampingRatio` | MouseJoint.GetSpringDampingRatio | 9 |
| `b2MouseJoint_GetSpringHertz` | MouseJoint.GetSpringHertz | 9 |
| `b2MouseJoint_GetTarget` | MouseJoint.GetTarget | 9 |
| `b2MouseJoint_SetMaxForce` | MouseJoint.SetMaxForce | 9 |
| `b2MouseJoint_SetSpringDampingRatio` | MouseJoint.SetSpringDampingRatio | 9 |
| `b2MouseJoint_SetSpringHertz` | MouseJoint.SetSpringHertz | 9 |
| `b2MouseJoint_SetTarget` | MouseJoint.SetTarget | 9 |
| `b2PrismaticJoint_EnableLimit` | PrismaticJoint.EnableLimit | 9 |
| `b2PrismaticJoint_EnableMotor` | PrismaticJoint.EnableMotor | 9 |
| `b2PrismaticJoint_EnableSpring` | PrismaticJoint.EnableSpring | 9 |
| `b2PrismaticJoint_GetLowerLimit` | PrismaticJoint.GetLowerLimit | 9 |
| `b2PrismaticJoint_GetMaxMotorForce` | PrismaticJoint.GetMaxMotorForce | 9 |
| `b2PrismaticJoint_GetMotorForce` | PrismaticJoint.GetMotorForce | 9 |
| `b2PrismaticJoint_GetMotorSpeed` | PrismaticJoint.GetMotorSpeed | 9 |
| `b2PrismaticJoint_GetSpeed` | PrismaticJoint.GetSpeed | 9 |
| `b2PrismaticJoint_GetSpringDampingRatio` | PrismaticJoint.GetSpringDampingRatio | 9 |
| `b2PrismaticJoint_GetSpringHertz` | PrismaticJoint.GetSpringHertz | 9 |
| `b2PrismaticJoint_GetTargetTranslation` | PrismaticJoint.GetTargetTranslation | 9 |
| `b2PrismaticJoint_GetTranslation` | PrismaticJoint.GetTranslation | 9 |
| `b2PrismaticJoint_GetUpperLimit` | PrismaticJoint.GetUpperLimit | 9 |
| `b2PrismaticJoint_IsLimitEnabled` | PrismaticJoint.IsLimitEnabled | 9 |
| `b2PrismaticJoint_IsMotorEnabled` | PrismaticJoint.IsMotorEnabled | 9 |
| `b2PrismaticJoint_IsSpringEnabled` | PrismaticJoint.IsSpringEnabled | 9 |
| `b2PrismaticJoint_SetLimits` | PrismaticJoint.SetLimits | 9 |
| `b2PrismaticJoint_SetMaxMotorForce` | PrismaticJoint.SetMaxMotorForce | 9 |
| `b2PrismaticJoint_SetMotorSpeed` | PrismaticJoint.SetMotorSpeed | 9 |
| `b2PrismaticJoint_SetSpringDampingRatio` | PrismaticJoint.SetSpringDampingRatio | 9 |
| `b2PrismaticJoint_SetSpringHertz` | PrismaticJoint.SetSpringHertz | 9 |
| `b2PrismaticJoint_SetTargetTranslation` | PrismaticJoint.SetTargetTranslation | 9 |
| `b2RevoluteJoint_EnableLimit` | RevoluteJoint.EnableLimit | 9 |
| `b2RevoluteJoint_EnableMotor` | RevoluteJoint.EnableMotor | 9 |
| `b2RevoluteJoint_EnableSpring` | RevoluteJoint.EnableSpring | 9 |
| `b2RevoluteJoint_GetAngle` | RevoluteJoint.GetAngle | 9 |
| `b2RevoluteJoint_GetLowerLimit` | RevoluteJoint.GetLowerLimit | 9 |
| `b2RevoluteJoint_GetMaxMotorTorque` | RevoluteJoint.GetMaxMotorTorque | 9 |
| `b2RevoluteJoint_GetMotorSpeed` | RevoluteJoint.GetMotorSpeed | 9 |
| `b2RevoluteJoint_GetMotorTorque` | RevoluteJoint.GetMotorTorque | 9 |
| `b2RevoluteJoint_GetSpringDampingRatio` | RevoluteJoint.GetSpringDampingRatio | 9 |
| `b2RevoluteJoint_GetSpringHertz` | RevoluteJoint.GetSpringHertz | 9 |
| `b2RevoluteJoint_GetTargetAngle` | RevoluteJoint.GetTargetAngle | 9 |
| `b2RevoluteJoint_GetUpperLimit` | RevoluteJoint.GetUpperLimit | 9 |
| `b2RevoluteJoint_IsLimitEnabled` | RevoluteJoint.IsLimitEnabled | 9 |
| `b2RevoluteJoint_IsMotorEnabled` | RevoluteJoint.IsMotorEnabled | 9 |
| `b2RevoluteJoint_IsSpringEnabled` | RevoluteJoint.IsSpringEnabled | 9 |
| `b2RevoluteJoint_SetLimits` | RevoluteJoint.SetLimits | 9 |
| `b2RevoluteJoint_SetMaxMotorTorque` | RevoluteJoint.SetMaxMotorTorque | 9 |
| `b2RevoluteJoint_SetMotorSpeed` | RevoluteJoint.SetMotorSpeed | 9 |
| `b2RevoluteJoint_SetSpringDampingRatio` | RevoluteJoint.SetSpringDampingRatio | 9 |
| `b2RevoluteJoint_SetSpringHertz` | RevoluteJoint.SetSpringHertz | 9 |
| `b2RevoluteJoint_SetTargetAngle` | RevoluteJoint.SetTargetAngle | 9 |
| `b2WeldJoint_GetAngularDampingRatio` | WeldJoint.GetAngularDampingRatio | 9 |
| `b2WeldJoint_GetAngularHertz` | WeldJoint.GetAngularHertz | 9 |
| `b2WeldJoint_GetLinearDampingRatio` | WeldJoint.GetLinearDampingRatio | 9 |
| `b2WeldJoint_GetLinearHertz` | WeldJoint.GetLinearHertz | 9 |
| `b2WeldJoint_SetAngularDampingRatio` | WeldJoint.SetAngularDampingRatio | 9 |
| `b2WeldJoint_SetAngularHertz` | WeldJoint.SetAngularHertz | 9 |
| `b2WeldJoint_SetLinearDampingRatio` | WeldJoint.SetLinearDampingRatio | 9 |
| `b2WeldJoint_SetLinearHertz` | WeldJoint.SetLinearHertz | 9 |
| `b2WheelJoint_EnableLimit` | WheelJoint.EnableLimit | 9 |
| `b2WheelJoint_EnableMotor` | WheelJoint.EnableMotor | 9 |
| `b2WheelJoint_EnableSpring` | WheelJoint.EnableSpring | 9 |
| `b2WheelJoint_GetLowerLimit` | WheelJoint.GetLowerLimit | 9 |
| `b2WheelJoint_GetMaxMotorTorque` | WheelJoint.GetMaxMotorTorque | 9 |
| `b2WheelJoint_GetMotorSpeed` | WheelJoint.GetMotorSpeed | 9 |
| `b2WheelJoint_GetMotorTorque` | WheelJoint.GetMotorTorque | 9 |
| `b2WheelJoint_GetSpringDampingRatio` | WheelJoint.GetSpringDampingRatio | 9 |
| `b2WheelJoint_GetSpringHertz` | WheelJoint.GetSpringHertz | 9 |
| `b2WheelJoint_GetUpperLimit` | WheelJoint.GetUpperLimit | 9 |
| `b2WheelJoint_IsLimitEnabled` | WheelJoint.IsLimitEnabled | 9 |
| `b2WheelJoint_IsMotorEnabled` | WheelJoint.IsMotorEnabled | 9 |
| `b2WheelJoint_IsSpringEnabled` | WheelJoint.IsSpringEnabled | 9 |
| `b2WheelJoint_SetLimits` | WheelJoint.SetLimits | 9 |
| `b2WheelJoint_SetMaxMotorTorque` | WheelJoint.SetMaxMotorTorque | 9 |
| `b2WheelJoint_SetMotorSpeed` | WheelJoint.SetMotorSpeed | 9 |
| `b2WheelJoint_SetSpringDampingRatio` | WheelJoint.SetSpringDampingRatio | 9 |
| `b2WheelJoint_SetSpringHertz` | WheelJoint.SetSpringHertz | 9 |


### Collision queries and geometry (50 symbols)

| Upstream symbol | Physics2D mapping | Stage |
|---|---|---|
| `b2ClipVector` | Protected internal C | 4 |
| `b2CollideCapsuleAndCircle` | Protected internal C | 4 |
| `b2CollideCapsules` | Protected internal C | 4 |
| `b2CollideChainSegmentAndCapsule` | Protected internal C | 4 |
| `b2CollideChainSegmentAndCircle` | Protected internal C | 4 |
| `b2CollideChainSegmentAndPolygon` | Protected internal C | 4 |
| `b2CollideCircles` | Protected internal C | 4 |
| `b2CollidePolygonAndCapsule` | Protected internal C | 4 |
| `b2CollidePolygonAndCircle` | Protected internal C | 4 |
| `b2CollidePolygons` | Protected internal C | 4 |
| `b2CollideSegmentAndCapsule` | Protected internal C | 4 |
| `b2CollideSegmentAndCircle` | Protected internal C | 4 |
| `b2CollideSegmentAndPolygon` | Protected internal C | 4 |
| `b2ComputeCapsuleAABB` | Protected internal C | 4 |
| `b2ComputeCapsuleMass` | Protected internal C | 4 |
| `b2ComputeCircleAABB` | Protected internal C | 4 |
| `b2ComputeCircleMass` | Protected internal C | 4 |
| `b2ComputeHull` | Protected internal C | 4 |
| `b2ComputePolygonAABB` | Protected internal C | 4 |
| `b2ComputePolygonMass` | Protected internal C | 4 |
| `b2ComputeSegmentAABB` | Protected internal C | 4 |
| `b2GetSweepTransform` | Protected internal G | 4 |
| `b2IsValidRay` | Protected internal I | 4 |
| `b2MakeBox` | Protected internal M | 4 |
| `b2MakeOffsetBox` | Protected internal M | 4 |
| `b2MakeOffsetPolygon` | Protected internal M | 4 |
| `b2MakeOffsetProxy` | Protected internal M | 4 |
| `b2MakeOffsetRoundedBox` | Protected internal M | 4 |
| `b2MakeOffsetRoundedPolygon` | Protected internal M | 4 |
| `b2MakePolygon` | Protected internal M | 4 |
| `b2MakeProxy` | Protected internal M | 4 |
| `b2MakeRoundedBox` | Protected internal M | 4 |
| `b2MakeSquare` | Protected internal M | 4 |
| `b2PointInCapsule` | Protected internal P | 4 |
| `b2PointInCircle` | Protected internal P | 4 |
| `b2PointInPolygon` | Protected internal P | 4 |
| `b2RayCastCapsule` | Protected internal R | 4 |
| `b2RayCastCircle` | Protected internal R | 4 |
| `b2RayCastPolygon` | Protected internal R | 4 |
| `b2RayCastSegment` | Protected internal R | 4 |
| `b2SegmentDistance` | Protected internal S | 4 |
| `b2ShapeCast` | Protected internal S | 4 |
| `b2ShapeCastCapsule` | Protected internal S | 4 |
| `b2ShapeCastCircle` | Protected internal S | 4 |
| `b2ShapeCastPolygon` | Protected internal S | 4 |
| `b2ShapeCastSegment` | Protected internal S | 4 |
| `b2ShapeDistance` | Protected internal S | 4 |
| `b2TimeOfImpact` | Protected internal T | 4 |
| `b2TransformPolygon` | Protected internal T | 4 |
| `b2ValidateHull` | Protected internal V | 4 |


### Dynamic tree (20 symbols)

| Upstream symbol | Physics2D mapping | Stage |
|---|---|---|
| `b2DynamicTree_Create` | Protected internal DynamicTree.Create | 5 |
| `b2DynamicTree_CreateProxy` | Protected internal DynamicTree.CreateProxy | 5 |
| `b2DynamicTree_Destroy` | Protected internal DynamicTree.Destroy | 5 |
| `b2DynamicTree_DestroyProxy` | Protected internal DynamicTree.DestroyProxy | 5 |
| `b2DynamicTree_EnlargeProxy` | Protected internal DynamicTree.EnlargeProxy | 5 |
| `b2DynamicTree_GetAABB` | Protected internal DynamicTree.GetAABB | 5 |
| `b2DynamicTree_GetAreaRatio` | Protected internal DynamicTree.GetAreaRatio | 5 |
| `b2DynamicTree_GetByteCount` | Excluded (v1): GC-managed node storage has no faithful byte count | 5 |
| `b2DynamicTree_GetCategoryBits` | Protected internal DynamicTree.GetCategoryBits | 5 |
| `b2DynamicTree_GetHeight` | Protected internal DynamicTree.GetHeight | 5 |
| `b2DynamicTree_GetProxyCount` | Protected internal DynamicTree.GetProxyCount | 5 |
| `b2DynamicTree_GetRootBounds` | Protected internal DynamicTree.GetRootBounds | 5 |
| `b2DynamicTree_GetUserData` | Protected internal DynamicTree.GetUserData | 5 |
| `b2DynamicTree_MoveProxy` | Protected internal DynamicTree.MoveProxy | 5 |
| `b2DynamicTree_Query` | Protected internal DynamicTree.Query | 5 |
| `b2DynamicTree_RayCast` | Protected internal DynamicTree.RayCast | 5 |
| `b2DynamicTree_Rebuild` | Protected internal DynamicTree.Rebuild | 5 |
| `b2DynamicTree_SetCategoryBits` | Protected internal DynamicTree.SetCategoryBits | 5 |
| `b2DynamicTree_ShapeCast` | Protected internal DynamicTree.ShapeCast | 5 |
| `b2DynamicTree_Validate` | Protected internal DynamicTree.Validate | 5 |
| `b2DynamicTree_ValidateNoEnlarged` | Protected internal DynamicTree.ValidateNoEnlarged | 5 |


### Broad phase (12 symbols)

The broad phase lives behind the world façade: `BroadPhase` is an
engine-internal class, and `BroadPhaseQueryContext` replaces the upstream
callback-plus-context traversal helper. The world drives pair consumption
through `BroadPhasePairSink`.

| Upstream symbol | Physics2D mapping | Stage |
|---|---|---|
| `b2CreateBroadPhase` | Protected internal BroadPhase constructor | 5 |
| `b2DestroyBroadPhase` | Released by garbage collection: the sets and trees are Objo objects with no unmanaged resources | 5 |
| `b2BroadPhase_CreateProxy` | Protected internal BroadPhase.CreateProxy | 5 |
| `b2BroadPhase_DestroyProxy` | Protected internal BroadPhase.DestroyProxy | 5 |
| `b2BroadPhase_MoveProxy` | Protected internal BroadPhase.MoveProxy | 5 |
| `b2BroadPhase_EnlargeProxy` | Protected internal BroadPhase.EnlargeProxy | 5 |
| `b2BroadPhase_RebuildTrees` | Protected internal BroadPhase.RebuildTrees | 5 |
| `b2BroadPhase_GetShapeIndex` | Protected internal BroadPhase.GetShapeIndex | 5 |
| `b2BroadPhase_TestOverlap` | Protected internal BroadPhase.TestOverlap | 5 |
| `b2UpdateBroadPhasePairs` | Protected internal BroadPhase.UpdatePairs with a BroadPhasePairSink | 5 |
| `b2BufferMove` | Protected internal BroadPhase.BufferMove | 5 |
| `b2ValidateBroadphase` / `b2ValidateNoEnlarged` | Protected internal BroadPhase.Validate / ValidateNoEnlarged | 5 |


### Maths helpers (8 symbols)

| Upstream symbol | Physics2D mapping | Stage |
|---|---|---|
| `b2Atan2` | PhysicsMaths.Atan2 (deterministic upstream port) | 3 |
| `b2ComputeCosSin` | PhysicsMaths.ComputeCosSin + CosSin | 3 |
| `b2ComputeRotationBetweenUnitVectors` | Protected internal C | 3 |
| `b2IsValidAABB` | Protected internal I | 3 |
| `b2IsValidFloat` | Double.IsFinite (Objo standard library, issue #1302) | 3 |
| `b2IsValidPlane` | Protected internal I | 3 |
| `b2IsValidRotation` | Protected internal I | 3 |
| `b2IsValidVec2` | Protected internal I | 3 |


### Definition defaults (17 symbols)

| Upstream symbol | Physics2D mapping | Stage |
|---|---|---|
| `b2DefaultBodyDef` | BodyDefinition with documented defaults | 6 |
| `b2DefaultChainDef` | ChainDefinition with documented defaults | 6 |
| `b2DefaultDebugDraw` | DebugDrawOptions with documented defaults | 10 |
| `b2DefaultDistanceJointDef` | DistanceJointDefinition with documented defaults | 9 |
| `b2DefaultExplosionDef` | ExplosionDefinition with documented defaults | 10 |
| `b2DefaultFilter` | Filter with documented defaults | 6 |
| `b2DefaultFilterJointDef` | FilterJointDefinition with documented defaults | 9 |
| `b2DefaultMotorJointDef` | MotorJointDefinition with documented defaults | 9 |
| `b2DefaultMouseJointDef` | MouseJointDefinition with documented defaults | 9 |
| `b2DefaultPrismaticJointDef` | PrismaticJointDefinition with documented defaults | 9 |
| `b2DefaultQueryFilter` | QueryFilter with documented defaults | 6 |
| `b2DefaultRevoluteJointDef` | RevoluteJointDefinition with documented defaults | 9 |
| `b2DefaultShapeDef` | ShapeDefinition with documented defaults | 6 |
| `b2DefaultSurfaceMaterial` | SurfaceMaterial with documented defaults | 6 |
| `b2DefaultWeldJointDef` | WeldJointDefinition with documented defaults | 9 |
| `b2DefaultWheelJointDef` | WheelJointDefinition with documented defaults | 9 |
| `b2DefaultWorldDef` | WorldSettings with documented defaults | 6 |


### Length scale and plane solving (3 symbols)

| Upstream symbol | Physics2D mapping | Stage |
|---|---|---|
| `b2GetLengthUnitsPerMeter` | Length-scale configuration only if pinned upstream requires it | 10 |
| `b2SetLengthUnitsPerMeter` | Length-scale configuration only if pinned upstream requires it | 10 |
| `b2SolvePlanes` | Protected internal plane solver used by World.CollideMover | 10 |


### Platform and allocator hooks (1 symbol)

| Upstream symbol | Physics2D mapping | Stage |
|---|---|---|
| `b2Hash` | PhysicsMaths.HashKey (Murmur3 finaliser behind PairKeySet) | 3 |

### Approved version 1 exclusions (10 symbols)

| Upstream symbol | Physics2D mapping | Stage |
|---|---|---|
| `b2GetByteCount` | Excluded (v1): C allocator statistics replaced by World.Counters | 0 |
| `b2GetMilliseconds` | Excluded (v1): native timers; benchmarks use the Objo clock | 0 |
| `b2GetMillisecondsAndReset` | Excluded (v1): native timers; benchmarks use the Objo clock | 0 |
| `b2GetTicks` | Excluded (v1): native timers; benchmarks use the Objo clock | 0 |
| `b2GetVersion` | Excluded (v1): the module version is documented in README.md | 0 |
| `b2DynamicTree_GetByteCount` | Excluded (v1): GC-managed node storage has no faithful byte count | 5 |
| `b2InternalAssertFcn` | Excluded (v1): internal validation uses exceptions and test assertions | 0 |
| `b2SetAllocator` | Excluded (v1): Objo manages memory; no custom C allocators | 0 |
| `b2SetAssertFcn` | Excluded (v1): internal validation uses exceptions and test assertions | 0 |
| `b2Yield` | Excluded (v1): task scheduler callbacks are out of scope | 0 |


## Upstream Source Mapping

Physics2D module source items live in `Shared/Sources/` as nested members of
the `Physics2D` module. The mapping below is updated as stages complete.
Upstream line counts are for orientation only.

| Upstream source | Physics2D source item | Stage |
|---|---|---|
| `src/constants.h`, `src/core.[ch]` (sentinels), `src/math_functions.[ch]` (deterministic helpers) | `PhysicsConstants`, `PhysicsMaths`, `CosSin` | 3 |
| `src/table.[ch]`, `src/ctz.h` | `PairKeySet` plus `PhysicsMaths.HashKey`/`TrailingZeros` | 3 |
| `src/id_pool.[ch]`, `src/array.[ch]` | `IdPool`, `GenerationalPool`, `IntegerList`, `DoubleList` | 3 |
| `src/bitset.[ch]` | `BitSet` | 3 |
| `src/hull.c` | `Hull` (gift-wrap with weld and collinear merge) | 4 |
| `src/geometry.c`, `src/shape.c` (mass and AABB methods) | `Circle`, `Capsule`, `Segment`, `Polygon`, `MassData`, `ShapeProxy`, value types (`RayCastInput`, `SegmentDistanceResult`, `Plane*`) | 4 |
| `src/distance.c` (GJK, barycentric simplex, ray/shape casts, time of impact) | `Distance` plus `DistanceInput`/`DistanceOutput`/`DistanceScratch`, `Casts` plus `CastOutput`/`CastScratch`, `TimeOfImpact` plus `TOIInput`/`TOIOutput`/`TOIScratch`, `Simplex*`, `SeparationFunction`, `Sweep` | 4 |
| `src/manifold.c` | `Collide` plus `CollideScratch`, `Manifold`, `ManifoldPoint`, `ChainSegment`, `ChainNormalType`, `ChainParams` | 4 |
| `src/mover.c` | `Mover` plus `Plane`, `PlaneResult`, `CollisionPlane`, `PlaneSolverResult` (world-level `CastMover`/`CollideMover` wrappers arrive in Stage 10) | 4 |
| `src/aabb.c` | `BoundsMath` | 5 |
| `src/dynamic_tree.c` | `DynamicTree` | 5 |
| `src/broad_phase.[ch]` | `BroadPhase` | 5 |
| `src/body.[ch]`, `src/shape.[ch]` | body/shape stores and façades | 6 |
| `src/world.[ch]`, `src/types.c` | `World` façade and world core including stepping and events | 8 |
| `src/contact.[ch]`, `src/contact_solver.c` | contact store and contact solver | 7 |
| `src/island.[ch]`, `src/solver_set.[ch]`, `src/solver.[ch]` | islands, solver sets, Soft Step solver, and continuous collision | 7/8 |
| `src/constraint_graph.[ch]` | constraint graph colours | 7 |
| `src/sensor.[ch]` | sensor tracking and events | 8 |
| `src/joint.[ch]` | joint store and common joint solver | 9 |
| `src/distance_joint.c` … `src/wheel_joint.c` | one source item per joint family | 9 |
| `src/mover.c` | Stage 4 delivers the standalone `Mover` solver; the world query wrappers below stay in Stage 10 | 4/10 |
| `src/timer.c`, `src/arena_allocator.c`, `src/atomic.h` | excluded (v1) — replaced by Objo clocks and GC | 0 |

## Upstream Test Mapping

Upstream tests are ported as Objo `TestCase` classes in the
`Physics2D.Tests` project, with Physics2D names and `Double` tolerances.
Golden numeric fixtures are generated from the pinned upstream source.

| Upstream test | Physics2D test source item | Stage |
|---|---|---|
| `test/test_math.c` | `FoundationMathsTests` | 3 |
| `test/test_table.c`, `test/test_bitset.c`, `test/test_id.c` | `FoundationPairKeySetTests`, `FoundationBitSetTests`, `FoundationIdentityTests` | 3 |
| `test/test_collision.c` | `GeometryHullTests`, `GeometryDistanceTests`, `GeometryCastTests`, `GeometryManifoldTests`, `GeometryTOITests`, plus golden fixtures `hull.txt`, `distance.txt`, `raycast.txt`, `shapecast.txt`, `toi.txt`, `manifold.txt` | 4 |
| `test/test_shape.c` | `GeometryShapeTests`, `GeometryMassTests`, plus golden fixtures `shape.txt`, `mass.txt` | 4 |
| `src/mover.c` behaviour | `GeometryMoverTests` | 4 |
| `test/test_sweep.c` overflow paths, `src/dynamic_tree.c` behaviour | `DynamicTreeTests` | 5 |
| `src/broad_phase.c` behaviour | `BroadPhaseTests` | 5 |
| `test/test_world.c` | `WorldTests`, `BodyTests`, `QueryTests` | 6 |
| `test/test_determinism.c` | `DeterminismTests` | 7 |
| solver/scenario coverage from `samples/` | solver scene tests | 7 |

Additional per-joint, sensor, and CCD scenarios follow the algorithms in their
upstream source files where the upstream test suite does not isolate them.

## Minimum Objo Version

| Field | Value |
|---|---|
| Development Studio/CLI version | 26.9.1 (`objo version`, built from the Objo checkout at commit `485c4fab`) |
| Runtime | .NET 10 SDK per `global.json` in the Objo checkout |
| Required standard-library features | `Vector2`, `Matrix`, `Maths`, `Array.Reserve`, generic arrays, `TestCase`/`Assert`, modules with nested source items, `System.AllocationCount` (Objo issue #1299), `Vector2.LeftPerpendicular`, `Vector2.RightPerpendicular`, `Matrix.Inverse`, `Matrix.InvertSelf`, `Matrix.Solve`, `Double.IsFinite` (Objo issue #1302) |
| Required engine fixes | constructor inheritance for module-owned classes (Objo issue #1315, engine commit `b9781ccc83a87b076f5c73bb77eb1a99b3da6119`) |
| Required Studio features | `.objosln` VCS solutions (format version 4), Test build scope, desktop projects |
| Documented minimum | Objo Studio 26.8.6 plus the Objo issue #1302 standard-library members and the issue #1315 engine fix, except the benchmark harness, bake-off kernels, and their tests, which additionally require the unreleased `System.AllocationCount` runtime property until Objo ships it |

When Physics2D adopts a newly added standard-library member, the minimum
version is updated here and in `docs/GETTING_STARTED.md`.

## Version Update Procedure

To re-pin against a newer Box2D release: update the tag and commit in this
file and `AGENTS.md`, regenerate golden fixtures from the new source, diff the
public symbol inventory, and record any behavioural changes in a new decision
document. Do not mix algorithms from two Box2D versions.
