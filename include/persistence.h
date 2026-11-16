#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "common.h"

int save_index_binary(const InvertedIndex* idx, const char* filepath);
InvertedIndex* load_index_binary(const char* filepath);
int export_results_csv(const SearchResult* results,
                       int result_count,
                       const InvertedIndex* idx,
                       const char* query,
                       const char* filepath);

#endif /* PERSISTENCE_H */
