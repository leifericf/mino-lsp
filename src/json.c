/*
 * json.c -- minimal JSON parser/emitter for LSP wire protocol.
 */

#include "json.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------- */
/* Constructors                                                         */
/* -------------------------------------------------------------------- */

js_val_t *js_null(void)
{
    js_val_t *v = calloc(1, sizeof(*v));
    if (!v) return NULL;
    v->type = JS_NULL;
    return v;
}

js_val_t *js_bool(int b)
{
    js_val_t *v = calloc(1, sizeof(*v));
    if (!v) return NULL;
    v->type = JS_BOOL;
    v->as.b = b ? 1 : 0;
    return v;
}

js_val_t *js_int(long long n)
{
    js_val_t *v = calloc(1, sizeof(*v));
    if (!v) return NULL;
    v->type = JS_INT;
    v->as.num = n;
    return v;
}

js_val_t *js_string(const char *s, size_t len)
{
    js_val_t *v = calloc(1, sizeof(*v));
    if (!v) return NULL;
    v->type = JS_STRING;
    v->as.str.data = malloc(len + 1);
    if (!v->as.str.data) { free(v); return NULL; }
    memcpy(v->as.str.data, s, len);
    v->as.str.data[len] = '\0';
    v->as.str.len = len;
    return v;
}

js_val_t *js_cstring(const char *s)
{
    return js_string(s, strlen(s));
}

js_val_t *js_array_new(void)
{
    js_val_t *v = calloc(1, sizeof(*v));
    if (!v) return NULL;
    v->type = JS_ARRAY;
    return v;
}

void js_array_push(js_val_t *arr, js_val_t *val)
{
    if (!arr || arr->type != JS_ARRAY) return;
    if (arr->as.arr.len % 8 == 0) {
        size_t newcap = arr->as.arr.len + 8;
        js_val_t **ni = realloc(arr->as.arr.items,
                                newcap * sizeof(js_val_t *));
        if (!ni) return;
        arr->as.arr.items = ni;
    }
    arr->as.arr.items[arr->as.arr.len++] = val;
}

js_val_t *js_object_new(void)
{
    js_val_t *v = calloc(1, sizeof(*v));
    if (!v) return NULL;
    v->type = JS_OBJECT;
    return v;
}

void js_object_set(js_val_t *obj, const char *key, js_val_t *val)
{
    size_t i, klen;
    if (!obj || obj->type != JS_OBJECT) return;

    /* Replace if key exists. */
    for (i = 0; i < obj->as.obj.len; i++) {
        if (strcmp(obj->as.obj.keys[i], key) == 0) {
            js_free(obj->as.obj.vals[i]);
            obj->as.obj.vals[i] = val;
            return;
        }
    }

    /* Grow arrays. */
    if (obj->as.obj.len >= obj->as.obj.cap) {
        size_t     newcap = obj->as.obj.cap ? obj->as.obj.cap * 2 : 8;
        char      **nk = realloc(obj->as.obj.keys, newcap * sizeof(char *));
        js_val_t  **nv = realloc(obj->as.obj.vals, newcap * sizeof(js_val_t *));
        if (!nk || !nv) return;
        obj->as.obj.keys = nk;
        obj->as.obj.vals = nv;
        obj->as.obj.cap  = newcap;
    }

    klen = strlen(key);
    obj->as.obj.keys[obj->as.obj.len] = malloc(klen + 1);
    if (!obj->as.obj.keys[obj->as.obj.len]) return;
    memcpy(obj->as.obj.keys[obj->as.obj.len], key, klen + 1);
    obj->as.obj.vals[obj->as.obj.len] = val;
    obj->as.obj.len++;
}

js_val_t *js_object_get(const js_val_t *obj, const char *key)
{
    size_t i;
    if (!obj || obj->type != JS_OBJECT) return NULL;
    for (i = 0; i < obj->as.obj.len; i++) {
        if (strcmp(obj->as.obj.keys[i], key) == 0)
            return obj->as.obj.vals[i];
    }
    return NULL;
}

