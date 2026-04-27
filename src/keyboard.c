/*
 * keyboard.c – keyboard input library for MiniOS.
 *
 * Owns the terminal raw-mode lifecycle and exposes three input primitives:
 *   - kbd_keyPressed()  non-blocking poll (uses O_NONBLOCK temporarily)
 *   - kbd_getchar()     blocking single-character read
 *   - kbd_readLine()    blocking full-line read (no editing)
 *
 * Uses only POSIX syscalls: tcgetattr/tcsetattr (termios.h),
 * read/write (unistd.h), fcntl (fcntl.h).
 * No standard C library functions (no strlen, no printf, etc.).
 */

 #include "keyboard.h"

 #include <errno.h>
 #include <fcntl.h>
 #include <termios.h>
 #include <unistd.h>
 
 /* ------------------------------------------------------------------ */
 /*  Terminal state                                                      */
 /* ------------------------------------------------------------------ */
 
 static struct termios g_saved_term;
 static int            g_raw_active = 0;
 
 /*
  * kbd_init: put the terminal into raw mode.
  * Disables: echo, canonical line buffering, Ctrl-C signal, output post-proc.
  * After this call, read() returns one byte at a time with no OS pre-processing.
  */
 void kbd_init(void) {
     struct termios raw;
 
     if (tcgetattr(STDIN_FILENO, &g_saved_term) != 0)
         return;
 
     raw = g_saved_term;
 
     /* Input flags: disable flow-ctrl, CR translation, parity check */
     raw.c_iflag &= (unsigned int)~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
     /* Output flags: disable all post-processing (\n → \r\n, etc.) */
     raw.c_oflag &= (unsigned int)~(OPOST);
     /* Control flags: 8-bit chars */
     raw.c_cflag |=  (unsigned int)(CS8);
     /* Local flags: disable echo, canonical mode, extended input, signals */
     raw.c_lflag &= (unsigned int)~(ECHO | ICANON | IEXTEN | ISIG);
 
     /* read() returns as soon as 1 byte arrives, no timeout */
     raw.c_cc[VMIN]  = 1;
     raw.c_cc[VTIME] = 0;
 
     if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0)
         g_raw_active = 1;
 }
 
 /* kbd_cleanup: restore the terminal to its pre-init state. */
 void kbd_cleanup(void) {
     if (g_raw_active) {
         tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved_term);
         g_raw_active = 0;
     }
 }
 
 /* ------------------------------------------------------------------ */
 /*  Input primitives                                                    */
 /* ------------------------------------------------------------------ */
 
 /*
  * kbd_keyPressed: non-blocking poll.
  * Temporarily enables O_NONBLOCK on stdin, attempts a 1-byte read,
  * then restores the original flags.
  * Returns the keycode (0-255) if a key is available, -1 otherwise.
  */
 int kbd_keyPressed(void) {
     unsigned char c;
     ssize_t       r;
     int           fl;
     int           saved_fl;
 
     fl       = fcntl(STDIN_FILENO, F_GETFL, 0);
     saved_fl = fl;
     (void)fcntl(STDIN_FILENO, F_SETFL, fl | O_NONBLOCK);
 
     r = read(STDIN_FILENO, &c, 1);
 
     (void)fcntl(STDIN_FILENO, F_SETFL, saved_fl);
 
     if (r == 1) return (int)c;
     return -1;
 }
 
 /*
  * kbd_getchar: blocking single-character read.
  * Returns the keycode (0-255), or -1 on EOF / unrecoverable error.
  * Retries automatically on EINTR (signal interruption).
  */
 int kbd_getchar(void) {
     unsigned char c;
     ssize_t       r;
 
     for (;;) {
         r = read(STDIN_FILENO, &c, 1);
         if (r == 1)  return (int)c;
         if (r == 0)  return -1;           /* EOF */
         if (errno == EINTR) continue;     /* interrupted by signal, retry */
         return -1;                        /* real error */
     }
 }
 
 /*
  * kbd_readLine: blocking full-line read (no editing or history).
  * Reads until '\n', EOF, or the buffer is full.
  * Strips '\r'; NUL-terminates the result.
  * Returns the number of characters stored (excluding NUL), or -1 on EOF.
  */
 int kbd_readLine(char *buf, int size) {
     int pos = 0;
 
     if (!buf || size <= 1) return -1;
 
     for (;;) {
         int c = kbd_getchar();
         if (c < 0) {
             buf[pos] = '\0';
             return (pos > 0) ? pos : -1;
         }
         if (c == '\r') continue;
         if (c == '\n') break;
         if (pos < size - 1)
             buf[pos++] = (char)c;
     }
     buf[pos] = '\0';
     return pos;
 }
 