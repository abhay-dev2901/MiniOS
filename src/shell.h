#ifndef SHELL_H
#define SHELL_H

void shell_init(void);

/* Mutates `line` (tokenization writes NULs). Returns 1 if the host process should exit. */
int shell_exec_line(char *line);

#endif
