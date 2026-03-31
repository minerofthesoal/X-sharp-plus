/*
 * X# (Xsharp) Lexer Implementation
 * ==================================
 * Full lexer for the X# language. Handles all keywords, operators,
 * numeric literals (decimal, hex, binary, octal), string literals
 * with interpolation, character literals, and comments.
 */

#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Token type name table ===== */
static const char* token_names[] = {
    [TOK_INT_LITERAL] = "INT_LITERAL",
    [TOK_FLOAT_LITERAL] = "FLOAT_LITERAL",
    [TOK_STRING_LITERAL] = "STRING_LITERAL",
    [TOK_INTERP_STRING_START] = "INTERP_STRING_START",
    [TOK_INTERP_STRING_MID] = "INTERP_STRING_MID",
    [TOK_INTERP_STRING_END] = "INTERP_STRING_END",
    [TOK_RUNE_LITERAL] = "RUNE_LITERAL",
    [TOK_IDENTIFIER] = "IDENTIFIER",
    [TOK_BLADE] = "BLADE",
    [TOK_SPARK] = "SPARK",
    [TOK_SCROLL] = "SCROLL",
    [TOK_RUNE] = "RUNE",
    [TOK_FATE] = "FATE",
    [TOK_VOID] = "VOID",
    [TOK_ARSENAL] = "ARSENAL",
    [TOK_FORGE] = "FORGE",
    [TOK_ENTITY] = "ENTITY",
    [TOK_REALM] = "REALM",
    [TOK_QUEST] = "QUEST",
    [TOK_SUMMON] = "SUMMON",
    [TOK_ETERNAL] = "ETERNAL",
    [TOK_MORPH] = "MORPH",
    [TOK_ORACLE] = "ORACLE",
    [TOK_OTHERWISE] = "OTHERWISE",
    [TOK_CYCLE] = "CYCLE",
    [TOK_WHILE] = "WHILE",
    [TOK_UNLEASH] = "UNLEASH",
    [TOK_SHATTER_CYCLE] = "SHATTER_CYCLE",
    [TOK_SKIP] = "SKIP",
    [TOK_CONJURE] = "CONJURE",
    [TOK_SELF] = "SELF",
    [TOK_SPELL] = "SPELL",
    [TOK_SHIELD] = "SHIELD",
    [TOK_DEFLECT] = "DEFLECT",
    [TOK_SHATTER] = "SHATTER",
    [TOK_TRUTH] = "TRUTH",
    [TOK_LIES] = "LIES",
    [TOK_ABYSS] = "ABYSS",
    [TOK_AT_GPU] = "AT_GPU",
    [TOK_AT_AI] = "AT_AI",
    [TOK_EXTENDS] = "EXTENDS",
    [TOK_IMPLEMENTS] = "IMPLEMENTS",
    [TOK_ENGRAVE] = "ENGRAVE",
    [TOK_AS] = "AS",
    [TOK_IN] = "IN",
    [TOK_FROM] = "FROM",
    [TOK_PLUS] = "PLUS",
    [TOK_MINUS] = "MINUS",
    [TOK_STAR] = "STAR",
    [TOK_SLASH] = "SLASH",
    [TOK_PERCENT] = "PERCENT",
    [TOK_POWER] = "POWER",
    [TOK_EQUAL_EQUAL] = "EQUAL_EQUAL",
    [TOK_BANG_EQUAL] = "BANG_EQUAL",
    [TOK_LESS] = "LESS",
    [TOK_GREATER] = "GREATER",
    [TOK_LESS_EQUAL] = "LESS_EQUAL",
    [TOK_GREATER_EQUAL] = "GREATER_EQUAL",
    [TOK_AND_AND] = "AND_AND",
    [TOK_OR_OR] = "OR_OR",
    [TOK_BANG] = "BANG",
    [TOK_AMPERSAND] = "AMPERSAND",
    [TOK_PIPE] = "PIPE",
    [TOK_CARET] = "CARET",
    [TOK_TILDE] = "TILDE",
    [TOK_LSHIFT] = "LSHIFT",
    [TOK_RSHIFT] = "RSHIFT",
    [TOK_EQUAL] = "EQUAL",
    [TOK_PLUS_EQUAL] = "PLUS_EQUAL",
    [TOK_MINUS_EQUAL] = "MINUS_EQUAL",
    [TOK_STAR_EQUAL] = "STAR_EQUAL",
    [TOK_SLASH_EQUAL] = "SLASH_EQUAL",
    [TOK_PERCENT_EQUAL] = "PERCENT_EQUAL",
    [TOK_POWER_EQUAL] = "POWER_EQUAL",
    [TOK_AMPERSAND_EQUAL] = "AMPERSAND_EQUAL",
    [TOK_PIPE_EQUAL] = "PIPE_EQUAL",
    [TOK_CARET_EQUAL] = "CARET_EQUAL",
    [TOK_LSHIFT_EQUAL] = "LSHIFT_EQUAL",
    [TOK_RSHIFT_EQUAL] = "RSHIFT_EQUAL",
    [TOK_PIPE_GREATER] = "PIPE_GREATER",
    [TOK_ARROW] = "ARROW",
    [TOK_FAT_ARROW] = "FAT_ARROW",
    [TOK_DOT] = "DOT",
    [TOK_DOT_DOT] = "DOT_DOT",
    [TOK_DOT_DOT_DOT] = "DOT_DOT_DOT",
    [TOK_QUESTION] = "QUESTION",
    [TOK_COLON] = "COLON",
    [TOK_DOUBLE_COLON] = "DOUBLE_COLON",
    [TOK_HASH] = "HASH",
    [TOK_LPAREN] = "LPAREN",
    [TOK_RPAREN] = "RPAREN",
    [TOK_LBRACE] = "LBRACE",
    [TOK_RBRACE] = "RBRACE",
    [TOK_LBRACKET] = "LBRACKET",
    [TOK_RBRACKET] = "RBRACKET",
    [TOK_COMMA] = "COMMA",
    [TOK_SEMICOLON] = "SEMICOLON",
    [TOK_INCREMENT] = "INCREMENT",
    [TOK_DECREMENT] = "DECREMENT",
    [TOK_EOF] = "EOF",
    [TOK_ERROR] = "ERROR",
};

