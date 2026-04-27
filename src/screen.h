#ifndef SCREEN_H
#define SCREEN_H

/*
 * screen.h – terminal rendering library for MiniOS.
 *
 * Provides cursor control (ANSI escape codes) and all character / string /
 * number output.  Internally uses string.c for int→string conversion so the
 * library pipeline is: string.c → screen.c → write() syscall.
 *
 * No <stdio.h> / <string.h> / <math.h> used.
 */

#include "minios_types.h"

/* ---- Cursor & display control ---- */
void screen_clear(void);                  /* erase display, home cursor     */
void screen_move(int row, int col);       /* move cursor to 1-based row,col */
void screen_cursor_left(int n);           /* move cursor n columns left      */
void screen_cursor_right(int n);          /* move cursor n columns right     */
void screen_erase_line_right(void);       /* erase from cursor to EOL        */
void screen_save_cursor(void);            /* ANSI save cursor position       */
void screen_restore_cursor(void);         /* ANSI restore cursor position    */

/* ---- Character / string output ---- */
void screen_putchar(char c);
void screen_print(const char *s);
void screen_print_n(const char *s, size_t n);

/* ---- Formatted number output (uses string.c – no format strings needed) ---- */
void screen_print_uint(unsigned long n);  /* decimal unsigned               */
void screen_print_int(long n);            /* decimal signed                 */
void screen_print_hex(unsigned long n);   /* hex with 0x prefix             */

#endif /* SCREEN_H */
