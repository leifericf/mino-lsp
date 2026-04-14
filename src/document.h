/*
 * document.h -- open document store for LSP.
 *
 * Tracks documents currently open in the editor by URI. Each document
 * holds the full text content (full-sync mode).
 */

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <stddef.h>

#define MAX_DOCUMENTS 64

typedef struct {
    char   *uri;
    char   *content;
    size_t  content_len;
    int     version;
} lsp_document_t;

/* Store a newly opened document. */
void doc_open(const char *uri, const char *content, size_t len, int version);

/* Replace the content of an existing document. */
void doc_change(const char *uri, const char *content, size_t len, int version);

/* Remove a document from the store. */
void doc_close(const char *uri);

/* Look up a document by URI. Returns NULL if not found. */
lsp_document_t *doc_find(const char *uri);

#endif /* DOCUMENT_H */
