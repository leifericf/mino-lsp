CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wpedantic -Wextra -O2
MINO_SRCS = mino/src/mino.c mino/src/eval_special.c mino/src/runtime_state.c \
            mino/src/runtime_error.c mino/src/runtime_env.c mino/src/runtime_gc.c \
            mino/src/val.c mino/src/vec.c mino/src/map.c mino/src/read.c \
            mino/src/print.c mino/src/prim.c mino/src/prim_numeric.c \
            mino/src/prim_collections.c mino/src/prim_sequences.c \
            mino/src/prim_string.c mino/src/prim_io.c mino/src/clone.c \
            mino/src/re.c
SRCS     = src/main.c src/json.c src/lsp.c src/document.c src/diagnostic.c $(MINO_SRCS)
TARGET   = mino-lsp

$(TARGET): $(SRCS) src/json.h src/lsp.h src/document.h src/diagnostic.h \
           mino/src/mino.h mino/src/mino_internal.h mino/src/prim_internal.h \
           mino/src/re.h
	$(CC) $(CFLAGS) -Imino/src -Isrc -o $@ $(SRCS) -lm

test: $(TARGET)
	tests/test_lsp.sh

clean:
	rm -f $(TARGET)

.PHONY: test clean
