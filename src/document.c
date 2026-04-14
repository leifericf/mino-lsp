/*
 * document.c -- open document store.
 */

#include "document.h"

#include <stdlib.h>
#include <string.h>

static lsp_document_t docs[MAX_DOCUMENTS];

static lsp_document_t *find_slot(const char *uri)
{
    int i;
    for (i = 0; i < MAX_DOCUMENTS; i++) {
        if (docs[i].uri && strcmp(docs[i].uri, uri) == 0)
            return &docs[i];
    }
    return NULL;
}

static lsp_document_t *find_empty(void)
{
    int i;
    for (i = 0; i < MAX_DOCUMENTS; i++) {
        if (!docs[i].uri)
            return &docs[i];
    }
    return NULL;
}

static void set_content(lsp_document_t *d, const char *content, size_t len)
{
    free(d->content);
    d->content = malloc(len + 1);
    if (d->content) {
        memcpy(d->content, content, len);
        d->content[len] = '\0';
        d->content_len = len;
    } else {
        d->content_len = 0;
    }
}

void doc_open(const char *uri, const char *content, size_t len, int version)
{
    lsp_document_t *d = find_slot(uri);
    size_t ulen;
    if (!d) d = find_empty();
    if (!d) return; /* at capacity */

    free(d->uri);
    ulen = strlen(uri);
    d->uri = malloc(ulen + 1);
    if (d->uri) memcpy(d->uri, uri, ulen + 1);
    set_content(d, content, len);
    d->version = version;
}

void doc_change(const char *uri, const char *content, size_t len, int version)
{
    lsp_document_t *d = find_slot(uri);
    if (!d) return;
    set_content(d, content, len);
    d->version = version;
}

void doc_close(const char *uri)
{
    lsp_document_t *d = find_slot(uri);
    if (!d) return;
    free(d->uri);
    free(d->content);
    memset(d, 0, sizeof(*d));
}

lsp_document_t *doc_find(const char *uri)
{
    return find_slot(uri);
}
