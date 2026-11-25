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

const char* persistence_last_error(void) {
    return last_error;
}

