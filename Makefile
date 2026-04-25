CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wpedantic -Wextra -O2
MINO_INCS = -Imino/src -Imino/src/public -Imino/src/runtime \
            -Imino/src/gc -Imino/src/eval -Imino/src/collections \
            -Imino/src/prim -Imino/src/async -Imino/src/interop \
            -Imino/src/diag -Imino/src/vendor/imath
MINO_SRCS = $(wildcard mino/src/public/*.c) \
            $(wildcard mino/src/runtime/*.c) \
            $(wildcard mino/src/gc/*.c) \
            $(wildcard mino/src/eval/*.c) \
            $(wildcard mino/src/collections/*.c) \
            $(wildcard mino/src/prim/*.c) \
            $(wildcard mino/src/async/*.c) \
            $(wildcard mino/src/interop/*.c) \
            $(wildcard mino/src/regex/*.c) \
            $(wildcard mino/src/diag/*.c) \
            $(wildcard mino/src/vendor/imath/*.c)
SRCS     = src/main.c src/json.c src/lsp.c src/document.c src/diagnostic.c $(MINO_SRCS)
TARGET   = mino-lsp

$(TARGET): $(SRCS) src/json.h src/lsp.h src/document.h src/diagnostic.h \
           mino/src/mino.h mino/src/diag/diag.h
	$(CC) $(CFLAGS) $(MINO_INCS) -Isrc -o $@ $(SRCS) -lm

test: $(TARGET)
	tests/test_lsp.sh

clean:
	rm -f $(TARGET)

.PHONY: test clean
