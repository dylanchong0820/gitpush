#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "search.h"
#include "index.h"
#include "common.h"
#include "tokenizer.h"

#define INSERTION_SORT_CUTOFF 16


//sorting
//higher score first (if equal score, then smaller doc_id first)
static int ranks_before(const SearchResult* a, const SearchResult* b){
    if((*a).score >(*b).score){
        return 1;
    }

    if((*a).score <(*b).score){
        return 0;
    }
    return (*a).doc_id < (*b).doc_id;
}

static void swap_result(SearchResult* a, SearchResult* b){
    SearchResult tmp=*a;
    *a=*b;
    *b=tmp;
}

//insertion sort from index low to high
static void insertion_sort_range(SearchResult*r, int lo, int hi){
    int j;
    for (int i=lo+1; i<=hi; i++){
        SearchResult key=r[i];
       j=i-1;
       while(j>=lo && ranks_before(&key, &r[j])){
        r[j+1]=r[j];
        j--;
       } 
       r[j+1]=key;
    }
}

//lomuto partition (mid of three pivot)
static int partition_range(SearchResult*r, int lo, int hi){
    int mid, i, j;
    mid=lo+(hi-lo)/2;
    SearchResult pivot;

    //makesure median in r[mid}
    if (ranks_before(&r[mid], &r[lo])){
        swap_result(&r[lo], &r[mid]);
    }

    if (ranks_before(&r[hi], &r[lo])){
        swap_result(&r[lo], &r[hi]);
    }

    if (ranks_before(&r[hi], &r[mid])){
        swap_result(&r[mid], &r[hi]);
    }

    swap_result(&r[mid], &r[hi]);
    pivot=r[hi];

    i=lo;
    for (j=lo; j<hi; j++){
        if (ranks_before(&r[j], &pivot)){
            swap_result(&r[i], &r[j]);
            i++;
        }
    }
    swap_result(&r[i], &r[hi]);
    return i;
}

//quicksort, recurses ion the smaller side, loop on the larger side
static void quicksort_range(SearchResult* r, int lo, int hi){
    while (hi-lo+1> INSERTION_SORT_CUTOFF){
        int p;
        p=partition_range(r, lo, hi);
        if (p-lo<hi-p){
            quicksort_range(r, lo, p-1);
            lo=p+1;
        } else{
            quicksort_range(r, p+1, hi);
            hi=p-1;
        }
    }
    insertion_sort_range(r, lo, hi);
}

void sort_search_results(SearchResult* results, int count){
    if (results==NULL || count<2){
        return;
    }
    quicksort_range(results, 0, count-1);
}

//query
//insert token into sorted array unless already present
static void insert_unique_sorted(const char** uniq, int* n, const char* token){
    int pos=0, k;
    while(pos< *n){
        int c =strcmp (uniq[pos], token);
        if(c==0){
            return; //ignore duplicated query term
        }
        if(c>0){
            break; //insertion point
        }
        pos++;
    }
    for (k= *n; k>pos; k--){
        uniq[k]=uniq[k-1];
    }
    uniq[pos]=token;
        (*n)++;
}

//main search function 
SearchResult* search_index(const InvertedIndex* idx, const StopwordList* stopwords, const char* raw_query, int top_k, int* out_result_count){
    char* query_copy=NULL;
    char** tokens=NULL;
    const char** uniq=NULL;
    double* scores= NULL;
    int* matched=NULL;
    SearchResult* results= NULL;
    int token_count=0, uniq_count=0;
    int max_id=-1, slots, i, t, match_count=0, out_count;
    size_t qlen;

    if (out_result_count==NULL){
        return NULL;
    }
    *out_result_count=0; 

    if (idx==NULL || raw_query==NULL || top_k<=NULL || (*idx).docs==NULL || (*idx).total_docs <=0){
        return NULL;
    }

    //normalize and tokenize
    qlen=strlen(raw_query);
    query_copy=(char*)malloc(qlen+1);
    if (query_copy==NULL){
        return NULL;
    }
    memcpy(query_copy, raw_query, qlen+1);
    normalize_text(query_copy);
    tokens=tokenize_text(query_copy, stopwords, &token_count);
    if (tokens==NULL || token_count<=0){
        goto cleanup; 
    }

    //distinct query term in abc order
    uniq= (const char**)malloc((size_t)token_count*sizeof(const char*));
    if (uniq==NULL){
        goto cleanup;
    }
    for (i=0; i<token_count; i++){
        if (tokens[i]!=NULL && tokens[i][0]!='\0'){
            insert_unique_sorted(uniq, &uniq_count, tokens[i]);
        }
    }
    if (uniq_count==0){
        goto cleanup;
    }

    //accumulate indexedd by doc_id
    for (i=0; i<(*idx).total_docs;i++){
        if ((*idx).docs[i].doc_id>max_id){
            max_id=(*idx).docs[i].doc_id;
        }
    }
    if (max_id<0){
        goto cleanup;
    }
    slots=max_id+1;
    scores=(double*)calloc((size_t)slots, sizeof(double));
    matched=(int*)calloc((size_t)slots, sizeof(int));
    if(scores==NULL || matched==NULL){
        goto cleanup;
    }

    //posting list and tf-idf
    for (t=0; t<uniq_count; t++){
        const TermNode* node=index_lookup(idx, uniq[t]);
        const PostingNode* p;
        double idf;

        if (node==NULL || (*node).doc_freq<=0){
            continue; //term is not in the index
        }

        idf=log(1.0+(double)(*idx).total_docs/(double)(*node).doc_freq);

        for (p=(*node).postings; p!=NULL; p=(*p).next){
            if((*p).term_freq<=0 || (*p).doc_id<0 || (*p).doc_id>=slots){
                continue;
            }
            scores[(*p).doc_id]+= (1.0+log((double)(*p).term_freq))*idf;
            matched[(*p).doc_id]++;
        }
    }

    //collect matching doc
    for (i=0; i<slots; i++){
        if (matched[i]>0){
            match_count++;
        }
    }

    if (match_count==0){
        goto cleanup;
    }

    results=(SearchResult*)malloc((size_t)match_count*sizeof(SearchResult));
    if (results==NULL){
        goto cleanup;
    }
    {
        int k=0;
        for (i=0; i<slots; i++){
            if (matched[i]>0){
                results[k].doc_id=i;
                results[k].score=scores[i];
                results[k].matched_terms_count=matched[i];
                k++;
            }
        }
    }

    //ranking and top_k
    sort_search_results(results, match_count);
    if (match_count<top_k){
        out_count=match_count;
    } else{
        out_count=top_k;
    }
    *out_result_count= out_count;

    cleanup:
    free(scores);
    free(matched);
    free(uniq);
    if (tokens!= NULL){
        free_tokens(tokens, token_count);
    }
    free(query_copy);
    return results; //null when noting found
}