#ifndef READLINE_H
#define READLINE_H

/*
 * readline – interactive line editor for MiniOS.
 *
 * Features:
 *   - Raw terminal mode (no echo, no canonical buffering)
 *   - Backspace / DEL support
 *   - Arrow-up / arrow-down for command history
 *   - Arrow-left / arrow-right for cursor movement
 *   - Ctrl-C clears the current line
 *   - Ctrl-L clears the screen
 *   - Up to READLINE_HIST_MAX entries stored in a ring buffer
 *
 * All state is kept in this module; no heap allocation is used.
 */

#define READLINE_HIST_MAX  64
#define READLINE_LINE_MAX 512

/* Must be called once before the main loop (saves terminal state, raw mode). */
void rl_init(void);

/* Restore canonical terminal mode.  Call before exit(). */
void rl_cleanup(void);

/*
 * rl_getline: display `prompt`, read a line with full editing support.
 * Stores the result (without the trailing newline) in `buf` (max `size` chars).
 * Returns the number of characters stored, or -1 on EOF / error.
 * Lines longer than READLINE_LINE_MAX-1 are truncated silently.
 */
int rl_getline(const char *prompt, char *buf, int size);

/* Print the stored history (oldest first).  Used by the 'history' command. */
void rl_print_history(void);

#endif /* READLINE_H */
