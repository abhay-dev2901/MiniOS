#include "vfs.h"

#include "math.h"
#include "memory.h"
#include "string.h"

#define VFS_NAME_MAX 128

enum {
    VFS_OK = 0,
    VFS_ERR_NO_MEM    = -1,
    VFS_ERR_EXISTS    = -2,
    VFS_ERR_NOT_FOUND = -3,
    VFS_ERR_NOT_DIR   = -4,
    VFS_ERR_NOT_FILE  = -5,
    VFS_ERR_BAD_PATH  = -6,
    VFS_ERR_IO        = -7,
    VFS_ERR_NOT_EMPTY = -8   /* directory is not empty */
};

typedef struct VfsNode VfsNode;
struct VfsNode {
    char name[VFS_NAME_MAX];
    int is_dir;
    VfsNode *parent;
    VfsNode *first_child;
    VfsNode *next_sibling;
    size_t file_len;
    size_t file_cap;
    char *file_data;
};

static VfsNode *g_root;

const char *vfs_strerror(int err) {
    switch (err) {
    case VFS_OK:            return "ok";
    case VFS_ERR_NO_MEM:    return "out of memory";
    case VFS_ERR_EXISTS:    return "already exists";
    case VFS_ERR_NOT_FOUND: return "not found";
    case VFS_ERR_NOT_DIR:   return "not a directory";
    case VFS_ERR_NOT_FILE:  return "not a file";
    case VFS_ERR_BAD_PATH:  return "bad path";
    case VFS_ERR_IO:        return "io error";
    case VFS_ERR_NOT_EMPTY: return "directory not empty";
    default:                return "unknown error";
    }
}


static VfsNode *node_alloc(void) {
    return (VfsNode *)my_malloc(sizeof(VfsNode));
}

static void trim_slashes(const char *path, char *out, size_t out_sz) {
    size_t i;
    size_t j;
    int last_was_slash;

    j = 0;
    last_was_slash = 1;
    for (i = 0; path[i] && j + 1 < out_sz; i++) {
        char c = path[i];
        if (c == '/') {
            if (!last_was_slash) {
                out[j++] = '/';
                last_was_slash = 1;
            }
        } else {
            out[j++] = c;
            last_was_slash = 0;
        }
    }
    while (j > 0 && out[j - 1] == '/')
        j--;
    out[j] = 0;
}

static VfsNode *find_child(const VfsNode *dir, const char *name) {
    VfsNode *c;
    if (!dir || !dir->is_dir)
        return NULL;
    for (c = dir->first_child; c; c = c->next_sibling) {
        if (my_strcmp(c->name, name) == 0)
            return c;
    }
    return NULL;
}

static void attach_child(VfsNode *dir, VfsNode *child) {
    child->parent = dir;
    child->next_sibling = dir->first_child;
    dir->first_child = child;
}

static int vfs_walk_dir_segments(const char *path, int create, VfsNode **out) {
    char buf[512];
    VfsNode *cur;
    size_t start;
    size_t i;

    if (!path)
        return VFS_ERR_BAD_PATH;

    trim_slashes(path, buf, sizeof(buf));
    if (buf[0] == 0) {
        *out = g_root;
        return VFS_OK;
    }

    cur = g_root;
    start = 0;
    for (i = 0;; i++) {
        if (buf[i] && buf[i] != '/')
            continue;

        if (i == start) {
            if (buf[i] == 0)
                break;
            start = i + 1;
            continue;
        }

        {
            char seg[VFS_NAME_MAX];
            size_t len = i - start;
            VfsNode *next;

            if (len >= sizeof(seg))
                return VFS_ERR_BAD_PATH;
            my_memcpy(seg, buf + start, len);
            seg[len] = 0;

            if (!cur->is_dir)
                return VFS_ERR_NOT_DIR;

            next = find_child(cur, seg);
            if (!next) {
                if (!create)
                    return VFS_ERR_NOT_FOUND;
                next = node_alloc();
                if (!next)
                    return VFS_ERR_NO_MEM;
                my_memset(next, 0, sizeof(*next));
                my_strcpy(next->name, seg);
                next->is_dir = 1;
                attach_child(cur, next);
            }
            if (!next->is_dir)
                return VFS_ERR_NOT_DIR;
            cur = next;
        }

        if (buf[i] == 0)
            break;
        start = i + 1;
    }

    *out = cur;
    return VFS_OK;
}

