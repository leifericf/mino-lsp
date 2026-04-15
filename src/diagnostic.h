/*
 * diagnostic.h -- parse-and-report diagnostics for LSP.
 *
 * Parses document content with mino_read(), evaluates forms with
 * mino_eval(), and publishes diagnostics via textDocument/publishDiagnostics.
 */

#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "mino.h"

/*
 * Parse and optionally evaluate all forms in `content`. Publishes a
 * textDocument/publishDiagnostics notification to stdout for the given URI.
 * Any previous diagnostics for this URI are replaced (LSP semantics).
 */
void diagnostic_check(const char *uri, const char *content,
                      mino_state_t *S, mino_env_t *env);

/*
 * Publish an empty diagnostics array for the given URI, clearing any
 * existing diagnostics in the editor.
 */
void diagnostic_clear(const char *uri);

#endif /* DIAGNOSTIC_H */
