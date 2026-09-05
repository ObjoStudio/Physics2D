# Release Candidate Checklist — Physics2D 1.0.0-rc.1

This checklist records the Stage 13 release-candidate gates and the exact
artifact hashes. Every gate below was executed from a clean checkout at the
recorded commit; raw evidence lives in the files referenced. Publishing
(pushes, releases, registry changes) is deliberately out of scope and requires
explicit user direction.

## Artifact identity

| Field | Value |
|---|---|
| Module version (`Physics2D.VERSION`) | `1.0.0-rc.1` (decision 0006) |
| Pinned upstream | Box2D `v3.1.1`, commit `8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3` |
| Minimum Objo | Studio 26.8.6 + issue #1302 standard library + issue #1315 engine fix (see `docs/PORTING.md`) |
| Reference machine | 16-inch MacBook Pro (Apple Silicon), Objo CLI 26.9.1 built from the in-development checkout |

## Artifact hashes

Recorded at the release-candidate commit (see git log for the exact SHA):

| Artifact | SHA-256 |
|---|---|
| `dist/Physics2D.objobasic` | `6784d0cfadb6247f056367ef1ed796e43ff3303ac8144c888d107cd77fb04f0e` |
| `docs/API.md` | `e9b81a7885256e2f0b2bff791a8d9b67b75629f86f0a306a2e4cc42e8a7412f4` |
| `LICENSE` | `6e390fc47ac4d60c9dee67d5e59f5017f1c6a61be739e09a7c9214bd25a21f44` |
| `THIRD_PARTY_NOTICES.md` | `4cafe06b04527015e676b821cc0de7b93acbfa5d2b4f6cb22b4327b3ebf19c40` |
| `benchmarks/results/stage13-release-candidate-2026-09-04T22-59-11.json` | `5f53bf7465283ef3f1906bd7e8a544e97ee3ac5f3f5c88d33317dfb428c810d5` |

Distribution determinism: `python3 tools/assemble_module.py` was run three
times on the same checkout; all runs produced byte-identical output (one
distinct SHA-256 across the runs).

## Gates

| Gate | Result | Evidence |
|---|---|---|
| Canonical solution compiles (`objo check`) | Pass | zero warnings |
| Full test suite from clean checkout | Pass | 337 Physics2D tests + 19 benchmark tests, 0 failed/0 skipped |
| Golden fixtures regenerate byte-identically | Pass | regenerated from a fresh extraction of the pinned upstream commit; `diff -r` clean |
| Distribution current and deterministic | Pass | `assemble_module.py --check`; three regenerations, one hash |
| API inventory audit | Pass | `tools/audit_api.py`: 329 rows, 241 member references, zero errors |
| Doc samples compile | Pass | `tools/check_doc_samples.py`: 17 samples across README, GETTING_STARTED, API, DEMO |
| Clean consumer build + run (dist only) | Pass | `tools/clean_consumer_check.sh`: documented falling-box example and the Smoke application built and run from a solution containing only `dist/Physics2D.objobasic` |
| Distribution inspection | Pass | `tools/inspect_distribution.py`: no external imports, desktop-only types, TODO markers, debug output, or undocumented public members |
| Release benchmark suite from clean checkout | Pass | `benchmarks/results/stage13-release-candidate-2026-09-04T22-59-11.json`: all 40 scenarios, every checksum bit-for-bit identical to the accepted Stage 12 record; median deltas within the identical-code drift band (the two visible outliers, `stage12-bullet-ccd-24` +26% and `stage12-tumbler-create-destroy` +6.3%, both sit inside the 377–476 ms and 510–618 ms spreads those scenarios showed across four identical-code runs — drift, not regression) |
| Demo smoke and `--soak` gate | Pass | full 30 simulated minutes (108,000 fixed steps), exit 0 after 13m25s wall under occlusion throttling, no runtime errors logged |
| §22 final autonomous audit | Pass | see the Stage 13 ledger row and the audit notes below; the one item requiring an unlocked session is the demo manual interaction pass, recorded as pending user verification |

## Final autonomous audit (§22) evidence

