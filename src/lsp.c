/*
 * lsp.c -- LSP method handlers.
 *
 * Supported methods:
 *   initialize, initialized, shutdown, exit,
 *   textDocument/didOpen, textDocument/didChange, textDocument/didClose,
 *   textDocument/completion, textDocument/hover.
 */

#include "lsp.h"
#include "json.h"
#include "document.h"
#include "diagnostic.h"
#include "mino.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* Server state                                                         */
/* -------------------------------------------------------------------- */

static mino_env_t *lsp_env         = NULL;
static int         initialized     = 0;
static int         shutdown_called = 0;

/* -------------------------------------------------------------------- */
/* Transport helpers                                                    */
/* -------------------------------------------------------------------- */

static void send_raw(js_val_t *msg)
{
    char  *buf = NULL;
    size_t len = 0;
    if (js_encode(msg, &buf, &len) == 0) {
        fprintf(stdout, "Content-Length: %zu\r\n\r\n", len);
        fwrite(buf, 1, len, stdout);
        fflush(stdout);
        free(buf);
    }
}

void lsp_send_response(js_val_t *id, js_val_t *result)
{
    js_val_t *msg = js_object_new();
    js_object_set(msg, "jsonrpc", js_cstring("2.0"));
    /* Clone id so caller can still free their copy. */
    if (id && id->type == JS_INT)
        js_object_set(msg, "id", js_int(id->as.num));
    else if (id && id->type == JS_STRING)
        js_object_set(msg, "id", js_string(id->as.str.data, id->as.str.len));
    else
        js_object_set(msg, "id", js_null());
    js_object_set(msg, "result", result);
    send_raw(msg);
    js_free(msg);
}

void lsp_send_error(js_val_t *id, int code, const char *message)
{
    js_val_t *msg = js_object_new();
    js_val_t *err = js_object_new();
    js_object_set(err, "code", js_int(code));
    js_object_set(err, "message", js_cstring(message));
    js_object_set(msg, "jsonrpc", js_cstring("2.0"));
    if (id && id->type == JS_INT)
        js_object_set(msg, "id", js_int(id->as.num));
    else if (id && id->type == JS_STRING)
        js_object_set(msg, "id", js_string(id->as.str.data, id->as.str.len));
    else
        js_object_set(msg, "id", js_null());
    js_object_set(msg, "error", err);
    send_raw(msg);
    js_free(msg);
}

void lsp_send_notification(const char *method, js_val_t *params)
{
    js_val_t *msg = js_object_new();
    js_object_set(msg, "jsonrpc", js_cstring("2.0"));
    js_object_set(msg, "method", js_cstring(method));
    /* Clone params into msg so caller can free their copy. */
    {
        js_val_t *p = js_object_new();
        size_t i;
        if (params && params->type == JS_OBJECT) {
            for (i = 0; i < params->as.obj.len; i++) {
                /* Steal the value pointers: set originals to null. */
                js_object_set(p, params->as.obj.keys[i],
                              params->as.obj.vals[i]);
                params->as.obj.vals[i] = NULL;
            }
        }
        js_object_set(msg, "params", p);
    }
    send_raw(msg);
    js_free(msg);
}

/* -------------------------------------------------------------------- */
/* Helper: extract symbol text at a position in document content        */
/* -------------------------------------------------------------------- */

/* Characters that terminate a symbol. */
static int is_sym_terminator(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
           c == ',' || c == ';' || c == '(' || c == ')' ||
           c == '[' || c == ']' || c == '{' || c == '}' ||
           c == '"' || c == '\'' || c == '`' || c == '~' ||
           c == '\0';
}

/*
 * Find the offset into content for a given line:character position.
 * Returns -1 if out of bounds.
 */
static long offset_of(const char *content, size_t content_len,
                       int line, int character)
{
    int cur_line = 0;
    size_t i;
    for (i = 0; i < content_len; i++) {
        if (cur_line == line) {
            size_t col_offset = i + (size_t)character;
            return col_offset <= content_len ? (long)col_offset : -1;
        }
        if (content[i] == '\n') cur_line++;
    }
    /* If we've counted past all lines, check if we're on the last line. */
    if (cur_line == line) {
        size_t col_offset = i + (size_t)character;
        return col_offset <= content_len ? (long)col_offset : -1;
    }
    return -1;
}

/*
 * Extract the symbol at a given offset. Scans backward and forward
 * to find symbol boundaries. Returns a malloc'd string, or NULL.
 */
