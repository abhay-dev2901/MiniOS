/*
 * main.c – MiniOS entry point.
 *
 * Uses rl_getline() from readline.c for raw-mode line editing with
 * arrow-key history, cursor movement, and Ctrl-C / Ctrl-L support.
 * The cooperative scheduler still runs once per rl_getline call;
 * for a finer granularity it would need to run inside the read loop,
 * but that requires merging the two state machines — acceptable trade-off.
 */

 #include "memory.h"
 #include "readline.h"
 #include "scheduler.h"
 #include "shell.h"
 #include "sys.h"
 #include "vfs.h"
 
 #define LINE_MAX 512
 
 int main(void) {
     char line[LINE_MAX];
     int  r;
 
     /* Initialise all subsystems */
     mem_init();
     vfs_init();
     scheduler_init();
     shell_init();
 
     /* Set raw terminal mode (readline) */
     rl_init();
 
     print("MiniOS v2 – custom allocator + VFS + cooperative scheduler\r\n");
     print("Type 'help' for a list of commands.\r\n");
 
     for (;;) {
         /* Run background tasks before blocking on input */
         scheduler_tick();
 
         r = rl_getline("minios> ", line, LINE_MAX);
         if (r < 0)       /* EOF or Ctrl-D */
             break;
         if (r == 0)      /* empty line */
             continue;
 
         if (shell_exec_line(line))
             break;
     }
 
     rl_cleanup();
     print("\r\n");
     return 0;
 }
 