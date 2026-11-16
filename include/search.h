#ifndef SEARCH_H
#define SEARCH_H

#include "common.h"
#include "tokenizer.h"

SearchResult* search_index(const InvertedIndex* idx,
                           const StopwordList* stopwords,
                           const char* raw_query,
                           int top_k,
                           int* out_result_count);
void sort_search_results(SearchResult* results, int count);

#endif /* SEARCH_H */
