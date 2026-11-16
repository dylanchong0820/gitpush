#ifndef INDEX_H
#define INDEX_H

#include "common.h"

InvertedIndex* create_index(int table_size, Document* docs, int total_docs);
void index_add_term(InvertedIndex* idx, const char* term, int doc_id);
TermNode* index_lookup(const InvertedIndex* idx, const char* term);
void free_index(InvertedIndex* idx);

#endif /* INDEX_H */
