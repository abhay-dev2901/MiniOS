# MiniOS — Quick Start & Command Reference

## How to Run

```bash
# Step 1 — Build the project
make

# Step 2 — Launch the shell
./minios

# Or build + run in one step
make run

# Clean the binary
make clean
```

> You need `cc` (clang or gcc) and a POSIX terminal (macOS or Linux).

---

## All Shell Commands

### Navigation

| Command | Example | What it does |
|---------|---------|--------------|
| `pwd` | `pwd` | Print current directory |
| `cd <path>` | `cd /home/user` | Change into a directory |
| `cd ..` | `cd ..` | Go up one directory level |
| `cd /` | `cd /` | Go back to root |

---

### Files & Directories

| Command | Example | What it does |
|---------|---------|--------------|
| `ls` | `ls` | List current directory |
| `ls <path>` | `ls /home` | List a specific directory |
| `ls -l` | `ls -l` | List with file sizes |
| `ls -l <path>` | `ls -l /home` | Long list a specific path |
| `mkdir <path>` | `mkdir /home` | Create a directory |
| `touch <path>` | `touch notes.txt` | Create an empty file |
| `rm <path>` | `rm notes.txt` | Delete a file or empty directory |
| `cp <src> <dst>` | `cp a.txt b.txt` | Copy a file |
| `mv <src> <dst>` | `mv a.txt /home/a.txt` | Move or rename a file/directory |

---

### Reading & Writing Files

| Command | Example | What it does |
|---------|---------|--------------|
| `write <file> <text>` | `write notes.txt Hello World` | Write text to file (creates if missing, overwrites) |
| `read <file>` | `read notes.txt` | Print file contents |
| `cat <file>` | `cat notes.txt` | Print file contents |
| `cat -n <file>` | `cat -n notes.txt` | Print file with line numbers |
| `echo <text>` | `echo Hello` | Print text to screen |
| `echo <text> > <file>` | `echo Hi > notes.txt` | Write text to file (overwrite) |
| `echo <text> >> <file>` | `echo Hi >> notes.txt` | Append text to file |

---

### Background Tasks

| Command | Example | What it does |
|---------|---------|--------------|
| `run bg counter` | `run bg counter` | Spawn a counter background task |
| `run bg spinner` | `run bg spinner` | Spawn a spinner background task |
| `ps` | `ps` | List all background tasks and their state |
| `kill <id>` | `kill 1` | Kill a background task by its ID |

---

### System Info

| Command | Example | What it does |
|---------|---------|--------------|
| `meminfo` | `meminfo` | Show memory usage (total / used / free / objects) |
| `mathinfo` | `mathinfo` | Run the custom math library demo (all functions + results) |
| `history` | `history` | Show command history |
| `clear` | `clear` | Clear the terminal screen |
| `help` | `help` | Print the built-in help listing |
| `exit` | `exit` | Exit MiniOS |
| `quit` | `quit` | Exit MiniOS (alias for exit) |

---

## Keyboard Shortcuts (in the shell)

| Key | Action |
|-----|--------|
| `↑` Arrow Up | Previous command in history |
| `↓` Arrow Down | Next command in history |
| `←` Arrow Left | Move cursor left |
| `→` Arrow Right | Move cursor right |
| `Home` | Jump cursor to start of line |
| `End` | Jump cursor to end of line |
| `Backspace` | Delete character to the left |
| `Ctrl + C` | Clear the current line |
| `Ctrl + L` | Clear the screen |
| `Ctrl + D` | Exit MiniOS (EOF) |

---

## Quick Example Session

```sh
make run

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
history
exit
```
