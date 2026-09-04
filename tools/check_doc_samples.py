#!/usr/bin/env python3
"""Compile-check every ```objo code sample in the user-facing documentation.

Each fenced `objo` block in README.md, docs/GETTING_STARTED.md, docs/API.md,
and docs/DEMO.md is assembled into one generated Objo application together
with the generated distribution module, and the solution is compiled with
`objo check`. This proves the documented samples compile against the shipped
module — the same guarantee the API examples test gives the overload families.

Blocks that declare their own top-level `Class`/`Module` are emitted at file
level; every other block becomes the body of a `Sub`. Blocks may reference
ambient variables from the surrounding prose; a small curated prelude per
(file, block index) supplies those declarations. The prelude only declares
ambient state — every module API call in a sample is compiled verbatim, so a
renamed public member fails this gate.

Usage: python3 tools/check_doc_samples.py [--keep]
Exits non-zero when a block fails to compile. --keep prints the generated
solution path instead of deleting it.
"""

import re
import shutil
import subprocess
import sys
import tempfile
import uuid
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DIST = REPO / "dist" / "Physics2D.objobasic"
CLI = Path("/Users/garry/Repos/Objo/src/studio/Objo.Cli/bin/Debug/net10.0/objo")

DOC_FILES = [
    REPO / "README.md",
    REPO / "docs" / "GETTING_STARTED.md",
    REPO / "docs" / "API.md",
    REPO / "docs" / "DEMO.md",
]

# Ambient declarations for prose-embedded fragments, keyed by
# (file name, 0-based objo block index). Prelude lines are emitted verbatim
# at the start of the generated Sub, before the sample body.
PRELUDES = {
    ("DEMO.md", 0): [
        "Var mLastTicks As Double = 0.0",
        "Var mAccumulator As Double = 0.0",
        "Var mWorld As New World()",
    ],
    ("DEMO.md", 1): [
        "Var width As Double = 1200.0",
        "Var height As Double = 800.0",
        "Var worldX As Double = 0.0",
        "Var worldY As Double = 0.0",
        "Var canvasX As Double = 0.0",
        "Var canvasY As Double = 0.0",
    ],
    ("DEMO.md", 2): ["Var world As New World()"],
    ("DEMO.md", 3): ["Var world As New World()"],
    ("DEMO.md", 4): [
        "Var world As New World()",
        "Var dropDef As New BodyDefinition()",
        "dropDef.Type = BodyType.DynamicBody",
        "Var drop As Body = world.CreateBody(dropDef)",
    ],
    ("DEMO.md", 5): [
        "Var world As New World()",
        "Var bounds As New AABB()",
        "Var mCursorHits As New ShapeHitList()",
    ],
    ("GETTING_STARTED.md", 3): ["Var world As New World()"],
    ("GETTING_STARTED.md", 4): ["Var world As New World()"],
    ("GETTING_STARTED.md", 5): [
        "Var world As New World()",
        "Var boxDef As New BodyDefinition()",
        "boxDef.Type = BodyType.DynamicBody",
        "Var box As Body = world.CreateBody(boxDef)",
    ],
    ("GETTING_STARTED.md", 6): [
        "Var width As Double = 1200.0",
        "Var height As Double = 800.0",
        "Var worldX As Double = 0.0",
        "Var worldY As Double = 0.0",
        "Var canvasX As Double = 0.0",
        "Var canvasY As Double = 0.0",
    ],
    ("GETTING_STARTED.md", 7): ["Var world As New World()"],
    ("GETTING_STARTED.md", 8): [
        "Var world As New World()",
        "Var dropDef As New BodyDefinition()",
        "dropDef.Type = BodyType.DynamicBody",
        "Var drop As Body = world.CreateBody(dropDef)",
    ],
    ("GETTING_STARTED.md", 9): [
        "Var world As New World()",
        "Var cursorBounds As New AABB()",
        "Var mCursorHits As New ShapeHitList()",
    ],
}


def extract_blocks(path: Path):
    """Returns [(index, [lines])] for fenced objo blocks, skipping bash fences."""
    text = path.read_text(encoding="utf-8")
    blocks = []
    pattern = re.compile(r"```objo\n(.*?)```", re.DOTALL)
    for match in pattern.finditer(text):
        lines = match.group(1).rstrip("\n").split("\n")
        blocks.append(lines)
    return blocks


def block_is_toplevel(lines):
    return any(re.match(r"^(Class|Module)\b", line.strip()) for line in lines)


def build_source():
    sections = ["Import Physics2D", ""]
    sample_count = 0
    for doc in DOC_FILES:
        rel = doc.name
        for index, lines in enumerate(extract_blocks(doc)):
            body = [ln for ln in lines if not re.match(r"^Import\s+Physics2D\s*$", ln.strip(), re.IGNORECASE)]
            while body and not body[-1].strip():
                body.pop()
            if not body:
                continue
            if block_is_toplevel(body):
                sections.append("# " + "-" * 70)
                sections.append(f"# {rel} block {index} (top-level declarations)")
                sections.append("# " + "-" * 70)
                sections.extend(body)
                sections.append("")
                sample_count += 1
                continue
            sections.append("# " + "-" * 70)
            sections.append(f"# {rel} block {index}")
            sections.append("# " + "-" * 70)
            sections.append("## Documentation sample from the repository docs; compile-checked by tools/check_doc_samples.py.")
            sections.append(f"Sub Sample{sample_count}()")
            for prelude_line in PRELUDES.get((rel, index), []):
                sections.append(prelude_line)
            sections.extend(body)
            sections.append("End Sub")
            sections.append("")
            sample_count += 1
    sections.append("Class DocSamplesHost Inherits CommandLineApplication")
    sections.append("Event Run(args() As String) As Integer")
    sections.append("Pragma Unused args")
    sections.append("Return 0")
    sections.append("End Event")
    sections.append("End Class")
    return "\n".join(sections) + "\n", sample_count


