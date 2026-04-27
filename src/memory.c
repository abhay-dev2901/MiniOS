#include "memory.h"

#include "math.h"
#include "string.h"

#define ARENA_SIZE (10UL * 1024UL * 1024UL)
#define MIN_SPLIT_PAYLOAD 64UL

typedef struct MemHdr MemHdr;
struct MemHdr {
    size_t size; /* payload bytes following this header */
    int free;    /* 1 if in free list */
    MemHdr *next;
};

static unsigned char g_arena[ARENA_SIZE];
static MemHdr *g_free_list;
static unsigned long g_used;
static unsigned long g_allocs;

static void *hdr_payload(MemHdr *h) {
    return (unsigned char *)h + sizeof(MemHdr);
}

static MemHdr *payload_to_hdr(void *p) {
    return (MemHdr *)((unsigned char *)p - sizeof(MemHdr));
}

static void free_list_insert_sorted(MemHdr *h) {
    MemHdr **pp = &g_free_list;

    h->free = 1;
    h->next = NULL;
    while (*pp && (unsigned char *)(*pp) < (unsigned char *)h)
        pp = &(*pp)->next;
    h->next = *pp;
    *pp = h;
}

static void free_list_remove(MemHdr *h) {
    MemHdr **pp = &g_free_list;
    while (*pp) {
        if (*pp == h) {
            *pp = h->next;
            h->next = NULL;
            return;
        }
        pp = &(*pp)->next;
    }
}

static void coalesce_sorted_free_list(void) {
    MemHdr *cur;

    cur = g_free_list;
    while (cur && cur->next) {
        unsigned char *a_end = (unsigned char *)cur + sizeof(MemHdr) + cur->size;
        if (a_end == (unsigned char *)cur->next && cur->next->free) {
            MemHdr *n = cur->next;
            cur->size += sizeof(MemHdr) + n->size;
            cur->next = n->next;
            n->next = NULL;
            continue;
        }
        cur = cur->next;
    }
}

void mem_init(void) {
    MemHdr *h;

    my_memset(g_arena, 0, sizeof(g_arena));
    g_free_list = NULL;
    g_used = 0;
    g_allocs = 0;

    h = (MemHdr *)g_arena;
    h->size = ARENA_SIZE - sizeof(MemHdr);
    h->free = 1;
    h->next = NULL;
    g_free_list = h;
}

void *my_malloc(size_t size) {
    MemHdr *cur;

    if (size == 0)
        return NULL;

    /*
     * Round the requested size up to the next 8-byte boundary.
     * This guarantees that all returned payloads satisfy the
     * strictest fundamental alignment required by any C type,
     * preventing unaligned-access faults on strict architectures.
     */
    size = my_align_up(size, 8UL);

    for (cur = g_free_list; cur; cur = cur->next) {
        if (!cur->free)
            continue;
        if (cur->size < size)
            continue;

        free_list_remove(cur);
        cur->free = 0;
        cur->next = NULL;

        if (cur->size >= size + sizeof(MemHdr) + MIN_SPLIT_PAYLOAD) {
            MemHdr *tail = (MemHdr *)((unsigned char *)cur + sizeof(MemHdr) + size);
            tail->size = cur->size - size - sizeof(MemHdr);
            tail->free = 1;
            tail->next = NULL;
            cur->size = size;
            free_list_insert_sorted(tail);
        }

        g_used += sizeof(MemHdr) + cur->size;
        g_allocs++;
        my_memset(hdr_payload(cur), 0, cur->size);
        return hdr_payload(cur);
    }

    return NULL;
}

void my_free(void *ptr) {
    MemHdr *h;

    if (!ptr)
        return;
    h = payload_to_hdr(ptr);
    if (h->free)
        return;

    g_used -= sizeof(MemHdr) + h->size;
    g_allocs--;
    free_list_insert_sorted(h);
    coalesce_sorted_free_list();
}

unsigned long mem_total_bytes(void) {
    return (unsigned long)ARENA_SIZE;
}

unsigned long mem_used_bytes(void) {
    return g_used;
}

unsigned long mem_free_bytes(void) {
    if (g_used > (unsigned long)ARENA_SIZE)
        return 0;
    return (unsigned long)ARENA_SIZE - g_used;
}

