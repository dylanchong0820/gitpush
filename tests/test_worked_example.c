#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "tokenizer.h"
#include "index.h"
#include "search.h"

/* Helper to allocate a PostingNode */
static PostingNode* make_posting(int doc_id, int term_freq, PostingNode* next) {
    PostingNode* p = (PostingNode*)malloc(sizeof(PostingNode));
    if (!p) return NULL;
    p->doc_id = doc_id;
    p->term_freq = term_freq;
    p->next = next;
    return p;
}

/* Helper to insert a TermNode into a mock hash bucket */
static void add_term(InvertedIndex* idx, int bucket, const char* word,
                     int doc_freq, PostingNode* postings) {
    TermNode* node = (TermNode*)malloc(sizeof(TermNode));
    if (!node) return;
    size_t len = strlen(word) + 1;
    node->term = (char*)malloc(len);
    memcpy(node->term, word, len);
    node->doc_freq = doc_freq;
    node->postings = postings;
    node->next = idx->buckets[bucket];
    idx->buckets[bucket] = node;
    idx->total_unique_terms++;
}

/* Mock index_lookup() matching include/index.h */
TermNode* index_lookup(const InvertedIndex* idx, const char* term) {
    if (!idx || !term || !idx->buckets) return NULL;
    for (int b = 0; b < idx->table_size; b++) {
        for (TermNode* cur = idx->buckets[b]; cur != NULL; cur = cur->next) {
            if (cur->term && strcmp(cur->term, term) == 0) {
                return cur;
            }
        }
    }
    return NULL;
}

int main(void) {
    /* 1. Set up the 3 documents from Slide 4:
     *    doc_id 0 -> doc01 (renewable tf=1, energy tf=1)
     *    doc_id 1 -> doc02 (renewable tf=1, energy tf=3)
     *    doc_id 2 -> doc03 (renewable tf=0, energy tf=0)
     */
    InvertedIndex idx = {0};
    idx.table_size = 16;
    idx.buckets = (TermNode**)calloc((size_t)idx.table_size, sizeof(TermNode*));
    idx.total_docs = 3;
    idx.docs = (Document*)calloc(3, sizeof(Document));

    idx.docs[0].doc_id = 0; strcpy(idx.docs[0].title, "doc01");
    idx.docs[1].doc_id = 1; strcpy(idx.docs[1].title, "doc02");
    idx.docs[2].doc_id = 2; strcpy(idx.docs[2].title, "doc03");

    /* Term "renewable": df = 2 -> doc01 (tf=1), doc02 (tf=1) */
    add_term(&idx, 0, "renewable", 2,
             make_posting(0, 1, make_posting(1, 1, NULL)));

    /* Term "energy": df = 2 -> doc01 (tf=1), doc02 (tf=3) */
    add_term(&idx, 1, "energy", 2,
             make_posting(0, 1, make_posting(1, 3, NULL)));

    /* 2. Load stopwords and run query "renewable energy" */
    StopwordList* stopwords = load_stopwords("stopwords.txt");
    int result_count = 0;
    SearchResult* results = search_index(&idx, stopwords, "renewable energy", 10, &result_count);

    /* 3. Print output matching Slide 4 */
    printf("Query: \"renewable energy\"\n");
    printf("Returned %d document(s):\n\n", result_count);
    for (int i = 0; i < result_count; i++) {
        int id = results[i].doc_id;
        printf("Rank %d: %s (doc_id=%d) | matched_terms=%d | score=%.4f\n",
               i + 1, idx.docs[id].title, id,
               results[i].matched_terms_count, results[i].score);
    }

    /* 4. Free allocated memory */
    free(results);
    free_stopwords(stopwords);
    for (int b = 0; b < idx.table_size; b++) {
        TermNode* cur = idx.buckets[b];
        while (cur) {
            TermNode* next_t = cur->next;
            PostingNode* p = cur->postings;
            while (p) {
                PostingNode* next_p = p->next;
                free(p);
                p = next_p;
            }
            free(cur->term);
            free(cur);
            cur = next_t;
        }
    }
    free(idx.buckets);
    free(idx.docs);
    return 0;
}
