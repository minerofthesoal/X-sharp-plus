/*
 * X# (Xsharp) Code Formatter
 * ============================
 * Reads source, tokenizes, and re-emits with consistent formatting.
 */

#ifndef XSHARP_FORMATTER_H
#define XSHARP_FORMATTER_H

#include <stdbool.h>

/* Formatter configuration */
typedef struct {
    int  indent_width;      /* number of spaces/tabs per indent level (default 4) */
    bool use_spaces;        /* true = spaces, false = tabs (default true) */
    int  max_line_length;   /* soft wrap limit (default 100) */
    bool insert_final_newline; /* ensure file ends with newline (default true) */
    bool trim_trailing_whitespace; /* remove trailing spaces (default true) */
} XsFormatConfig;

/* Return a default configuration. */
XsFormatConfig xs_format_config_default(void);

/* Format a file in-place. Returns true on success. */
bool xs_format_file(const char *path, const XsFormatConfig *config);

/* Format source text and return a newly allocated formatted string.
 * Caller must free the result. Returns NULL on error. */
char *xs_format_string(const char *source, const XsFormatConfig *config);

#endif /* XSHARP_FORMATTER_H */
