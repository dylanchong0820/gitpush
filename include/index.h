#ifndef INDEX_H
#define INDEX_H

#include <stdio.h>
#include "common.h"

/* Creates an empty index.
   - table_size <= 0 falls back to HASH_TABLE_SIZE.
   - docs: array owned by the LOADER. The index only borrows it (never frees it)
     and resets/fills docs[i].total_terms and docs[i].unique_terms.
   Returns NULL on bad arguments or allocation failure. */
InvertedIndex* create_index(int table_size, Document* docs, int total_docs);

/* Records one occurrence of `term` in document `doc_id`.
   PRECONDITION: index one document completely before starting the next
   (doc_ids must be non-decreasing). Returns false on bad input, empty or
   over-long terms, out-of-order doc_ids, or allocation failure. */
bool index_add_term(InvertedIndex* idx, const char* term, int doc_id);

/* Returns the term's node, or NULL if absent. The result belongs to the index:
   read it, never free it. Posting lists are sorted by ascending doc_id. */
TermNode* index_lookup(const InvertedIndex* idx, const char* term);

/* Prints index statistics (terms, postings, hash table health) to `out`. */
void index_print_stats(const InvertedIndex* idx, FILE* out);

/* Frees everything the index allocated (postings, terms, buckets, the index).
   Does NOT free idx->docs. Safe to call with NULL. */
void free_index(InvertedIndex* idx);

#endif /* INDEX_H */