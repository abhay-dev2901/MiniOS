#include "shell.h"

#include "memory.h"
#include "scheduler.h"
#include "string.h"
#include "sys.h"
#include "vfs.h"

#define MAX_ARGS 32
#define PATH_MAX 512

static char g_cwd[PATH_MAX] = "/";

static void resolve_path(const char *rel, char *out, size_t outsz) {
    size_t clen;

    if (!rel || !out || outsz == 0) {
        if (out && outsz)
            out[0] = 0;
        return;
    }

    if (rel[0] == '/') {
        my_strncpy(out, rel, outsz);
        out[outsz - 1] = 0;
        return;
    }

    clen = my_strlen(g_cwd);
    if (clen + 1 + my_strlen(rel) + 2 >= outsz) {
        out[0] = 0;
        return;
    }

    my_strcpy(out, g_cwd);
    if (!(clen == 1 && g_cwd[0] == '/')) {
        out[clen] = '/';
        out[clen + 1] = 0;
    }
    my_strcat(out, rel);
}

static void ls_emit(const char *name, int is_dir, void *ctx) {
    (void)ctx;
    print(is_dir ? "d  " : "-  ");
    print(name);
    print("\n");
}

static void cwd_pop(void) {
    size_t n;

    n = my_strlen(g_cwd);
    if (n <= 1)
        return;
    n--;
    while (n > 0 && g_cwd[n] != '/')
        n--;
    if (n == 0) {
        g_cwd[0] = '/';
        g_cwd[1] = 0;
        return;
    }
    g_cwd[n] = 0;
}

static void cmd_help(void) {
    print("MiniOS shell (custom libc subset)\n");
    print("  help\n");
    print("  pwd / cd <path|..>\n");
    print("  ls [path] / mkdir <path> / touch <path> / cat <path>\n");
    print("  echo <text...> | echo <text...> > <file>\n");
    print("  meminfo\n");
    print("  ps / run bg <counter|spinner>\n");
    print("  exit\n");
}

static void cmd_meminfo(void) {
    print("sim_ram_total=");
    print_uint(mem_total_bytes());
    print(" used=");
    print_uint(mem_used_bytes());
    print(" free=");
    print_uint(mem_free_bytes());
    print(" live_objects=");
    print_uint(mem_alloc_count());
    print("\n");
}

static void cmd_pwd(void) {
    print(g_cwd);
    print("\n");
}

static void cmd_cd(char *arg) {
    char abs[PATH_MAX];

    if (!arg || !arg[0]) {
        print("cd: missing operand\n");
        return;
    }

    if (my_strcmp(arg, "..") == 0) {
        cwd_pop();
        return;
    }

    resolve_path(arg, abs, sizeof(abs));
    if (abs[0] == 0) {
        print("cd: path too long\n");
        return;
    }

    if (!vfs_is_dir(abs)) {
        print("cd: not a directory: ");
        print(arg);
        print("\n");
        return;
    }

    my_strncpy(g_cwd, abs, sizeof(g_cwd));
    g_cwd[sizeof(g_cwd) - 1] = 0;
}

static void cmd_ls(char *path) {
    char abs[PATH_MAX];
    int st;

    if (!path || !path[0]) {
        my_strncpy(abs, g_cwd, sizeof(abs));
        abs[sizeof(abs) - 1] = 0;
    } else {
        resolve_path(path, abs, sizeof(abs));
    }

    st = vfs_list_dir(abs, ls_emit, NULL);
    if (st != 0) {
        print("ls: ");
        print(vfs_strerror(st));
        print("\n");
    }
}

static void cmd_mkdir(char *path) {
    char abs[PATH_MAX];
    int st;

    if (!path || !path[0]) {
        print("mkdir: missing operand\n");
        return;
    }
    resolve_path(path, abs, sizeof(abs));
    st = vfs_make_dir(abs);
    if (st != 0) {
        print("mkdir: ");
        print(vfs_strerror(st));
        print("\n");
    }
}

static void cmd_touch(char *path) {
    char abs[PATH_MAX];
    int st;

    if (!path || !path[0]) {
        print("touch: missing operand\n");
        return;
    }
    resolve_path(path, abs, sizeof(abs));
    st = vfs_create_file(abs);
    if (st != 0) {
        print("touch: ");
        print(vfs_strerror(st));
        print("\n");
    }
}

