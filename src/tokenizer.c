#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tokenizer.h"


StopwordList* load_stopwords(const char* filepath) {
    if (filepath == NULL) {
        return NULL;
    }

    FILE* fp = fopen(filepath, "r");
    if (fp == NULL) {
        return NULL;
    }

    StopwordList* list = malloc(sizeof(StopwordList));
    if (list == NULL) {
        fclose(fp);
        return NULL;
    }

    int capacity = 64;
    list->count = 0;
    list->words = malloc((size_t)capacity * sizeof(char*));
    if (list->words == NULL) {
        free(list);
        fclose(fp);
        return NULL;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        
        normalize_text(buffer);

        char* start = buffer;
        while (*start == ' ') {
            start++;
        }
        if (*start == '\0') {
            continue; 
        }

        char* end = start + strlen(start) - 1;
        while (end > start && *end == ' ') {
            *end = '\0';
            end--;
        }

        
        if (list->count >= capacity) {
            capacity *= 2;
            char** temp = realloc(list->words, (size_t)capacity * sizeof(char*));
            if (temp == NULL) {
                free_stopwords(list);
                fclose(fp);
                return NULL;
            }
            list->words = temp;
        }

        size_t len = strlen(start);
        list->words[list->count] = malloc(len + 1);
        if (list->words[list->count] == NULL) {
            free_stopwords(list);
            fclose(fp);
            return NULL;
        }
        memcpy(list->words[list->count], start, len + 1);
        list->count++;
    }

    fclose(fp);
    return list;
}


void free_stopwords(StopwordList* list) {
    if (list == NULL) {
        return;
    }
    for (int i = 0; i < list->count; i++) {
        free(list->words[i]);
    }
    free(list->words);
    free(list);
}


bool is_stopword(const StopwordList* list, const char* word) {
    if (list == NULL || word == NULL) {
        return false;
    }
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->words[i], word) == 0) {
            return true;
        }
    }
    return false;
}


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


char** tokenize_text(char* normalized_text, const StopwordList* stopwords, int* out_token_count) {
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
        while (*cursor == ' ') {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }

        char* word_start = cursor;
        while (*cursor != ' ' && *cursor != '\0') {
            cursor++;
        }

        size_t word_len = (size_t)(cursor - word_start);

        
        char saved_char = *cursor;
        *cursor = '\0';
        bool skip = is_stopword(stopwords, word_start);
        *cursor = saved_char;

        if (skip) {
            continue;
        }

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


void free_tokens(char** tokens, int token_count) {
    if (tokens == NULL) {
        return;
    }
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    free(tokens);
}
