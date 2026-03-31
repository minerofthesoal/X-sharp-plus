/*
 * X# (Xsharp) Code Formatter Implementation
 * ============================================
 */

#include "formatter.h"
#include "../lexer/lexer.h"
#include "../lexer/token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/* ===== Default Configuration ===== */
XsFormatConfig xs_format_config_default(void) {
    XsFormatConfig config;
    config.indent_width = 4;
    config.use_spaces = true;
    config.max_line_length = 100;
    config.insert_final_newline = true;
    config.trim_trailing_whitespace = true;
    return config;
}

/* ===== Output Buffer ===== */
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
    int    current_col;
} FmtBuffer;

static void buf_init(FmtBuffer *b) {
    b->cap = 4096;
    b->data = (char *)malloc(b->cap);
    b->len = 0;
    b->current_col = 0;
    if (b->data) b->data[0] = '\0';
}

static void buf_append(FmtBuffer *b, const char *str, size_t slen) {
    if (b->len + slen + 1 > b->cap) {
        while (b->len + slen + 1 > b->cap) b->cap *= 2;
        b->data = (char *)realloc(b->data, b->cap);
    }
    memcpy(b->data + b->len, str, slen);
    b->len += slen;
    b->data[b->len] = '\0';
    /* Update column tracking */
    for (size_t i = 0; i < slen; i++) {
        if (str[i] == '\n') b->current_col = 0;
        else b->current_col++;
    }
}

static void buf_append_str(FmtBuffer *b, const char *str) {
    buf_append(b, str, strlen(str));
}

static void buf_append_char(FmtBuffer *b, char c) {
    buf_append(b, &c, 1);
}

static void buf_free(FmtBuffer *b) {
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

/* ===== Indent Helper ===== */
static void emit_indent(FmtBuffer *b, int level, const XsFormatConfig *cfg) {
    for (int i = 0; i < level; i++) {
        if (cfg->use_spaces) {
            for (int j = 0; j < cfg->indent_width; j++) {
                buf_append_char(b, ' ');
            }
        } else {
            buf_append_char(b, '\t');
        }
    }
}

/* ===== Token Classification Helpers ===== */
static bool is_keyword_token(TokenType t) {
    return (t >= TOK_BLADE && t <= TOK_FROM);
}

static bool is_binary_operator(TokenType t) {
    return (t == TOK_PLUS || t == TOK_MINUS || t == TOK_STAR ||
            t == TOK_SLASH || t == TOK_PERCENT || t == TOK_POWER ||
            t == TOK_EQUAL_EQUAL || t == TOK_BANG_EQUAL ||
            t == TOK_LESS || t == TOK_GREATER ||
            t == TOK_LESS_EQUAL || t == TOK_GREATER_EQUAL ||
            t == TOK_AND_AND || t == TOK_OR_OR ||
            t == TOK_AMPERSAND || t == TOK_PIPE || t == TOK_CARET ||
            t == TOK_LSHIFT || t == TOK_RSHIFT ||
            t == TOK_PIPE_GREATER || t == TOK_DOT_DOT);
}

static bool is_assignment_operator(TokenType t) {
    return (t == TOK_EQUAL || t == TOK_PLUS_EQUAL || t == TOK_MINUS_EQUAL ||
            t == TOK_STAR_EQUAL || t == TOK_SLASH_EQUAL ||
            t == TOK_PERCENT_EQUAL || t == TOK_POWER_EQUAL ||
            t == TOK_AMPERSAND_EQUAL || t == TOK_PIPE_EQUAL ||
            t == TOK_CARET_EQUAL || t == TOK_LSHIFT_EQUAL ||
            t == TOK_RSHIFT_EQUAL);
}

static bool needs_space_before(TokenType prev, TokenType cur) {
    /* No space after open paren/bracket or before close paren/bracket */
    if (prev == TOK_LPAREN || prev == TOK_LBRACKET) return false;
    if (cur == TOK_RPAREN || cur == TOK_RBRACKET) return false;

    /* No space before comma, semicolon, colon */
    if (cur == TOK_COMMA || cur == TOK_SEMICOLON) return false;

    /* No space around member access dot */
    if (prev == TOK_DOT || cur == TOK_DOT) return false;

    /* No space between identifier and open paren (function call) */
    if ((prev == TOK_IDENTIFIER || prev == TOK_RPAREN) && cur == TOK_LPAREN) return false;

    /* No space between identifier and open bracket (indexing) */
    if ((prev == TOK_IDENTIFIER || prev == TOK_RPAREN) && cur == TOK_LBRACKET) return false;

    /* No space after unary operators */
    if (prev == TOK_BANG || prev == TOK_TILDE) return false;

    /* No space around :: */
    if (prev == TOK_DOUBLE_COLON || cur == TOK_DOUBLE_COLON) return false;

    /* No space before increment/decrement */
    if (cur == TOK_INCREMENT || cur == TOK_DECREMENT) return false;

    /* Space around binary operators */
    if (is_binary_operator(cur) || is_binary_operator(prev)) return true;

    /* Space around assignment operators */
    if (is_assignment_operator(cur) || is_assignment_operator(prev)) return true;

    /* Space after comma */
    if (prev == TOK_COMMA) return true;

    /* Space after colon in type annotations */
    if (prev == TOK_COLON) return true;

    /* Space before colon */
    if (cur == TOK_COLON) return true;

    /* Space after keywords */
    if (is_keyword_token(prev)) return true;

    /* Space before open brace */
    if (cur == TOK_LBRACE) return true;

    /* Space after close brace (unless followed by semicolon/comma) */
    if (prev == TOK_RBRACE && cur != TOK_SEMICOLON && cur != TOK_COMMA) return true;

    /* Space between arrow tokens and surrounding */
    if (prev == TOK_ARROW || cur == TOK_ARROW) return true;
    if (prev == TOK_FAT_ARROW || cur == TOK_FAT_ARROW) return true;

    /* Space between identifier and keyword */
    if (prev == TOK_IDENTIFIER && is_keyword_token(cur)) return true;

    /* Default: space between most tokens */
    if (prev == TOK_IDENTIFIER && cur == TOK_IDENTIFIER) return true;
    if (prev == TOK_INT_LITERAL && cur == TOK_IDENTIFIER) return true;

    return false;
}

/* ===== Trim Trailing Whitespace from a Line ===== */
static void trim_trailing_whitespace(char *line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t')) {
        line[--len] = '\0';
    }
}

