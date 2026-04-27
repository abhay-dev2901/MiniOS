# MiniOS — Mini Operating System Simulator

A fully hand-rolled OS shell simulator written in pure C.
No `<string.h>`, no `<math.h>`, no `malloc()` — every subsystem is built from scratch.

---

## Project Description

MiniOS simulates the core components of an operating system entirely in software.
It boots an interactive shell backed by:

- A **custom memory allocator** (first-fit arena, 10 MB virtual RAM)
- A **virtual file system** (in-memory tree, arbitrary depth)
- A **cooperative task scheduler** (background tasks run between commands)
- A **raw-mode line editor** with command history and arrow-key navigation
- Five foundational **custom C libraries** described below

The full library pipeline is:

```
keyboard.c  →  string.c  →  math.c  →  memory.c  →  screen.c  →  terminal
   input       parsing      logic     allocation     rendering
```

---

## Five Custom Libraries

| Library | File | What it implements |
|---------|------|--------------------|
| **Math** | `math.c` / `math.h` | abs, min/max, align\_up/down, is\_pow2, next\_pow2, log2, clz/ctz/popcount, sqrt (Newton-Raphson), gcd, lcm, safe udiv/umod |
| **String** | `string.c` / `string.h` | strlen, strcpy, strncpy, strcat, strcmp, strncmp, memcpy, memset, tokenize, `int_to_str`, `uint_to_str` |
| **Memory** | `memory.c` / `memory.h` | 10 MB static arena, first-fit allocator with splitting & coalescing, 8-byte alignment via `math.c` |
| **Screen** | `screen.c` / `screen.h` | clear, cursor move/left/right, erase-EOL, putchar, print, print\_int/uint/hex (uses `string.c` internally) |
| **Keyboard** | `keyboard.c` / `keyboard.h` | raw mode (termios), non-blocking `keyPressed()`, blocking `getchar()`, `readLine()` |

---

## Build Instructions

> Requires: `cc` (clang or gcc), POSIX terminal (macOS or Linux)

```bash
# Build
make

# Build and run immediately
make run

# Clean the binary
make clean
```

---

## How to Run

```bash
make
./minios
```

The shell prompt `minios>` will appear. Type `help` to see all commands.

---

## Shell Commands

### File System

| Command | Description |
|---------|-------------|
| `pwd` | Print working directory |
| `cd <path>` | Change directory |
| `cd ..` | Go up one level |
| `ls` | List directory |
| `ls -l` | List with file sizes |
| `mkdir <path>` | Create directory |
| `touch <path>` | Create empty file |
| `rm <path>` | Remove file or empty directory |
| `cp <src> <dst>` | Copy a file |
| `mv <src> <dst>` | Move / rename |
| `cat <file>` | Print file contents |
| `cat -n <file>` | Print with line numbers |
| `echo <text>` | Print text |
| `echo <text> > <file>` | Write to file (overwrite) |
| `echo <text> >> <file>` | Append to file |

### System

| Command | Description |
|---------|-------------|
| `meminfo` | Memory allocator statistics |
| `mathinfo` | Live demo of the math library |
| `ps` | List background tasks |
| `run bg counter` | Spawn counter background task |
| `run bg spinner` | Spawn spinner background task |
| `kill <id>` | Kill a background task by ID |
| `history` | Show command history |
| `clear` | Clear the screen |
| `help` | Show all commands |
| `exit` / `quit` | Exit MiniOS |

---

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `↑` / `↓` | Navigate command history |
| `←` / `→` | Move cursor in current line |
| `Home` / `End` | Jump to start / end of line |
| `Backspace` | Delete character to the left |
| `Ctrl+C` | Clear the current input line |
| `Ctrl+L` | Clear screen |
| `Ctrl+D` | Exit (EOF) |

---

## Example Session

```sh
mkdir /home
mkdir /home/user
touch /home/user/notes.txt
echo "Hello MiniOS" > /home/user/notes.txt
echo "Second line" >> /home/user/notes.txt
cat -n /home/user/notes.txt

ls -l /home/user

cp /home/user/notes.txt /home/user/backup.txt
mv /home/user/backup.txt /home/user/archive.txt
rm /home/user/archive.txt

run bg counter
run bg spinner
ps
kill 1
ps

meminfo
mathinfo
history
exit
```

---

## Project Structure

```
os-2/
├── Makefile
├── README.md
├── COMMANDS.md          ← Quick command cheat-sheet
├── MiniOS_Features.md   ← Full technical feature reference
└── src/
    ├── main.c           ← Entry point
    ├── math.c / .h      ← Library 1: Custom math
    ├── string.c / .h    ← Library 2: Custom string + int-to-str
    ├── memory.c / .h    ← Library 3: Arena allocator
    ├── screen.c / .h    ← Library 4: Terminal rendering
    ├── keyboard.c / .h  ← Library 5: Keyboard input
    ├── vfs.c / .h       ← Virtual file system
    ├── scheduler.c / .h ← Cooperative task scheduler
    ├── readline.c / .h  ← Line editor with history
    ├── shell.c / .h     ← Command dispatcher
    ├── sys.c / .h       ← I/O facade (delegates to screen.c)
    └── minios_types.h   ← size_t, ssize_t, NULL (no stddef.h)
```

---

## Known Issues / Self-Evaluation

| Area | Status | Notes |
|------|--------|-------|
| Shell commands | ✅ All working | echo, ls, cat, rm, cp, mv, mkdir, touch, kill, ps, history, clear |
| VFS | ✅ Working | In-memory only; not persisted across runs |
| Memory allocator | ✅ Working | First-fit; no garbage collection |
| Task scheduler | ✅ Working | Cooperative; tasks only tick between commands |
| Arrow-key history | ✅ Working | Tested on macOS Terminal and iTerm2 |
| Background tasks | ✅ Working | `counter` and `spinner` both functional |
| Persistence | ❌ Not implemented | All data is lost when `exit` is called — by design |
| Signals (Ctrl+Z) | ❌ Not handled | ISIG is disabled in raw mode |
