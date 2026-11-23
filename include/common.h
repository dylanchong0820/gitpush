#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#define MAX_PATH_LEN 256
#define MAX_WORD_LEN 64
#define HASH_TABLE_SIZE 4096

/*Use the header files gang lesgo*/

/* Metadata for a single loaded document */
typedef struct {
    int doc_id;
    char filepath[MAX_PATH_LEN];
    char title[MAX_PATH_LEN];
    int total_terms;
    int unique_terms;
} Document;

/* Node in a term's posting list (maps term -> doc_id + frequency) */
typedef struct PostingNode {
    int doc_id;
    int term_freq;
    struct PostingNode* next;
} PostingNode;


/* Node in the hash table vocabulary */
typedef struct TermNode {
    char* term;
    int doc_freq;
    PostingNode* postings;
    PostingNode* tail;      /* last posting, for O(1) append (owned by index.c) */
    struct TermNode* next;  /* Collision chain pointer */
} TermNode;

/* Master Inverted Index structure */
typedef struct {
    TermNode** buckets;
    int table_size;
    int total_unique_terms;
    int total_postings;
    Document* docs;
    int total_docs;
    long total_corpus_terms;
} InvertedIndex;

/* Single ranked search result */
typedef struct {
    int doc_id;
    double score;
    int matched_terms_count;
} SearchResult;

/* IR Evaluation metrics */
typedef struct {
    double precision_at_k;
    double recall_at_k;
    double mrr;
    double latency_ms;
} EvalMetrics;

#endif /* COMMON_H */

