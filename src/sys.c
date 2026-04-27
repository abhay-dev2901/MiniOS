/*
 * sys.c – system I/O facade for MiniOS.
 *
 * All output is now delegated to screen.c (which uses string.c internally),
 * making the full library pipeline explicit:
 *
 *   keyboard.c → string.c → math.c → screen.c → terminal
 *
 * sys_init() is kept for backward compatibility but is a no-op in v2
 * because keyboard.c now owns the terminal mode.
 */

 #include "sys.h"

 #include "screen.h"   /* canonical output module */
 
 void sys_init(void) {
     /* Terminal mode is now managed by keyboard.c / kbd_init().
      * This function is intentionally left empty. */
 }
 
 void print(const char *s)              { screen_print(s); }
 void print_n(const char *s, size_t n)  { screen_print_n(s, n); }
 void print_uint(unsigned long n)       { screen_print_uint(n); }
 void print_int(long n)                 { screen_print_int(n); }
 void print_hex(unsigned long n)        { screen_print_hex(n); }
 
 /*
  * read_line: retained for compatibility; now wraps kbd_readLine()
  * via a direct read() loop so that sys.c stays independent of keyboard.h.
  */
 #include <errno.h>
 #include <unistd.h>
 
 int read_line(char *buf, int size) {
     int pos = 0;
     if (size <= 0) return 0;
     while (pos < size - 1) {
         char    c;
         ssize_t r = read(0, &c, 1);
         if (r == 0) break;
         if (r < 0) {
             if (errno == EINTR || errno == EAGAIN) continue;
             break;
         }
         if (c == '\n') break;
         if (c == '\r') continue;
         buf[pos++] = c;
     }
     buf[pos] = '\0';
     return pos;
 }
 