#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdbool.h>

typedef struct {
    char** words;
    int count;
} StopwordList;

StopwordList* load_stopwords(const char* filepath);
void free_stopwords(StopwordList* list);
bool is_stopword(const StopwordList* list, const char* word);

void normalize_text(char* text);
char** tokenize_text(char* normalized_text, const StopwordList* stopwords, int* out_token_count);
void free_tokens(char** tokens, int token_count);

#endif /* TOKENIZER_H */
