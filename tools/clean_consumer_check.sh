#!/bin/zsh
# Clean consumer proof for the release candidate.
#
# Builds and RUNS two command-line applications in a disposable solution
# that contains only the generated distribution module:
#   1. the documented falling-box example (verbatim from README.md);
#   2. the repository's Physics2D.Smoke application (verbatim source).
# This proves a consumer can install dist/Physics2D.objobasic, write
# `Import Physics2D`, build, and simulate - with nothing else from this
# repository.
set -euo pipefail

REPO_ROOT="${0:A:h:h}"
cd "$REPO_ROOT"

run_objo() {
  if [[ -n "${OBJO:-}" ]]; then
    "$OBJO" "$@"
  elif [[ -x "$HOME/Repos/Objo/src/studio/Objo.Cli/bin/Debug/net10.0/objo" ]]; then
    "$HOME/Repos/Objo/src/studio/Objo.Cli/bin/Debug/net10.0/objo" "$@"
  elif [[ -d "$HOME/Repos/Objo/src/studio/Objo.Cli" ]]; then
    dotnet run --project "$HOME/Repos/Objo/src/studio/Objo.Cli" -- "$@"
  elif command -v objo >/dev/null 2>&1 && objo --version >/dev/null 2>&1; then
    objo "$@"
  else
    echo "no usable objo CLI: set \$OBJO or provide ~/Repos/Objo" >&2
    return 1
  fi
}

python3 tools/assemble_module.py --check

TMP_ROOT="$(mktemp -d /tmp/physics2d-consumer.XXXXXX)"
trap 'rm -rf "$TMP_ROOT"' EXIT

mkdir -p "$TMP_ROOT/Shared/Sources"
cp dist/Physics2D.objobasic "$TMP_ROOT/Shared/Sources/Physics2D.objobasic"

MODULE_ID="$(uuidgen | tr 'A-Z' 'a-z')"
cd "$TMP_ROOT"
python3 - "$MODULE_ID" <<'PYEOF'
import json, sys
meta = {
    "Version": 4, "Name": "Physics2D", "Id": sys.argv[1], "Kind": "Module",
    "BuildScope": "Application", "Namespace": "", "Folder": "",
    "ParentModule": "", "ParentModuleId": "", "AvailableInDesigner": False,
    "CodeFile": "Physics2D.objobasic", "LayoutFile": "",
    "Notes": [], "InspectorBehaviour": [],
}
open("Shared/Sources/Physics2D.source.json", "w").write(json.dumps(meta, indent=2) + "\n")
PYEOF

write_class_sidecar() {
  python3 - "$1" "$2" "$3" "$4" <<'PYEOF'
import json, sys
name, guid, codefile = sys.argv[1], sys.argv[2], sys.argv[3]
meta = {
    "Version": 4, "Name": name, "Id": guid, "Kind": "Class",
    "BuildScope": "Application", "Namespace": "", "Folder": "",
    "ParentModule": "", "ParentModuleId": "", "AvailableInDesigner": False,
    "CodeFile": codefile, "LayoutFile": "",
    "Notes": [], "InspectorBehaviour": [],
}
open(sys.argv[4], "w").write(json.dumps(meta, indent=2) + "\n")
PYEOF
}

write_project() {
  python3 - "$1" "$2" "$3" <<'PYEOF'
import json, sys
guid, name, path = sys.argv[1], sys.argv[2], sys.argv[3]
project = {
    "Version": 4, "Id": guid, "Name": name, "Type": "CommandLine",
    "BuildSettings": {
        "AppName": name, "MajorVersion": 1, "MinorVersion": 0, "PatchVersion": 0,
        "CommandLineArgs": "", "TargetRids": [], "WorkerProjectIds": [],
        "DefaultWindow": "",
    },
    "EmptyFolders": [],
}
open(f"{path}/project.json", "w").write(json.dumps(project, indent=2) + "\n")
PYEOF
}

# --- Project 1: the documented falling-box example -------------------------
FALLING_ID="$(uuidgen | tr 'A-Z' 'a-z')"
mkdir -p "$TMP_ROOT/Projects/FallingBox/Sources"
cat > "$TMP_ROOT/Projects/FallingBox/Sources/App-$FALLING_ID.objobasic" <<'EOF'
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
EOF
write_class_sidecar "App" "$FALLING_ID" "App-$FALLING_ID.objobasic" \
  "$TMP_ROOT/Projects/FallingBox/Sources/App-$FALLING_ID.source.json"
write_project "$FALLING_ID" "FallingBox" "$TMP_ROOT/Projects/FallingBox"

# --- Project 2: the repository Smoke application ---------------------------
SMOKE_ID="$(uuidgen | tr 'A-Z' 'a-z')"
SMOKE_SRC="$REPO_ROOT/Projects/Physics2D.Smoke/Sources/App-ffde86c2.objobasic"
mkdir -p "$TMP_ROOT/Projects/Smoke/Sources"
cp "$SMOKE_SRC" "$TMP_ROOT/Projects/Smoke/Sources/App-$SMOKE_ID.objobasic"
write_class_sidecar "App" "$SMOKE_ID" "App-$SMOKE_ID.objobasic" \
  "$TMP_ROOT/Projects/Smoke/Sources/App-$SMOKE_ID.source.json"
write_project "$SMOKE_ID" "Smoke" "$TMP_ROOT/Projects/Smoke"

# --- Solution --------------------------------------------------------------
SOLUTION_ID="$(uuidgen | tr 'A-Z' 'a-z')"
cat > "$TMP_ROOT/Physics2D.ConsumerCheck.objosln" <<EOF
{
  "Version": 4,
  "Id": "$SOLUTION_ID",
  "Name": "Physics2D.ConsumerCheck",
  "LastSavedStudioVersion": "26.8.6",
  "ActiveProject": "FallingBox",
  "ApplicationProjectId": "$(uuidgen | tr 'A-Z' 'a-z')",
  "Projects": [
    {"Id": "$FALLING_ID", "Path": "Projects/FallingBox", "Name": "FallingBox", "Type": "CommandLine"},
    {"Id": "$SMOKE_ID", "Path": "Projects/Smoke", "Name": "Smoke", "Type": "CommandLine"}
  ],
  "EmptyFolders": [],
  "AiCustomInstructions": ""
}
EOF

echo "== building FallingBox =="
run_objo build "$TMP_ROOT/Physics2D.ConsumerCheck.objosln" --project FallingBox --output "$TMP_ROOT/build/fallingbox" | tail -2
echo "== building Smoke =="
run_objo build "$TMP_ROOT/Physics2D.ConsumerCheck.objosln" --project Smoke --output "$TMP_ROOT/build/smoke" | tail -2

FALLING_BIN=$(find "$TMP_ROOT/build/fallingbox" -type f -name "FallingBox" -perm +111 | head -1)
SMOKE_BIN=$(find "$TMP_ROOT/build/smoke" -type f -name "Smoke" -perm +111 | head -1)
[[ -n "$FALLING_BIN" ]] || { echo "FallingBox binary not found" >&2; exit 1; }
[[ -n "$SMOKE_BIN" ]] || { echo "Smoke binary not found" >&2; exit 1; }

echo "== running FallingBox =="
"$FALLING_BIN"
echo "== running Smoke =="
"$SMOKE_BIN"

echo "clean consumer check passed"