void vfs_init(void) {
    g_root = node_alloc();
    if (!g_root)
        return;
    my_memset(g_root, 0, sizeof(*g_root));
    my_strcpy(g_root->name, "/");
    g_root->is_dir = 1;
}

static int ensure_parent(const char *path, int create_dirs, VfsNode **parent, char *name) {
    char tmp[512];
    size_t i;
    size_t last_slash = 0;
    int has_slash = 0;

    if (!path || path[0] == 0)
        return VFS_ERR_BAD_PATH;

    my_strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;

    for (i = 0; tmp[i]; i++) {
        if (tmp[i] == '/') {
            last_slash = i;
            has_slash = 1;
        }
    }

    if (!has_slash) {
        /* single segment relative to root */
        my_strcpy(name, tmp);
        *parent = g_root;
        return VFS_OK;
    }

    if (last_slash == 0) {
        /* "/foo" */
        if (tmp[1] == 0)
            return VFS_ERR_BAD_PATH;
        my_strcpy(name, tmp + 1);
        *parent = g_root;
        return VFS_OK;
    }

    tmp[last_slash] = 0;
    if (vfs_walk_dir_segments(tmp, create_dirs, parent) != VFS_OK)
        return VFS_ERR_NOT_FOUND;
    my_strcpy(name, tmp + last_slash + 1);
    if (name[0] == 0)
        return VFS_ERR_BAD_PATH;
    return VFS_OK;
}

int vfs_make_dir(const char *path) {
    VfsNode *parent;
    char name[VFS_NAME_MAX];
    VfsNode *n;
    int st;

    if (!g_root)
        return VFS_ERR_NO_MEM;

    st = ensure_parent(path, 1, &parent, name);
    if (st != VFS_OK)
        return st;
    if (!parent->is_dir)
        return VFS_ERR_NOT_DIR;
    if (find_child(parent, name))
        return VFS_ERR_EXISTS;

    n = node_alloc();
    if (!n)
        return VFS_ERR_NO_MEM;
    my_memset(n, 0, sizeof(*n));
    my_strcpy(n->name, name);
    n->is_dir = 1;
    attach_child(parent, n);
    return VFS_OK;
}

int vfs_create_file(const char *path) {
    VfsNode *parent;
    char name[VFS_NAME_MAX];
    VfsNode *n;
    int st;

    if (!g_root)
        return VFS_ERR_NO_MEM;

    st = ensure_parent(path, 1, &parent, name);
    if (st != VFS_OK)
        return st;
    if (!parent->is_dir)
        return VFS_ERR_NOT_DIR;
    if (find_child(parent, name))
        return VFS_ERR_EXISTS;

    n = node_alloc();
    if (!n)
        return VFS_ERR_NO_MEM;
    my_memset(n, 0, sizeof(*n));
    my_strcpy(n->name, name);
    n->is_dir = 0;
    attach_child(parent, n);
    return VFS_OK;
}

static int split_parent_name(const char *path, VfsNode **parent, char *name) {
    char tmp[512];
    size_t i;
    size_t last_slash = 0;
    int has_slash = 0;

    if (!path || path[0] == 0)
        return VFS_ERR_BAD_PATH;

    my_strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;

    for (i = 0; tmp[i]; i++) {
        if (tmp[i] == '/') {
            last_slash = i;
            has_slash = 1;
        }
    }

    if (!has_slash) {
        *parent = g_root;
        my_strcpy(name, tmp);
        return VFS_OK;
    }

    if (last_slash == 0) {
        if (tmp[1] == 0)
            return VFS_ERR_BAD_PATH;
        my_strcpy(name, tmp + 1);
        *parent = g_root;
        return VFS_OK;
    }

    tmp[last_slash] = 0;
    if (vfs_walk_dir_segments(tmp, 0, parent) != VFS_OK)
        return VFS_ERR_NOT_FOUND;
    my_strcpy(name, tmp + last_slash + 1);
    if (name[0] == 0)
        return VFS_ERR_BAD_PATH;
    return VFS_OK;
}

