# Changelog

All notable changes to mino-lsp are documented here.

## Unreleased

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
