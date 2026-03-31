/*
 * X# (Xsharp) REPL - Read-Eval-Print Loop
 * ==========================================
 * Interactive line-by-line execution with history,
 * multi-line input, and meta-commands.
 */

#ifndef XSHARP_REPL_H
#define XSHARP_REPL_H

#include <stdbool.h>

/* Maximum input line length */
#define REPL_MAX_LINE 4096
#define REPL_MAX_HISTORY 1000
#define REPL_MAX_INPUT (REPL_MAX_LINE * 64)

/* Start the interactive REPL. Returns exit code. */
int xs_repl_start(void);

/* Execute a single line/block in the REPL context.
 * Returns true if execution succeeded. */
bool xs_repl_execute_line(const char* line);

/* Load and execute a file in the REPL context. */
bool xs_repl_load_file(const char* path);

/* Save current REPL history to a file. */
bool xs_repl_save_history(const char* path);

#endif /* XSHARP_REPL_H */