static char *symbol_at(const char *content, size_t content_len, long offset)
{
    long start, end;
    size_t slen;
    char *sym;

    if (offset < 0 || (size_t)offset > content_len) return NULL;

    /* If we're right at a terminator, try one position back. */
    if ((size_t)offset >= content_len ||
        is_sym_terminator(content[offset])) {
        if (offset > 0 && !is_sym_terminator(content[offset - 1]))
            offset--;
        else
            return NULL;
    }

    /* Scan backward. */
    start = offset;
    while (start > 0 && !is_sym_terminator(content[start - 1]))
        start--;

    /* Scan forward. */
    end = offset;
    while ((size_t)end < content_len && !is_sym_terminator(content[end]))
        end++;

    slen = (size_t)(end - start);
    if (slen == 0) return NULL;

    sym = malloc(slen + 1);
    if (!sym) return NULL;
    memcpy(sym, content + start, slen);
    sym[slen] = '\0';
    return sym;
}

/*
 * Extract the prefix (partial symbol) being typed at cursor position.
 * For completion, we scan backward from the cursor. Returns malloc'd string.
 */
static char *prefix_at(const char *content, size_t content_len, long offset)
{
    long start;
    size_t plen;
    char *prefix;

    if (offset < 0 || (size_t)offset > content_len) return NULL;

    start = offset;
    while (start > 0 && !is_sym_terminator(content[start - 1]))
        start--;

    plen = (size_t)(offset - start);
    if (plen == 0) return NULL;

    prefix = malloc(plen + 1);
    if (!prefix) return NULL;
    memcpy(prefix, content + start, plen);
    prefix[plen] = '\0';
    return prefix;
}

/* -------------------------------------------------------------------- */
/* Method handlers                                                      */
/* -------------------------------------------------------------------- */

static void handle_initialize(js_val_t *id, js_val_t *params)
{
    js_val_t *result       = js_object_new();
    js_val_t *capabilities = js_object_new();
    js_val_t *text_sync    = js_object_new();
    js_val_t *completion   = js_object_new();
    js_val_t *triggers     = js_array_new();
    js_val_t *server_info  = js_object_new();

    (void)params;

    /* textDocumentSync: full content on every change. */
    js_object_set(text_sync, "openClose", js_bool(1));
    js_object_set(text_sync, "change", js_int(1)); /* Full */

    /* completionProvider with trigger characters. */
    js_array_push(triggers, js_cstring("("));
    js_array_push(triggers, js_cstring(" "));
    js_object_set(completion, "triggerCharacters", triggers);

    js_object_set(capabilities, "textDocumentSync", text_sync);
    js_object_set(capabilities, "completionProvider", completion);
    js_object_set(capabilities, "hoverProvider", js_bool(1));

    js_object_set(server_info, "name", js_cstring("mino-lsp"));
    js_object_set(server_info, "version", js_cstring("0.1.0"));

    js_object_set(result, "capabilities", capabilities);
    js_object_set(result, "serverInfo", server_info);

    lsp_send_response(id, result);

    /* Initialize the mino environment. */
    if (!lsp_env) {
        lsp_env = mino_env_new();
        mino_install_core(lsp_env);
        /* No mino_install_io -- LSP env has no I/O side effects. */
    }
}

static void handle_initialized(js_val_t *id, js_val_t *params)
{
    (void)id;
    (void)params;
    initialized = 1;
}

static void handle_shutdown(js_val_t *id, js_val_t *params)
{
    (void)params;
    shutdown_called = 1;
    lsp_send_response(id, js_null());
}

static int handle_exit(js_val_t *id, js_val_t *params)
{
    (void)id;
    (void)params;
    if (lsp_env) {
        mino_env_free(lsp_env);
        lsp_env = NULL;
    }
    return 1; /* signal main loop to exit */
}

static void handle_did_open(js_val_t *id, js_val_t *params)
{
    js_val_t   *td;
    const char *uri, *text;

    (void)id;
    td = js_object_get(params, "textDocument");
    if (!td) return;

    uri  = js_object_get_str(td, "uri");
    text = js_object_get_str(td, "text");
    if (!uri || !text) return;

    doc_open(uri, text, strlen(text),
             (int)js_object_get_int(td, "version", 0));
    diagnostic_check(uri, text, lsp_env);
}

