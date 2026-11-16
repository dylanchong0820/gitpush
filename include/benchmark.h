#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "common.h"
#include "tokenizer.h"

double run_linear_scan_baseline(const InvertedIndex* idx,
                                const StopwordList* stopwords,
                                const char* raw_query);
void execute_benchmark_suite(const InvertedIndex* idx, const StopwordList* stopwords);
EvalMetrics evaluate_query_quality(const SearchResult* results,
                                   int result_count,
                                   const int* relevant_doc_ids,
                                   int relevant_count,
                                   int k);

#endif /* BENCHMARK_H */
