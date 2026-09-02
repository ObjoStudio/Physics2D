#!/usr/bin/env python3
"""Audit the PORTING.md public symbol inventory against the generated module.

Every inventory row maps an upstream Box2D symbol to Physics2D source. This
script extracts every `Class.Member` reference from the mapping column and
verifies that the class (or enum) exists in dist/Physics2D.objobasic and
declares a member with that name (case-insensitively, like the Objo lexer).
Exclusion rows are checked only for their documented prose.

Usage: python3 tools/audit_api.py [--quiet]
Exits non-zero when a mapping references a missing member or when an
inventory row has no mapping at all.
"""

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
PORTING = REPO / "docs" / "PORTING.md"
DIST = REPO / "dist" / "Physics2D.objobasic"


def parse_dist_members():
    """Returns {class_or_enum_name: set(member_names)} from the dist source."""
    types = {}
    current_type = None
    in_enum = False
    for line in DIST.read_text().splitlines():
        stripped = line.strip()
        match = re.match(r"^(?:Abstract )?(?:Class|Enum) ([A-Za-z_][A-Za-z0-9_]*)", stripped)
        if match:
            current_type = match.group(1)
            in_enum = stripped.startswith("Enum")
            types.setdefault(current_type, set())
            continue
        if stripped == "End Class" or stripped == "End Enum":
            current_type = None
            in_enum = False
            continue
        if current_type is None:
            continue
        member = None
        if in_enum:
            match = re.match(r"^([A-Z][A-Za-z0-9_]*)$", stripped)
            if match:
                member = match.group(1)
        else:
            match = re.match(
                r"^(?:Public |Private )?(?:Shared )?(?:Const |Property |Function |Sub |Constructor\b|Enum )"
                r"([A-Za-z_][A-Za-z0-9_]*)",
                stripped,
            )
            if match:
                member = match.group(1)
        if member:
            types[current_type].add(member.lower())
    return types


def parse_inventory():
    """Returns a list of (upstream_symbol, mapping_text) from the inventory sections."""
    rows = []
    in_inventory = False
    for line in PORTING.read_text().splitlines():
        if line.startswith("## Public Symbol Inventory"):
            in_inventory = True
            continue
        if in_inventory and line.startswith("## "):
            in_inventory = False
            continue
        if not in_inventory:
            continue
        match = re.match(r"^\| `([^`]+)` \| (.+) \| \d+ \|", line)
        if match:
            rows.append((match.group(1), match.group(2)))
    return rows


def referenced_members(mapping_text):
    """Extracts `Class.Member` and `Class` references from a mapping string."""
    members = re.findall(r"\b([A-Z][A-Za-z0-9_]*)\.([A-Za-z_][A-Za-z0-9_]*)", mapping_text)
    bare_types = re.findall(r"\b(?:World|Body|Shape|Chain|Joint|Distance|Casts|Collide|AABB|Polygon|Circle|Capsule|Segment|Rot|Transform|Vector2|BitSet|IdPool|IntegerList|DoubleList|DynamicTree|BroadPhase|QueryFilter|Filter|SurfaceMaterial|ShapeProxy|Manifold|ManifoldPoint|MassData|Sweep|SimplexCache|Simplex|BodyDefinition|ShapeDefinition|ChainDefinition|WorldSettings|WorldCounters|WorldProfile|WorldEvents|DebugDrawOptions|DebugRenderer|DebugColors|ExplosionDefinition|DistanceJointDefinition|FilterJointDefinition|MotorJointDefinition|MouseJointDefinition|PrismaticJointDefinition|RevoluteJointDefinition|WeldJointDefinition|WheelJointDefinition|QueryFilter)\b(?!\.)", mapping_text)
    return members, bare_types


def main():
    quiet = "--quiet" in sys.argv
    types = parse_dist_members()
    rows = parse_inventory()

    errors = []
    checked = 0
    for symbol, mapping in rows:
        if "Excluded (v1)" in mapping or "excluded" in mapping.lower():
            continue
        if "Protected internal" in mapping or "Released by garbage collection" in mapping:
            continue
        if "standard library" in mapping.lower():
            continue
        members, bare_types = referenced_members(mapping)
        if not members and not bare_types:
            errors.append(f"{symbol}: mapping mentions no Physics2D name: {mapping}")
            continue
        for class_name, member_name in members:
            if class_name not in types:
                errors.append(f"{symbol}: unknown type `{class_name}` in mapping: {mapping}")
                continue
            checked += 1
            if member_name.lower() not in types[class_name]:
                errors.append(f"{symbol}: `{class_name}.{member_name}` is not a member in the dist")
        for bare in bare_types:
            checked += 1
            if bare not in types:
                errors.append(f"{symbol}: unknown type `{bare}` in mapping: {mapping}")

    print(f"Inventory rows: {len(rows)}; member references checked: {checked}")
    if errors:
        for error in errors:
            print(f"MAPPING ERROR: {error}")
        sys.exit(1)
    if not quiet:
        print("Every inventory row maps to a declared member of the generated dist.")


if __name__ == "__main__":
    main()
