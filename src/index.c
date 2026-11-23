#include "index.h"

#include <stdlib.h>
#include <string.h>

/* djb2 string hash: h = h * 33 + c, starting from 5381. */
static unsigned long hash_term(const char* s)
{
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*s++) != 0) {
        h = ((h << 5) + h) + (unsigned long)c;
    }
    return h;
}

/* Heap copy of a string. Caller owns the result. (strdup is not C11.) */
static char* copy_string(const char* s)
{
    size_t n = strlen(s) + 1;
    char* p = malloc(n);
    if (p == NULL) return NULL;
    memcpy(p, s, n);
    return p;
}

/* Walks one collision chain looking for an exact match. */
static TermNode* find_in_chain(TermNode* node, const char* term)
{
    while (node != NULL) {
        if (strcmp(node->term, term) == 0) return node;
        node = node->next;
    }
    return NULL;
}

InvertedIndex* create_index(int table_size, Document* docs, int total_docs)
{
    if (table_size <= 0) table_size = HASH_TABLE_SIZE;
    if (total_docs < 0 || (total_docs > 0 && docs == NULL)) return NULL;

    InvertedIndex* idx = calloc(1, sizeof *idx);
    if (idx == NULL) return NULL;

    idx->buckets = calloc((size_t)table_size, sizeof *idx->buckets);
    if (idx->buckets == NULL) {
        free(idx);
        return NULL;
    }

    idx->table_size = table_size;
    idx->docs = docs;               /* borrowed: owned by the loader */
    idx->total_docs = total_docs;

    /* The index is the single source of truth for these two counters. */
    for (int i = 0; i < total_docs; i++) {
        docs[i].total_terms = 0;
        docs[i].unique_terms = 0;
    }
    return idx;
}

bool index_add_term(InvertedIndex* idx, const char* term, int doc_id)
{
    if (idx == NULL || term == NULL || term[0] == '\0') return false;
    if (strlen(term) >= MAX_WORD_LEN) return false;
    if (doc_id < 0 || doc_id >= idx->total_docs) return false;

    unsigned long b = hash_term(term) % (unsigned long)idx->table_size;
    TermNode* t = find_in_chain(idx->buckets[b], term);

    if (t == NULL) {                        /* first time we see this word */
        t = calloc(1, sizeof *t);
        if (t == NULL) return false;
        t->term = copy_string(term);
        if (t->term == NULL) {
            free(t);
            return false;
        }
        t->next = idx->buckets[b];          /* insert at front of the chain */
        idx->buckets[b] = t;
        idx->total_unique_terms++;
    }

    if (t->tail != NULL && t->tail->doc_id > doc_id) {
        return false;                       /* documents indexed out of order */
    }

    if (t->tail != NULL && t->tail->doc_id == doc_id) {
        t->tail->term_freq++;               /* same doc again: just count it */
    } else {
        PostingNode* p = calloc(1, sizeof *p);   /* first time in this doc */
        if (p == NULL) return false;
        p->doc_id = doc_id;
        p->term_freq = 1;
        if (t->tail != NULL) t->tail->next = p;
        else t->postings = p;
        t->tail = p;
        t->doc_freq++;
        idx->total_postings++;
        idx->docs[doc_id].unique_terms++;
    }

    idx->docs[doc_id].total_terms++;
    idx->total_corpus_terms++;
    return true;
}

TermNode* index_lookup(const InvertedIndex* idx, const char* term)
{
    if (idx == NULL || term == NULL || term[0] == '\0') return NULL;
    unsigned long b = hash_term(term) % (unsigned long)idx->table_size;
    return find_in_chain(idx->buckets[b], term);
}

void index_print_stats(const InvertedIndex* idx, FILE* out)
{
    if (idx == NULL || out == NULL) return;

    int used_buckets = 0, max_chain = 0, max_df = 0;
    const char* max_term = "";

    for (int i = 0; i < idx->table_size; i++) {
        int chain = 0;
        for (const TermNode* t = idx->buckets[i]; t != NULL; t = t->next) {
            chain++;
            if (t->doc_freq > max_df) {
                max_df = t->doc_freq;
                max_term = t->term;
            }
        }
        if (chain > 0) used_buckets++;
        if (chain > max_chain) max_chain = chain;
    }

    double avg = idx->total_unique_terms > 0
                     ? (double)idx->total_postings / idx->total_unique_terms : 0.0;
    double load = (double)idx->total_unique_terms / idx->table_size;

    fprintf(out, "Documents:            %d\n", idx->total_docs);
    fprintf(out, "Total terms:          %ld\n", idx->total_corpus_terms);
    fprintf(out, "Unique terms:         %d\n", idx->total_unique_terms);
    fprintf(out, "Posting entries:      %d\n", idx->total_postings);
    fprintf(out, "Avg postings/term:    %.2f\n", avg);
    fprintf(out, "Longest posting list: %d (\"%s\")\n", max_df, max_term);
    fprintf(out, "Hash buckets:         %d (%d used, load factor %.2f)\n",
            idx->table_size, used_buckets, load);
    fprintf(out, "Longest chain:        %d\n", max_chain);
}

/* Frees postings -> term strings -> term nodes -> bucket array -> index.
   idx->docs is NOT freed here: the loader owns it. */
void free_index(InvertedIndex* idx)
{
    if (idx == NULL) return;

    for (int i = 0; i < idx->table_size; i++) {
        TermNode* t = idx->buckets[i];
        while (t != NULL) {
            TermNode* next_term = t->next;      /* save before freeing */
            PostingNode* p = t->postings;
            while (p != NULL) {
                PostingNode* next_post = p->next;
                free(p);
                p = next_post;
            }
            free(t->term);
            free(t);
            t = next_term;
        }
    }
    free(idx->buckets);
    free(idx);
}