#include "string.h"

size_t my_strlen(const char *s) {
    size_t n = 0;
    while (s[n])
        n++;
    return n;
}

void my_strcpy(char *dst, const char *src) {
    while ((*dst++ = *src++) != 0)
        ;
}

void my_strcat(char *dst, const char *src) {
    dst += my_strlen(dst);
    while ((*dst++ = *src++) != 0)
        ;
}

void my_strncpy(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = 0;
}

int my_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int my_strncmp(const char *a, const char *b, size_t n) {
    if (n == 0)
        return 0;
    while (n-- > 0 && *a && (*a == *b)) {
        if (n == 0)
            return 0;
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

void *my_memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--)
        *d++ = *s++;
    return dst;
}

void *my_memset(void *dst, int c, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    unsigned char v = (unsigned char)c;
    while (n--)
        *d++ = v;
    return dst;
}

int my_tokenize(char *line, char **argv, int max_argv) {
    int argc = 0;
    char *p = line;
    int in_token = 0;

    while (*p) {
        char ch = *p;
        int is_space = (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');

        if (!in_token && !is_space) {
            if (argc >= max_argv - 1)
                break;
            argv[argc++] = p;
            in_token = 1;
        } else if (in_token && is_space) {
            *p = 0;
            in_token = 0;
        }
        p++;
    }
    argv[argc] = NULL;
    return argc;
}