static void cmd_cat(char *path) {
    char abs[PATH_MAX];
    char buf[4096];
    size_t got;
    int st;

    if (!path || !path[0]) {
        print("cat: missing operand\n");
        return;
    }
    resolve_path(path, abs, sizeof(abs));
    st = vfs_read_file(abs, buf, sizeof(buf), &got);
    if (st != 0) {
        print("cat: ");
        print(vfs_strerror(st));
        print("\n");
        return;
    }
    if (got)
        print_n(buf, got);
    print("\n");
}

static int build_echo_payload(char **argv, int start, int end, char *out, size_t outsz) {
    int i;
    size_t pos = 0;

    out[0] = 0;
    for (i = start; i < end; i++) {
        size_t chunk = my_strlen(argv[i]);
        if (pos + chunk + 2 >= outsz)
            return -1;
        if (pos > 0) {
            out[pos++] = ' ';
            out[pos] = 0;
        }
        my_strcpy(out + pos, argv[i]);
        pos += chunk;
    }
    return 0;
}

static void cmd_echo(int argc, char **argv) {
    char payload[1024];
    char abs[PATH_MAX];
    int st;

    if (argc < 2) {
        print("\n");
        return;
    }

    if (argc >= 4 && argv[argc - 2] && my_strcmp(argv[argc - 2], ">") == 0) {
        if (build_echo_payload(argv, 1, argc - 2, payload, sizeof(payload)) != 0) {
            print("echo: argument list too long\n");
            return;
        }
        resolve_path(argv[argc - 1], abs, sizeof(abs));
        st = vfs_write_file(abs, payload, my_strlen(payload), 0);
        if (st != 0) {
            print("echo: ");
            print(vfs_strerror(st));
            print("\n");
        }
        return;
    }

    if (build_echo_payload(argv, 1, argc, payload, sizeof(payload)) != 0) {
        print("echo: argument list too long\n");
        return;
    }
    print(payload);
    print("\n");
}

static void cmd_run(int argc, char **argv) {
    if (argc < 3 || my_strcmp(argv[1], "bg") != 0) {
        print("run: usage: run bg <counter|spinner>\n");
        return;
    }
    if (scheduler_spawn_bg(argv[2]) != 0) {
        print("run: unknown task or out of memory\n");
        return;
    }
    print("spawned background task: ");
    print(argv[2]);
    print("\n");
}

void shell_init(void) {
    g_cwd[0] = '/';
    g_cwd[1] = 0;
}

int shell_exec_line(char *line) {
    char *argv[MAX_ARGS];
    int argc;

    argc = my_tokenize(line, argv, MAX_ARGS);
    if (argc == 0)
        return 0;

    if (my_strcmp(argv[0], "help") == 0) {
        cmd_help();
    } else if (my_strcmp(argv[0], "exit") == 0 || my_strcmp(argv[0], "quit") == 0) {
        print("bye\n");
        return 1;
    } else if (my_strcmp(argv[0], "meminfo") == 0) {
        cmd_meminfo();
    } else if (my_strcmp(argv[0], "pwd") == 0) {
        cmd_pwd();
    } else if (my_strcmp(argv[0], "cd") == 0) {
        if (argc < 2) {
            print("cd: missing operand\n");
        } else {
            cmd_cd(argv[1]);
        }
    } else if (my_strcmp(argv[0], "ls") == 0) {
        cmd_ls(argc > 1 ? argv[1] : NULL);
    } else if (my_strcmp(argv[0], "mkdir") == 0) {
        if (argc < 2) {
            print("mkdir: missing operand\n");
        } else {
            cmd_mkdir(argv[1]);
        }
    } else if (my_strcmp(argv[0], "touch") == 0) {
        if (argc < 2) {
            print("touch: missing operand\n");
        } else {
            cmd_touch(argv[1]);
        }
    } else if (my_strcmp(argv[0], "cat") == 0) {
        if (argc < 2) {
            print("cat: missing operand\n");
        } else {
            cmd_cat(argv[1]);
        }
    } else if (my_strcmp(argv[0], "echo") == 0) {
        cmd_echo(argc, argv);
    } else if (my_strcmp(argv[0], "ps") == 0) {
        scheduler_dump_ps();
    } else if (my_strcmp(argv[0], "run") == 0) {
        cmd_run(argc, argv);
    } else {
        print("unknown command: ");
        print(argv[0]);
        print("\n");
    }
    return 0;
}
