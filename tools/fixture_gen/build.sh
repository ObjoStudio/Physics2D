#!/bin/zsh
# Builds the golden fixture generator against the pinned Box2D checkout.
# The pinned source must be extracted separately; see docs/PORTING.md.
set -euo pipefail

REPO_ROOT="${0:A:h:h:h}"
BOX2D_SRC="${BOX2D_SRC:-/var/folders/41/jxp3r_ys705gdd3b0r12_4140000gn/T/opencode/box2d}"
OUT_DIR="$REPO_ROOT/build/tools"
mkdir -p "$OUT_DIR"
cd "$OUT_DIR"
clang -O2 -std=c11 \
  -I"$BOX2D_SRC/include" \
  "$REPO_ROOT/tools/fixture_gen/fixture_gen.c" \
  "$BOX2D_SRC"/src/*.c \
  -lm -o fixture_gen
echo "built $OUT_DIR/fixture_gen"
