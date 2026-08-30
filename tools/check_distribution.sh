#!/bin/zsh
# Clean-room distribution check.
#
# Regenerates the single-file module distribution and compiles a disposable
# solution that contains only the distribution plus a smoke application. This
# proves a consumer project can use the distribution without the canonical
# Shared Code sources.
set -euo pipefail

REPO_ROOT="${0:A:h:h}"
cd "$REPO_ROOT"

# Resolve the objo CLI: $OBJO if set, the in-development Objo checkout (the
# engine source of truth), then an installed objo as a last resort.
run_objo() {
  if [[ -n "${OBJO:-}" ]]; then
    "$OBJO" "$@"
  elif [[ -d "$HOME/Repos/Objo/src/studio/Objo.Cli" ]]; then
    dotnet run --project "$HOME/Repos/Objo/src/studio/Objo.Cli" -- "$@"
  elif command -v objo >/dev/null 2>&1 && objo --version >/dev/null 2>&1; then
    objo "$@"
  else
    echo "no usable objo CLI: set \$OBJO or provide ~/Repos/Objo" >&2
    return 1
  fi
}

python3 tools/assemble_module.py

TMP_ROOT="$(mktemp -d /tmp/physics2d-dist-check.XXXXXX)"
trap 'rm -rf "$TMP_ROOT"' EXIT

mkdir -p "$TMP_ROOT/Shared/Sources" "$TMP_ROOT/Projects/Smoke/Sources"
cp dist/Physics2D.objobasic "$TMP_ROOT/Shared/Sources/Physics2D.objobasic"

(
cd "$TMP_ROOT"

MODULE_ID="$(uuidgen | tr 'A-Z' 'a-z')"
APP_ID="$(uuidgen | tr 'A-Z' 'a-z')"
SOLUTION_ID="$(uuidgen | tr 'A-Z' 'a-z')"
MODULE_ID8="${MODULE_ID:0:8}"
APP_ID8="${APP_ID:0:8}"

python3 - "$MODULE_ID" "$MODULE_ID8" <<'PYEOF'
import json, sys
guid, id8 = sys.argv[1], sys.argv[2]
meta = {
    "Version": 4, "Name": "Physics2D", "Id": guid, "Kind": "Module",
    "BuildScope": "Application", "Namespace": "", "Folder": "",
    "ParentModule": "", "ParentModuleId": "", "AvailableInDesigner": False,
    "CodeFile": f"Physics2D-{id8}.objobasic", "LayoutFile": "",
    "Notes": [], "InspectorBehaviour": [],
}
open(f"Shared/Sources/Physics2D-{id8}.source.json", "w").write(json.dumps(meta, indent=2) + "\n")
PYEOF

mv "$TMP_ROOT/Shared/Sources/Physics2D.objobasic" "$TMP_ROOT/Shared/Sources/Physics2D-$MODULE_ID8.objobasic"

cat > "$TMP_ROOT/Projects/Smoke/Sources/App-$APP_ID8.objobasic" <<'EOF'
Import Physics2D

Class App Inherits CommandLineApplication
Event Run(args() As String) As Integer
Pragma Unused args
Print("Physics2D distribution imports and compiles.")
Return 0
End Event
End Class
EOF

python3 - "$APP_ID" "$APP_ID8" <<'PYEOF'
import json, sys
guid, id8 = sys.argv[1], sys.argv[2]
meta = {
    "Version": 4, "Name": "App", "Id": guid, "Kind": "Class",
    "BuildScope": "Application", "Namespace": "", "Folder": "",
    "ParentModule": "", "ParentModuleId": "", "AvailableInDesigner": False,
    "CodeFile": f"App-{id8}.objobasic", "LayoutFile": "",
    "Notes": [], "InspectorBehaviour": [],
}
open(f"Projects/Smoke/Sources/App-{id8}.source.json", "w").write(json.dumps(meta, indent=2) + "\n")
PYEOF

cat > "$TMP_ROOT/Projects/Smoke/project.json" <<EOF
{
  "Version": 4,
  "Id": "$APP_ID",
  "Name": "Smoke",
  "Type": "CommandLine",
  "BuildSettings": {
    "AppName": "Smoke",
    "MajorVersion": 1,
    "MinorVersion": 0,
    "PatchVersion": 0,
    "CommandLineArgs": "",
    "TargetRids": [],
    "WorkerProjectIds": [],
    "DefaultWindow": ""
  },
  "EmptyFolders": []
}
EOF

cat > "$TMP_ROOT/Physics2D.DistCheck.objosln" <<EOF
{
  "Version": 4,
  "Id": "$SOLUTION_ID",
  "Name": "Physics2D.DistCheck",
  "LastSavedStudioVersion": "26.8.6",
  "ActiveProject": "Smoke",
  "ApplicationProjectId": "$(uuidgen | tr 'A-Z' 'a-z')",
  "Projects": [
    {
      "Id": "$APP_ID",
      "Path": "Projects/Smoke",
      "Name": "Smoke",
      "Type": "CommandLine"
    }
  ],
  "EmptyFolders": [],
  "AiCustomInstructions": ""
}
EOF

run_objo check "$TMP_ROOT/Physics2D.DistCheck.objosln" --project Smoke 2>&1 | tail -1
echo "clean-room distribution check passed"
)
