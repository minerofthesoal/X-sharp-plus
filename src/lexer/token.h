#ifndef XSHARP_TOKEN_H
#define XSHARP_TOKEN_H

#include <stddef.h>
#include <stdint.h>

/* ===== Token types for the X# language ===== */
typedef enum {
    /* --- Literals --- */
    TOK_INT_LITERAL = 0,     /* 42, 0xFF, 0b1010, 0o77 */
    TOK_FLOAT_LITERAL,       /* 3.14, 1e10 */
    TOK_STRING_LITERAL,      /* "hello" */
    TOK_INTERP_STRING_START, /* "hello #{ -- start of interpolated string */
    TOK_INTERP_STRING_MID,   /* }...#{ -- middle segment */
    TOK_INTERP_STRING_END,   /* }...end" -- end segment */
    TOK_RUNE_LITERAL,        /* 'a' */
    TOK_IDENTIFIER,          /* user-defined names */

    /* --- Keywords: types --- */
    TOK_BLADE,   /* blade (int) */
    TOK_SPARK,   /* spark (float) */
    TOK_SCROLL,  /* scroll (string) */
    TOK_RUNE,    /* rune (char) */
    TOK_FATE,    /* fate (bool) */
    TOK_VOID,    /* void */
    TOK_ARSENAL, /* arsenal (array) */

    /* --- Keywords: declarations --- */
    TOK_FORGE,   /* forge (function) */
    TOK_ENTITY,  /* entity (class) */
    TOK_REALM,   /* realm (namespace) */
    TOK_QUEST,   /* quest (main) */
    TOK_SUMMON,  /* summon (import) */
    TOK_ETERNAL, /* eternal (const) */
    TOK_MORPH,   /* morph (var/let mutable) */

    /* --- Keywords: control flow --- */
    TOK_ORACLE,        /* oracle (if) */
    TOK_OTHERWISE,     /* otherwise (else) */
    TOK_CYCLE,         /* cycle (for) */
    TOK_WHILE,         /* while */
    TOK_UNLEASH,       /* unleash (return) */
    TOK_SHATTER_CYCLE, /* shatter_cycle (break) */
    TOK_SKIP,          /* skip (continue) */

    /* --- Keywords: OOP & misc --- */
    TOK_CONJURE, /* conjure (new) */
    TOK_SELF,    /* self (this) */
    TOK_SPELL,   /* spell (lambda) */
    TOK_SHIELD,  /* shield (try) */
    TOK_DEFLECT, /* deflect (catch) */
    TOK_SHATTER, /* shatter (throw) */

    /* --- Keywords: literals --- */
    TOK_TRUTH, /* truth (true) */
    TOK_LIES,  /* lies (false) */
    TOK_ABYSS, /* abyss (null) */

    /* --- Keywords: special blocks --- */
    TOK_AT_GPU, /* @gpu */
    TOK_AT_AI,  /* @ai */

    /* --- Keywords: extras --- */
    TOK_EXTENDS,    /* extends (for entity inheritance) */
    TOK_IMPLEMENTS, /* implements */
    TOK_ENGRAVE,    /* engrave (print) */
    TOK_AS,         /* as (casting) */
    TOK_IN,         /* in (for-in loops) */
    TOK_FROM,       /* from (import from) */

    /* --- Operators: arithmetic --- */
    TOK_PLUS,    /* + */
    TOK_MINUS,   /* - */
    TOK_STAR,    /* * */
    TOK_SLASH,   /* / */
    TOK_PERCENT, /* % */
    TOK_POWER,   /* ** */

    /* --- Operators: comparison --- */
    TOK_EQUAL_EQUAL,   /* == */
    TOK_BANG_EQUAL,    /* != */
    TOK_LESS,          /* < */
    TOK_GREATER,       /* > */
    TOK_LESS_EQUAL,    /* <= */
    TOK_GREATER_EQUAL, /* >= */

    /* --- Operators: logical --- */
    TOK_AND_AND, /* && */
    TOK_OR_OR,   /* || */
    TOK_BANG,    /* ! */

    /* --- Operators: bitwise --- */
    TOK_AMPERSAND, /* & */
    TOK_PIPE,      /* | */
    TOK_CARET,     /* ^ */
    TOK_TILDE,     /* ~ */
    TOK_LSHIFT,    /* << */
    TOK_RSHIFT,    /* >> */

    /* --- Operators: assignment --- */
    TOK_EQUAL,           /* = */
    TOK_PLUS_EQUAL,      /* += */
    TOK_MINUS_EQUAL,     /* -= */
    TOK_STAR_EQUAL,      /* *= */
    TOK_SLASH_EQUAL,     /* /= */
    TOK_PERCENT_EQUAL,   /* %= */
    TOK_POWER_EQUAL,     /* **= */
    TOK_AMPERSAND_EQUAL, /* &= */
    TOK_PIPE_EQUAL,      /* |= */
    TOK_CARET_EQUAL,     /* ^= */
    TOK_LSHIFT_EQUAL,    /* <<= */
    TOK_RSHIFT_EQUAL,    /* >>= */

    /* --- Operators: special --- */
    TOK_PIPE_GREATER, /* |> (pipe) */
    TOK_ARROW,        /* -> */
    TOK_FAT_ARROW,    /* => */
    TOK_DOT,          /* . */
    TOK_DOT_DOT,      /* .. (range) */
    TOK_DOT_DOT_DOT,  /* ... (spread) */
    TOK_QUESTION,     /* ? */
    TOK_COLON,        /* : */
    TOK_DOUBLE_COLON, /* :: */
    TOK_HASH,         /* # */

    /* --- Delimiters --- */
    TOK_LPAREN,    /* ( */
    TOK_RPAREN,    /* ) */
    TOK_LBRACE,    /* { */
    TOK_RBRACE,    /* } */
    TOK_LBRACKET,  /* [ */
    TOK_RBRACKET,  /* ] */
    TOK_COMMA,     /* , */
    TOK_SEMICOLON, /* ; */

    /* --- Special --- */
    TOK_INCREMENT, /* ++ */
    TOK_DECREMENT, /* -- */

    /* --- Meta --- */
    TOK_EOF,
    TOK_ERROR,

    TOK_COUNT /* total number of token types */
} TokenType;

/* Token structure holding type, lexeme, position info */
typedef struct {
    TokenType type;
    const char* start; /* pointer into source */
    size_t length;
    int line;
    int column;

    /* For numeric literals, store the parsed value */
    union {
        int64_t int_val;
        double float_val;
    } literal;
} Token;

/* Return a human-readable name for a token type */
const char* token_type_name(TokenType type);

/* Print a token for debugging */
void token_print(const Token* tok);

#endif /* XSHARP_TOKEN_H */
