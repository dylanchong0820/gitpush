#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tokenizer.h"

/*
 * Converts all uppercase letters to lowercase and replaces any
 * non-alphanumeric character (punctuation, newlines, tabs) with a space.
 */
void normalize_text(char* text) {
    if (text == NULL) {
        return;
    }

    char* ptr = text;
    while (*ptr != '\0') {
        unsigned char ch = (unsigned char)*ptr;
        if (isalnum(ch)) {
            *ptr = (char)tolower(ch);
        } else {
            *ptr = ' ';
        }
        ptr++;
    }
}

/*
 * Splits a normalized string on whitespace into a dynamically allocated
 * array of strings. Ignores empty tokens caused by repeated spaces.
 * (Stopword filtering will be wired in during the next commit).
 */
char** tokenize_text(char* normalized_text, const StopwordList* stopwords, int* out_token_count) {
    (void)stopwords; /* Silences unused-parameter compiler warning until next commit */

    if (out_token_count == NULL) {
        return NULL;
    }
    *out_token_count = 0;

    if (normalized_text == NULL) {
        return NULL;
    }

    int capacity = 16;
    int count = 0;
    char** tokens = malloc((size_t)capacity * sizeof(char*));
    if (tokens == NULL) {
        return NULL;
    }

    char* cursor = normalized_text;
    while (*cursor != '\0') {
        /* 1. Skip leading spaces */
        while (*cursor == ' ') {
            cursor++;
        }

        /* If we hit the end after skipping spaces, stop */
        if (*cursor == '\0') {
            break;
        }

        /* 2. Mark the start of the word and scan until the next space or '\0' */
        char* word_start = cursor;
        while (*cursor != ' ' && *cursor != '\0') {
            cursor++;
        }

        size_t word_len = (size_t)(cursor - word_start);

        /* 3. Resize the token pointer array if full */
        if (count >= capacity) {
            capacity *= 2;
            char** temp = realloc(tokens, (size_t)capacity * sizeof(char*));
            if (temp == NULL) {
                free_tokens(tokens, count);
                *out_token_count = 0;
                return NULL;
            }
            tokens = temp;
        }

        /* 4. Allocate exact memory for this token (+1 for '\0') and copy it */
        tokens[count] = malloc(word_len + 1);
        if (tokens[count] == NULL) {
            free_tokens(tokens, count);
            *out_token_count = 0;
            return NULL;
        }
        memcpy(tokens[count], word_start, word_len);
        tokens[count][word_len] = '\0';
        count++;
    }

    *out_token_count = count;
    return tokens;
}

/*
 * Frees every individual token string, then frees the outer pointer array.
 */
void free_tokens(char** tokens, int token_count) {
    if (tokens == NULL) {
        return;
    }
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    free(tokens);
}
