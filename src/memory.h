#ifndef MEMORY_H
#define MEMORY_H

#include "minios_types.h"

void mem_init(void);
void *my_malloc(size_t size);
void my_free(void *ptr);

unsigned long mem_total_bytes(void);
unsigned long mem_used_bytes(void);
unsigned long mem_free_bytes(void);
unsigned long mem_alloc_count(void);

#endif