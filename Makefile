CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wpedantic -Wextra -O2
MINO_INCS = -Imino/src -Imino/src/public -Imino/src/runtime \
            -Imino/src/gc -Imino/src/eval -Imino/src/collections \
            -Imino/src/prim -Imino/src/async -Imino/src/interop \
            -Imino/src/regex -Imino/src/diag -Imino/src/vendor/imath
MINO_SRCS = mino/src/eval/mino.c mino/src/diag/diag.c mino/src/eval/eval_special.c \
            mino/src/eval/eval_special_defs.c mino/src/eval/eval_special_bindings.c \
            mino/src/eval/eval_special_control.c mino/src/eval/eval_special_fn.c \
            mino/src/runtime/runtime_state.c mino/src/runtime/runtime_var.c \
            mino/src/runtime/runtime_error.c mino/src/runtime/runtime_env.c \
            mino/src/gc/runtime_gc.c mino/src/gc/runtime_gc_roots.c \
            mino/src/gc/runtime_gc_major.c mino/src/gc/runtime_gc_barrier.c \
            mino/src/gc/runtime_gc_minor.c mino/src/gc/runtime_gc_trace.c \
            mino/src/runtime/runtime_module.c \
            mino/src/collections/transient.c \
            mino/src/public/public_gc.c mino/src/public/public_embed.c \
            mino/src/collections/val.c mino/src/collections/vec.c \
            mino/src/collections/map.c mino/src/collections/rbtree.c \
            mino/src/eval/read.c mino/src/eval/print.c \
            mino/src/prim/prim.c mino/src/prim/prim_numeric.c \
            mino/src/prim/prim_bignum.c mino/src/vendor/imath/imath.c \
            mino/src/prim/prim_collections.c mino/src/prim/prim_sequences.c \
            mino/src/prim/prim_lazy.c \
            mino/src/prim/prim_string.c mino/src/prim/prim_io.c \
            mino/src/prim/prim_reflection.c \
            mino/src/prim/prim_meta.c mino/src/prim/prim_regex.c \
            mino/src/prim/prim_stateful.c \
            mino/src/prim/prim_module.c \
            mino/src/prim/prim_fs.c mino/src/prim/prim_proc.c \
            mino/src/prim/prim_host.c mino/src/interop/host_interop.c \
            mino/src/collections/clone.c mino/src/regex/re.c \
            mino/src/async/async_scheduler.c mino/src/async/async_timer.c \
            mino/src/prim/prim_async.c
SRCS     = src/main.c src/json.c src/lsp.c src/document.c src/diagnostic.c $(MINO_SRCS)
TARGET   = mino-lsp

$(TARGET): $(SRCS) src/json.h src/lsp.h src/document.h src/diagnostic.h \
           mino/src/mino.h mino/src/mino_internal.h \
           mino/src/diag/diag.h mino/src/prim/prim_internal.h \
           mino/src/regex/re.h
	$(CC) $(CFLAGS) $(MINO_INCS) -Isrc -o $@ $(SRCS) -lm

test: $(TARGET)
	tests/test_lsp.sh

clean:
	rm -f $(TARGET)

.PHONY: test clean
