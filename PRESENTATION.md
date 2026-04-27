# MiniOS — Capstone Project Presentation

## Track B: Mini Operating System

---

## 1. Project Summary

MiniOS is a complete Mini Operating System Simulator built entirely in C
**without any standard C library** (no `<string.h>`, no `<math.h>`, no `malloc()`).

Every subsystem — memory management, string parsing, math, screen rendering,
and keyboard input — is hand-implemented from scratch and wired together as a
library pipeline.

---

## 2. The Five Custom Libraries

### Library 1 — `math.c` / `math.h`
> **Pipeline role:** Computes logic and boundary checks.

| Function group | Functions |
|---|---|
| Scalar | `my_abs`, `my_labs`, `my_min`, `my_max`, `my_min_sz`, `my_max_sz` |
| Alignment | `my_align_up`, `my_align_down` — used by `memory.c` for 8-byte aligned allocs |
| Power-of-2 | `my_is_pow2`, `my_next_pow2` — used by `vfs.c` for buffer sizing |
| Bit ops | `my_clz`, `my_ctz`, `my_popcount` — demonstrated via `mathinfo` |
| Arithmetic | `my_sqrt_int`, `my_gcd`, `my_lcm`, `my_udiv`, `my_umod` (safe ÷0) |

---

### Library 2 — `string.c` / `string.h`
> **Pipeline role:** Parses all keyboard input; converts numbers to strings for rendering.

| Function | Purpose |
|---|---|
| `my_strlen` | Count string bytes |
| `my_strcpy / strncpy / strcat` | String copy and concat |
| `my_strcmp / strncmp` | String comparison (used in every shell command dispatch) |
| `my_memcpy / memset` | Raw memory operations |
| `my_tokenize` | Split input lines into `argv[]` — used by shell.c |
| `my_uint_to_str` | Integer → string (base 2–16) — used by screen.c |
| `my_int_to_str` | Signed integer → string — used by screen.c |

---

### Library 3 — `memory.c` / `memory.h`
> **Pipeline role:** Allocates all runtime data structures (VFS nodes, task contexts).

- **Virtual RAM:** 10 MB static arena (`g_arena[10485760]`)
- **Algorithm:** First-fit free-list with block splitting
- **Coalescing:** Adjacent free blocks merged on every `my_free()`
- **Alignment:** Every allocation rounded up to 8 bytes via `math.c`'s `my_align_up()`
- **Zero-init:** Every allocation is zero-filled on return

```
sim_ram_total=10485760   (10 MB virtual RAM)
used=          <grows as files & tasks are created>
live_objects=  <count of currently allocated blocks>
```

---

### Library 4 — `screen.c` / `screen.h`
> **Pipeline role:** All terminal output. Uses `string.c` for number rendering.

| Function | What it does |
|---|---|
| `screen_clear()` | Erase terminal, home cursor |
| `screen_move(row, col)` | Position cursor (ANSI ESC sequence) |
| `screen_cursor_left/right(n)` | Move cursor by n columns |
| `screen_erase_line_right()` | Erase from cursor to EOL |
| `screen_print(s)` | Print a string via `write()` syscall |
| `screen_print_uint(n)` | Uses `my_uint_to_str()` → no libc number formatting |
| `screen_print_int(n)` | Uses `my_int_to_str()` → signed decimal |
| `screen_print_hex(n)` | Hex with `0x` prefix |

---

### Library 5 — `keyboard.c` / `keyboard.h`
> **Pipeline role:** Captures all keyboard input in raw mode.

| Function | What it does |
|---|---|
| `kbd_init()` | Switch terminal to raw mode (no echo, char-by-char via `termios`) |
| `kbd_cleanup()` | Restore terminal on exit |
| `kbd_keyPressed()` | **Non-blocking** poll — returns keycode or `-1` |
| `kbd_getchar()` | **Blocking** single-character read |
| `kbd_readLine()` | Blocking full-line read (no editing) |

---

## 3. Integration Pipeline

```
User types a key
      │
      ▼
keyboard.c  ──kbd_getchar()──►  readline.c  ──► shell_exec_line()
    raw char                    editing +              │
                                history           my_tokenize()  ← string.c
                                                       │
                                              command dispatch
                                             /    |    |    \
                                          vfs.c  math.c  memory.c  scheduler.c
                                             \    |    |    /
                                              screen_print()   ◄── screen.c
                                                uses string.c
                                                      │
                                                   terminal
```

---

## 4. Track B Requirements — Compliance Table

| Requirement | Implemented | Where |
|---|---|---|
| `echo` command | ✅ | `shell.c` → `vfs_write_file` |
| `ls` command | ✅ | `shell.c` → `vfs_list_dir` |
| `touch` command | ✅ | `shell.c` → `vfs_create_file` |
| `write` command | ✅ | `shell.c` `cmd_write()` |
| `read` command | ✅ | `shell.c` `cmd_read()` |
| `run` command | ✅ | `shell.c` → `scheduler_spawn_bg` |
| All parsing via `string.c` | ✅ | `my_tokenize()` on every command |
| VFS in virtual RAM via `memory.c` | ✅ | Every `VfsNode` from `my_malloc()` |
| Background task runs concurrently | ✅ | `scheduler_tick()` each main loop |
| No standard C library for core logic | ✅ | Only `read()`/`write()` POSIX syscalls |
| No hard-coded values | ✅ | All logic computed dynamically |

---

## 5. All Shell Commands

```
pwd   cd   ls [-l]   mkdir   touch   rm   cp   mv
write <file> <text>   read <file>
cat [-n]   echo [> / >>]
meminfo   mathinfo   ps   run bg   kill   history   clear   exit
```

---

## 6. Phase Completion

| Phase | Status | Evidence |
|---|---|---|
| Phase 1 — Libraries + basic shell | ✅ | All 5 libraries compile clean; shell boots, prints prompt, parses commands |
| Phase 2 — Full OS features | ✅ | VFS works (create/read/write/delete files in virtual RAM); background tasks run without freezing the prompt |

---

## 7. How to Run

```bash
make        # build (requires clang/gcc + POSIX terminal)
./minios    # launch the shell

# Quick demo sequence:
mkdir /demo
write /demo/hello.txt Hello World
read /demo/hello.txt
ls -l /demo
run bg counter
ps
kill 1
meminfo
mathinfo
exit
```

---

## 8. File Structure

```
os-2/
├── Makefile
├── README.md            ← Build + usage guide
├── COMMANDS.md          ← Quick command cheat-sheet
├── MiniOS_Features.md   ← Full technical reference
├── PRESENTATION.md      ← This document
└── src/
    ├── math.c / .h      ← Library 1
    ├── string.c / .h    ← Library 2
    ├── memory.c / .h    ← Library 3
    ├── screen.c / .h    ← Library 4
    ├── keyboard.c / .h  ← Library 5
    ├── vfs.c / .h       ← Virtual File System
    ├── scheduler.c / .h ← Cooperative Task Scheduler
    ├── readline.c / .h  ← Line editor + history
    ├── shell.c / .h     ← Command dispatcher
    ├── sys.c / .h       ← I/O facade (delegates to screen.c)
    ├── main.c           ← Entry point
    └── minios_types.h   ← Portable types (no stddef.h)
```
