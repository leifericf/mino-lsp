/*
 * diagnostic.c -- parse-and-report diagnostics.
 */

#include "diagnostic.h"
#include "json.h"
#include "lsp.h"
#include "mino.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Count newlines from start up to (but not including) pos. */
static int count_lines(const char *start, const char *pos)
{
    int lines = 0;
    const char *p;
    for (p = start; p < pos; p++) {
        if (*p == '\n') lines++;
    }
    return lines;
}

/*
 * Try to extract a line number from an error message in the format
 * "file:N: message" or just "N: message". Returns the message part
 * (without the prefix) and sets *line to the 0-indexed line number.
 * If no line number is found, *line is set to fallback.
 */
static const char *parse_error_line(const char *err, int fallback, int *line)
{
    const char *p = err;
    const char *colon;

    /* Skip optional filename prefix. */
    colon = strchr(p, ':');
    if (colon) {
        const char *after = colon + 1;
        char *endptr;
        long n = strtol(after, &endptr, 10);
        if (endptr > after && *endptr == ':' && n > 0) {
            *line = (int)n - 1; /* convert to 0-indexed */
            return endptr + 1 + (endptr[1] == ' ' ? 1 : 0);
        }
    }

    *line = fallback;
    return err;
}

static js_val_t *make_diagnostic(int line, int severity, const char *msg)
{
    js_val_t *diag  = js_object_new();
    js_val_t *range = js_object_new();
    js_val_t *start = js_object_new();
    js_val_t *end   = js_object_new();

    js_object_set(start, "line", js_int(line));
    js_object_set(start, "character", js_int(0));
    js_object_set(end, "line", js_int(line));
    js_object_set(end, "character", js_int(0));
    js_object_set(range, "start", start);
    js_object_set(range, "end", end);

    js_object_set(diag, "range", range);
    js_object_set(diag, "severity", js_int(severity));
    js_object_set(diag, "source", js_cstring("mino"));
    js_object_set(diag, "message", js_cstring(msg));

    return diag;
}

static void publish(const char *uri, js_val_t *diag_array)
{
    js_val_t *params = js_object_new();
    js_object_set(params, "uri", js_cstring(uri));
    js_object_set(params, "diagnostics", diag_array);
    lsp_send_notification("textDocument/publishDiagnostics", params);
    js_free(params);
}

void diagnostic_check(const char *uri, const char *content,
                      mino_state_t *S, mino_env_t *env)
{
    const char *cursor = content;
    js_val_t   *arr    = js_array_new();

    /* Set step limit to prevent infinite loops during eval. */
    mino_set_limit(S, MINO_LIMIT_STEPS, 100000);

    for (;;) {
        const char   *end  = NULL;
        mino_val_t   *form = mino_read(S, cursor, &end);

        if (!form) {
            const char *err = mino_last_error(S);
            if (err && err[0] != '\0') {
                /* Parse error. Estimate line from cursor position. */
                int line = count_lines(content, end ? end : cursor);
                js_array_push(arr, make_diagnostic(line, 1, err));
            }
            break; /* EOF or error -- reader stops at first error */
        }

        /* Attempt eval for semantic errors. */
        {
            mino_val_t *result = mino_eval(S, form, env);
            if (!result) {
                const char *err = mino_last_error(S);
                if (err && err[0] != '\0') {
                    int         line = 0;
                    const char *msg;
                    char        errbuf[2048];

                    /* Copy error immediately -- next mino call overwrites. */
                    strncpy(errbuf, err, sizeof(errbuf) - 1);
                    errbuf[sizeof(errbuf) - 1] = '\0';

                    msg = parse_error_line(errbuf,
                              count_lines(content, cursor), &line);
                    js_array_push(arr, make_diagnostic(line, 1, msg));
                }
            }
        }

        if (!end) break;
        cursor = end;
    }

    /* Reset step limit. */
    mino_set_limit(S, MINO_LIMIT_STEPS, 0);

    publish(uri, arr);
    /* arr is freed inside publish via js_free(params) which frees children */
}

void diagnostic_clear(const char *uri)
{
    publish(uri, js_array_new());
}
