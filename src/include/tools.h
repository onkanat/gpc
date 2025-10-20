#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Maximum text buffer size for markdown content
#define MAX_MARKDOWN_SIZE 16384
#define MAX_LINE_LENGTH 1024

// Structure to hold markdown content and metadata
typedef struct {
    char* content;
    size_t size;
    bool is_loaded;
    char filename[256];
} markdown_content_t;

// Function declarations
bool tools_load_markdown_file(const char* filepath, markdown_content_t* content);
void tools_free_markdown_content(markdown_content_t* content);
char* tools_read_file_to_string(const char* filepath, size_t* out_size);

#endif // TOOLS_H