#ifndef STRNDUP_H
#define STRNDUP_H

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
// Windows implementation of strndup
static inline char* strndup(const char* s, size_t n) {
    size_t len = strnlen(s, n);
    char* dup = (char*)malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len);
        dup[len] = '\0';
    }
    return dup;
}
#endif

#endif // STRNDUP_H 