# MiniOS Simulator — Feature Reference

> A bare-metal-style OS simulator written in pure C, running entirely in user
> space without the standard C library (only POSIX syscalls: `read`, `write`,
> `tcgetattr`/`tcsetattr`).  Every subsystem—memory, file system, scheduler,
> and shell—is hand-rolled from scratch.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Memory Allocator](#memory-allocator)
3. [Virtual File System (VFS)](#virtual-file-system-vfs)
4. [Cooperative Task Scheduler](#cooperative-task-scheduler)
5. [Interactive Shell](#interactive-shell)
6. [Line Editor (readline)](#line-editor-readline)
7. [Screen Library](#screen-library)
8. [Keyboard Library](#keyboard-library)
9. [Custom LibC Subset](#custom-libc-subset)
10. [Shell Command Reference](#shell-command-reference)
11. [Build & Run](#build--run)
12. [File Map](#file-map)

---

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────────┐
│                           main.c                                 │
│   rl_getline() ──► shell_exec_line() ──► commands                │
│              scheduler_tick() on each iteration                  │
└────────┬───────────┬──────────────┬───────────────────────────── ┘
         │           │              │
    readline.c    shell.c      scheduler.c
  (history,edit) (dispatch)   (bg tasks)
         │           │              │
    keyboard.c    vfs.c         memory.c
  (raw tty I/O) (in-mem FS)   (arena alloc)
         │           │
      screen.c    string.c    math.c
   (all output)  (parsing)  (alignment,
  uses string.c              pow2, gcd…)
```

**Library pipeline (keyboard → string → math → memory → screen):**

1. `keyboard.c` puts terminal in raw mode; `kbd_getchar()` feeds bytes to `readline.c`.
2. `readline.c` assembles a complete line and calls `shell_exec_line()`.
3. `shell.c` calls `my_tokenize()` from `string.c` to split arguments.
4. Command handlers use `math.c` for boundary logic and `memory.c` (via VFS) for allocation.
5. All output goes through `screen.c`, which calls `my_uint_to_str()` from `string.c`.

---

## Memory Allocator

| Label | `memory.c` / `memory.h` |
|-------|--------------------------|

### Description

A **first-fit free-list allocator** backed by a single 10 MB static arena
(`g_arena`).  On startup the entire arena is one large free block.  Every
allocation is preceded by a `MemHdr` header that records the payload size
and a *free* flag.

### Key Design Decisions

| Feature | Detail |
|---------|--------|
| **Arena size** | `ARENA_SIZE = 10 MB` (compile-time constant) |
| **Algorithm** | First-fit: walks free list, takes first block that fits |
| **Splitting** | Remainder block created when leftover ≥ `sizeof(MemHdr) + 64` bytes |
| **Freeing** | Block inserted into sorted free list; adjacent free blocks are **coalesced** immediately |
| **Zero-init** | Every allocation is zero-filled (`my_memset`) |
| **No system calls** | All allocations come from `g_arena[]`; no `brk`/`mmap` |

### Public API

| Function | Returns | Description |
|----------|---------|-------------|
| `mem_init()` | `void` | Reset arena; create initial free block |
| `my_malloc(size)` | `void *` | Allocate `size` bytes (zero-filled) |
| `my_free(ptr)` | `void` | Return block; coalesce neighbours |
| `mem_total_bytes()` | `unsigned long` | Arena capacity |
| `mem_used_bytes()` | `unsigned long` | Bytes currently allocated (including headers) |
| `mem_free_bytes()` | `unsigned long` | Available bytes |
| `mem_alloc_count()` | `unsigned long` | Live object count |

### Shell command

```
meminfo
```

Output: `sim_ram_total=10485760 used=2048 free=10483712 live_objects=3`

---

## Virtual File System (VFS)

| Label | `vfs.c` / `vfs.h` |
|-------|--------------------|

### Description

An **in-memory hierarchical file system** implemented as a tree of
`VfsNode` structs allocated from the custom heap.  Supports arbitrary-depth
paths, files with dynamic content, and empty-directory removal.

### Node Structure

```c
struct VfsNode {
    char      name[128];      /* entry name (not full path)         */
    int       is_dir;         /* 1 = directory, 0 = regular file    */
    VfsNode  *parent;         /* parent directory node              */
    VfsNode  *first_child;    /* first child (linked list head)     */
    VfsNode  *next_sibling;   /* next sibling in parent's list      */
    size_t    file_len;       /* current data length (files only)   */
    size_t    file_cap;       /* allocated buffer capacity          */
    char     *file_data;      /* heap-allocated file content buffer */
};
```

### Write Buffer Growth Strategy

File buffers grow by **doubling** (starting at 256 B), keeping realloc
count at O(log n) for sequential writes.

### Error Codes

| Code | Meaning |
|------|---------|
| `VFS_OK (0)` | Success |
| `VFS_ERR_NO_MEM (-1)` | Allocator exhausted |
| `VFS_ERR_EXISTS (-2)` | Path already exists |
| `VFS_ERR_NOT_FOUND (-3)` | Path does not exist |
| `VFS_ERR_NOT_DIR (-4)` | Expected directory, got file |
| `VFS_ERR_NOT_FILE (-5)` | Expected file, got directory |
| `VFS_ERR_BAD_PATH (-6)` | Null, empty, or malformed path |
| `VFS_ERR_IO (-7)` | Integer overflow in size calculation |
| `VFS_ERR_NOT_EMPTY (-8)` | `rm` on a non-empty directory |

### Public API

| Function | Label | Description |
|----------|-------|-------------|
| `vfs_init()` | CORE | Allocate root node `/` |
| `vfs_is_dir(path)` | CORE | Returns 1 if `path` is a directory |
| `vfs_make_dir(path)` | CORE | Create directory (parents must exist) |
| `vfs_create_file(path)` | CORE | Create empty regular file |
| `vfs_write_file(path, data, len, append)` | CORE | Write or append to file |
| `vfs_read_file(path, buf, sz, &got)` | CORE | Read up to `sz` bytes |
| `vfs_list_dir(path, emit, ctx)` | CORE | Enumerate directory entries via callback |
| `vfs_remove(path)` | **NEW** | Remove file or **empty** directory |
| `vfs_copy_file(src, dst)` | **NEW** | Duplicate file content to new path |
| `vfs_rename(src, dst)` | **NEW** | Rename / move node (atomic pointer swap) |
| `vfs_stat(path, &is_dir, &size)` | **NEW** | Query metadata without reading content |
| `vfs_strerror(err)` | UTIL | Human-readable error string |

---

## Cooperative Task Scheduler

| Label | `scheduler.c` / `scheduler.h` |
|-------|-------------------------------|

### Description

A **cooperative round-robin scheduler** that maintains a singly-linked list
of `Task` nodes.  `scheduler_tick()` is called by the main loop once per
`rl_getline` return, giving each live task one step per user command.

### Task Structure

```c
struct Task {
    int        id;     /* monotonically increasing unique ID   */
    int        alive;  /* 1 = running, 0 = dead (killed)       */
    const char *name;  /* "counter" | "spinner"                */
    TaskStepFn  step;  /* function pointer to per-tick handler  */
    void       *ctx;   /* private task context (heap-allocated) */
    Task       *next;  /* next task in linked list              */
};
```

### Built-in Task Types

| Task | What it does |
|------|-------------|
| `counter` | Increments a 64-bit counter each tick |
| `spinner` | Runs a 4096-iteration hash loop each tick (CPU-bound simulation) |

### Public API

| Function | Label | Description |
|----------|-------|-------------|
| `scheduler_init()` | CORE | Free all tasks; reset ID counter |
| `scheduler_tick()` | CORE | One step for every live task |
| `scheduler_spawn_bg(name)` | CORE | Allocate and enqueue a named task |
| `scheduler_kill(id)` | **NEW** | Mark task `id` as dead (non-destructive) |
| `scheduler_dump_ps()` | CORE | Print process table (`ps`) |

### `kill` semantics

Killed tasks remain in the list with `alive=0` so their ID never gets
reused within a session.  `ps` shows them as `off`.

---

## Interactive Shell

| Label | `shell.c` / `shell.h` |
|-------|------------------------|

### Description

The shell tokenises each input line with `my_tokenize`, then dispatches to
command handler functions (`cmd_*`).  It maintains a **current working
directory** (`g_cwd`) and resolves all relative paths before calling into VFS.

### Path Resolution Rules

| Input | Resolved to |
|-------|-------------|
| `/foo/bar` | `/foo/bar` (absolute, used as-is) |
| `bar` | `<cwd>/bar` |
| `..` in `cd` | Parent of `g_cwd` (pointer walk) |
| `/` in `cd` | Root |

### New Shell Features (v2)

| Feature | Label | Detail |
|---------|-------|--------|
| `rm` | **NEW** | Calls `vfs_remove`; refuses non-empty dirs |
| `cp` | **NEW** | Calls `vfs_copy_file`; dst must not exist |
| `mv` | **NEW** | Calls `vfs_rename`; atomic pointer re-link |
| `write` | **NEW** | `write <file> <text>` — create + overwrite via VFS |
| `read` | **NEW** | `read <file>` — print file contents (spec command) |
| `ls -l` | **NEW** | Long listing with type, size (bytes), and name |
| `cat -n` | **NEW** | Print file with line numbers |
| `echo >> file` | **NEW** | Append mode redirect (previously only `>`) |
| `kill <id>` | **NEW** | Forwards to `scheduler_kill` |
| `history` | **NEW** | Prints rl history via `rl_print_history` |
| `clear` | **NEW** | Delegates to `screen_clear()` |
| `mathinfo` | **NEW** | Live demo of all math library functions |

---

## Line Editor (readline)

| Label | `readline.c` / `readline.h` |
|-------|------------------------------|

### Description

A self-contained interactive line editor that uses `keyboard.c` for raw input
and `screen.c` for all output. All state is **statically allocated** — zero heap usage.

### Features

| Feature | Label | Detail |
|---------|-------|--------|
| Raw terminal mode | CORE | Delegated to `keyboard.c` |
| Backspace / DEL | CORE | Deletes char to the left; redraws tail via `screen.c` |
| Arrow LEFT / RIGHT | CORE | Moves cursor; inserts at arbitrary position |
| Arrow UP / DOWN | **NEW** | Navigate command history ring buffer |
| Home / End | **NEW** | Jump cursor to line start / end |
| Ctrl-C | **NEW** | Clears current line; re-displays prompt |
| Ctrl-L | **NEW** | Clears screen via `screen_clear()`; redraws |
| History ring buffer | **NEW** | Stores last 64 unique non-empty commands |
| History deduplication | **NEW** | Consecutive identical commands stored once |
| `history` command | **NEW** | `rl_print_history()` prints numbered list |
| Graceful EOF | CORE | Returns `-1` on Ctrl-D / pipe close |
| Terminal restore | CORE | `rl_cleanup()` → `kbd_cleanup()` on exit |

---

## Screen Library

| Label | `screen.c` / `screen.h` |
|-------|-------------------------|

### Description

All terminal output in MiniOS goes through `screen.c`. Integer-to-string
conversion uses `string.c`'s `my_uint_to_str` / `my_int_to_str`, making the
`string → screen → terminal` pipeline explicit and measurable.

### Public API

| Function | Description |
|----------|-------------|
| `screen_clear()` | Erase entire display; home cursor |
| `screen_move(row, col)` | ANSI cursor position (1-based) |
| `screen_cursor_left(n)` | Move n columns left |
| `screen_cursor_right(n)` | Move n columns right |
| `screen_erase_line_right()` | Erase from cursor to EOL |
| `screen_save_cursor()` | Save cursor position |
| `screen_restore_cursor()` | Restore saved cursor position |
| `screen_putchar(c)` | Write one character |
| `screen_print(s)` | Write NUL-terminated string |
| `screen_print_n(s, n)` | Write n bytes |
| `screen_print_uint(n)` | Decimal unsigned long (via `string.c`) |
| `screen_print_int(n)` | Decimal signed long (via `string.c`) |
| `screen_print_hex(n)` | Hex with `0x` prefix (via `string.c`) |

---

## Keyboard Library

| Label | `keyboard.c` / `keyboard.h` |
|-------|-----------------------------|

### Description

Owns the POSIX terminal raw-mode lifecycle and exposes three input primitives.
No standard C library functions used — only `termios.h`, `unistd.h`, `fcntl.h`.

### Public API

| Function | Description |
|----------|-------------|
| `kbd_init()` | Enable raw mode (`tcsetattr`, disables echo/canonical) |
| `kbd_cleanup()` | Restore original terminal state |
| `kbd_keyPressed()` | **Non-blocking**: returns keycode or -1 if no key pending |
| `kbd_getchar()` | **Blocking**: returns next raw keycode or -1 on EOF |
| `kbd_readLine(buf, size)` | **Blocking**: full line read with no editing |

### `keyPressed` implementation
Temporarily sets `O_NONBLOCK` on stdin via `fcntl`, attempts a 1-byte `read()`,
then restores the original flags — no threads, no signals.


### History Implementation

```
g_hist[READLINE_HIST_MAX][READLINE_LINE_MAX]   ← ring buffer
g_hist_head   ← index of next write slot
g_hist_count  ← total pushes ever (for sequence numbering)

hist_get(offset=1) → last command
hist_get(offset=2) → second-to-last
...
```

---

## Custom LibC Subset

### `string.c` / `string.h`

| Function | Description |
|----------|-------------|
| `my_strlen(s)` | Byte length of a NUL-terminated string |
| `my_strcpy(dst, src)` | Copy string (no bounds check; caller ensures) |
| `my_strncpy(dst, src, n)` | Copy at most `n` bytes, always NUL-terminate |
| `my_strcat(dst, src)` | Append `src` to `dst` |
| `my_strcmp(a, b)` | Lexicographic compare; returns `<0 / 0 / >0` |
| `my_strncmp(a, b, n)` | Compare at most `n` chars |
| `my_memcpy(dst, src, n)` | Copy `n` bytes |
| `my_memset(dst, c, n)` | Fill `n` bytes with value `c` |
| `my_tokenize(line, argv, max)` | Split line on ASCII whitespace in-place |

### `sys.c` / `sys.h`

| Function | Description |
|----------|-------------|
| `sys_init()` | Set stdin to `O_NONBLOCK` (legacy; not called in v2) |
| `print(s)` | `write(1, s, strlen(s))` |
| `print_n(s, n)` | `write(1, s, n)` |
| `print_uint(n)` | Decimal unsigned long → stdout |
| `print_int(n)` | Decimal signed long → stdout |
| `print_hex(n)` | Hex unsigned long with `0x` prefix → stdout |
| `read_line(buf, size)` | Canonical blocking line read (legacy) |

---

## Shell Command Reference

### File System Commands

| Command | Syntax | Label | Description |
|---------|--------|-------|-------------|
| `pwd` | `pwd` | CORE | Print current directory |
| `cd` | `cd <path\|..>` | CORE | Change directory |
| `ls` | `ls [-l] [path]` | ENHANCED | List directory; `-l` shows sizes |
| `mkdir` | `mkdir <path>` | CORE | Create directory |
| `touch` | `touch <path>` | CORE | Create empty file |
| `rm` | `rm <path>` | **NEW** | Remove file or empty directory |
| `cp` | `cp <src> <dst>` | **NEW** | Copy a file |
| `mv` | `mv <src> <dst>` | **NEW** | Rename / move file or directory |
| `write` | `write <file> <text...>` | **NEW** | Write text to file (create if absent) |
| `read` | `read <file>` | **NEW** | Print file contents (spec-required) |
| `cat` | `cat [-n] <path>` | ENHANCED | Print file (`-n` = with line numbers) |
| `echo` | `echo <text> [>\|>> <file>]` | ENHANCED | Print or redirect/append text |

### System Commands

| Command | Syntax | Label | Description |
|---------|--------|-------|-------------|
| `meminfo` | `meminfo` | CORE | Allocator stats |
| `mathinfo` | `mathinfo` | **NEW** | Live math library demo |
| `ps` | `ps` | CORE | List background tasks |
| `run` | `run bg <counter\|spinner>` | CORE | Spawn background task |
| `kill` | `kill <id>` | **NEW** | Kill background task by ID |
| `history` | `history` | **NEW** | Print command history |
| `clear` | `clear` | **NEW** | Clear terminal screen |
| `help` | `help` | ENHANCED | Updated command listing |
| `exit` / `quit` | `exit` | CORE | Exit MiniOS |

### Usage Examples

```sh
# Create a directory tree and write a file
mkdir /home
mkdir /home/user
touch /home/user/notes.txt
echo Hello MiniOS > /home/user/notes.txt
echo World! >> /home/user/notes.txt
cat -n /home/user/notes.txt

# Copy, rename, then remove
cp /home/user/notes.txt /home/user/notes_backup.txt
mv /home/user/notes_backup.txt /home/user/archive.txt
ls -l /home/user
rm /home/user/archive.txt

# Background task management
run bg counter
run bg spinner
ps
kill 1
ps

# Memory status
meminfo

# History navigation
history
# Use ↑/↓ arrows to cycle through previous commands
```

---

## Build & Run

```sh
# Build (requires clang or gcc)
make

# Build and run immediately
make run

# Clean binary
make clean
```

**Requirements:** A C99-compatible compiler + POSIX terminal (`termios`).
Tested on macOS (Apple clang) and Linux (gcc 12+).

---

## File Map

| File | Role | Label |
|------|------|-------|
| `src/main.c` | Entry point; subsystem init; main readline loop | CORE |
| `src/math.c` | Custom math library (20 functions) | **LIB 1** |
| `src/math.h` | Math public API | **LIB 1** |
| `src/string.c` | Custom string + int-to-string library | **LIB 2** |
| `src/string.h` | String public API | **LIB 2** |
| `src/memory.c` | First-fit arena allocator (10 MB virtual RAM) | **LIB 3** |
| `src/memory.h` | Allocator public API | **LIB 3** |
| `src/screen.c` | Terminal rendering (cursor, print, numbers) | **LIB 4** |
| `src/screen.h` | Screen public API | **LIB 4** |
| `src/keyboard.c` | Raw mode, keyPressed, getchar, readLine | **LIB 5** |
| `src/keyboard.h` | Keyboard public API | **LIB 5** |
| `src/vfs.c` | In-memory hierarchical file system | CORE |
| `src/vfs.h` | VFS public API | CORE |
| `src/scheduler.c` | Cooperative round-robin task scheduler | CORE |
| `src/scheduler.h` | Scheduler public API | CORE |
| `src/shell.c` | Command dispatcher and all cmd_* handlers | CORE |
| `src/shell.h` | Shell public API | CORE |
| `src/readline.c` | Interactive line editor with history | ENHANCED |
| `src/readline.h` | Readline public API | ENHANCED |
| `src/sys.c` | I/O facade — delegates to screen.c | CORE |
| `src/sys.h` | Legacy I/O API | CORE |
| `src/minios_types.h` | `size_t`, `ssize_t`, `NULL` (no stddef.h) | CORE |
| `Makefile` | Build rules | CORE |
| `README.md` | Build, run, and command guide | DOC |
| `COMMANDS.md` | Quick command cheat-sheet | DOC |
| `PRESENTATION.md` | Capstone project presentation | DOC |
| `MiniOS_Features.md` | Full technical feature reference (this file) | DOC |
