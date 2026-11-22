#ifndef UI_H
#define UI_H

#include "common.h"

void run_cli_loop(void);
void display_statistics(const InvertedIndex* idx);
void display_search_results(const SearchResult* results, int count, const InvertedIndex* idx);
void view_document_content(const InvertedIndex* idx, int doc_id);

#endif
