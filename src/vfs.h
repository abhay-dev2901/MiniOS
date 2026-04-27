#ifndef VFS_H
#define VFS_H

#include "minios_types.h"

void vfs_init(void);

int vfs_is_dir(const char *path);

/* Core file / directory operations */
int vfs_make_dir(const char *path);
int vfs_create_file(const char *path);
int vfs_write_file(const char *path, const void *data, size_t len, int append);
int vfs_read_file(const char *path, void *buf, size_t buf_size, size_t *out_len);
int vfs_list_dir(const char *path, void (*emit)(const char *name, int is_dir, void *ctx), void *ctx);

/* Extended VFS operations */
int vfs_remove(const char *path);         /* remove file or empty directory */
int vfs_copy_file(const char *src, const char *dst);  /* copy file contents */
int vfs_rename(const char *src, const char *dst);     /* rename / move node */
int vfs_stat(const char *path, int *out_is_dir, size_t *out_size); /* query metadata */

const char *vfs_strerror(int err);

#endif
