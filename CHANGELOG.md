# Changelog

All notable changes to mino-lsp are documented here.

## Unreleased

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
