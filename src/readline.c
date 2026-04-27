/*
 * readline.c – interactive line editor for MiniOS.
 *
 * Library pipeline:
 *   keyboard.c (raw chars) → readline.c (editing/history) → screen.c (output)
 *
 * rl_init / rl_cleanup delegate to keyboard.c for terminal raw mode.
 * All output goes through screen.c (which in turn uses string.c).
 */

 #include "readline.h"

 #include <unistd.h>   /* STDIN_FILENO / STDOUT_FILENO constants only */
 
 #include "keyboard.h"
 #include "screen.h"
 #include "string.h"
 
 /* ------------------------------------------------------------------ */
 /*  History ring buffer                                                 */
 /* ------------------------------------------------------------------ */
 
 static char g_hist[READLINE_HIST_MAX][READLINE_LINE_MAX];
 static int  g_hist_count = 0;   /* total entries ever pushed (may exceed cap) */
 static int  g_hist_head  = 0;   /* ring index of the *next* slot to write     */
 
 /* Number of valid entries currently stored */
 static int hist_stored(void) {
     return g_hist_count < READLINE_HIST_MAX ? g_hist_count : READLINE_HIST_MAX;
 }
 
 /* Push a line into history (skip empty lines and duplicates of last entry) */
 static void hist_push(const char *line) {
     int last;
 
     if (!line || line[0] == '\0')
         return;
 
     /* Deduplicate consecutive identical commands */
     if (g_hist_count > 0) {
         last = (g_hist_head - 1 + READLINE_HIST_MAX) % READLINE_HIST_MAX;
         if (my_strcmp(g_hist[last], line) == 0)
             return;
     }
 
     my_strncpy(g_hist[g_hist_head], line, READLINE_LINE_MAX - 1);
     g_hist[g_hist_head][READLINE_LINE_MAX - 1] = '\0';
     g_hist_head = (g_hist_head + 1) % READLINE_HIST_MAX;
     g_hist_count++;
 }
 
 /*
  * Get a history entry relative to the present.
  * offset=1 → last command, offset=2 → second-to-last, etc.
  * Returns NULL if offset is out of range.
  */
 static const char *hist_get(int offset) {
     int stored = hist_stored();
     int idx;
 
     if (offset < 1 || offset > stored)
         return NULL;
     idx = (g_hist_head - offset + READLINE_HIST_MAX * 2) % READLINE_HIST_MAX;
     return g_hist[idx];
 }
 
 void rl_print_history(void) {
     int stored = hist_stored();
     int i;
     int num = g_hist_count - stored + 1;
 
     if (stored == 0) {
         screen_print("(no history)\n");
         return;
     }
     for (i = stored; i >= 1; i--) {
         const char *e = hist_get(i);
         if (!e) continue;
         screen_print_int((long)num++);
         screen_print("  ");
         screen_print(e);
         screen_print("\n");
     }
 }
 
 /* ------------------------------------------------------------------ */
 /*  Terminal state — now delegated to keyboard.c                        */
 /* ------------------------------------------------------------------ */
 
 void rl_init(void)    { kbd_init(); }
 void rl_cleanup(void) { kbd_cleanup(); }
 
 /* ------------------------------------------------------------------ */
 /*  rl_getline                                                          */
 /* ------------------------------------------------------------------ */
 
 int rl_getline(const char *prompt, char *buf, int size) {
     /* Editing buffer (one extra byte for NUL) */
     static char line[READLINE_LINE_MAX];
     int len   = 0;   /* number of chars in line[]        */
     int pos   = 0;   /* cursor position within line[]    */
     int hoff  = 0;   /* history offset (0 = live buffer) */
 
     /* Snapshot of the user's live typing when scrolling history */
     static char live_snap[READLINE_LINE_MAX];
     live_snap[0] = '\0';
 
     if (size <= 0) return 0;
 
     screen_print(prompt);
 
     for (;;) {
         int c = kbd_getchar();
         unsigned char uc;
 
         uc = (unsigned char)c;
 
         if (c < 0) {
             screen_print("\r\n");
             buf[0] = '\0';
             return -1;
         }
 
         /* ---- Ctrl-C: abort line ---- */
         if (uc == 3) {
             screen_print("^C\r\n");
             len = 0; pos = 0; hoff = 0;
             line[0] = '\0';
             screen_print(prompt);
             continue;
         }
 
         /* ---- Ctrl-L: clear screen ---- */
         if (uc == 12) {
             screen_clear();
             screen_print(prompt);
             screen_print_n(line, (size_t)len);
             if (pos < len) screen_cursor_left(len - pos);
             continue;
         }
 
         /* ---- Enter ---- */
         if (uc == '\r' || uc == '\n') {
             line[len] = '\0';
             screen_print("\r\n");
             /* Copy into caller buffer (truncate if needed) */
             my_strncpy(buf, line, (size_t)(size - 1));
             buf[size - 1] = '\0';
             hist_push(buf);
             return (int)my_strlen(buf);
         }
 
         /* ---- Backspace (0x7f DEL or 0x08 BS) ---- */
         if (uc == 127 || uc == 8) {
             if (pos > 0) {
                 int tail = len - pos;
                 my_memcpy(line + pos - 1, line + pos, (size_t)tail);
                 len--; pos--;
                 line[len] = '\0';
                 screen_cursor_left(1);
                 screen_print_n(line + pos, (size_t)tail);
                 screen_erase_line_right();
                 if (tail > 0) screen_cursor_left(tail);
             }
             continue;
         }
 
         /* ---- Escape sequence (arrows, etc.) ---- */
         if (uc == '\033') {
             unsigned char seq[2];
             int s0 = kbd_getchar();
             if (s0 < 0 || s0 != '[') continue;
             seq[0] = (unsigned char)s0;
             int s1 = kbd_getchar();
             if (s1 < 0) continue;
             seq[1] = (unsigned char)s1;
             (void)seq[0];
 
             /* Arrow UP → older history */
             if (seq[1] == 'A') {
                 int next_hoff = hoff + 1;
                 const char *entry = hist_get(next_hoff);
                 if (!entry) continue;
                 if (hoff == 0) {
                     line[len] = '\0';
                     my_strncpy(live_snap, line, READLINE_LINE_MAX - 1);
                 }
                 hoff = next_hoff;
                 if (pos > 0) screen_cursor_left(pos);
                 screen_erase_line_right();
                 my_strncpy(line, entry, READLINE_LINE_MAX - 1);
                 line[READLINE_LINE_MAX - 1] = '\0';
                 len = (int)my_strlen(line);
                 pos = len;
                 screen_print_n(line, (size_t)len);
                 continue;
             }
 
             /* Arrow DOWN → newer history / live buffer */
             if (seq[1] == 'B') {
                 if (hoff == 0) continue;
                 hoff--;
                 if (pos > 0) screen_cursor_left(pos);
                 screen_erase_line_right();
                 if (hoff == 0) {
                     my_strncpy(line, live_snap, READLINE_LINE_MAX - 1);
                 } else {
                     const char *entry = hist_get(hoff);
                     if (entry) my_strncpy(line, entry, READLINE_LINE_MAX - 1);
                 }
                 line[READLINE_LINE_MAX - 1] = '\0';
                 len = (int)my_strlen(line);
                 pos = len;
                 screen_print_n(line, (size_t)len);
                 continue;
             }
 
             /* Arrow LEFT */
             if (seq[1] == 'D') {
                 if (pos > 0) { pos--; screen_cursor_left(1); }
                 continue;
             }
 
             /* Arrow RIGHT */
             if (seq[1] == 'C') {
                 if (pos < len) { pos++; screen_cursor_right(1); }
                 continue;
             }
 
             /* Home */
             if (seq[1] == 'H' || seq[1] == '1') {
                 if (pos > 0) screen_cursor_left(pos);
                 pos = 0;
                 continue;
             }
 
             /* End */
             if (seq[1] == 'F' || seq[1] == '4') {
                 if (pos < len) { screen_cursor_right(len - pos); pos = len; }
                 continue;
             }
 
             continue;
         }
 
         /* ---- Printable character ---- */
         if (uc >= 32 && uc < 127) {
             if (len >= READLINE_LINE_MAX - 1) continue;
             {
                 int tail = len - pos;
                 if (tail > 0)
                     my_memcpy(line + pos + 1, line + pos, (size_t)tail);
                 line[pos++] = (char)uc;
                 len++;
                 line[len] = '\0';
                 screen_print_n(line + pos - 1, (size_t)(tail + 1));
                 if (tail > 0) screen_cursor_left(tail);
             }
             continue;
         }
     }
 }
 