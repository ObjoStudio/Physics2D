# Golden fixtures for Physics2D.
#
# All numeric fixtures were generated from Box2D tag v3.1.1, commit
# 8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3 (see docs/PORTING.md).
#
# Generation command (from the repository root, after building the tool):
#   for f in maths hull distance raycast shapecast manifold mass scene_falling scene_pyramid scene_stack joint_distance joint_mouse joint_motor joint_revolute joint_prismatic joint_weld joint_wheel; do
#     ./build/tools/fixture_gen $f > testdata/golden/$f.txt
#   done
#
# The generator source is tools/fixture_gen/fixture_gen.c.

## Tool checksums

Generated on 2026-08-30.

| File | SHA-256 |
|---|---|
| `tools/fixture_gen/fixture_gen.c` | 907c3b987bbb84e6ac3b46336572c72b1515a6bb2f289503c43f373b40f9a86c |
| `testdata/golden/distance.txt` | e3308d9c1aa25d8cd5a715fd8103b28ceaa8a3964cbe513ad424ecdf4eabd26c |
| `testdata/golden/hull.txt` | 48d12525c5a1a837349d3180c74b640769f56ae78139121be649f2c45a058121 |
| `testdata/golden/joint_distance.txt` | ff5bf412c046aed8f4abf71d3855d977914ef770d7c1ae24c5ab24384d8f6a67 |
| `testdata/golden/joint_mouse.txt` | 87312b76153d92d69dcdd690d192143faeaaa334f3f426b5a973a92ace197c72 |
| `testdata/golden/joint_motor.txt` | ac82f90abaa938d38b3991adaee5de017ac4c0a267257e97042c6d9a6f4fe6c7 |
| `testdata/golden/joint_revolute.txt` | e74b23d9e29316ce423e362841fd58bbc01ef0fe566feeeee6952ecaf4e178c6 |
| `testdata/golden/joint_prismatic.txt` | 9a7687a0a883f696589f9f6401bb3716f5ec79738e13b48e937b8bd9cf755292 |
| `testdata/golden/joint_weld.txt` | eee16c6a0fac545f198b8dbe90890db01ae16ffb28ad16ae8f460ce1b4457ec9 |
| `testdata/golden/joint_wheel.txt` | 9ac2ef5aab6026a386bb739f4d06f6684553abc94567757677a388d9ad147cd9 |
| `testdata/golden/manifold.txt` | d3364d2db2bd598856668578b7910b44c9183245fafca1ccfb3e5db980c35701 |
| `testdata/golden/mass.txt` | 880a0a41bb111488631f89e8864d9ee0c4f20df0d335cddd79a1a83422aad736 |
| `testdata/golden/maths.txt` | b88060fa94260d4850309e753a72fb42cc02184d6931371def7ce708ad1e186f |
| `testdata/golden/raycast.txt` | 0af403496a65a3589ad0933be1308ee3ba2c944160c6ad31fdcd9785a9729faf |
| `testdata/golden/scene_falling.txt` | dcc814229d44d5ab1428d0297a3e25f09f6deff621f6359ce8f7581bb4528d69 |
| `testdata/golden/scene_pyramid.txt` | aa59cfee70890b98fa588a65ee45ff5754d823c9c4abdf7f3b160cb095b531ff |
| `testdata/golden/scene_stack.txt` | a77c7e44bf16904f2723d9bb9bf5c4f3277a08f7a03426e4562d1fd278d8ea8c |
| `testdata/golden/shapecast.txt` | 91b59b20ff517e9f0f0141e609cabe4745da47618ac17fb0f323308c5ff305a4 |

## Fixture line formats

Fields are separated by `|`; numbers within a field are separated by single
spaces. All numbers are printed with `%.9g` from C `float` computations, so
consumers compare with the tolerance classes in the `Tolerances` module, not
with exact equality. A trailing `1`/`0` integer encodes a Boolean.

- `maths|case|ax ay bx by|dot cross svx svy vsx vsy addx addy subx suby lerpx lerpy length dist normx normy rotx roty atan2`
- `hull|case|count|x y ...|valid` — CCW hull points; `valid` is `b2ValidateHull`.
- `poly|case|count|x y ...|radius` and `box|case|count|x y ...|radius` — emitted
  only when the preceding hull is valid.
- `segdist|case|fraction1 fraction2 distanceSquared closest1x closest1y closest2x closest2y`
- `gjk|circle_box|distance pointAx pointAy pointBx pointBy cacheCount`
- `ray|shape|case|fraction normalx normaly pointx pointy iterations hit` — shape
  is `circle`, `capsule`, `segment`, or `polygon`; `iterations` is the bucket
  count from the `b2RayResult`.
- `shapecast|shape|case|fraction normalx normaly pointx pointy hit`
- `manifold|name|case|pointCount|normalx normaly` followed by exactly two
  point records `pointx pointy anchorAx anchorAy anchorBx anchorBy separation
  pointId persisted`; missing points are zero-filled.