1. **Public API vs Stage 0 inventory** — `tools/audit_api.py` (329 rows, 241
   member references, zero errors) verifies inventory → dist; the public
   surface check in `tools/inspect_distribution.py` verifies the reverse
   direction (every public member documented or internal-marked).
2. **Leakage sweeps** — no `TODO`/`FIXME`/`HACK`/`XXX` in code; `b2*`
   appears only in comments as upstream attribution; no empty `Catch`
   blocks; no debug output in the dist; no skipped tests (0 skipped in the
   full suite); expected values come from golden fixtures and upstream
   probes, not hard-coded results.
3. **Hot-path allocation sweep** — no allocating `New Vector2`/`Transform`/
   `Rot` constructions, `RemoveAt`, dictionaries, or iteration allocations in
   solver, contact, broad-phase, or tree sources; the only hits are
   documented cold paths (debug draw, explosion setup, construction). The
   runtime zero-allocation gates measure the actual step paths.
4. **Post-stress validation** — the longest seeded stress scenarios
   (`TestGenerationalPoolSeededStress` 11.09 s, `TestMatchesScanListReference`
   13.46 s, `TestIdPoolSeededStress` 7.03 s) and every tree, island, contact,
   and joint invariant test pass in the clean-checkout suite.
5. **Suite twice in different order** — `objo test` has no shuffle/order
   option (checked against the CLI). The suite ran twice in fresh processes
   (pre-fix and final clean-checkout runs, 336 then 337 tests) with
   identical results apart from the added version test.
6. **Fresh-process checksums** — all 40 benchmark scenario checksums
   bit-for-bit identical to the accepted Stage 12 record.
7. **Complete Release benchmark comparison** — see the gate table above.
8. **Golden fixture regeneration** — regenerated from a fresh extraction of
   the pinned commit; byte-identical to the committed fixtures.
9. **Double distribution regeneration** — three regenerations, one SHA-256.
10. **New consumer from documentation alone** — command-line: the documented
    falling-box example (verbatim from README) plus the Smoke application,
    built and run from a dist-only solution
    (`tools/clean_consumer_check.sh`); desktop: `MiniDesktop`, written from
    `docs/GETTING_STARTED.md` and `docs/API.md` alone, built on the first
    attempt and smoke-run against the distribution module.
11. **Demo scene cycling + manual pass** — the automated `--soak` gate cycled
    all eight scenes for 30 simulated minutes with exit 0 and no errors. The
    manual interaction pass requires an unlocked desktop session; the
    machine was locked during this audit, so that verification remains for
    the user. Stage 11's orientation pass (screenshots of all eight scenes and
    the interaction controls) is the last recorded manual verification.
12. **Teaching-quality documentation review** — README, GETTING_STARTED,
    CONTRIBUTING written this stage; API/ARCHITECTURE/DEMO/PERFORMANCE/
    PORTING reviewed and corrected (stale `World.Step` name, stale diagram
    note, missing `ContactManifoldPool`, two non-compiling fragments, bare
    `Min`); all 17 doc samples now compile mechanically.
13. **`Import Physics2D` requirement** — both consumer solutions contain
    only the distribution module and standard Objo imports; no other
    component of this repository is referenced.

## Single-source installation record

Objo Studio (as of the development checkout this release candidate was built
against) does not ship a redistributable source-package format: the
`SolutionStorageFormat.Package` mode is a directory-based solution storage
format, not an importable module package, and no `.objopackage` exists in the
Objo source. The distribution method therefore remains the single generated
`dist/Physics2D.objobasic` module source (decision 0002), installed by adding
it to a project as Shared Code. An `.objopackage` artifact may be added in a
future release once Studio ships source packages.

## Before tagging a release

1. Confirm the version suffix: dropping `-rc.N` from `Const VERSION` in the
   root module source is the release act (decision 0006); regenerate the
   distribution and re-record hashes.
2. Re-run `python3 tools/assemble_module.py --check`,
   `python3 tools/audit_api.py`, `python3 tools/inspect_distribution.py`,
   `python3 tools/check_doc_samples.py`, and `tools/check_distribution.sh`.
3. Re-run the full test suite and the Release benchmark suite; confirm every
   scenario checksum against the accepted record.
4. Publishing requires explicit user direction (repository rule).