/* ===== Main Formatting Engine ===== */
char *xs_format_string(const char *source, const XsFormatConfig *config) {
    if (!source || !config) return NULL;

    /* Tokenize the source */
    Lexer lexer;
    lexer_init(&lexer, source);
    int token_count = 0;
    Token *tokens = lexer_tokenize_all(&lexer, &token_count);
    if (!tokens || token_count == 0) {
        return strdup(source);
    }

    FmtBuffer out;
    buf_init(&out);

    int indent_level = 0;
    bool at_line_start = true;
    TokenType prev_type = TOK_EOF;
    int prev_line = 1;

    for (int i = 0; i < token_count; i++) {
        Token *tok = &tokens[i];

        if (tok->type == TOK_EOF) break;
        if (tok->type == TOK_ERROR) {
            /* Pass through error tokens as-is */
            buf_append(&out, tok->start, tok->length);
            continue;
        }

        /* Handle line breaks: preserve blank lines between top-level declarations */
        if (tok->line > prev_line) {
            int blank_lines = tok->line - prev_line;
            if (blank_lines > 2) blank_lines = 2; /* max 2 blank lines */

            for (int b = 0; b < blank_lines; b++) {
                buf_append_char(&out, '\n');
            }
            at_line_start = true;
        }

        /* Decrease indent before closing brace */
        if (tok->type == TOK_RBRACE && indent_level > 0) {
            indent_level--;
        }

        /* Emit indentation at start of line */
        if (at_line_start) {
            emit_indent(&out, indent_level, config);
            at_line_start = false;
        } else {
            /* Space between tokens on the same line */
            if (needs_space_before(prev_type, tok->type)) {
                buf_append_char(&out, ' ');
            }
        }

        /* Emit token text */
        buf_append(&out, tok->start, tok->length);

        /* Increase indent after opening brace */
        if (tok->type == TOK_LBRACE) {
            indent_level++;
        }

        /* After semicolons, add newline */
        if (tok->type == TOK_SEMICOLON) {
            /* Check if next token is on the same line in the original */
            if (i + 1 < token_count && tokens[i + 1].type != TOK_EOF) {
                buf_append_char(&out, '\n');
                at_line_start = true;
            }
        }

        /* After opening brace, add newline */
        if (tok->type == TOK_LBRACE) {
            buf_append_char(&out, '\n');
            at_line_start = true;
        }

        /* After closing brace, add newline (unless followed by else/deflect/semicolon) */
        if (tok->type == TOK_RBRACE) {
            bool next_continues = false;
            if (i + 1 < token_count) {
                TokenType nt = tokens[i + 1].type;
                next_continues = (nt == TOK_OTHERWISE || nt == TOK_DEFLECT ||
                                  nt == TOK_SEMICOLON || nt == TOK_COMMA);
            }
            if (!next_continues) {
                buf_append_char(&out, '\n');
                at_line_start = true;
            }
        }

        prev_type = tok->type;
        prev_line = tok->line;
    }

    /* Ensure final newline */
    if (config->insert_final_newline && out.len > 0 && out.data[out.len - 1] != '\n') {
        buf_append_char(&out, '\n');
    }

    free(tokens);

    /* Post-process: trim trailing whitespace from each line if configured */
    if (config->trim_trailing_whitespace && out.data) {
        /* Process line by line in place */
        char *result = (char *)malloc(out.len + 1);
        size_t rlen = 0;
        const char *p = out.data;
        while (*p) {
            const char *line_start = p;
            while (*p && *p != '\n') p++;
            size_t line_len = (size_t)(p - line_start);

            /* Trim trailing whitespace from this line */
            while (line_len > 0 && (line_start[line_len - 1] == ' ' ||
                                     line_start[line_len - 1] == '\t')) {
                line_len--;
            }
            memcpy(result + rlen, line_start, line_len);
            rlen += line_len;
            if (*p == '\n') {
                result[rlen++] = '\n';
                p++;
            }
        }
        result[rlen] = '\0';
        buf_free(&out);
        return result;
    }

    char *result = out.data;
    /* Don't call buf_free because we're returning the data */
    return result;
}

/* ===== Format a File In-place ===== */
bool xs_format_file(const char *path, const XsFormatConfig *config) {
    if (!path || !config) return false;

    /* Read the file */
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) { fclose(f); return false; }

    char *source = (char *)malloc((size_t)size + 1);
    if (!source) { fclose(f); return false; }

    size_t nread = fread(source, 1, (size_t)size, f);
    source[nread] = '\0';
    fclose(f);

    /* Format */
    char *formatted = xs_format_string(source, config);
    free(source);

    if (!formatted) return false;

    /* Write back */
    f = fopen(path, "wb");
    if (!f) {
        free(formatted);
        return false;
    }

    size_t flen = strlen(formatted);
    size_t written = fwrite(formatted, 1, flen, f);
    fclose(f);
    free(formatted);

    return written == flen;
}
