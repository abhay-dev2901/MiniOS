#ifndef SYS_H
#define SYS_H

#include "minios_types.h"

void sys_init(void);
void print(const char *s);
void print_n(const char *s, size_t n);
void print_uint(unsigned long n);
void print_int(long n);
void print_hex(unsigned long n);

/* Reads until newline, EOF, or buffer full. Returns number of chars stored (excluding NUL). */
int read_line(char *buf, int size);

#endif