unsigned long mem_alloc_count(void) {
    return g_allocs;
}












// #include "memory.h"

// #include "string.h"

// #define ARENA_SIZE (10UL * 1024UL * 1024UL)
// #define MIN_SPLIT_PAYLOAD 64UL

// typedef struct MemHdr MemHdr;
// struct MemHdr {
//     size_t size; /* payload bytes following this header */
//     int free;    /* 1 if in free list */
//     MemHdr *next;
// };

// static unsigned char g_arena[ARENA_SIZE];
// static MemHdr *g_free_list;
// static unsigned long g_used;
// static unsigned long g_allocs;

// static void *hdr_payload(MemHdr *h) {
//     return (unsigned char *)h + sizeof(MemHdr);
// }

// static MemHdr *payload_to_hdr(void *p) {
//     return (MemHdr *)((unsigned char *)p - sizeof(MemHdr));
// }

// static void free_list_insert_sorted(MemHdr *h) {
//     MemHdr **pp = &g_free_list;

//     h->free = 1;
//     h->next = NULL;
//     while (*pp && (unsigned char *)(*pp) < (unsigned char *)h)
//         pp = &(*pp)->next;
//     h->next = *pp;
//     *pp = h;
// }

// static void free_list_remove(MemHdr *h) {
//     MemHdr **pp = &g_free_list;
//     while (*pp) {
//         if (*pp == h) {
//             *pp = h->next;
//             h->next = NULL;
//             return;
//         }
//         pp = &(*pp)->next;
//     }
// }

// static void coalesce_sorted_free_list(void) {
//     MemHdr *cur;

//     cur = g_free_list;
//     while (cur && cur->next) {
//         unsigned char *a_end = (unsigned char *)cur + sizeof(MemHdr) + cur->size;
//         if (a_end == (unsigned char *)cur->next && cur->next->free) {
//             MemHdr *n = cur->next;
//             cur->size += sizeof(MemHdr) + n->size;
//             cur->next = n->next;
//             n->next = NULL;
//             continue;
//         }
//         cur = cur->next;
//     }
// }

// void mem_init(void) {
//     MemHdr *h;

//     my_memset(g_arena, 0, sizeof(g_arena));
//     g_free_list = NULL;
//     g_used = 0;
//     g_allocs = 0;

//     h = (MemHdr *)g_arena;
//     h->size = ARENA_SIZE - sizeof(MemHdr);
//     h->free = 1;
//     h->next = NULL;
//     g_free_list = h;
// }

// void *my_malloc(size_t size) {
//     MemHdr *cur;

//     if (size == 0)
//         return NULL;

//     for (cur = g_free_list; cur; cur = cur->next) {
//         if (!cur->free)
//             continue;
//         if (cur->size < size)
//             continue;

//         free_list_remove(cur);
//         cur->free = 0;
//         cur->next = NULL;

//         if (cur->size >= size + sizeof(MemHdr) + MIN_SPLIT_PAYLOAD) {
//             MemHdr *tail = (MemHdr *)((unsigned char *)cur + sizeof(MemHdr) + size);
//             tail->size = cur->size - size - sizeof(MemHdr);
//             tail->free = 1;
//             tail->next = NULL;
//             cur->size = size;
//             free_list_insert_sorted(tail);
//         }

//         g_used += sizeof(MemHdr) + cur->size;
//         g_allocs++;
//         my_memset(hdr_payload(cur), 0, cur->size);
//         return hdr_payload(cur);
//     }

//     return NULL;
// }

// void my_free(void *ptr) {
//     MemHdr *h;

//     if (!ptr)
//         return;
//     h = payload_to_hdr(ptr);
//     if (h->free)
//         return;

//     g_used -= sizeof(MemHdr) + h->size;
//     g_allocs--;
//     free_list_insert_sorted(h);
//     coalesce_sorted_free_list();
// }

// unsigned long mem_total_bytes(void) {
//     return (unsigned long)ARENA_SIZE;
// }

// unsigned long mem_used_bytes(void) {
//     return g_used;
// }

// unsigned long mem_free_bytes(void) {
//     if (g_used > (unsigned long)ARENA_SIZE)
//         return 0;
//     return (unsigned long)ARENA_SIZE - g_used;
// }

// unsigned long mem_alloc_count(void) {
//     return g_allocs;
// }
