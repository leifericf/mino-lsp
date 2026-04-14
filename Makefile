CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wpedantic -Wextra -O2
SRCS     = src/main.c src/json.c src/lsp.c src/document.c src/diagnostic.c mino/mino.c
TARGET   = mino-lsp

$(TARGET): $(SRCS) src/json.h src/lsp.h src/document.h src/diagnostic.h mino/mino.h
	$(CC) $(CFLAGS) -Imino -Isrc -o $@ $(SRCS)

test: $(TARGET)
	tests/test_lsp.sh

clean:
	rm -f $(TARGET)

.PHONY: test clean