static VfsNode *lookup_file(const char *path) {
    VfsNode *dir;
    char name[VFS_NAME_MAX];
    VfsNode *n;

    if (!g_root)
        return NULL;
    if (split_parent_name(path, &dir, name) != VFS_OK)
        return NULL;
    if (name[0] == 0)
        return NULL;
    n = find_child(dir, name);
    if (!n || n->is_dir)
        return NULL;
    return n;
}

static VfsNode *lookup_any(const char *path) {
    VfsNode *dir;
    char name[VFS_NAME_MAX];
    VfsNode *n;

    if (!g_root)
        return NULL;

    if (path[0] == '/' && path[1] == 0)
        return g_root;

    if (split_parent_name(path, &dir, name) != VFS_OK)
        return NULL;
    if (name[0] == 0)
        return dir;
    n = find_child(dir, name);
    return n;
}

int vfs_is_dir(const char *path) {
    VfsNode *n;

    if (!g_root)
        return 0;
    n = lookup_any(path);
    if (!n)
        return 0;
    return n->is_dir ? 1 : 0;
}

int vfs_write_file(const char *path, const void *data, size_t len, int append) {
    VfsNode *n = lookup_file(path);
    char *newbuf;
    size_t newcap;
    size_t base;

    if (!n)
        return VFS_ERR_NOT_FOUND;
    if (n->is_dir)
        return VFS_ERR_NOT_FILE;

    base = append ? n->file_len : 0;
    newcap = base + len;
    if (newcap < base)
        return VFS_ERR_IO;

    if (n->file_cap < newcap) {
        /*
         * Grow to the next power of two >= newcap.
         * my_next_pow2 replaces the manual doubling loop and also
         * handles the edge case where newcap is already a power of two.
         * Minimum capacity: 256 bytes.
         */
        size_t cap = my_max_sz(my_next_pow2(newcap), 256UL);
        newbuf = (char *)my_malloc(cap);
        if (!newbuf)
            return VFS_ERR_NO_MEM;
        if (n->file_data) {
            if (base > 0)
                my_memcpy(newbuf, n->file_data, base);
            my_free(n->file_data);
        }
        n->file_data = newbuf;
        n->file_cap = cap;
    }

    if (len && data)
        my_memcpy(n->file_data + base, data, len);
    n->file_len = base + len;
    return VFS_OK;
}

int vfs_read_file(const char *path, void *buf, size_t buf_size, size_t *out_len) {
    VfsNode *n = lookup_file(path);
    size_t copy;

    if (!n)
        return VFS_ERR_NOT_FOUND;
    /* Use my_min_sz so the copy length is always within bounds. */
    copy = my_min_sz(n->file_len, buf_size);
    if (copy && n->file_data && buf)
        my_memcpy(buf, n->file_data, copy);
    if (out_len)
        *out_len = copy;
    return VFS_OK;
}

typedef struct {
    void (*emit)(const char *name, int is_dir, void *ctx);
    void *ctx;
} ListCtx;

static void list_emit(VfsNode *dir, ListCtx *lc) {
    VfsNode *c;
    for (c = dir->first_child; c; c = c->next_sibling) {
        lc->emit(c->name, c->is_dir, lc->ctx);
    }
}

int vfs_list_dir(const char *path, void (*emit)(const char *name, int is_dir, void *ctx), void *ctx) {
    VfsNode *n;
    ListCtx lc;

    n = lookup_any(path);
    if (!n)
        return VFS_ERR_NOT_FOUND;
    if (!n->is_dir)
        return VFS_ERR_NOT_DIR;

    lc.emit = emit;
    lc.ctx = ctx;
    list_emit(n, &lc);
    return VFS_OK;
}

/* ---- Extended VFS operations ---- */

/*
 * vfs_stat: query metadata for any path.
 * Fills *out_is_dir (1 = directory) and *out_size (byte length for files).
 */
