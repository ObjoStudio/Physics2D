#!/usr/bin/env python3
"""Deterministic Physics2D module assembler.

Reads the canonical Shared Code source graph and emits one explicit
``Module Physics2D ... End Module`` source file to dist/Physics2D.objobasic.

Design (see docs/decisions/0002-module-distribution.md):
- the root module source plus every nested source item, in a stable
  topological order (parents before children, children sorted by name), is
  concatenated into one module block;
- line endings are normalised to LF and the file ends with exactly one newline;
- the header records provenance and a content checksum of all input sources;
- declarations outside the root module, duplicate names, and unresolved parent
  references are rejected;
- output is byte-identical for an unchanged checkout.

Usage:
  tools/assemble_module.py            # regenerate dist/Physics2D.objobasic
  tools/assemble_module.py --check    # fail if the committed file is stale
"""

import hashlib
import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SHARED_SOURCES = REPO_ROOT / "Shared" / "Sources"
OUTPUT = REPO_ROOT / "dist" / "Physics2D.objobasic"

MODULE_NAME = "Physics2D"


def load_sources():
    sources = []
    for meta_path in sorted(SHARED_SOURCES.glob("*.source.json")):
        meta = json.loads(meta_path.read_text(encoding="utf-8"))
        code_path = SHARED_SOURCES / meta["CodeFile"]
        if not code_path.exists():
            raise SystemExit(f"missing code file for source {meta['Name']}: {code_path}")
        code = code_path.read_text(encoding="utf-8")
        sources.append(
            {
                "id": meta["Id"],
                "name": meta["Name"],
                "kind": meta["Kind"],
                "parent_module_id": meta.get("ParentModuleId", ""),
                "parent_module": meta.get("ParentModule", ""),
                "code": code,
                "path": code_path,
            }
        )
    return sources


def normalise(code: str) -> str:
    lines = code.replace("\r\n", "\n").replace("\r", "\n").split("\n")
    # Studio stores member bodies without leading whitespace; trim each line so
    # authoring slips do not change the distribution.
    return "\n".join(line.strip() for line in lines).strip("\n")


def strip_module_wrapper(code: str, name: str) -> str:
    """Removes the ``Module <name>``/``End Module`` wrapper from the root source."""
    text = normalise(code)
    pattern = re.compile(
        rf"^Module\s+{re.escape(name)}\s*$", re.IGNORECASE | re.MULTILINE
    )
    match = pattern.search(text)
    if not match:
        raise SystemExit(f"root source does not declare Module {name}")
    body = text[match.end():]
    end_match = None
    for end in re.finditer(r"^End Module\s*$", body, re.IGNORECASE | re.MULTILINE):
        end_match = end
    if end_match is None:
        raise SystemExit(f"root source does not close Module {name}")
    return body[: end_match.start()].strip("\n")


def check_no_module_blocks(body: str, source_name: str) -> None:
    if re.search(r"^\s*Module\s+\w+", body, re.MULTILINE | re.IGNORECASE):
        raise SystemExit(
            f"source '{source_name}' declares a Module block; nested items must be plain declarations"
        )


def main() -> int:
    check_only = "--check" in sys.argv[1:]
    sources = load_sources()
    if not sources:
        raise SystemExit("no Shared Code sources found")

    by_id = {}
    roots = []
    for source in sources:
        if source["id"] in by_id:
            raise SystemExit(f"duplicate source id {source['id']}")
        by_id[source["id"]] = source
        if not source["parent_module_id"] and not source["parent_module"]:
            roots.append(source)

    if len(roots) != 1:
        raise SystemExit(f"expected exactly one root module source, found {len(roots)}")
    root = roots[0]
    if root["kind"].lower() != "module" or root["name"] != MODULE_NAME:
        raise SystemExit(f"root source must be Module {MODULE_NAME}")

    # Gather children per parent id (parents must exist by id).
    children_by_parent = {}
    for source in sources:
        if source is root:
            continue
        parent_id = source["parent_module_id"]
        if not parent_id:
            raise SystemExit(
                f"source '{source['name']}' must declare ParentModuleId (the {MODULE_NAME} module id)"
            )
        if parent_id not in by_id:
            raise SystemExit(
                f"source '{source['name']}' references unknown parent module id {parent_id}"
            )
        parent = by_id[parent_id]
        if parent["kind"].lower() != "module":
            raise SystemExit(f"parent of '{source['name']}' is not a module")
        children_by_parent.setdefault(parent_id, []).append(source)

    # Deterministic content checksum over all inputs.
    hasher = hashlib.sha256()
    for source in sorted(sources, key=lambda s: (s["name"], s["id"])):
        hasher.update(normalise(source["code"]).encode("utf-8"))
        hasher.update(b"\x00")
    content_hash = hasher.hexdigest()

    # Per-input hashes let the test suite detect a stale distribution without
    # reproducing the assembly logic in Objo.
    input_lines = []
    for source in sorted(sources, key=lambda s: s["path"].name):
        source_hash = hashlib.sha256(normalise(source["code"]).encode("utf-8")).hexdigest()
        input_lines.append(f"# Input: {source['path'].name} {source_hash}")

    # Assemble: root body, then nested items sorted by name before End Module.
    root_body = strip_module_wrapper(root["code"], root["name"])
    flat = []
    for source in sources:
        if source is root:
            continue
        body = normalise(source["code"])
        check_no_module_blocks(body, source["name"])
        flat.append((source["name"], body))

    nested = "\n\n".join(body for _, body in sorted(flat, key=lambda item: item[0]))

    header = (
        "# Physics2D — generated single-file distribution.\n"
        "#\n"
        "# This file is generated by tools/assemble_module.py from Shared/Sources.\n"
        "# Do not edit by hand: changes will be overwritten and will desynchronise\n"
        "# the generated module from its canonical source.\n"
        "#\n"
        + "\n".join(input_lines)
        + "\n#\n"
        f"# Source content checksum (SHA-256): {content_hash}\n"
        "# Upstream algorithms: Box2D v3.1.1, commit\n"
        "# 8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3 (MIT licence).\n"
        "# See THIRD_PARTY_NOTICES.md and docs/PORTING.md.\n"
    )

    assembled = (
        header
        + "\nModule "
        + MODULE_NAME
        + "\n"
        + root_body
        + ("\n\n" + nested if nested else "")
        + "\nEnd Module\n"
    )

    if check_only:
        if not OUTPUT.exists():
            print(f"STALE: {OUTPUT} does not exist")
            return 1
        current = OUTPUT.read_text(encoding="utf-8")
        recorded = re.search(r"Source content checksum \(SHA-256\): ([0-9a-f]+)", current)
        if recorded is None or recorded.group(1) != content_hash:
            print("STALE: distribution checksum does not match Shared Code")
            return 1
        if current != assembled:
            print("STALE: distribution content differs from regenerated output")
            return 1
        print("dist/Physics2D.objobasic is current")
        return 0

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(assembled, encoding="utf-8")
    print(f"wrote {OUTPUT} ({len(assembled.encode('utf-8'))} bytes, checksum {content_hash})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
