#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include "file_loader.h"

/* Helper: checks if a filename ends with ".txt" */
static bool has_txt_extension(const char* filename) {
    if (filename == NULL) {
        return false;
    }
    size_t len = strlen(filename);
    if (len < 5) { /* Must have at least 1 char before ".txt" */
        return false;
    }
    return strcmp(filename + (len - 4), ".txt") == 0;
}

/* Helper: extracts the first non-empty line of a file as its title, or falls back to filename */
static void extract_title(const char* filepath, const char* fallback_name, char* out_title, size_t max_len) {
    FILE* fp = fopen(filepath, "r");
    if (fp != NULL) {
        char line[MAX_PATH_LEN];
        while (fgets(line, sizeof(line), fp) != NULL) {
            /* Strip trailing newline/carriage return */
            size_t len = strlen(line);
            while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ')) {
                line[--len] = '\0';
            }
            if (len > 0) {
                strncpy(out_title, line, max_len - 1);
                out_title[max_len - 1] = '\0';
                fclose(fp);
                return;
            }
        }
        fclose(fp);
    }
    strncpy(out_title, fallback_name, max_len - 1);
    out_title[max_len - 1] = '\0';
}

/*
 * Scans folder_path for valid, readable, non-empty .txt files.
 * Allocates and returns a dynamic array of Document structs.
 * Sets *out_doc_count to the number of valid documents loaded.
 */
Document* load_document_metadata(const char* folder_path, int* out_doc_count) {
    if (out_doc_count == NULL) {
        return NULL;
    }
    *out_doc_count = 0;

    if (folder_path == NULL) {
        return NULL;
    }

    DIR* dir = opendir(folder_path);
    if (dir == NULL) {
        perror("Error opening document directory");
        return NULL;
    }

    int capacity = 64;
    int count = 0;
    Document* docs = malloc((size_t)capacity * sizeof(Document));
    if (docs == NULL) {
        closedir(dir);
        return NULL;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip hidden files ('.', '..', '.DS_Store') and non-.txt files */
        if (entry->d_name[0] == '.' || !has_txt_extension(entry->d_name)) {
            continue;
        }

        char full_path[MAX_PATH_LEN];
        int written = snprintf(full_path, sizeof(full_path), "%s/%s", folder_path, entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(full_path)) {
            fprintf(stderr, "Warning: Skipping file with path too long: %s\n", entry->d_name);
            continue;
        }

        /* Verify file is readable and non-empty */
        FILE* fp = fopen(full_path, "rb");
        if (fp == NULL) {
            fprintf(stderr, "Warning: Cannot open unreadable file: %s\n", full_path);
            continue;
        }
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fclose(fp);

        if (file_size <= 0) {
            fprintf(stderr, "Warning: Skipping empty file: %s\n", full_path);
            continue;
        }

        /* Grow Document registry dynamically if needed */
        if (count >= capacity) {
            capacity *= 2;
            Document* temp = realloc(docs, (size_t)capacity * sizeof(Document));
            if (temp == NULL) {
                free(docs);
                closedir(dir);
                *out_doc_count = 0;
                return NULL;
            }
            docs = temp;
        }

        /* Populate Document metadata */
        docs[count].doc_id = count;
        strncpy(docs[count].filepath, full_path, MAX_PATH_LEN - 1);
        docs[count].filepath[MAX_PATH_LEN - 1] = '\0';

        extract_title(full_path, entry->d_name, docs[count].title, MAX_PATH_LEN);
        docs[count].total_terms = 0;
        docs[count].unique_terms = 0;

        count++;
    }

    closedir(dir);

    if (count == 0) {
        free(docs);
        return NULL;
    }

    *out_doc_count = count;
    return docs;
}

/*
 * Reads the entire contents of a text file into a dynamically allocated string.
 * Returns NULL if the file is missing, unreadable, or empty.
 * Caller must free() the returned pointer.
 */
char* read_file_content(const char* filepath) {
    if (filepath == NULL) {
        return NULL;
    }

    FILE* fp = fopen(filepath, "rb");
    if (fp == NULL) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    long size = ftell(fp);
    if (size <= 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);

    char* buffer = malloc((size_t)size + 1);
    if (buffer == NULL) {
        fclose(fp);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, (size_t)size, fp);
    fclose(fp);

    if (bytes_read != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}
