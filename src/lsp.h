/*
 * lsp.h -- LSP method handlers and transport helpers.
 */

#ifndef LSP_H
#define LSP_H

#include "json.h"

/* --- Transport ------------------------------------------------------- */

/* Send a JSON-RPC response (has id). */
void lsp_send_response(js_val_t *id, js_val_t *result);

/* Send a JSON-RPC error response. */
void lsp_send_error(js_val_t *id, int code, const char *message);

/* Send a JSON-RPC notification (no id). */
void lsp_send_notification(const char *method, js_val_t *params);

/* --- Dispatch -------------------------------------------------------- */

/* Dispatch a parsed JSON-RPC message. Returns 1 if the server should exit. */
int lsp_dispatch(js_val_t *msg);

#endif /* LSP_H */