def build_solution(tmp: Path):
    (tmp / "Shared" / "Sources").mkdir(parents=True)
    (tmp / "Projects" / "DocSamples" / "Sources").mkdir(parents=True)
    shutil.copy(DIST, tmp / "Shared" / "Sources" / "Physics2D.objobasic")

    module_id = str(uuid.uuid4())
    module_meta = {
        "Version": 4, "Name": "Physics2D", "Id": module_id, "Kind": "Module",
        "BuildScope": "Application", "Namespace": "", "Folder": "",
        "ParentModule": "", "ParentModuleId": "", "AvailableInDesigner": False,
        "CodeFile": "Physics2D.objobasic", "LayoutFile": "",
        "Notes": [], "InspectorBehaviour": [],
    }
    (tmp / "Shared" / "Sources" / "Physics2D.source.json").write_text(
        __import__("json").dumps(module_meta, indent=2) + "\n", encoding="utf-8"
    )

    source, count = build_source()
    (tmp / "Projects" / "DocSamples" / "Sources" / "DocSamples.objobasic").write_text(
        source, encoding="utf-8"
    )
    app_id = str(uuid.uuid4())
    app_meta = {
        "Version": 4, "Name": "DocSamples", "Id": app_id, "Kind": "Class",
        "BuildScope": "Application", "Namespace": "", "Folder": "",
        "ParentModule": "", "ParentModuleId": "", "AvailableInDesigner": False,
        "CodeFile": "DocSamples.objobasic", "LayoutFile": "",
        "Notes": [], "InspectorBehaviour": [],
    }
    (tmp / "Projects" / "DocSamples" / "Sources" / "DocSamples.source.json").write_text(
        __import__("json").dumps(app_meta, indent=2) + "\n", encoding="utf-8"
    )
    (tmp / "Projects" / "DocSamples" / "project.json").write_text(
        "{\n"
        '  "Version": 4,\n'
        f'  "Id": "{app_id}",\n'
        '  "Name": "DocSamples",\n'
        '  "Type": "CommandLine",\n'
        '  "BuildSettings": {\n'
        '    "AppName": "DocSamples",\n'
        '    "MajorVersion": 1,\n'
        '    "MinorVersion": 0,\n'
        '    "PatchVersion": 0,\n'
        '    "CommandLineArgs": "",\n'
        '    "TargetRids": [],\n'
        '    "WorkerProjectIds": [],\n'
        '    "DefaultWindow": ""\n'
        "  },\n"
        '  "EmptyFolders": []\n'
        "}\n",
        encoding="utf-8",
    )
    sln_id = str(uuid.uuid4())
    (tmp / "DocSamplesCheck.objosln").write_text(
        "{\n"
        '  "Version": 4,\n'
        f'  "Id": "{sln_id}",\n'
        '  "Name": "DocSamplesCheck",\n'
        '  "LastSavedStudioVersion": "26.8.6",\n'
        '  "ActiveProject": "DocSamples",\n'
        f'  "ApplicationProjectId": "{str(uuid.uuid4())}",\n'
        '  "Projects": [\n'
        "    {\n"
        f'      "Id": "{app_id}",\n'
        '      "Path": "Projects/DocSamples",\n'
        '      "Name": "DocSamples",\n'
        '      "Type": "CommandLine"\n'
        "    }\n"
        "  ],\n"
        '  "EmptyFolders": [],\n'
        '  "AiCustomInstructions": ""\n'
        "}\n",
        encoding="utf-8",
    )
    return count


def main() -> int:
    keep = "--keep" in sys.argv[1:]
    tmp = Path(tempfile.mkdtemp(prefix="physics2d-docsamples."))
    try:
        count = build_solution(tmp)
        result = subprocess.run(
            [str(CLI), "check", str(tmp / "DocSamplesCheck.objosln")],
            capture_output=True, text=True,
        )
        output = (result.stdout + result.stderr).strip()
        errors = [line for line in output.splitlines() if line.startswith("Error:")]
        warnings = [line for line in output.splitlines() if line.startswith("Warning:")]
        if result.returncode != 0 or errors:
            print(output)
            print(f"FAILED: doc samples did not compile ({count} samples)")
            print(f"solution kept at {tmp}")
            return 1
        for warning in warnings:
            print(warning)
        print(f"doc samples compile: {count} samples across {len(DOC_FILES)} documents")
        if keep:
            print(f"solution kept at {tmp}")
        else:
            shutil.rmtree(tmp)
        return 0
    finally:
        pass


if __name__ == "__main__":
    sys.exit(main())
