CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wpedantic -Wextra -O2
MINO_SRCS = mino/src/mino.c mino/src/diag.c mino/src/eval_special.c \
            mino/src/eval_special_defs.c mino/src/eval_special_bindings.c \
            mino/src/eval_special_control.c mino/src/eval_special_fn.c \
            mino/src/runtime_state.c mino/src/runtime_var.c \
            mino/src/runtime_error.c mino/src/runtime_env.c mino/src/runtime_gc.c \
            mino/src/val.c mino/src/vec.c mino/src/map.c mino/src/rbtree.c \
            mino/src/read.c \
            mino/src/print.c mino/src/prim.c mino/src/prim_numeric.c \
            mino/src/prim_collections.c mino/src/prim_sequences.c \
            mino/src/prim_string.c mino/src/prim_io.c mino/src/prim_reflection.c \
            mino/src/prim_meta.c mino/src/prim_regex.c mino/src/prim_stateful.c \
            mino/src/prim_module.c \
            mino/src/prim_fs.c mino/src/prim_proc.c \
            mino/src/prim_host.c mino/src/host_interop.c \
            mino/src/clone.c mino/src/re.c \
            mino/src/async_buffer.c mino/src/async_channel.c \
            mino/src/async_handler.c mino/src/async_select.c \
            mino/src/async_scheduler.c mino/src/async_timer.c \
            mino/src/prim_async.c
SRCS     = src/main.c src/json.c src/lsp.c src/document.c src/diagnostic.c $(MINO_SRCS)
TARGET   = mino-lsp

$(TARGET): $(SRCS) src/json.h src/lsp.h src/document.h src/diagnostic.h \
           mino/src/mino.h mino/src/mino_internal.h mino/src/diag.h mino/src/prim_internal.h \
           mino/src/re.h
	$(CC) $(CFLAGS) -Imino/src -Isrc -o $@ $(SRCS) -lm

test: $(TARGET)
	tests/test_lsp.sh

clean:
	rm -f $(TARGET)

.PHONY: test clean
