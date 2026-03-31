/*
 * X# (Xsharp) Parser - Recursive Descent Parser
 * ================================================
 */

#ifndef XSHARP_PARSER_H
#define XSHARP_PARSER_H

#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include <stdbool.h>

#define PARSER_MAX_ERROR 512
#define PARSER_MAX_ERRORS 64

typedef struct {
    Token* tokens;
    int token_count;
    int current; /* index of current token */

    /* Error reporting */
    char errors[PARSER_MAX_ERRORS][PARSER_MAX_ERROR];
    int error_count;
    bool had_error;
    bool panic_mode; /* in panic mode, suppress cascading errors */
} Parser;

/* Initialize parser with a token array (from lexer_tokenize_all) */
void parser_init(Parser* parser, Token* tokens, int token_count);

/* Parse the full program, returning the root AST node (NODE_PROGRAM) */
AstNode* parser_parse_program(Parser* parser);

/* Parse a single expression (useful for REPL) */
AstNode* parser_parse_expression(Parser* parser);

/* Check if parsing produced errors */
bool parser_had_error(const Parser* parser);

/* Print all errors */
void parser_print_errors(const Parser* parser);

#endif /* XSHARP_PARSER_H */
