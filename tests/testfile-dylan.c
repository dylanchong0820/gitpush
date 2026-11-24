/* tests/testfile-dylan.c - unit tests for src/index.c (Member 2) */
#include "index.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Splits a pre-normalised sentence on spaces and indexes each word. */
static void add_doc(InvertedIndex* idx, int doc_id, const char* text)
{
    char* copy = malloc(strlen(text) + 1);
    assert(copy != NULL);
    strcpy(copy, text);
    for (char* w = strtok(copy, " "); w != NULL; w = strtok(NULL, " ")) {
        assert(index_add_term(idx, w, doc_id));
    }
    free(copy);
}

static void run_checks(int table_size)
{
    Document docs[4];
    memset(docs, 0, sizeof docs);
    for (int i = 0; i < 4; i++) docs[i].doc_id = i;

    InvertedIndex* idx = create_index(table_size, docs, 4);
    assert(idx != NULL);

    add_doc(idx, 0, "renewable energy can reduce dependence on fossil fuels solar power is widely used");
    add_doc(idx, 1, "wind energy and solar energy are important renewable energy sources");
    add_doc(idx, 2, "public transport can reduce traffic congestion in cities");
    add_doc(idx, 3, "");                                  /* empty document */

    /* "energy": 1x in doc 0, 3x in doc 1 */
    TermNode* t = index_lookup(idx, "energy");
    assert(t != NULL && t->doc_freq == 2);
    assert(t->postings->doc_id == 0 && t->postings->term_freq == 1);
    assert(t->postings->next->doc_id == 1 && t->postings->next->term_freq == 3);
    assert(t->tail == t->postings->next);

    t = index_lookup(idx, "renewable");
    assert(t != NULL && t->doc_freq == 2);
    t = index_lookup(idx, "transport");
    assert(t != NULL && t->doc_freq == 1 && t->postings->doc_id == 2);

    assert(index_lookup(idx, "quantumbanana") == NULL);   /* missing term */
    assert(index_lookup(idx, "") == NULL);                /* empty query */

    /* counters filled into the loader's Document array */
    assert(docs[0].total_terms == 13 && docs[0].unique_terms == 13);
    assert(docs[1].total_terms == 10 && docs[1].unique_terms == 8);
    assert(docs[2].total_terms == 8  && docs[2].unique_terms == 8);
    assert(docs[3].total_terms == 0  && docs[3].unique_terms == 0);
    assert(idx->total_corpus_terms == 31);
    assert(idx->total_unique_terms == 24);
    assert(idx->total_postings == 29);

    /* bad input is rejected, not a crash */
    char too_long[100];
    memset(too_long, 'a', sizeof too_long - 1);
    too_long[sizeof too_long - 1] = '\0';
    assert(!index_add_term(idx, too_long, 2));
    assert(!index_add_term(idx, "", 2));
    assert(!index_add_term(idx, NULL, 2));
    assert(!index_add_term(idx, "x", -1));
    assert(!index_add_term(idx, "x", 4));                 /* doc_id out of range */
    assert(!index_add_term(idx, "energy", 0));            /* out-of-order doc */
    assert(!index_add_term(NULL, "x", 0));
    assert(create_index(10, NULL, 3) == NULL);

    index_print_stats(idx, stdout);
    free_index(idx);
    free_index(NULL);                                     /* must be safe */
}

int main(void)
{
    printf("--- default table size ---\n");
    run_checks(0);
    printf("--- 1 bucket (every word collides) ---\n");
    run_checks(1);
    printf("All index tests passed.\n");
    return 0;
}

