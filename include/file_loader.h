#ifndef FILE_LOADER_H
#define FILE_LOADER_H

#include "common.h"

Document* load_document_metadata(const char* folder_path, int* out_doc_count);
char* read_file_content(const char* filepath);

#endif /* FILE_LOADER_H */
