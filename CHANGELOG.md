# Changelog

All notable changes to mino-lsp are documented here.

## Unreleased

- Tracking mino v0.96.8 (Canon-Parity cycle: real `MINO_VOLATILE`
  primitive, stateful-transducer rewrites, lazy-seq recur-on-skip,
  transient reductions, comp/partial/some-fn/every-pred unrolling
  plus `into` 0/1-arg and `unchecked-divide-int`, `iteration` from
  Clojure 1.11, `clojure.core.async` namespace wrap with `merge`/`into`
  renames, the `:refer :all` transitive-drag fix, and the chunked-seq
  family with two new value types — `MINO_CHUNK` and
  `MINO_CHUNKED_CONS` — plus eight primitives). The new primitives
  surface automatically through the primitive table that backs
  completion and hover; the hover type-name switch in `src/lsp.c`
  gains explicit cases for `MINO_VOLATILE` (rendered as `volatile`),
  `MINO_CHUNK` (rendered as `chunk`), and `MINO_CHUNKED_CONS`
  (rendered as `list`, matching the runtime), so hover on a value of
  one of the new types reports the correct type-name instead of
  `unknown`. The public C API is additive only.
- Tracking mino v0.95.5 (Clojure-side hygiene pass: bundled stdlib
  refactor across `src/core.clj`, `lib/clojure/*`, `lib/core/*`, and
  `lib/mino/*`). No LSP-side changes — the hygiene pass is internal
  to the mino-side library; the public C surface and bundled
  primitive set are unchanged.
- Tracking mino v0.94.0 (empty-list canon parity). v0.94.0's
  `MINO_EMPTY_LIST` enum entry surfaces in the hover type-name
  switch as `list` (same display as `MINO_CONS`); also added the
  pre-existing `MINO_TYPE`, `MINO_RECORD`, and `MINO_FUTURE` cases
  so the `-Wswitch` warning stops firing.
- Tracking mino v0.93.0 (C refactoring pass; bundled `mino deps` and
  `mino task` tooling; bootstrap Makefile). The Makefile gains three
  gen-mino-header entries for the new `lib/mino/*` sources that v0.93.0
  bakes into the binary. No LSP-side changes; hover and completion
  continue to pick up new vars automatically.
- Tracking mino v0.74.0 (deferred core surface): `*ns*` as a real var,
  `bound-fn` / `bound-fn*`, `read` with options, `clojure.edn/read`,
  `destructure`, and regex capture groups with `re-matcher` /
  `re-groups`. No LSP-side changes needed; hover and completion pick
  up the new surface through the existing primitive enumeration.
- Tracking mino v0.73.0 (first-class namespaces): each namespace owns
  its own root binding table, vars are first-class objects, auto-
  resolved keywords and namespaced map literals land at read time,
  and source files use `.clj` instead of `.mino`. LSP test fixtures
  and document URIs swap to `.clj` alongside the migration. Makefile
  picks up the new `runtime/ns_env.c` and `runtime/path_buf.c` TUs.
- Tracking mino v0.48.0: character type (`MINO_CHAR`), sorted
  collections (`MINO_SORTED_MAP`, `MINO_SORTED_SET`), first-class vars
  (`MINO_VAR`), transients (`MINO_TRANSIENT`), and the structured
  exception + argument-parsing helpers in `public_embed.c`. Hover
  type dispatch covers all new tags. Makefile extended to compile the
  new `transient.c` and `public_embed.c` TUs.
- Tracking mino v0.42.0: generational + incremental garbage collector,
  public GC API, literal-builder barrier fix. Makefile extended to
  compile the new `runtime_gc_roots.c`, `runtime_gc_major.c`,
  `runtime_gc_barrier.c`, `runtime_gc_minor.c`, `public_gc.c`, and
  `prim_lazy.c` TUs.
- Tracking mino v0.39.1 (task runner, `str-replace` primitive,
  `file-mtime` primitive, Windows CI)
