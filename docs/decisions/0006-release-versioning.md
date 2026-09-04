# Decision 0006 — Release Versioning And Compatibility Record

- **Status:** Accepted
- **Date:** 2026-09-04

## Context

Stage 13 turns the finished port into a release candidate. A user who adds
`dist/Physics2D.objobasic` to a project needs two facts recorded with the
artifact itself: which version of Physics2D they installed, and which Objo
version the module requires. Objo Studio has no source-package format yet
(see decision 0002), so there is no package manifest; the record must travel
inside the module source without introducing any runtime dependency or
package-manager concept.

The version 1 API is frozen (decision 0005). Adding a version constant is an
additive, non-breaking change: it removes no member and changes no signature.

## Decision

1. **Semantic version constant.** The root module source declares
   `Const VERSION As String` following SemVer 2.0.0. The value at the Stage 13
   release candidate is `1.0.0-rc.1`; dropping the `-rc.N` suffix is the
   release act and requires explicit user direction (no publishing happens
   from this repository without it). Applications read `Physics2D.VERSION`.
2. **Generated-file compatibility record.** `tools/assemble_module.py` parses
   `Const VERSION` from the root module source (failing if missing) and
   mirrors it into the generated distribution header together with the
   minimum Objo compatibility record. The header states that the module
   requires Objo Studio 26.8.6 or newer including the issue #1302
   standard-library additions and the issue #1315 constructor-inheritance
   fix, and that `System.AllocationCount` (issue #1299) is needed only by the
   repository's test suite and benchmarks, not by the module. Parsing the
   constant keeps the header and the runtime value from drifting.
3. **No runtime dependency.** The compatibility record is documentation in a
   comment plus one string constant. Nothing reads a manifest, registry, or
   package service at build or run time.
4. **Reference document.** `tools/generate_api_docs.py` emits a "Module
   constants" section in `docs/API.md` generated from the dist declarations,
   so the reference stays generated-only and cannot drift from the shipped
   source.
5. **Compatibility updates.** Any change to the minimum Objo version follows
   `docs/PORTING.md` ("Minimum Objo Version"): update that table, the
   assembler's compatibility record, and `docs/GETTING_STARTED.md`
   together, and add the documented compatibility test when the port adopts
   a newly released standard-library member.

## Consequences

- Support diagnostics can print `Physics2D.VERSION`; the generated file
  answers "which build is this?" without opening the repository.
- The release-candidate checklist (`docs/RELEASE_CHECKLIST.md`) records the
  exact artifact hashes against this version string.
- The staged plan's "distribution" deliverable remains the single
  `.objobasic` file; an `.objopackage` may be added later per decision 0002
  when Studio ships source packages, carrying the same version string.
