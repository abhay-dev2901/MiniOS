#ifndef KEYBOARD_H
#define KEYBOARD_H

/*
 * keyboard.h – keyboard input library for MiniOS.
 *
 * Provides:
 *   kbd_init()       – switch terminal to raw mode (no echo, char-by-char)
 *   kbd_cleanup()    – restore terminal to canonical mode
 *   kbd_keyPressed() – non-blocking: returns keycode or -1 if no key pending
 *   kbd_getchar()    – blocking: wait for and return next raw keycode
 *   kbd_readLine()   – blocking: read a full line (no history/editing)
 *
 * Only uses <termios.h>, <unistd.h>, and <fcntl.h> (POSIX syscalls).
 * No <stdio.h> / <string.h> used.
 */

/* Initialise raw terminal mode.  Call once before any input. */
void kbd_init(void);

/* Restore the terminal to its original mode.  Call before exit(). */
void kbd_cleanup(void);

/*
 * kbd_keyPressed: non-blocking poll.
 * Returns the next pending keycode (0-255), or -1 if no key is available.
 */
int  kbd_keyPressed(void);

/*
 * kbd_getchar: blocking raw character read.
 * Returns the keycode (0-255), or -1 on EOF / error.
 */
int  kbd_getchar(void);

/*
 * kbd_readLine: read a full line of text (blocking, no line-editing).
 * Stores up to `size-1` characters in `buf` and NUL-terminates.
 * Returns the number of characters stored, or -1 on EOF.
 */
int  kbd_readLine(char *buf, int size);

#endif /* KEYBOARD_H */
