/*
 * main.c -- mino-lsp server.
 *
 * A minimal Language Server Protocol server for the mino language.
 * Single-threaded, communicates over stdin/stdout using JSON-RPC 2.0
 * with Content-Length framing.
 *
 * Usage: mino-lsp
 */

#include "json.h"
#include "lsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* Content-Length framing                                                */
/* -------------------------------------------------------------------- */

/*
 * Read the Content-Length header from stdin. The LSP base protocol
 * requires "Content-Length: N\r\n\r\n" before each JSON message.
 * Returns the content length, or -1 on EOF/error.
 */
static int read_header(void)
{
    char   line[256];
    int    content_length = -1;

    for (;;) {
        if (!fgets(line, sizeof(line), stdin))
            return -1; /* EOF */

        /* Empty line (just \r\n) marks end of headers. */
        if (strcmp(line, "\r\n") == 0 || strcmp(line, "\n") == 0)
            break;

        if (strncmp(line, "Content-Length:", 15) == 0) {
            content_length = atoi(line + 15);
        }
        /* Ignore other headers (e.g., Content-Type). */
    }

    return content_length;
}

/*
 * Read exactly `len` bytes from stdin into a newly allocated buffer.
 * Returns the buffer (caller must free), or NULL on error.
 */
static char *read_body(int len)
{
    char  *buf;
    size_t total = 0;

    if (len <= 0) return NULL;

    buf = malloc((size_t)len + 1);
    if (!buf) return NULL;

    while (total < (size_t)len) {
        size_t n = fread(buf + total, 1, (size_t)len - total, stdin);
        if (n == 0) { free(buf); return NULL; }
        total += n;
    }

    buf[len] = '\0';
    return buf;
}

/* -------------------------------------------------------------------- */
/* Main loop                                                            */
/* -------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    /* All logging goes to stderr; stdout is reserved for LSP messages. */
    fprintf(stderr, "mino-lsp: starting\n");

    for (;;) {
        int       content_length;
        char     *body;
        js_val_t *msg;
        int       should_exit;

        content_length = read_header();
        if (content_length < 0) break; /* EOF */

        body = read_body(content_length);
        if (!body) break;

        msg = js_parse(body, (size_t)content_length, NULL);
        free(body);

        if (!msg) {
            fprintf(stderr, "mino-lsp: failed to parse JSON message\n");
            continue;
        }

        should_exit = lsp_dispatch(msg);
        js_free(msg);

        if (should_exit) break;
    }

    fprintf(stderr, "mino-lsp: exiting\n");
    return 0;
}