const char* token_type_name(TokenType type) {
    if (type >= 0 && type < TOK_COUNT) {
        return token_names[type] ? token_names[type] : "UNKNOWN";
    }
    return "UNKNOWN";
}

void token_print(const Token* tok) {
    printf("[%3d:%-3d] %-20s '%.*s'", tok->line, tok->column, token_type_name(tok->type),
           (int)tok->length, tok->start);
    if (tok->type == TOK_INT_LITERAL) {
        printf("  (= %lld)", (long long)tok->literal.int_val);
    } else if (tok->type == TOK_FLOAT_LITERAL) {
        printf("  (= %g)", tok->literal.float_val);
    }
    printf("\n");
}

/* ===== Keyword table ===== */
typedef struct {
    const char* word;
    TokenType type;
} Keyword;

static const Keyword keywords[] = {{"blade", TOK_BLADE},
                                   {"spark", TOK_SPARK},
                                   {"scroll", TOK_SCROLL},
                                   {"rune", TOK_RUNE},
                                   {"fate", TOK_FATE},
                                   {"void", TOK_VOID},
                                   {"arsenal", TOK_ARSENAL},
                                   {"forge", TOK_FORGE},
                                   {"entity", TOK_ENTITY},
                                   {"realm", TOK_REALM},
                                   {"quest", TOK_QUEST},
                                   {"summon", TOK_SUMMON},
                                   {"eternal", TOK_ETERNAL},
                                   {"morph", TOK_MORPH},
                                   {"oracle", TOK_ORACLE},
                                   {"otherwise", TOK_OTHERWISE},
                                   {"cycle", TOK_CYCLE},
                                   {"while", TOK_WHILE},
                                   {"unleash", TOK_UNLEASH},
                                   {"shatter_cycle", TOK_SHATTER_CYCLE},
                                   {"skip", TOK_SKIP},
                                   {"conjure", TOK_CONJURE},
                                   {"self", TOK_SELF},
                                   {"spell", TOK_SPELL},
                                   {"shield", TOK_SHIELD},
                                   {"deflect", TOK_DEFLECT},
                                   {"shatter", TOK_SHATTER},
                                   {"truth", TOK_TRUTH},
                                   {"lies", TOK_LIES},
                                   {"abyss", TOK_ABYSS},
                                   {"extends", TOK_EXTENDS},
                                   {"implements", TOK_IMPLEMENTS},
                                   {"engrave", TOK_ENGRAVE},
                                   {"as", TOK_AS},
                                   {"in", TOK_IN},
                                   {"from", TOK_FROM},
                                   {NULL, 0}};

