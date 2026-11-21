#include <stdio.h>
#include <string.h>
#include "tokenizer.h"

int main(void) {
    char sample[] = "  Renewable-energy!!   Solar,   POWER 2026...  ";
    int count = 0;

    printf("Original:   \"%s\"\n", sample);
    normalize_text(sample);
    printf("Normalized: \"%s\"\n", sample);

    char** tokens = tokenize_text(sample, NULL, &count);
    printf("Token count: %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("  [%d] %s\n", i, tokens[i]);
    }
    free_tokens(tokens, count);

    /* Edge case: empty / whitespace-only string */
    char empty[] = "   ---   ";
    normalize_text(empty);
    char** empty_tokens = tokenize_text(empty, NULL, &count);
    printf("Empty test count: %d\n", count);
    free_tokens(empty_tokens, count);

    return 0;
}