int vfs_stat(const char *path, int *out_is_dir, size_t *out_size) {
    VfsNode *n;

    if (!g_root)
        return VFS_ERR_NO_MEM;
    n = lookup_any(path);
    if (!n)
        return VFS_ERR_NOT_FOUND;
    if (out_is_dir)
        *out_is_dir = n->is_dir ? 1 : 0;
    if (out_size)
        *out_size = n->is_dir ? 0 : n->file_len;
    return VFS_OK;
}

/*
 * vfs_remove: remove a file or an empty directory.
 * Returns VFS_ERR_NOT_EMPTY if a non-empty directory is targeted.
 */
int vfs_remove(const char *path) {
    VfsNode *parent;
    char name[VFS_NAME_MAX];
    VfsNode *n;
    VfsNode **pp;
    int st;

    if (!g_root)
        return VFS_ERR_NO_MEM;

    /* Protect root */
    if (path[0] == '/' && path[1] == 0)
        return VFS_ERR_BAD_PATH;

    st = split_parent_name(path, &parent, name);
    if (st != VFS_OK)
        return st;
    if (name[0] == 0)
        return VFS_ERR_BAD_PATH;

    /* Walk parent's child list to find and unlink */
    pp = &parent->first_child;
    n  = NULL;
    while (*pp) {
        if (my_strcmp((*pp)->name, name) == 0) {
            n = *pp;
            break;
        }
        pp = &(*pp)->next_sibling;
    }
    if (!n)
        return VFS_ERR_NOT_FOUND;

    /* Refuse to remove a non-empty directory */
    if (n->is_dir && n->first_child)
        return VFS_ERR_NOT_EMPTY;

    /* Unlink from sibling chain */
    *pp = n->next_sibling;

    /* Release file data if applicable */
    if (!n->is_dir && n->file_data)
        my_free(n->file_data);
    my_free(n);
    return VFS_OK;
}

/*
 * vfs_rename: atomically move/rename a node within the VFS tree.
 * The destination path must not already exist.
 */
int vfs_rename(const char *src, const char *dst) {
    VfsNode *src_parent, *dst_parent;
    char src_name[VFS_NAME_MAX], dst_name[VFS_NAME_MAX];
    VfsNode *n;
    VfsNode **pp;
    int st;

    if (!g_root)
        return VFS_ERR_NO_MEM;

    st = split_parent_name(src, &src_parent, src_name);
    if (st != VFS_OK) return st;
    st = split_parent_name(dst, &dst_parent, dst_name);
    if (st != VFS_OK) return st;

    if (src_name[0] == 0 || dst_name[0] == 0)
        return VFS_ERR_BAD_PATH;

    /* Locate source node */
    pp = &src_parent->first_child;
    n  = NULL;
    while (*pp) {
        if (my_strcmp((*pp)->name, src_name) == 0) {
            n = *pp;
            break;
        }
        pp = &(*pp)->next_sibling;
    }
    if (!n)
        return VFS_ERR_NOT_FOUND;

    /* Destination must be absent */
    if (find_child(dst_parent, dst_name))
        return VFS_ERR_EXISTS;

    /* Unlink from source parent */
    *pp = n->next_sibling;

    /* Update name */
    my_strncpy(n->name, dst_name, VFS_NAME_MAX - 1);
    n->name[VFS_NAME_MAX - 1] = 0;

    /* Attach to destination parent */
    n->parent       = dst_parent;
    n->next_sibling = dst_parent->first_child;
    dst_parent->first_child = n;
    return VFS_OK;
}

/*
 * vfs_copy_file: duplicate a regular file to a new path.
 * The destination file must not exist beforehand.
 */
int vfs_copy_file(const char *src, const char *dst) {
    VfsNode *sn;
    int st;

    sn = lookup_file(src);
    if (!sn)
        return VFS_ERR_NOT_FOUND;

    st = vfs_create_file(dst);
    if (st != VFS_OK)
        return st;

    if (sn->file_len > 0)
        return vfs_write_file(dst, sn->file_data, sn->file_len, 0);
    return VFS_OK;
}