static void handle_did_change(js_val_t *id, js_val_t *params)
{
    js_val_t   *td, *changes, *change;
    const char *uri, *text;

    (void)id;
    td = js_object_get(params, "textDocument");
    if (!td) return;
    uri = js_object_get_str(td, "uri");
    if (!uri) return;

    changes = js_object_get(params, "contentChanges");
    if (!changes || changes->type != JS_ARRAY || changes->as.arr.len == 0)
        return;

    /* Full sync: take the first (and only) change. */
    change = changes->as.arr.items[0];
    text = js_object_get_str(change, "text");
    if (!text) return;

    doc_change(uri, text, strlen(text),
               (int)js_object_get_int(td, "version", 0));
    diagnostic_check(uri, text, lsp_env);
}

static void handle_did_close(js_val_t *id, js_val_t *params)
{
    js_val_t   *td;
    const char *uri;

    (void)id;
    td = js_object_get(params, "textDocument");
    if (!td) return;
    uri = js_object_get_str(td, "uri");
    if (!uri) return;

    doc_close(uri);
    diagnostic_clear(uri);
}

static void handle_completion(js_val_t *id, js_val_t *params)
{
    js_val_t       *td, *pos_obj;
    const char     *uri;
    int             line, character;
    lsp_document_t *doc;
    long            off;
    char           *pfx;
    js_val_t       *items;
    mino_val_t     *all_syms;

    td = js_object_get(params, "textDocument");
    pos_obj = js_object_get(params, "position");
    if (!td || !pos_obj) {
        lsp_send_response(id, js_array_new());
        return;
    }

    uri       = js_object_get_str(td, "uri");
    line      = (int)js_object_get_int(pos_obj, "line", 0);
    character = (int)js_object_get_int(pos_obj, "character", 0);

    doc = uri ? doc_find(uri) : NULL;
    if (!doc) {
        lsp_send_response(id, js_array_new());
        return;
    }

    off = offset_of(doc->content, doc->content_len, line, character);
    pfx = prefix_at(doc->content, doc->content_len, off);

    items = js_array_new();

    /* Get all symbols via (apropos ""). */
    mino_set_limit(MINO_LIMIT_STEPS, 100000);
    all_syms = mino_eval_string("(apropos \"\")", lsp_env);
    mino_set_limit(MINO_LIMIT_STEPS, 0);

    if (all_syms) {
        mino_val_t *cur = all_syms;
        while (cur && !mino_is_nil(cur) && mino_is_cons(cur)) {
            mino_val_t *sym = mino_car(cur);
            const char *name = NULL;
            size_t      nlen = 0;

            /* apropos returns symbols; extract name from either type. */
            if (sym) {
                if (sym->type == MINO_SYMBOL || sym->type == MINO_STRING ||
                    sym->type == MINO_KEYWORD) {
                    name = sym->as.s.data;
                    nlen = sym->as.s.len;
                }
            }

            if (name && nlen > 0) {
                /* Filter by prefix. */
                int match = 1;
                if (pfx) {
                    size_t plen = strlen(pfx);
                    if (nlen < plen || memcmp(name, pfx, plen) != 0)
                        match = 0;
                }
                if (match) {
                    js_val_t   *item = js_object_new();
                    mino_val_t *val;
                    int         kind = 6; /* Variable */

                    js_object_set(item, "label", js_string(name, nlen));

                    /* Determine CompletionItemKind from value type. */
                    val = mino_env_get(lsp_env, name);
                    if (val) {
                        if (val->type == MINO_FN || val->type == MINO_PRIM)
                            kind = 3; /* Function */
                        else if (val->type == MINO_MACRO)
                            kind = 14; /* Keyword */
                    }
                    js_object_set(item, "kind", js_int(kind));
                    js_array_push(items, item);
                }
            }
            cur = mino_cdr(cur);
        }
    }

    free(pfx);
    lsp_send_response(id, items);
}

