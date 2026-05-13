# Changelog

All notable changes to mino-lsp are documented here.

## Unreleased

- Tracking mino v0.145.0 through v0.149.1. Five minor cycles plus
  the v0.149.1 bug-fix roll-up covering the hash contract for
  sequential and sorted collections, sorted-collection dissoc
  count, `ex-info` 3-arity cause, catch metadata preservation,
  `fmt_ensure` / `(sh ...)` OOM cleanup, and the `pclose` `-1`
  sentinel. The mino runtime added bytecode-VM source files under
  `src/eval/bc/`; the Makefile's `MINO_SRCS` glob now includes
  that subdirectory so the embedded runtime links the BC entry
  points (`_mino_bc_run`, `_mino_bc_compile_fn`,
  `_mino_bc_check_require`, `_mino_bc_declined`). The public C
  surface is unchanged and `src/lsp.c` doesn't call any
  VM-internal paths -- this is a drop-in submodule bump otherwise.

- Tracking mino v0.105.0 through v0.144.5 (Bytecode-VM Cycle: a
  lazily-compiled register-based bytecode VM that handles user
  fns by default, with the tree-walker remaining as a fallback
  for declined forms and top-level evaluation. Tight inner loops
  now run within constant factors of Lua 5.5 on integer-counter
  and arithmetic-chain shapes. The public C surface is unchanged
  and `src/lsp.c` does not call any of the VM-internal paths,
  so this is a drop-in submodule bump). Cycle release-pipeline
  follow-ups also covered: GC fix that traces the compiled-
  bytecode record through the remembered set (v0.144.1),
  build-flag patches that silence gcc's `-Wclobbered` heuristic
  across Linux gcc and Windows mingw (v0.144.2-v0.144.4), and
  a real correctness fix in `OP_PUSHCATCH` for nested-try
  re-throw on stricter compilers (v0.144.5).
- Tracking mino v0.103.0 (Worker-List Lock Split: brief
  `worker_list_lock` separated from the recursive `state_lock` so
  a tight embedder loop can no longer starve future / agent
  worker entry-link or exit-detach. Lock order: state_lock outer,
  worker_list_lock inner. The public C surface is unchanged and
  `src/lsp.c` does not call any of the worker-bookkeeping paths,
  so this is a drop-in submodule bump). The hover type-name
  switch in `src/lsp.c` gains explicit cases for the seven
  previously-unhandled value tags accumulated since the last
  pass: `MINO_FLOAT32` (rendered as `float`),
  `MINO_MAP_ENTRY` (`map-entry`), `MINO_UUID` (`uuid`),
  `MINO_REGEX` (`regex`), `MINO_HOST_ARRAY` (`host-array`),
  `MINO_TX_REF` (`ref`), `MINO_AGENT` (`agent`). The pre-existing
  `-Wswitch` warning stops firing.
- Tracking mino v0.102.1 (Agents finish MVP cycle: per-state agent
  workers + run-queues with separate POOLED / SOLO pools for
  `send` / `send-off`; public C-API perimeter for embedders
  (`mino_send`, `mino_send_off`, `mino_await`, `mino_await_for`,
  `mino_agent_error`, `mino_restart_agent`); `await` and
  `await-for` now actually block; `shutdown-agents` joins both
  pool workers; `restart-agent` accepts `:clear-actions true`;
  v0.102.1 patch closed an adversarial-test pass with a doc
  accuracy fix for the thread-budget message and added 11
  `abort()` rationale comments + LOC allowlist updates).
- Tracking mino v0.99.x — v0.101.1 (multiple cycles: typed-array
  cycle, scoop pipeline, STM commit atomicity hardening,
  agent cross-state defense + `mino_pcall` STM/agent
  propagation; mino-lsp itself unchanged across these).
- Tracking mino v0.98.5 (Hygiene + Closure cycle: macro hygiene fix
  in `qq_qualify_symbol` so syntax-quoted bare symbols inside a
  macro body qualify against the macro's defining namespace not the
  consumer's `*ns*` (closes the silent
  `with-out-str`-after-`:refer :all` miscompile and the
  `unbound symbol: chan*` failure for `(a/go ...)` called from
  outside `clojure.core.async`); `compare` gains the canon
  cross-type total order
  `nil < false < true < numbers < strings < symbols < keywords`;
  `clojure.string/split` gains the 3-arg `limit` arity; vector seqs
  and lazy `range` auto-chunk into 32-element chunks so
  `(chunked-seq? (seq [1 2 3]))` is `true` and
  `(reduce + (map inc (filter odd? (range 1e6))))`-style pipelines
  run end-to-end chunked; `array-map` insertion-order semantics
  verified to already match canon; `random-seed!` primitive plus a
  minimal `clojure.test.check` port (generators, properties,
  `quick-check`; shrinking deferred) backing
  `clojure.spec.alpha/gen` and `clojure.spec.alpha/exercise`).
  Makefile bundled-stdlib list grows the three new
  `lib_clojure_test_check*.h` headers; otherwise the build is a
  drop-in submodule bump.
- Tracking mino v0.97.5 (Kwargs + Audit + Hygiene cycle: kwargs
  destructuring matches Clojure 1.11 (inline pairs, trailing map,
  mixed; `:or` defaults eval correctly); `iteration` rewritten to
  canon `& {:keys [...]}` shape; `sort-by` and `reductions` gain
  multi-arity; `src/core.clj` 80-char wrap; `defn` lifted so six
  bootstrap `def + fn` forms become `defn`; `clojure.core.async`
  gains canon `reduce` / `transduce` / `split` / `partition-by`;
  `clojure.spec.alpha` gains `abbrev` / `describe`). No LSP-side
  changes — the new core.async and spec.alpha fns surface
  automatically through completion / hover via the bundled-stdlib
  introspection table; the public C surface is unchanged.
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