/* ===== Lexer helpers ===== */

void lexer_init(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->current = source;
    lexer->token_start = source;
    lexer->line = 1;
    lexer->column = 1;
    lexer->token_start_col = 1;
    lexer->error_msg[0] = '\0';
    lexer->had_error = false;
    lexer->interp_depth = 0;
    memset(lexer->brace_depth_at_interp, 0, sizeof(lexer->brace_depth_at_interp));
}

bool lexer_is_at_end(const Lexer* lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
    char c = *lexer->current;
    lexer->current++;
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static char peek(const Lexer* lexer) {
    return *lexer->current;
}

static char peek_next(const Lexer* lexer) {
    if (*lexer->current == '\0')
        return '\0';
    return lexer->current[1];
}

static bool match(Lexer* lexer, char expected) {
    if (*lexer->current != expected)
        return false;
    advance(lexer);
    return true;
}

static Token make_token(Lexer* lexer, TokenType type) {
    Token tok;
    tok.type = type;
    tok.start = lexer->token_start;
    tok.length = (size_t)(lexer->current - lexer->token_start);
    tok.line = lexer->line;
    tok.column = lexer->token_start_col;
    tok.literal.int_val = 0;
    return tok;
}

static Token error_token(Lexer* lexer, const char* msg) {
    Token tok;
    tok.type = TOK_ERROR;
    tok.start = msg;
    tok.length = strlen(msg);
    tok.line = lexer->line;
    tok.column = lexer->column;
    tok.literal.int_val = 0;
    snprintf(lexer->error_msg, LEXER_MAX_ERROR, "Line %d, Col %d: %s", lexer->line, lexer->column,
             msg);
    lexer->had_error = true;
    return tok;
}

/* ===== Skip whitespace and comments ===== */
static void skip_whitespace(Lexer* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
        case ' ':
        case '\t':
        case '\r':
            advance(lexer);
            break;
        case '\n':
            advance(lexer);
            break;
        case '~':
            if (peek_next(lexer) == '~') {
                /* Single-line comment: ~~ */
                advance(lexer);
                advance(lexer);
                while (!lexer_is_at_end(lexer) && peek(lexer) != '\n') {
                    advance(lexer);
                }
            } else if (peek_next(lexer) == '*') {
                /* Multi-line comment: ~* ... *~ */
                advance(lexer);
                advance(lexer);
                int depth = 1;
                while (!lexer_is_at_end(lexer) && depth > 0) {
                    if (peek(lexer) == '~' && peek_next(lexer) == '*') {
                        advance(lexer);
                        advance(lexer);
                        depth++;
                    } else if (peek(lexer) == '*' && peek_next(lexer) == '~') {
                        advance(lexer);
                        advance(lexer);
                        depth--;
                    } else {
                        advance(lexer);
                    }
                }
            } else {
                return;
            }
            break;
        default:
            return;
        }
    }
}