const char *js_object_get_str(const js_val_t *obj, const char *key)
{
    js_val_t *v = js_object_get(obj, key);
    if (v && v->type == JS_STRING) return v->as.str.data;
    return NULL;
}

long long js_object_get_int(const js_val_t *obj, const char *key,
                            long long fallback)
{
    js_val_t *v = js_object_get(obj, key);
    if (v && v->type == JS_INT) return v->as.num;
    return fallback;
}

/* -------------------------------------------------------------------- */
/* Parser                                                               */
/* -------------------------------------------------------------------- */

static js_val_t *parse_value(const char *buf, size_t len, size_t *consumed);

static void skip_ws(const char *buf, size_t len, size_t *pos)
{
    while (*pos < len &&
           (buf[*pos] == ' ' || buf[*pos] == '\t' ||
            buf[*pos] == '\n' || buf[*pos] == '\r'))
        (*pos)++;
}

static js_val_t *parse_string(const char *buf, size_t len, size_t *pos)
{
    size_t  start;
    char   *out;
    size_t  olen = 0;
    size_t  cap  = 64;

    if (*pos >= len || buf[*pos] != '"') return NULL;
    (*pos)++; /* skip opening quote */

    out = malloc(cap);
    if (!out) return NULL;

    start = *pos;
    (void)start;

    while (*pos < len && buf[*pos] != '"') {
        if (buf[*pos] == '\\' && *pos + 1 < len) {
            char c = buf[*pos + 1];
            char esc = 0;
            switch (c) {
            case '"':  esc = '"';  break;
            case '\\': esc = '\\'; break;
            case '/':  esc = '/';  break;
            case 'b':  esc = '\b'; break;
            case 'f':  esc = '\f'; break;
            case 'n':  esc = '\n'; break;
            case 'r':  esc = '\r'; break;
            case 't':  esc = '\t'; break;
            case 'u':
                /* Pass through \uXXXX as literal bytes for now.
                 * LSP content is UTF-8; we don't need to decode surrogates. */
                if (*pos + 5 < len) {
                    if (olen + 6 >= cap) {
                        cap *= 2;
                        out = realloc(out, cap);
                        if (!out) return NULL;
                    }
                    memcpy(out + olen, buf + *pos, 6);
                    olen += 6;
                    *pos += 6;
                    continue;
                }
                esc = 'u';
                break;
            default:
                esc = c;
                break;
            }
            if (olen + 1 >= cap) {
                cap *= 2;
                out = realloc(out, cap);
                if (!out) return NULL;
            }
            out[olen++] = esc;
            *pos += 2;
        } else {
            if (olen + 1 >= cap) {
                cap *= 2;
                out = realloc(out, cap);
                if (!out) return NULL;
            }
            out[olen++] = buf[*pos];
            (*pos)++;
        }
    }

    if (*pos >= len) { free(out); return NULL; } /* unterminated */
    (*pos)++; /* skip closing quote */

    {
        js_val_t *v = js_string(out, olen);
        free(out);
        return v;
    }
}

static js_val_t *parse_number(const char *buf, size_t len, size_t *pos)
{
    long long num = 0;
    int       neg = 0;

    if (*pos < len && buf[*pos] == '-') { neg = 1; (*pos)++; }

    if (*pos >= len || buf[*pos] < '0' || buf[*pos] > '9') return NULL;

    while (*pos < len && buf[*pos] >= '0' && buf[*pos] <= '9') {
        num = num * 10 + (buf[*pos] - '0');
        (*pos)++;
    }

    /* Skip fractional and exponent parts (LSP doesn't need them). */
    if (*pos < len && buf[*pos] == '.') {
        (*pos)++;
        while (*pos < len && buf[*pos] >= '0' && buf[*pos] <= '9')
            (*pos)++;
    }
    if (*pos < len && (buf[*pos] == 'e' || buf[*pos] == 'E')) {
        (*pos)++;
        if (*pos < len && (buf[*pos] == '+' || buf[*pos] == '-'))
            (*pos)++;
        while (*pos < len && buf[*pos] >= '0' && buf[*pos] <= '9')
            (*pos)++;
    }

    return js_int(neg ? -num : num);
}

