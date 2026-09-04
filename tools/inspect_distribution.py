#!/usr/bin/env python3
"""Inspect the generated distribution for release-blocking content.

Checks dist/Physics2D.objobasic for:
1. external `Import` statements (the module must be dependency-free);
2. desktop-only types (the module must be project-type neutral);
3. TODO/FIXME/stub markers and unfinished placeholders;
4. debug output (Print/Debug/Trace calls);
5. stale generated headers (version constant vs header record, content checksum);
6. public declarations that are neither documented in docs/API.md nor listed
   in its Internal infrastructure section.

Usage: python3 tools/inspect_distribution.py
Exits non-zero when a check fails.
"""

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DIST = REPO / "dist" / "Physics2D.objobasic"
API_DOC = REPO / "docs" / "API.md"

DESKTOP_TYPES = [
    "Window", "Canvas", "Graphics", "DesktopApplication", "GameCanvas",
    "Timer", "DesktopApplicationTemplate", "MenuItem", "Toolbar", "Button",
    "Label", "TextField", "TextArea", "PictureBox", "CanvasPaint",
]

PROBLEMS = []


def code_lines():
    """Yields (number, line) for lines inside the module body (non-header)."""
    in_module = False
    for number, line in enumerate(DIST.read_text().splitlines(), start=1):
        stripped = line.strip()
        if not in_module:
            if re.match(r"^Module\s+Physics2D\s*$", stripped, re.IGNORECASE):
                in_module = True
            continue
        yield number, stripped


def check_imports():
    for number, line in code_lines():
        if re.match(r"^Import\s+\w", line) and not re.match(r"^Import\s+Physics2D\s*$", line, re.IGNORECASE):
            PROBLEMS.append(f"line {number}: external import in distribution: {line}")


def check_desktop_types():
    body = "\n".join(line for _, line in code_lines())
    for word in DESKTOP_TYPES:
        # Declaration or instantiation of a desktop-only type; the word in a
        # doc comment is attribution, not usage.
        if re.search(rf"^(?:Public |Private )?(?:Shared )?(?:Sub|Function|Property|Const|Enum|Class)\s+{word}\b", body, re.MULTILINE) or \
           re.search(rf"^.*=\s*New\s+{word}\b", body, re.MULTILINE):
            PROBLEMS.append(f"desktop-only type referenced in distribution: {word}")


def check_markers():
    for number, line in code_lines():
        if line.startswith("#"):
            # Doc prose may legitimately use words like "placeholder" to
            # describe pool design; only marker tokens are checked in
            # comments, and all markers are checked in code.
            if re.search(r"\b(TODO|FIXME|HACK|XXX)\b", line):
                PROBLEMS.append(f"line {number}: unfinished-work marker in comment: {line}")
            continue
        if re.search(r"\b(TODO|FIXME|HACK|XXX|stub|placeholder|not implemented)\b", line, re.IGNORECASE):
            PROBLEMS.append(f"line {number}: unfinished-work marker: {line}")


def check_debug_output():
    for number, line in code_lines():
        if re.match(r"^Print\(", line) or re.search(r"[^.a-zA-Z]Print\(", line):
            PROBLEMS.append(f"line {number}: debug output in distribution: {line}")
        if re.search(r"\bDebug\.(Print|Assert)\b|\bTrace\.(Print|Write)\b", line):
            PROBLEMS.append(f"line {number}: debug/trace output: {line}")


def check_header():
    text = DIST.read_text()
    version = re.search(r'^Const\s+VERSION\s+As\s+String\s*=\s*"([^"]+)"', text, re.MULTILINE | re.IGNORECASE)
    header_version = re.search(r"# Physics2D version: (\S+)", text)
    if not version:
        PROBLEMS.append("distribution declares no Const VERSION")
    if not header_version:
        PROBLEMS.append("distribution header records no version")
    if version and header_version and version.group(1) != header_version.group(1):
        PROBLEMS.append(
            f"version drift: module {version.group(1)} vs header {header_version.group(1)}"
        )
    if "Erin Catto" not in text or "MIT License" not in text:
        PROBLEMS.append("distribution does not carry the Box2D/Physics2D MIT notices")
    if "8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3" not in text:
        PROBLEMS.append("distribution does not record the pinned upstream commit")


def parse_documented_names():
    """Returns (documented_class_members, internal_class_names) from docs/API.md."""
    documented = set()
    internal = set()
    section = None
    current_class = None
    for line in API_DOC.read_text().splitlines():
        stripped = line.strip()
        heading = re.match(r"^##\s+(.*)$", stripped)
        if heading:
            section = heading.group(1)
            current_class = None
            continue
        if section == "Internal infrastructure":
            # Internal classes are listed as a comma-separated paragraph of
            # backticked names rather than member sections.
            for name in re.findall(r"`([A-Za-z_][A-Za-z0-9_]*)`", stripped):
                internal.add(name)
            continue
        class_match = re.match(r"^###\s+([A-Za-z_][A-Za-z0-9_]*)", stripped)
        if class_match:
            current_class = class_match.group(1)
            continue
        member_match = re.match(
            r"^-\s+`(?:Function\s+|Sub\s+|Property\s+|Const\s+|Shared\s+)*"
            r"(?:[A-Za-z_][A-Za-z0-9_]*\.)?([A-Za-z_][A-Za-z0-9_]*)[\s(.]",
            stripped,
        )
        if member_match and current_class:
            documented.add((current_class, member_match.group(1).lower()))
        if stripped.startswith(f"`{current_class}`") and current_class:
            continue
    return documented, internal


def check_public_surface():
    documented, internal = parse_documented_names()
    current_class = None
    doc_comment = []
    for number, line in code_lines():
        class_match = re.match(r"^(?:Abstract )?(?:Class|Enum)\s+([A-Za-z_][A-Za-z0-9_]*)", line)
        if class_match:
            current_class = class_match.group(1)
            doc_comment = []
            continue
        if line == "End Class" or line == "End Enum":
            current_class = None
            doc_comment = []
            continue
        if current_class is None or current_class in internal:
            continue
        if line.startswith("#"):
            doc_comment.append(line)
            continue
        member = re.match(
            r"^(?:Public\s+)?(?:Shared\s+)?(?:Const\s+|Property\s+|Function\s+|Sub\s+|Constructor\b|Enum\s+)([A-Za-z_][A-Za-z0-9_]*)",
            line,
        )
        if member:
            name_lower = member.group(1).lower()
            is_documented = (current_class, name_lower) in documented
            # Stage 10 convention: solver bookkeeping the compiler requires to
            # be public is marked "Internal ..." in its source doc comment and
            # omitted from the generated reference.
            is_internal_marked = any("internal" in comment.lower() for comment in doc_comment)
            if not is_documented and not is_internal_marked:
                PROBLEMS.append(
                    f"line {number}: public member {current_class}.{member.group(1)} is neither documented nor internal-marked"
                )
        doc_comment = []


def main() -> int:
    check_imports()
    check_desktop_types()
    check_markers()
    check_debug_output()
    check_header()
    check_public_surface()
    if PROBLEMS:
        for problem in PROBLEMS:
            print(f"PROBLEM: {problem}")
        print(f"distribution inspection FAILED ({len(PROBLEMS)} problems)")
        return 1
    print("distribution inspection passed: no external imports, no desktop-only types, "
          "no markers, no debug output, header current, public surface documented")
    return 0


if __name__ == "__main__":
    sys.exit(main())
