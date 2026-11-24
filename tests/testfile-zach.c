#include <stdio.h>
#include <stdlib.h>
#include "file_loader.h"
#include "tokenizer.h"

int main(void) {
    StopwordList* stopwords = load_stopwords("stopwords.txt");
    printf("Loaded %d stopwords.\n", stopwords ? stopwords->count : 0);

    int doc_count = 0;
    Document* docs = load_document_metadata("data", &doc_count);
    printf("Loaded %d valid documents.\n\n", doc_count);

    for (int i = 0; i < doc_count; i++) {
        char* content = read_file_content(docs[i].filepath);
        if (content == NULL) continue;

        normalize_text(content);
        int token_count = 0;
        char** tokens = tokenize_text(content, stopwords, &token_count);
        docs[i].total_terms = token_count;

        printf("Doc [%d] (%s) -> Title: \"%s\" | Filtered Terms: %d\n",
               docs[i].doc_id, docs[i].filepath, docs[i].title, docs[i].total_terms);

        free_tokens(tokens, token_count);
        free(content);
    }

    free(docs);
    free_stopwords(stopwords);
    return 0;
}