static js_val_t *parse_array(const char *buf, size_t len, size_t *pos)
{
    js_val_t *arr = js_array_new();
    if (!arr) return NULL;

    (*pos)++; /* skip '[' */
    skip_ws(buf, len, pos);

    if (*pos < len && buf[*pos] == ']') {
        (*pos)++;
        return arr;
    }

    for (;;) {
        js_val_t *item;
        skip_ws(buf, len, pos);
        item = parse_value(buf, len, pos);
        if (!item) { js_free(arr); return NULL; }
        js_array_push(arr, item);
        skip_ws(buf, len, pos);
        if (*pos < len && buf[*pos] == ',') {
            (*pos)++;
        } else {
            break;
        }
    }

    if (*pos >= len || buf[*pos] != ']') { js_free(arr); return NULL; }
    (*pos)++;
    return arr;
}

static js_val_t *parse_object(const char *buf, size_t len, size_t *pos)
{
    js_val_t *obj = js_object_new();
    if (!obj) return NULL;

    (*pos)++; /* skip '{' */
    skip_ws(buf, len, pos);

    if (*pos < len && buf[*pos] == '}') {
        (*pos)++;
        return obj;
    }

    for (;;) {
        js_val_t *key, *val;
        skip_ws(buf, len, pos);
        key = parse_string(buf, len, pos);
        if (!key) { js_free(obj); return NULL; }

        skip_ws(buf, len, pos);
        if (*pos >= len || buf[*pos] != ':') {
            js_free(key);
            js_free(obj);
            return NULL;
        }
        (*pos)++; /* skip ':' */

        skip_ws(buf, len, pos);
        val = parse_value(buf, len, pos);
        if (!val) { js_free(key); js_free(obj); return NULL; }

        js_object_set(obj, key->as.str.data, val);
        js_free(key);

        skip_ws(buf, len, pos);
        if (*pos < len && buf[*pos] == ',') {
            (*pos)++;
        } else {
            break;
        }
    }

    if (*pos >= len || buf[*pos] != '}') { js_free(obj); return NULL; }
    (*pos)++;
    return obj;
}

static js_val_t *parse_value(const char *buf, size_t len, size_t *pos)
{
    skip_ws(buf, len, pos);
    if (*pos >= len) return NULL;

    switch (buf[*pos]) {
    case '"': return parse_string(buf, len, pos);
    case '{': return parse_object(buf, len, pos);
    case '[': return parse_array(buf, len, pos);
    case 't':
        if (*pos + 4 <= len && memcmp(buf + *pos, "true", 4) == 0) {
            *pos += 4;
            return js_bool(1);
        }
        return NULL;
    case 'f':
        if (*pos + 5 <= len && memcmp(buf + *pos, "false", 5) == 0) {
            *pos += 5;
            return js_bool(0);
        }
        return NULL;
    case 'n':
        if (*pos + 4 <= len && memcmp(buf + *pos, "null", 4) == 0) {
            *pos += 4;
            return js_null();
        }
        return NULL;
    default:
        if (buf[*pos] == '-' ||
            (buf[*pos] >= '0' && buf[*pos] <= '9'))
            return parse_number(buf, len, pos);
        return NULL;
    }
}

js_val_t *js_parse(const char *buf, size_t len, size_t *consumed)
{
    size_t    pos = 0;
    js_val_t *v   = parse_value(buf, len, &pos);
    if (consumed) *consumed = pos;
    return v;
}

/* -------------------------------------------------------------------- */
/* Encoder                                                              */
/* -------------------------------------------------------------------- */

typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} encbuf_t;

static int enc_grow(encbuf_t *b, size_t need)
{
    if (b->len + need <= b->cap) return 0;
    {
        size_t newcap = b->cap ? b->cap : 128;
        char  *nd;
        while (newcap < b->len + need) newcap *= 2;
        nd = realloc(b->data, newcap);
        if (!nd) return -1;
        b->data = nd;
        b->cap  = newcap;
    }
    return 0;
}

