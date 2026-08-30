# Decision 0002 — Module Distribution Format

- **Status:** Accepted
- **Date:** 2026-08-30

## Context

Users add Physics2D to their own Objo Studio projects with
`Import Physics2D`. Objo Studio currently distributes user code either as a
`.objo` solution archive or as source in a `.objosln` VCS package; a source
package format (`.objopackage`) is not yet available in the minimum supported
Studio version.

## Decision

1. `Shared/Sources/` is the canonical editable source. The module is authored
   as the root `Physics2D` module plus nested source items, each in its own
   `.objobasic` file with a `.source.json` sidecar.
2. `dist/Physics2D.objobasic` is a generated, deterministic, self-contained
   single-file distribution that assembles every nested source item into one
   explicit `Module Physics2D ... End Module` block. It is committed so users
   can copy one file into their project.
3. The distribution is produced only by the packaging tool under `tools/`; it
   is never edited by hand. The test suite fails when the committed artifact is
   stale relative to Shared Code.
4. The distribution header records the generator, the source commit state, the
   licence, and a do-not-edit warning.
5. When the minimum supported Studio version ships `.objopackage` source
   packages, an additional generated `.objopackage` artifact may be added. The
   single-file form is kept during version 1.

## Consequences

- The `.objosln` tree and the distribution must expose identical public API
  and behaviour; the smoke project compiles against the generated distribution,
  not Shared Code, to enforce this.
- The assembler must be deterministic: stable declaration order, normalised
  line endings, and byte-identical output from an unchanged checkout.