/* ===== Number scanning ===== */
static Token scan_number(Lexer* lexer) {
    Token tok = {0};
    tok.start = lexer->token_start;
    tok.line = lexer->line;
    tok.column = lexer->token_start_col;

    bool is_float = false;

    /* Check for hex, binary, octal prefixes */
    if (*lexer->token_start == '0' && lexer->current - lexer->token_start == 1) {
        char next = peek(lexer);
        if (next == 'x' || next == 'X') {
            advance(lexer); /* consume x */
            if (!isxdigit(peek(lexer))) {
                return error_token(lexer, "Expected hex digit after 0x");
            }
            while (isxdigit(peek(lexer)) || peek(lexer) == '_') {
                advance(lexer);
            }
            tok.type = TOK_INT_LITERAL;
            tok.length = (size_t)(lexer->current - lexer->token_start);
            /* parse hex value */
            int64_t val = 0;
            for (const char* p = lexer->token_start + 2; p < lexer->current; p++) {
                if (*p == '_')
                    continue;
                val <<= 4;
                if (*p >= '0' && *p <= '9')
                    val |= (*p - '0');
                else if (*p >= 'a' && *p <= 'f')
                    val |= (*p - 'a' + 10);
                else if (*p >= 'A' && *p <= 'F')
                    val |= (*p - 'A' + 10);
            }
            tok.literal.int_val = val;
            return tok;
        }
        if (next == 'b' || next == 'B') {
            advance(lexer);
            if (peek(lexer) != '0' && peek(lexer) != '1') {
                return error_token(lexer, "Expected binary digit after 0b");
            }
            while (peek(lexer) == '0' || peek(lexer) == '1' || peek(lexer) == '_') {
                advance(lexer);
            }
            tok.type = TOK_INT_LITERAL;
            tok.length = (size_t)(lexer->current - lexer->token_start);
            int64_t val = 0;
            for (const char* p = lexer->token_start + 2; p < lexer->current; p++) {
                if (*p == '_')
                    continue;
                val = (val << 1) | (*p - '0');
            }
            tok.literal.int_val = val;
            return tok;
        }
        if (next == 'o' || next == 'O') {
            advance(lexer);
            if (peek(lexer) < '0' || peek(lexer) > '7') {
                return error_token(lexer, "Expected octal digit after 0o");
            }
            while ((peek(lexer) >= '0' && peek(lexer) <= '7') || peek(lexer) == '_') {
                advance(lexer);
            }
            tok.type = TOK_INT_LITERAL;
            tok.length = (size_t)(lexer->current - lexer->token_start);
            int64_t val = 0;
            for (const char* p = lexer->token_start + 2; p < lexer->current; p++) {
                if (*p == '_')
                    continue;
                val = (val << 3) | (*p - '0');
            }
            tok.literal.int_val = val;
            return tok;
        }
    }

    /* Decimal integer or float */
    while (isdigit(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    /* Check for fractional part */
    if (peek(lexer) == '.' && (isdigit(peek_next(lexer)))) {
        is_float = true;
        advance(lexer); /* consume . */
        while (isdigit(peek(lexer)) || peek(lexer) == '_') {
            advance(lexer);
        }
    }

    /* Check for exponent */
    if (peek(lexer) == 'e' || peek(lexer) == 'E') {
        is_float = true;
        advance(lexer);
        if (peek(lexer) == '+' || peek(lexer) == '-') {
            advance(lexer);
        }
        if (!isdigit(peek(lexer))) {
            return error_token(lexer, "Expected digit in exponent");
        }
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }

    tok.length = (size_t)(lexer->current - lexer->token_start);

    if (is_float) {
        tok.type = TOK_FLOAT_LITERAL;
        /* Build a clean string without underscores for strtod */
        char buf[128];
        int j = 0;
        for (const char* p = lexer->token_start; p < lexer->current && j < 126; p++) {
            if (*p != '_')
                buf[j++] = *p;
        }
        buf[j] = '\0';
        tok.literal.float_val = strtod(buf, NULL);
    } else {
        tok.type = TOK_INT_LITERAL;
        int64_t val = 0;
        for (const char* p = lexer->token_start; p < lexer->current; p++) {
            if (*p == '_')
                continue;
            val = val * 10 + (*p - '0');
        }
        tok.literal.int_val = val;
    }

    return tok;
}

/* ===== String scanning ===== */
/* Scans a string literal. Handles escape sequences and interpolation #{...}. */
static Token scan_string(Lexer* lexer) {
    /* lexer->token_start points at the opening quote */
    while (!lexer_is_at_end(lexer)) {
        char c = peek(lexer);
        if (c == '\\') {
            advance(lexer); /* backslash */
            if (!lexer_is_at_end(lexer))
                advance(lexer); /* escaped char */
            continue;
        }
        if (c == '#' && peek_next(lexer) == '{') {
            /* String interpolation start */
            Token tok = make_token(lexer, TOK_INTERP_STRING_START);
            advance(lexer); /* # */
            advance(lexer); /* { */
            if (lexer->interp_depth < 16) {
                lexer->brace_depth_at_interp[lexer->interp_depth] = 1;
                lexer->interp_depth++;
            }
            return tok;
        }
        if (c == '"') {
            advance(lexer); /* closing quote */
            return make_token(lexer, TOK_STRING_LITERAL);
        }
        if (c == '\n') {
            /* Multi-line strings are allowed */
        }
        advance(lexer);
    }
    return error_token(lexer, "Unterminated string literal");
}

/* Resume scanning a string after an interpolation expression ends (after }) */
static Token scan_string_continuation(Lexer* lexer) {
    lexer->token_start = lexer->current;
    lexer->token_start_col = lexer->column;

    while (!lexer_is_at_end(lexer)) {
        char c = peek(lexer);
        if (c == '\\') {
            advance(lexer);
            if (!lexer_is_at_end(lexer))
                advance(lexer);
            continue;
        }
        if (c == '#' && peek_next(lexer) == '{') {
            /* Another interpolation */
            Token tok = make_token(lexer, TOK_INTERP_STRING_MID);
            advance(lexer); /* # */
            advance(lexer); /* { */
            if (lexer->interp_depth < 16) {
                lexer->brace_depth_at_interp[lexer->interp_depth] = 1;
                lexer->interp_depth++;
            }
            return tok;
        }
        if (c == '"') {
            advance(lexer);
            Token tok = make_token(lexer, TOK_INTERP_STRING_END);
            return tok;
        }
        advance(lexer);
    }
    return error_token(lexer, "Unterminated interpolated string");
}

/* ===== Character literal ===== */
static Token scan_rune_literal(Lexer* lexer) {
    if (peek(lexer) == '\\') {
        advance(lexer); /* backslash */
        advance(lexer); /* escaped char */
    } else {
        advance(lexer); /* the character */
    }
    if (peek(lexer) != '\'') {
        return error_token(lexer, "Unterminated character literal");
    }
    advance(lexer); /* closing ' */
    return make_token(lexer, TOK_RUNE_LITERAL);
}

/* ===== Identifier / keyword ===== */
static Token scan_identifier(Lexer* lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    /* Check against keyword table */
    size_t len = (size_t)(lexer->current - lexer->token_start);
    for (const Keyword* kw = keywords; kw->word != NULL; kw++) {
        if (strlen(kw->word) == len && memcmp(kw->word, lexer->token_start, len) == 0) {
            return make_token(lexer, kw->type);
        }
    }

    return make_token(lexer, TOK_IDENTIFIER);
}

/* ===== Main scanning function ===== */
Token lexer_next_token(Lexer* lexer) {
    /* If we are inside a string interpolation and see }, it might end the interp */
    if (lexer->interp_depth > 0) {
        /* We are in interpolated string mode: normal tokens are returned
           until we hit the matching closing brace */
    }

    skip_whitespace(lexer);

    lexer->token_start = lexer->current;
    lexer->token_start_col = lexer->column;

    if (lexer_is_at_end(lexer)) {
        return make_token(lexer, TOK_EOF);
    }

    char c = advance(lexer);

    /* Identifiers and keywords */
    if (isalpha(c) || c == '_') {
        return scan_identifier(lexer);
    }

    /* Numbers */
    if (isdigit(c)) {
        return scan_number(lexer);
    }

    /* String literals */
    if (c == '"') {
        return scan_string(lexer);
    }

    /* Character literals */
    if (c == '\'') {
        return scan_rune_literal(lexer);
    }

    /* @ annotations: @gpu, @ai */
    if (c == '@') {
        if (peek(lexer) == 'g' && lexer->current[1] == 'p' && lexer->current[2] == 'u' &&
            !isalnum(lexer->current[3]) && lexer->current[3] != '_') {
            advance(lexer);
            advance(lexer);
            advance(lexer);
            return make_token(lexer, TOK_AT_GPU);
        }
        if (peek(lexer) == 'a' && lexer->current[1] == 'i' && !isalnum(lexer->current[2]) &&
            lexer->current[2] != '_') {
            advance(lexer);
            advance(lexer);
            return make_token(lexer, TOK_AT_AI);
        }
        return error_token(lexer, "Unknown @ directive");
    }

    /* Operators and punctuation */
    switch (c) {
    case '(':
        return make_token(lexer, TOK_LPAREN);
    case ')':
        return make_token(lexer, TOK_RPAREN);
    case '{':
        if (lexer->interp_depth > 0) {
            lexer->brace_depth_at_interp[lexer->interp_depth - 1]++;
        }
        return make_token(lexer, TOK_LBRACE);
    case '}':
        if (lexer->interp_depth > 0) {
            lexer->brace_depth_at_interp[lexer->interp_depth - 1]--;
            if (lexer->brace_depth_at_interp[lexer->interp_depth - 1] == 0) {
                /* End of interpolation expression, continue scanning string */
                lexer->interp_depth--;
                return scan_string_continuation(lexer);
            }
        }
        return make_token(lexer, TOK_RBRACE);
    case '[':
        return make_token(lexer, TOK_LBRACKET);
    case ']':
        return make_token(lexer, TOK_RBRACKET);
    case ',':
        return make_token(lexer, TOK_COMMA);
    case ';':
        return make_token(lexer, TOK_SEMICOLON);
    case '?':
        return make_token(lexer, TOK_QUESTION);
    case '#':
        return make_token(lexer, TOK_HASH);
    case '~':
        /* ~ as bitwise NOT (comments already handled in skip_whitespace) */
        return make_token(lexer, TOK_TILDE);

    case '.':
        if (peek(lexer) == '.') {
            advance(lexer);
            if (peek(lexer) == '.') {
                advance(lexer);
                return make_token(lexer, TOK_DOT_DOT_DOT);
            }
            return make_token(lexer, TOK_DOT_DOT);
        }
        return make_token(lexer, TOK_DOT);

    case ':':
        if (match(lexer, ':'))
            return make_token(lexer, TOK_DOUBLE_COLON);
        return make_token(lexer, TOK_COLON);

    case '+':
        if (match(lexer, '+'))
            return make_token(lexer, TOK_INCREMENT);
        if (match(lexer, '='))
            return make_token(lexer, TOK_PLUS_EQUAL);
        return make_token(lexer, TOK_PLUS);

    case '-':
        if (match(lexer, '-'))
            return make_token(lexer, TOK_DECREMENT);
        if (match(lexer, '='))
            return make_token(lexer, TOK_MINUS_EQUAL);
        if (match(lexer, '>'))
            return make_token(lexer, TOK_ARROW);
        return make_token(lexer, TOK_MINUS);

    case '*':
        if (match(lexer, '*')) {
            if (match(lexer, '='))
                return make_token(lexer, TOK_POWER_EQUAL);
            return make_token(lexer, TOK_POWER);
        }
        if (match(lexer, '='))
            return make_token(lexer, TOK_STAR_EQUAL);
        return make_token(lexer, TOK_STAR);

    case '/':
        if (match(lexer, '='))
            return make_token(lexer, TOK_SLASH_EQUAL);
        return make_token(lexer, TOK_SLASH);

    case '%':
        if (match(lexer, '='))
            return make_token(lexer, TOK_PERCENT_EQUAL);
        return make_token(lexer, TOK_PERCENT);

    case '=':
        if (match(lexer, '='))
            return make_token(lexer, TOK_EQUAL_EQUAL);
        if (match(lexer, '>'))
            return make_token(lexer, TOK_FAT_ARROW);
        return make_token(lexer, TOK_EQUAL);

    case '!':
        if (match(lexer, '='))
            return make_token(lexer, TOK_BANG_EQUAL);
        return make_token(lexer, TOK_BANG);

    case '<':
        if (match(lexer, '<')) {
            if (match(lexer, '='))
                return make_token(lexer, TOK_LSHIFT_EQUAL);
            return make_token(lexer, TOK_LSHIFT);
        }
        if (match(lexer, '='))
            return make_token(lexer, TOK_LESS_EQUAL);
        return make_token(lexer, TOK_LESS);

    case '>':
        if (match(lexer, '>')) {
            if (match(lexer, '='))
                return make_token(lexer, TOK_RSHIFT_EQUAL);
            return make_token(lexer, TOK_RSHIFT);
        }
        if (match(lexer, '='))
            return make_token(lexer, TOK_GREATER_EQUAL);
        return make_token(lexer, TOK_GREATER);

    case '&':
        if (match(lexer, '&'))
            return make_token(lexer, TOK_AND_AND);
        if (match(lexer, '='))
            return make_token(lexer, TOK_AMPERSAND_EQUAL);
        return make_token(lexer, TOK_AMPERSAND);

    case '|':
        if (match(lexer, '|'))
            return make_token(lexer, TOK_OR_OR);
        if (match(lexer, '>'))
            return make_token(lexer, TOK_PIPE_GREATER);
        if (match(lexer, '='))
            return make_token(lexer, TOK_PIPE_EQUAL);
        return make_token(lexer, TOK_PIPE);

    case '^':
        if (match(lexer, '='))
            return make_token(lexer, TOK_CARET_EQUAL);
        return make_token(lexer, TOK_CARET);
    }

    return error_token(lexer, "Unexpected character");
}

Token lexer_peek_token(Lexer* lexer) {
    /* Save state */
    const char* saved_current = lexer->current;
    const char* saved_start = lexer->token_start;
    int saved_line = lexer->line;
    int saved_col = lexer->column;
    int saved_start_col = lexer->token_start_col;
    bool saved_err = lexer->had_error;
    int saved_interp = lexer->interp_depth;

    Token tok = lexer_next_token(lexer);

    /* Restore state */
    lexer->current = saved_current;
    lexer->token_start = saved_start;
    lexer->line = saved_line;
    lexer->column = saved_col;
    lexer->token_start_col = saved_start_col;
    lexer->had_error = saved_err;
    lexer->interp_depth = saved_interp;

    return tok;
}

Token* lexer_tokenize_all(Lexer* lexer, int* count) {
    int capacity = 256;
    int n = 0;
    Token* tokens = (Token*)malloc(sizeof(Token) * capacity);
    if (!tokens) {
        *count = 0;
        return NULL;
    }

    for (;;) {
        if (n >= capacity) {
            capacity *= 2;
            Token* tmp = (Token*)realloc(tokens, sizeof(Token) * capacity);
            if (!tmp) {
                free(tokens);
                *count = 0;
                return NULL;
            }
            tokens = tmp;
        }
        tokens[n] = lexer_next_token(lexer);
        if (tokens[n].type == TOK_EOF) {
            n++;
            break;
        }
        if (tokens[n].type == TOK_ERROR) {
            n++;
            break;
        }
        n++;
    }

    *count = n;
    return tokens;
}
