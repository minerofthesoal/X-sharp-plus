#ifndef XSHARP_LEXER_H
#define XSHARP_LEXER_H

#include "token.h"
#include <stdbool.h>

/* Maximum error message length */
#define LEXER_MAX_ERROR 256

/* Lexer state */
typedef struct {
    const char* source;      /* full source text */
    const char* current;     /* current position in source */
    const char* token_start; /* start of current token */
    int line;
    int column;
    int token_start_col;

    /* Error reporting */
    char error_msg[LEXER_MAX_ERROR];
    bool had_error;

    /* Interpolation tracking: depth of nested #{} in strings */
    int interp_depth;
    int brace_depth_at_interp[16]; /* max nesting 16 */
} Lexer;

/* Initialize a lexer with source code */
void lexer_init(Lexer* lexer, const char* source);

/* Scan and return the next token */
Token lexer_next_token(Lexer* lexer);

/* Peek at the next token without consuming it */
Token lexer_peek_token(Lexer* lexer);

/* Return true if the lexer has reached end of source */
bool lexer_is_at_end(const Lexer* lexer);

/* Tokenize entire source into a dynamic array of tokens.
 * Caller must free the returned array.
 * Sets *count to the number of tokens (including EOF). */
Token* lexer_tokenize_all(Lexer* lexer, int* count);

#endif /* XSHARP_LEXER_H */
