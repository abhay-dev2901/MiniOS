#ifndef STRING_H
#define STRING_H

#include "minios_types.h"

size_t my_strlen(const char *s);
void my_strcpy(char *dst, const char *src);
void my_strncpy(char *dst, const char *src, size_t n);
void my_strcat(char *dst, const char *src);
int my_strcmp(const char *a, const char *b);
int my_strncmp(const char *a, const char *b, size_t n);
void *my_memcpy(void *dst, const void *src, size_t n);
void *my_memset(void *dst, int c, size_t n);

/* Splits `line` in place on ASCII whitespace; writes pointers into argv. */
int my_tokenize(char *line, char **argv, int max_argv);

#endif
