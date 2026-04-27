/*
 * screen.c – terminal rendering library for MiniOS.
 *
 * Pipeline: string.c (int→str) → screen.c → write() syscall → terminal.
 *
 * All ANSI escape sequences are hand-assembled from literals; no sprintf,
 * no printf, no <stdio.h>.
 */

 #include "screen.h"

 #include <unistd.h>   /* write() – hardware abstraction syscall */
 
 #include "string.h"   /* my_uint_to_str, my_int_to_str, my_strlen */
 
 /* ------------------------------------------------------------------ */
 /*  Internal raw output                                                 */
 /* ------------------------------------------------------------------ */
 
 static void raw_write(const char *s, size_t n) {
     if (s && n > 0)
         (void)write(STDOUT_FILENO, s, n);
 }
 
 static void raw_str(const char *s) {
     if (s) raw_write(s, my_strlen(s));
 }
 
 /* Write a small non-negative integer as decimal ASCII (for escape codes). */
 static void raw_int(int n) {
     char buf[12];
     if (my_uint_to_str((unsigned long)n, buf, sizeof(buf), 10) > 0)
         raw_str(buf);
 }
 
 /* ------------------------------------------------------------------ */
 /*  Cursor & display                                                    */
 /* ------------------------------------------------------------------ */
 
 void screen_clear(void) {
     /* ESC[2J  – erase entire display */
     /* ESC[H   – move cursor to home  */
     raw_str("\033[2J\033[H");
 }
 
 void screen_move(int row, int col) {
     /* ESC[<row>;<col>H */
     raw_str("\033[");
     raw_int(row);
     raw_write(";", 1);
     raw_int(col);
     raw_write("H", 1);
 }
 
 void screen_cursor_left(int n) {
     if (n <= 0) return;
     raw_str("\033[");
     raw_int(n);
     raw_write("D", 1);
 }
 
 void screen_cursor_right(int n) {
     if (n <= 0) return;
     raw_str("\033[");
     raw_int(n);
     raw_write("C", 1);
 }
 
 void screen_erase_line_right(void) {
     raw_str("\033[K");
 }
 
 void screen_save_cursor(void) {
     raw_str("\033[s");
 }
 
 void screen_restore_cursor(void) {
     raw_str("\033[u");
 }
 
 /* ------------------------------------------------------------------ */
 /*  Character / string output                                           */
 /* ------------------------------------------------------------------ */
 
 void screen_putchar(char c) {
     raw_write(&c, 1);
 }
 
 void screen_print(const char *s) {
     raw_str(s);
 }
 
 void screen_print_n(const char *s, size_t n) {
     raw_write(s, n);
 }
 
 /* ------------------------------------------------------------------ */
 /*  Formatted number output                                             */
 /*  Uses my_uint_to_str / my_int_to_str from string.c – this is the   */
 /*  explicit library pipeline: string.c → screen.c → terminal.         */
 /* ------------------------------------------------------------------ */
 
 void screen_print_uint(unsigned long n) {
     char buf[24];
     if (my_uint_to_str(n, buf, sizeof(buf), 10) > 0)
         raw_str(buf);
 }
 
 void screen_print_int(long n) {
     char buf[24];
     if (my_int_to_str(n, buf, sizeof(buf), 10) > 0)
         raw_str(buf);
 }
 
 void screen_print_hex(unsigned long n) {
     char buf[20];
     raw_str("0x");
     if (my_uint_to_str(n, buf, sizeof(buf), 16) > 0)
         raw_str(buf);
 }
 