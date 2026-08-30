# Golden fixtures for Physics2D.
#
# All numeric fixtures were generated from Box2D tag v3.1.1, commit
# 8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3 (see docs/PORTING.md).
#
# Generation command (from the repository root, after building the tool):
#   for f in maths hull distance raycast shapecast manifold mass scene_falling scene_pyramid scene_stack; do
#     ./build/tools/fixture_gen $f > testdata/golden/$f.txt
#   done
#
# The generator source is tools/fixture_gen/fixture_gen.c.

## Tool checksums

Generated on 2026-08-30.

| File | SHA-256 |
|---|---|
| `tools/fixture_gen/fixture_gen.c` | 42900e3c787a82343fdd3620f39f574021431e148b8c85d710c6bc7776f1bb14 |
| `testdata/golden/distance.txt` | e3308d9c1aa25d8cd5a715fd8103b28ceaa8a3964cbe513ad424ecdf4eabd26c |
| `testdata/golden/hull.txt` | 48d12525c5a1a837349d3180c74b640769f56ae78139121be649f2c45a058121 |
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