static void handle_hover(js_val_t *id, js_val_t *params)
{
    js_val_t       *td, *pos_obj;
    const char     *uri;
    int             line, character;
    lsp_document_t *doc;
    long            off;
    char           *sym;
    mino_val_t     *val;

    td = js_object_get(params, "textDocument");
    pos_obj = js_object_get(params, "position");
    if (!td || !pos_obj) {
        lsp_send_response(id, js_null());
        return;
    }

    uri       = js_object_get_str(td, "uri");
    line      = (int)js_object_get_int(pos_obj, "line", 0);
    character = (int)js_object_get_int(pos_obj, "character", 0);

    doc = uri ? doc_find(uri) : NULL;
    if (!doc) {
        lsp_send_response(id, js_null());
        return;
    }

    off = offset_of(doc->content, doc->content_len, line, character);
    sym = symbol_at(doc->content, doc->content_len, off);
    if (!sym) {
        lsp_send_response(id, js_null());
        return;
    }

    val = mino_env_get(lsp_env, sym);
    if (!val) {
        free(sym);
        lsp_send_response(id, js_null());
        return;
    }

    /* Build hover content. */
    {
        char        hover_buf[4096];
        const char *type_name = "unknown";
        char        expr[512];
        js_val_t   *result, *contents;

        /* Get type name. */
        switch (val->type) {
        case MINO_NIL:     type_name = "nil";     break;
        case MINO_BOOL:    type_name = "boolean"; break;
        case MINO_INT:     type_name = "integer"; break;
        case MINO_FLOAT:   type_name = "float";   break;
        case MINO_STRING:  type_name = "string";  break;
        case MINO_SYMBOL:  type_name = "symbol";  break;
        case MINO_KEYWORD: type_name = "keyword"; break;
        case MINO_CONS:    type_name = "list";    break;
        case MINO_VECTOR:  type_name = "vector";  break;
        case MINO_MAP:     type_name = "map";     break;
        case MINO_SET:     type_name = "set";     break;
        case MINO_PRIM:    type_name = "primitive"; break;
        case MINO_FN:      type_name = "function"; break;
        case MINO_MACRO:   type_name = "macro";   break;
        case MINO_HANDLE:  type_name = "handle";  break;
        case MINO_RECUR:     type_name = "recur";     break;
        case MINO_ATOM:      type_name = "atom";      break;
        case MINO_LAZY:      type_name = "lazy-seq";  break;
        case MINO_TAIL_CALL: type_name = "tail-call"; break;
        }

        /* Try to get docstring via (doc sym). */
        snprintf(expr, sizeof(expr), "(doc '%s)", sym);
        mino_set_limit(MINO_LIMIT_STEPS, 100000);
        {
            mino_val_t *doc_result = mino_eval_string(expr, lsp_env);
            const char *docstr = NULL;
            size_t      doclen = 0;

            if (doc_result && !mino_is_nil(doc_result))
                mino_to_string(doc_result, &docstr, &doclen);

            if (docstr && doclen > 0)
                snprintf(hover_buf, sizeof(hover_buf),
                         "**%s** *:%s*\n\n%.*s", sym, type_name,
                         (int)doclen, docstr);
            else
                snprintf(hover_buf, sizeof(hover_buf),
                         "**%s** *:%s*", sym, type_name);
        }
        mino_set_limit(MINO_LIMIT_STEPS, 0);

        result = js_object_new();
        contents = js_object_new();
        js_object_set(contents, "kind", js_cstring("markdown"));
        js_object_set(contents, "value", js_cstring(hover_buf));
        js_object_set(result, "contents", contents);

        free(sym);
        lsp_send_response(id, result);
    }
}

/* -------------------------------------------------------------------- */
/* Dispatch table                                                       */
/* -------------------------------------------------------------------- */

typedef void (*lsp_handler_fn)(js_val_t *id, js_val_t *params);

static const struct {
    const char     *method;
    lsp_handler_fn  handler;
    int             is_notification;
} method_table[] = {
    { "initialize",                handle_initialize,  0 },
    { "initialized",               handle_initialized, 1 },
    { "shutdown",                  handle_shutdown,    0 },
    { "textDocument/didOpen",      handle_did_open,    1 },
    { "textDocument/didChange",    handle_did_change,  1 },
    { "textDocument/didClose",     handle_did_close,   1 },
    { "textDocument/completion",   handle_completion,  0 },
    { "textDocument/hover",        handle_hover,       0 },
    { NULL,                        NULL,               0 }
};

int lsp_dispatch(js_val_t *msg)
{
    const char *method = js_object_get_str(msg, "method");
    js_val_t   *id     = js_object_get(msg, "id");
    js_val_t   *params = js_object_get(msg, "params");
    int         i;

    if (!method) return 0;

    /* Special case: exit is handled differently (returns exit signal). */
    if (strcmp(method, "exit") == 0)
        return handle_exit(id, params);

    for (i = 0; method_table[i].method != NULL; i++) {
        if (strcmp(method, method_table[i].method) == 0) {
            method_table[i].handler(id, params);
            return 0;
        }
    }

    /* Unknown method. If it has an id, send MethodNotFound error. */
    if (id) {
        lsp_send_error(id, -32601, "Method not found");
    }
    /* Unknown notifications are silently ignored per LSP spec. */

    return 0;
}