static int enc_append(encbuf_t *b, const char *s, size_t n)
{
    if (enc_grow(b, n) < 0) return -1;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    return 0;
}

static int enc_char(encbuf_t *b, char c)
{
    return enc_append(b, &c, 1);
}

static int enc_val(encbuf_t *b, const js_val_t *val);

static int enc_string(encbuf_t *b, const char *s, size_t slen)
{
    size_t i;
    if (enc_char(b, '"') < 0) return -1;
    for (i = 0; i < slen; i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
        case '"':  if (enc_append(b, "\\\"", 2) < 0) return -1; break;
        case '\\': if (enc_append(b, "\\\\", 2) < 0) return -1; break;
        case '\b': if (enc_append(b, "\\b",  2) < 0) return -1; break;
        case '\f': if (enc_append(b, "\\f",  2) < 0) return -1; break;
        case '\n': if (enc_append(b, "\\n",  2) < 0) return -1; break;
        case '\r': if (enc_append(b, "\\r",  2) < 0) return -1; break;
        case '\t': if (enc_append(b, "\\t",  2) < 0) return -1; break;
        default:
            if (c < 0x20) {
                char esc[8];
                int  n = snprintf(esc, sizeof(esc), "\\u%04x", c);
                if (enc_append(b, esc, (size_t)n) < 0) return -1;
            } else {
                if (enc_char(b, (char)c) < 0) return -1;
            }
            break;
        }
    }
    return enc_char(b, '"');
}

static int enc_val(encbuf_t *b, const js_val_t *val)
{
    if (!val) return enc_append(b, "null", 4);

    switch (val->type) {
    case JS_NULL:
        return enc_append(b, "null", 4);

    case JS_BOOL:
        return val->as.b
            ? enc_append(b, "true", 4)
            : enc_append(b, "false", 5);

    case JS_INT: {
        char tmp[32];
        int  n = snprintf(tmp, sizeof(tmp), "%lld", val->as.num);
        return enc_append(b, tmp, (size_t)n);
    }

    case JS_STRING:
        return enc_string(b, val->as.str.data, val->as.str.len);

    case JS_ARRAY: {
        size_t i;
        if (enc_char(b, '[') < 0) return -1;
        for (i = 0; i < val->as.arr.len; i++) {
            if (i > 0 && enc_char(b, ',') < 0) return -1;
            if (enc_val(b, val->as.arr.items[i]) < 0) return -1;
        }
        return enc_char(b, ']');
    }

    case JS_OBJECT: {
        size_t i;
        if (enc_char(b, '{') < 0) return -1;
        for (i = 0; i < val->as.obj.len; i++) {
            if (i > 0 && enc_char(b, ',') < 0) return -1;
            if (enc_string(b, val->as.obj.keys[i],
                           strlen(val->as.obj.keys[i])) < 0)
                return -1;
            if (enc_char(b, ':') < 0) return -1;
            if (enc_val(b, val->as.obj.vals[i]) < 0) return -1;
        }
        return enc_char(b, '}');
    }
    }

    return -1;
}

int js_encode(const js_val_t *val, char **buf, size_t *len)
{
    encbuf_t b = {NULL, 0, 0};
    if (enc_val(&b, val) < 0) {
        free(b.data);
        return -1;
    }
    *buf = b.data;
    *len = b.len;
    return 0;
}

/* -------------------------------------------------------------------- */
/* Cleanup                                                              */
/* -------------------------------------------------------------------- */

void js_free(js_val_t *val)
{
    size_t i;
    if (!val) return;
    switch (val->type) {
    case JS_STRING:
        free(val->as.str.data);
        break;
    case JS_ARRAY:
        for (i = 0; i < val->as.arr.len; i++)
            js_free(val->as.arr.items[i]);
        free(val->as.arr.items);
        break;
    case JS_OBJECT:
        for (i = 0; i < val->as.obj.len; i++) {
            free(val->as.obj.keys[i]);
            js_free(val->as.obj.vals[i]);
        }
        free(val->as.obj.keys);
        free(val->as.obj.vals);
        break;
    default:
        break;
    }
    free(val);
}
