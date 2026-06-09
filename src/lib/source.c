#include "yap/all.h"

char* yap_pos_string(yap_source s, unsigned int line, unsigned int col){
    return strus_newf("%s:%u:%u", s.label, line+1, col);
}

size_t yap_read_file_to_string(const char *path, char **out) {
    *out = NULL;  // default to NULL in case of failure

    FILE *f = fopen(path, "rb");
    if (!f) {
        return 0; // failed to open
    }

    // Seek to end to determine size
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return 0;
    }
    rewind(f);

    // Allocate buffer (+1 for null terminator)
    char *buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(f);
        return 0;
    }

    // Read file contents
    size_t read_size = fread(buffer, 1, (size_t)size, f);
    fclose(f);

    if (read_size != (size_t)size) {
        free(buffer);
        return 0;
    }

    buffer[size] = '\0'; // null terminate
    *out = buffer;
    return (size_t)size;
}