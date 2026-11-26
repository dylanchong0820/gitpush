/* persistence.c - saves/loads the index to index.bin and exports reports. */
/* File layout: header, document records, term records, checksum at the end. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* fixed-size integer types, so the file looks the same on every computer */
#include <stdint.h>
#include "persistence.h"

/* identify our file: name tag, version number, and a byte-order test value */
#define FILE_MAGIC        "SIDX"
#define FILE_VERSION      1u
#define BYTE_ORDER_CHECK  0x01020304u

/* upper limits, used to reject a damaged file with silly values */
#define MAX_TABLE_SIZE    (1 << 22)
#define MAX_DOCS          1000000
#define MAX_TERM_LEN      1024

/* Sizes of the pieces of the file (used for sanity checks) */
#define HEADER_SIZE       36
#define CHECKSUM_SIZE     4
#define DOC_RECORD_SIZE   (4 + MAX_PATH_LEN + MAX_PATH_LEN + 4 + 4)
#define MIN_TERM_RECORD   17   /* 4+4+1+4+4: a term with a 1-letter name */
#define POSTING_RECORD    8

/* starting value for the checksum (FNV-1a), which detects a damaged file */
#define CHECKSUM_START    2166136261u

/* last error text, so the menu can show the user what went wrong */

static char last_error[256] = "";

static void set_error(const char* msg) {
    strncpy(last_error, msg, sizeof(last_error) - 1);
    last_error[sizeof(last_error) - 1] = '\0';
}

const char* persistence_last_error(void) {
    return last_error;
}

/* Checksum (FNV-1a, 32 bit)-----------------------------------------*/

// Mixes bytes into a number. If the file changes, the number changes.
static uint32_t checksum_update(uint32_t hash, const void* data, size_t n) {
    const unsigned char* p = data;
    for (size_t i = 0; i < n; i++) {
        hash ^= p[i];
        hash *= 16777619u;
    }
    return hash;
}
 
/* Writing helpers                                                     */

/* A Writer wraps the output file, the running checksum and an "ok" flag.
 * Once one fwrite fails, ok becomes 0 and all later writes are skipped,
 * so we only need to check ok once at the end. */


// Bundles the file, the checksum so far, and an ok flag (0 = a write failed).
 typedef struct {
    FILE* f;
    uint32_t sum;
    int ok;
} Writer;
// Writes bytes to the file and adds them to the checksum.
static void write_bytes(Writer* w, const void* data, size_t n) {
    if (!w->ok || n == 0) return;
    if (fwrite(data, 1, n, w->f) != n) {
        w->ok = 0;
        return;
    }
    w->sum = checksum_update(w->sum, data, n);
}

// Helpers to write one number of a fixed size.
static void write_i32(Writer* w, int32_t v)  { write_bytes(w, &v, sizeof v); }
static void write_u32(Writer* w, uint32_t v) { write_bytes(w, &v, sizeof v); }
static void write_i64(Writer* w, int64_t v)  { write_bytes(w, &v, sizeof v); }
 
/* Walks the whole index once. Counts the terms and postings that are really there and checks nothing is obviously broken. 
Returns 1 if fine, 0 if not.(We store the real counts, not the counters in the struct, so the file is
always self-consistent.) */
static int count_index(const InvertedIndex* idx, int32_t* out_terms, int32_t* out_postings) {
    int32_t terms = 0;
    int32_t postings = 0;
 
    for (int b = 0; b < idx->table_size; b++) {
        for (const TermNode* t = idx->buckets[b]; t != NULL; t = t->next) {
            if (t->term == NULL) return 0;
            size_t len = strlen(t->term);
            if (len == 0 || len > MAX_TERM_LEN) return 0;
            terms++;
 
            for (const PostingNode* p = t->postings; p != NULL; p = p->next) {
                // Counts the real terms and postings, and checks nothing is broken.   
                if (p->doc_id < 0 || p->doc_id >= idx->total_docs) return 0;
                postings++;
            }
        }
    }
 
    *out_terms = terms;
    *out_postings = postings;
    return 1;
}
 
/* ------------------------------------------------------------------ */
/* save_index_binary                                                   */
/* ------------------------------------------------------------------ */
 
int save_index_binary(const InvertedIndex* idx, const char* filepath) {
    // Clear any old error message.
    set_error("");

    // Stop early if the inputs are missing.
    if (idx == NULL || filepath == NULL) {
        set_error("save: no index or file path was given.");
        return -1;
    }

    // Stop early if the index looks damaged.
    if (idx->buckets == NULL || idx->table_size <= 0 || idx->table_size > MAX_TABLE_SIZE ||
        idx->total_docs < 0 || idx->total_docs > MAX_DOCS ||
        (idx->total_docs > 0 && idx->docs == NULL)) {
        set_error("save: the index structure is not valid.");
        return -1;
    }

    // Count terms and postings first, so the header has the right numbers.
    int32_t term_count = 0;
    int32_t posting_count = 0;
    if (!count_index(idx, &term_count, &posting_count)) {
        set_error("save: the index contains a bad term or posting entry.");
        return -1;
    }

    // Write to a temporary file, then rename it, so a failed save keeps the old file.
    char tmp_path[MAX_PATH_LEN + 8];
    int len = snprintf(tmp_path, sizeof tmp_path, "%s.tmp", filepath);
    if (len < 0 || (size_t)len >= sizeof tmp_path) {
        set_error("save: the file path is too long.");
        return -1;
    }

    // Open the temp file for writing in binary mode.
    FILE* f = fopen(tmp_path, "wb");
    if (f == NULL) {
        set_error("save: cannot create the file (does the folder exist?).");
        return -1;
    }

    // Start the writer with an empty checksum.
    Writer w = { f, CHECKSUM_START, 1 };

    // Header: name tag, version, byte-order test, then the sizes.
    write_bytes(&w, FILE_MAGIC, 4);
    write_u32(&w, FILE_VERSION);
    write_u32(&w, BYTE_ORDER_CHECK);
    write_i32(&w, idx->table_size);
    write_i32(&w, idx->total_docs);
    write_i32(&w, term_count);
    write_i32(&w, posting_count);
    write_i64(&w, (int64_t)idx->total_corpus_terms);

    // Footer: the checksum, which is not counted in itself.
    uint32_t sum = w.sum;
    if (w.ok && fwrite(&sum, 1, sizeof sum, f) != sizeof sum) {
        w.ok = 0;
    }

    // Closing can fail too (for example a full disk).
    if (fclose(f) != 0) {
        w.ok = 0;
    }

    // If anything failed, delete the temp file and report it.
    if (!w.ok) {
        remove(tmp_path);
        set_error("save: writing the file failed (disk full?).");
        return -1;
    }

    // Swap the finished temp file in as the real file.
    // (On Windows rename() fails if the target exists, so delete the old one and retry.)
    if (rename(tmp_path, filepath) != 0) {
        remove(filepath);
        if (rename(tmp_path, filepath) != 0) {
            remove(tmp_path);
            set_error("save: could not replace the old index file.");
            return -1;
        }
    }

    // Success.
    return 0;
}
 