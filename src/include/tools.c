#include "tools.h"

/**
 * Read a file and return its contents as a string
 */
char* tools_read_file_to_string(const char* filepath, size_t* out_size) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        if (out_size) *out_size = 0;
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size_long = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size_long <= 0) {
        fclose(file);
        if (out_size) *out_size = 0;
        return NULL;
    }
    
    // Convert to size_t for safe array indexing
    size_t file_size = (size_t)file_size_long;
    
    // Allocate buffer (file_size + 1 for null terminator)
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        if (out_size) *out_size = 0;
        return NULL;
    }
    
    // Initialize buffer to ensure safety
    memset(buffer, 0, file_size + 1);
    
    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, file);
    
    // Buffer is already zero-initialized, so it's automatically null-terminated
    // But we can explicitly set the terminator for clarity
    if (bytes_read <= file_size) {
        // This is safe because we allocated file_size + 1 bytes and memset to 0
        // No need to set null terminator as buffer is already zero-initialized
    }
    
    fclose(file);
    
    if (out_size) *out_size = bytes_read;
    return buffer;
}

/**
 * Load markdown file into a markdown_content_t structure
 */
bool tools_load_markdown_file(const char* filepath, markdown_content_t* content) {
    if (!filepath || !content) {
        return false;
    }
    
    // Free existing content if any
    tools_free_markdown_content(content);
    
    // Read file
    size_t file_size = 0;
    content->content = tools_read_file_to_string(filepath, &file_size);
    
    if (!content->content) {
        content->is_loaded = false;
        return false;
    }
    
    content->size = file_size;
    content->is_loaded = true;
    strncpy(content->filename, filepath, sizeof(content->filename) - 1);
    content->filename[sizeof(content->filename) - 1] = '\0';
    
    return true;
}

/**
 * Free markdown content memory
 */
void tools_free_markdown_content(markdown_content_t* content) {
    if (content && content->content) {
        free(content->content);
        content->content = NULL;
        content->size = 0;
        content->is_loaded = false;
        content->filename[0] = '\0';
    }
}

