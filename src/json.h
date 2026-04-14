/*
 * json.h -- minimal JSON parser/emitter for LSP wire protocol.
 *
 * Supports the subset needed by JSON-RPC 2.0:
 *   null, booleans, integers, strings, arrays, objects.
 * No floating-point -- LSP uses integers for line/column.
 */

#ifndef JSON_H
#define JSON_H

#include <stddef.h>

typedef enum {
    JS_NULL,
    JS_BOOL,
    JS_INT,
    JS_STRING,
    JS_ARRAY,
    JS_OBJECT
} js_type_t;

typedef struct js_val {
    js_type_t type;
    union {
        int                                             b;
        long long                                       num;
        struct { char *data; size_t len; }              str;
        struct { struct js_val **items; size_t len; }   arr;
        struct {
            char          **keys;
            struct js_val **vals;
            size_t          len;
            size_t          cap;
        } obj;
    } as;
} js_val_t;

/* --- Constructors --------------------------------------------------- */

js_val_t *js_null(void);
js_val_t *js_bool(int b);
js_val_t *js_int(long long n);
js_val_t *js_string(const char *s, size_t len);
js_val_t *js_cstring(const char *s);              /* NUL-terminated */

js_val_t *js_array_new(void);
void      js_array_push(js_val_t *arr, js_val_t *val);

js_val_t *js_object_new(void);
void      js_object_set(js_val_t *obj, const char *key, js_val_t *val);
js_val_t *js_object_get(const js_val_t *obj, const char *key);

/* Convenience: get a string field as C string, or NULL. */
const char *js_object_get_str(const js_val_t *obj, const char *key);
/* Convenience: get an integer field, or fallback. */
long long   js_object_get_int(const js_val_t *obj, const char *key,
                              long long fallback);

/* --- Codec ---------------------------------------------------------- */

/*
 * Parse one JSON value from buf (length len).
 * On success: returns value, *consumed = bytes read.
 * On error:   returns NULL.
 */
js_val_t *js_parse(const char *buf, size_t len, size_t *consumed);

/*
 * Encode a value to a dynamically allocated buffer.
 * Caller must free(*buf) when done.
 * Returns 0 on success, -1 on failure.
 */
int js_encode(const js_val_t *val, char **buf, size_t *len);

/* --- Cleanup -------------------------------------------------------- */

void js_free(js_val_t *val);

#endif /* JSON_H */
