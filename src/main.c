#include <errno.h>
#include <unistd.h>

#include "memory.h"
#include "scheduler.h"
#include "shell.h"
#include "sys.h"
#include "vfs.h"

static char g_line[512];
static int g_line_len;

static int append_char(char c) {
    if (g_line_len >= (int)sizeof(g_line) - 1)
        return -1;
    g_line[g_line_len++] = c;
    g_line[g_line_len] = 0;
    return 0;
}

int main(void) {
    mem_init();
    vfs_init();
    scheduler_init();
    shell_init();
    sys_init();

    print("MiniOS simulator (custom allocator + VFS + cooperative scheduler)\n");
    print("Type 'help'. Raw I/O uses read/write; everything else is custom code.\n");
    print("minios> ");

    g_line_len = 0;
    g_line[0] = 0;

    for (;;) {
        char c;
        ssize_t r;

        scheduler_tick();

        r = read(0, &c, 1);
        if (r == 1) {
            if (c == '\r')
                continue;
            if (c == '\n') {
                int stop;

                g_line[g_line_len] = 0;
                stop = (g_line_len > 0) ? shell_exec_line(g_line) : 0;
                g_line_len = 0;
                g_line[0] = 0;
                if (stop)
                    break;
                print("minios> ");
                continue;
            }

            if (append_char(c) != 0) {
                print("\nline too long (discarded)\n");
                g_line_len = 0;
                g_line[0] = 0;
                print("minios> ");
            }
            continue;
        }

        if (r == 0)
            break;

        if (errno == EINTR)
            continue;
        if (errno == EAGAIN)
            continue;
        break;
    }

    print("\n");
    return 0;
}