- `mass|shape|case|mass centerX centerY rotationalInertia` — shape is `circle`,
  `capsule`, or `polygon`.
- `scene|name|bodies|bodyCount|movedCount`, then one
  `scene|name|body|index|positionx positiony angle awake` line per dumped body
  (the first `dumpBodyLimit` bodies after the step).
- `joint|case|body|index|positionx positiony angle awake`, then one
  `joint|case|joint|forceX forceY torque length motorForce` line per distance
  joint case. Cases: `rope` (rigid joint, two falling boxes, 90 frames),
  `limit` (limited soft joint catching a body shot downward, 120 frames),
  `spring` (soft joint oscillating to rest, 240 frames), `motor` (motor
  driving a limited spring joint against gravity, 240 frames). Every frame
  steps with the upstream default substep count of four.
- `joint|case|body|index|positionx positiony angle awake`, then one
  `joint|case|mouse|forceX forceY torque` line per mouse joint case. Cases:
  `drag` (centre-anchored soft drag that sleeps, is retargeted and woken,
  then re-settles, 180 frames), `weak` (one-newton clamp saturated below the
  2.5 newton weight, constant 6 m/s^2 fall, 150 frames), `stiff` (20 Hz
  well-damped retargeted drag, 180 frames). Every frame steps with the
  upstream default substep count of four.
- `joint|case|body|index|positionx positiony angle awake`, then one
  `joint|case|motor|forceX forceY torque` line per motor joint case. Cases:
  `pose` (default 0.3 correction factor drives the box up and half a radian
  over, 240 frames), `retarget` (box starts at its pose, sleeps, then a
  unit correction factor retargets it via the runtime setters with an
  explicit wake, 240 frames), `clamped` (3 N force just above the 2.5 N
  weight saturates the linear clamp through the transit, 240 frames). Every
  frame steps with the upstream default substep count of four.
- `joint|case|body|index|positionx positiony angle awake`, then one
  `joint|case|revolute|forceX forceY torque angle` line per revolute joint
  case. Every case pins a 0.25 kg box to a static ground through a hinge at
  (2, -0.25) through the box's top edge and ends asleep: `hinge` (plain
  hinge, kicked to 1 rad/s at frame sixty with heavy angular damping, 240
  frames), `spring` (2 Hz 0.6-damped spring driving toward 0.8 rad with a
  gravity sag to about 0.59 rad, 240 frames), `brake` (zero-speed motor as
  a 0.5 N-m torque-clamped brake on a kicked box, 240 frames), `limits`
  (limits [0.5, 0.9] holding the box at the lower limit against gravity,
  240 frames). Every frame steps with the upstream default substep count of
  four.
- `joint|case|body|index|positionx positiony angle awake`, then one
  `joint|case|prismatic|forceX forceY torque translation` line per
  prismatic joint case. Every case pins a 0.25 kg box to a static ground
  through a slider anchored at (0, -0.5) and ends asleep: `limits` (X-axis
  rail, kicked to 1.5 m/s at frame thirty, upper limit stops the box at
  translation 1, 240 frames), `tilted` (diagonal (1, 1) rail carrying a
  gravity component, lower limit holds the box at translation -0.5, 240
  frames), `spring` (3 Hz 0.7-damped spring driving to translation 0.5,
  240 frames), `brake` (zero-speed motor as a 5 N force-clamped brake on a
  box kicked to 1.5 m/s at frame thirty, 240 frames). Every frame steps
  with the upstream default substep count of four.
- `joint|case|body|index|positionx positiony angle awake` twice per case
  (body 0 and 1), then one `joint|case|weld|forceX forceY torque` line per
  weld joint case. Each case welds two 0.25 kg boxes together in the air
  and drops the pair onto a static ground: `rigid` (zero hertz weld, the
  pair topples and comes to rest, 300 frames), `soft` (2 Hz 0.5-damped
  weld, the pair flexes and holds a bent pose, 300 frames). Every frame
  steps with the upstream default substep count of four. The rigid case's
  settled force is a frozen last-solved impulse sensitive to the
  millimetre-scale rest pose; its test uses a documented 0.06 N absolute
  band.
- `joint|case|body|index|positionx positiony angle awake` twice per case
  (body 0 chassis and body 1 wheel), then one
  `joint|case|wheel|forceX forceY torque` line per wheel joint case. Each
  case hangs a 0.5 kg wheel below a 2 kg chassis through a suspension
  anchored at the chassis bottom: `stiff` (zero hertz suspension, the car
  lands with the wheel resting on the ground and the slide free, 300
  frames), `soft` (2 Hz 0.7-damped suspension holding the chassis with a
  gravity sag, 300 frames), `brake` (zero-speed motor as a 10 N-m
  torque-clamped brake on a wheel kicked to 10 rad/s at frame thirty; the
  car rolls and settles on its tail, 300 frames). Every frame steps with
  the upstream default substep count of four. The brake case's settled
  force uses a documented 0.06 N absolute band for the same frozen-impulse
  reason as the weld fixture.
