/*
 * X# Lexer Tests
 * ================
 * Tests for tokenization of all token types, edge cases, error handling.
 */

#include "../test_framework.h"
#include "../../src/lexer/lexer.h"
#include "../../src/lexer/token.h"

/* ===== Helper: tokenize and get token at index ===== */
static Token get_token(const char *source, int index) {
    Lexer lexer;
    lexer_init(&lexer, source);
    Token tok;
    for (int i = 0; i <= index; i++) {
        tok = lexer_next_token(&lexer);
    }
    return tok;
}

static int count_tokens(const char *source) {
    Lexer lexer;
    lexer_init(&lexer, source);
    int count = 0;
    Token tok;
    do {
        tok = lexer_next_token(&lexer);
        count++;
    } while (tok.type != TOK_EOF && tok.type != TOK_ERROR);
    return count;
}

/* ===== Integer Literal Tests ===== */

static void test_integer_decimal(void) {
    TEST_CASE("Integer: decimal literal");
    Token tok = get_token("42", 0);
    ASSERT_EQ(tok.type, TOK_INT_LITERAL);
    ASSERT_EQ(tok.literal.int_val, 42);
}

static void test_integer_zero(void) {
    TEST_CASE("Integer: zero");
    Token tok = get_token("0", 0);
    ASSERT_EQ(tok.type, TOK_INT_LITERAL);
    ASSERT_EQ(tok.literal.int_val, 0);
}

static void test_integer_hex(void) {
    TEST_CASE("Integer: hex literal 0xFF");
    Token tok = get_token("0xFF", 0);
    ASSERT_EQ(tok.type, TOK_INT_LITERAL);
    ASSERT_EQ(tok.literal.int_val, 255);
}

static void test_integer_binary(void) {
    TEST_CASE("Integer: binary literal 0b1010");
    Token tok = get_token("0b1010", 0);
    ASSERT_EQ(tok.type, TOK_INT_LITERAL);
    ASSERT_EQ(tok.literal.int_val, 10);
}

static void test_integer_octal(void) {
    TEST_CASE("Integer: octal literal 0o77");
    Token tok = get_token("0o77", 0);
    ASSERT_EQ(tok.type, TOK_INT_LITERAL);
    ASSERT_EQ(tok.literal.int_val, 63);
}

/* ===== Float Literal Tests ===== */

static void test_float_basic(void) {
    TEST_CASE("Float: basic 3.14");
    Token tok = get_token("3.14", 0);
    ASSERT_EQ(tok.type, TOK_FLOAT_LITERAL);
    ASSERT_FLOAT_EQ(tok.literal.float_val, 3.14, 0.001);
}

static void test_float_scientific(void) {
    TEST_CASE("Float: scientific notation 1e10");
    Token tok = get_token("1e10", 0);
    ASSERT_EQ(tok.type, TOK_FLOAT_LITERAL);
    ASSERT_FLOAT_EQ(tok.literal.float_val, 1e10, 1.0);
}

/* ===== String Literal Tests ===== */

static void test_string_basic(void) {
    TEST_CASE("String: basic literal");
    Token tok = get_token("\"hello\"", 0);
    ASSERT_EQ(tok.type, TOK_STRING_LITERAL);
    ASSERT_EQ(tok.length, 7); /* includes quotes */
}

static void test_string_empty(void) {
    TEST_CASE("String: empty string");
    Token tok = get_token("\"\"", 0);
    ASSERT_EQ(tok.type, TOK_STRING_LITERAL);
}

static void test_rune_literal(void) {
    TEST_CASE("Rune: character literal");
    Token tok = get_token("'a'", 0);
    ASSERT_EQ(tok.type, TOK_RUNE_LITERAL);
}

/* ===== Keyword Tests ===== */

static void test_keyword_blade(void) {
    TEST_CASE("Keyword: blade");
    Token tok = get_token("blade", 0);
    ASSERT_EQ(tok.type, TOK_BLADE);
}

static void test_keyword_spark(void) {
    TEST_CASE("Keyword: spark");
    Token tok = get_token("spark", 0);
    ASSERT_EQ(tok.type, TOK_SPARK);
}

static void test_keyword_scroll(void) {
    TEST_CASE("Keyword: scroll");
    Token tok = get_token("scroll", 0);
    ASSERT_EQ(tok.type, TOK_SCROLL);
}

static void test_keyword_forge(void) {
    TEST_CASE("Keyword: forge");
    Token tok = get_token("forge", 0);
    ASSERT_EQ(tok.type, TOK_FORGE);
}

static void test_keyword_entity(void) {
    TEST_CASE("Keyword: entity");
    Token tok = get_token("entity", 0);
    ASSERT_EQ(tok.type, TOK_ENTITY);
}

static void test_keyword_quest(void) {
    TEST_CASE("Keyword: quest");
    Token tok = get_token("quest", 0);
    ASSERT_EQ(tok.type, TOK_QUEST);
}

static void test_keyword_oracle(void) {
    TEST_CASE("Keyword: oracle");
    Token tok = get_token("oracle", 0);
    ASSERT_EQ(tok.type, TOK_ORACLE);
}

static void test_keyword_otherwise(void) {
    TEST_CASE("Keyword: otherwise");
    Token tok = get_token("otherwise", 0);
    ASSERT_EQ(tok.type, TOK_OTHERWISE);
}

static void test_keyword_cycle(void) {
    TEST_CASE("Keyword: cycle");
    Token tok = get_token("cycle", 0);
    ASSERT_EQ(tok.type, TOK_CYCLE);
}

static void test_keyword_while(void) {
    TEST_CASE("Keyword: while");
    Token tok = get_token("while", 0);
    ASSERT_EQ(tok.type, TOK_WHILE);
}

static void test_keyword_unleash(void) {
    TEST_CASE("Keyword: unleash");
    Token tok = get_token("unleash", 0);
    ASSERT_EQ(tok.type, TOK_UNLEASH);
}

static void test_keyword_morph(void) {
    TEST_CASE("Keyword: morph");
    Token tok = get_token("morph", 0);
    ASSERT_EQ(tok.type, TOK_MORPH);
}

static void test_keyword_eternal(void) {
    TEST_CASE("Keyword: eternal");
    Token tok = get_token("eternal", 0);
    ASSERT_EQ(tok.type, TOK_ETERNAL);
}

static void test_keyword_truth_lies(void) {
    TEST_CASE("Keywords: truth and lies");
    Token tok1 = get_token("truth", 0);
    ASSERT_EQ(tok1.type, TOK_TRUTH);
    Token tok2 = get_token("lies", 0);
    ASSERT_EQ(tok2.type, TOK_LIES);
}

static void test_keyword_abyss(void) {
    TEST_CASE("Keyword: abyss");
    Token tok = get_token("abyss", 0);
    ASSERT_EQ(tok.type, TOK_ABYSS);
}

static void test_keyword_conjure(void) {
    TEST_CASE("Keyword: conjure");
    Token tok = get_token("conjure", 0);
    ASSERT_EQ(tok.type, TOK_CONJURE);
}

static void test_keyword_self(void) {
    TEST_CASE("Keyword: self");
    Token tok = get_token("self", 0);
    ASSERT_EQ(tok.type, TOK_SELF);
}

static void test_keyword_engrave(void) {
    TEST_CASE("Keyword: engrave");
    Token tok = get_token("engrave", 0);
    ASSERT_EQ(tok.type, TOK_ENGRAVE);
}

static void test_keyword_at_gpu(void) {
    TEST_CASE("Keyword: @gpu");
    Token tok = get_token("@gpu", 0);
    ASSERT_EQ(tok.type, TOK_AT_GPU);
}

static void test_keyword_at_ai(void) {
    TEST_CASE("Keyword: @ai");
    Token tok = get_token("@ai", 0);
    ASSERT_EQ(tok.type, TOK_AT_AI);
}

/* ===== Operator Tests ===== */

static void test_operators_arithmetic(void) {
    TEST_CASE("Operators: arithmetic");
    ASSERT_EQ(get_token("+", 0).type, TOK_PLUS);
    ASSERT_EQ(get_token("-", 0).type, TOK_MINUS);
    ASSERT_EQ(get_token("*", 0).type, TOK_STAR);
    ASSERT_EQ(get_token("/", 0).type, TOK_SLASH);
    ASSERT_EQ(get_token("%", 0).type, TOK_PERCENT);
    ASSERT_EQ(get_token("**", 0).type, TOK_POWER);
}

static void test_operators_comparison(void) {
    TEST_CASE("Operators: comparison");
    ASSERT_EQ(get_token("==", 0).type, TOK_EQUAL_EQUAL);
    ASSERT_EQ(get_token("!=", 0).type, TOK_BANG_EQUAL);
    ASSERT_EQ(get_token("<", 0).type, TOK_LESS);
    ASSERT_EQ(get_token(">", 0).type, TOK_GREATER);
    ASSERT_EQ(get_token("<=", 0).type, TOK_LESS_EQUAL);
    ASSERT_EQ(get_token(">=", 0).type, TOK_GREATER_EQUAL);
}

static void test_operators_logical(void) {
    TEST_CASE("Operators: logical");
    ASSERT_EQ(get_token("&&", 0).type, TOK_AND_AND);
    ASSERT_EQ(get_token("||", 0).type, TOK_OR_OR);
    ASSERT_EQ(get_token("!", 0).type, TOK_BANG);
}

static void test_operators_assignment(void) {
    TEST_CASE("Operators: assignment");
    ASSERT_EQ(get_token("=", 0).type, TOK_EQUAL);
    ASSERT_EQ(get_token("+=", 0).type, TOK_PLUS_EQUAL);
    ASSERT_EQ(get_token("-=", 0).type, TOK_MINUS_EQUAL);
    ASSERT_EQ(get_token("*=", 0).type, TOK_STAR_EQUAL);
    ASSERT_EQ(get_token("/=", 0).type, TOK_SLASH_EQUAL);
}

static void test_operators_special(void) {
    TEST_CASE("Operators: special");
    ASSERT_EQ(get_token("|>", 0).type, TOK_PIPE_GREATER);
    ASSERT_EQ(get_token("->", 0).type, TOK_ARROW);
    ASSERT_EQ(get_token("=>", 0).type, TOK_FAT_ARROW);
    ASSERT_EQ(get_token(".", 0).type, TOK_DOT);
    ASSERT_EQ(get_token("..", 0).type, TOK_DOT_DOT);
    ASSERT_EQ(get_token("...", 0).type, TOK_DOT_DOT_DOT);
}

/* ===== Delimiter Tests ===== */

static void test_delimiters(void) {
    TEST_CASE("Delimiters");
    ASSERT_EQ(get_token("(", 0).type, TOK_LPAREN);
    ASSERT_EQ(get_token(")", 0).type, TOK_RPAREN);
    ASSERT_EQ(get_token("{", 0).type, TOK_LBRACE);
    ASSERT_EQ(get_token("}", 0).type, TOK_RBRACE);
    ASSERT_EQ(get_token("[", 0).type, TOK_LBRACKET);
    ASSERT_EQ(get_token("]", 0).type, TOK_RBRACKET);
    ASSERT_EQ(get_token(",", 0).type, TOK_COMMA);
    ASSERT_EQ(get_token(";", 0).type, TOK_SEMICOLON);
}

/* ===== Identifier Tests ===== */

static void test_identifier_simple(void) {
    TEST_CASE("Identifier: simple name");
    Token tok = get_token("myVar", 0);
    ASSERT_EQ(tok.type, TOK_IDENTIFIER);
    ASSERT_EQ(tok.length, 5);
}

static void test_identifier_underscore(void) {
    TEST_CASE("Identifier: with underscore");
    Token tok = get_token("_private_var", 0);
    ASSERT_EQ(tok.type, TOK_IDENTIFIER);
}

static void test_identifier_alphanumeric(void) {
    TEST_CASE("Identifier: alphanumeric");
    Token tok = get_token("item2", 0);
    ASSERT_EQ(tok.type, TOK_IDENTIFIER);
    ASSERT_EQ(tok.length, 5);
}

/* ===== Comment Tests ===== */

static void test_comment_skipped(void) {
    TEST_CASE("Comment: ~~ line comment skipped");
    Token tok = get_token("~~ this is a comment\n42", 0);
    ASSERT_EQ(tok.type, TOK_INT_LITERAL);
    ASSERT_EQ(tok.literal.int_val, 42);
}

/* ===== Multi-token Tests ===== */

static void test_multi_token_expression(void) {
    TEST_CASE("Multi-token: simple expression");
    Lexer lexer;
    lexer_init(&lexer, "x + 42");
    Token t1 = lexer_next_token(&lexer);
    Token t2 = lexer_next_token(&lexer);
    Token t3 = lexer_next_token(&lexer);
    Token t4 = lexer_next_token(&lexer);
    ASSERT_EQ(t1.type, TOK_IDENTIFIER);
    ASSERT_EQ(t2.type, TOK_PLUS);
    ASSERT_EQ(t3.type, TOK_INT_LITERAL);
    ASSERT_EQ(t4.type, TOK_EOF);
}

static void test_multi_token_function_decl(void) {
    TEST_CASE("Multi-token: function declaration");
    Lexer lexer;
    lexer_init(&lexer, "forge add(a: blade, b: blade) -> blade");
    Token tokens[20];
    int i = 0;
    do {
        tokens[i] = lexer_next_token(&lexer);
    } while (tokens[i++].type != TOK_EOF && i < 20);

    ASSERT_EQ(tokens[0].type, TOK_FORGE);
    ASSERT_EQ(tokens[1].type, TOK_IDENTIFIER); /* add */
    ASSERT_EQ(tokens[2].type, TOK_LPAREN);
    ASSERT_EQ(tokens[3].type, TOK_IDENTIFIER); /* a */
    ASSERT_EQ(tokens[4].type, TOK_COLON);
    ASSERT_EQ(tokens[5].type, TOK_BLADE);
}

static void test_line_tracking(void) {
    TEST_CASE("Line and column tracking");
    Lexer lexer;
    lexer_init(&lexer, "x\ny\nz");
    Token t1 = lexer_next_token(&lexer);
    Token t2 = lexer_next_token(&lexer);
    Token t3 = lexer_next_token(&lexer);
    ASSERT_EQ(t1.line, 1);
    ASSERT_EQ(t2.line, 2);
    ASSERT_EQ(t3.line, 3);
}

static void test_eof_at_end(void) {
    TEST_CASE("EOF: at end of input");
    Token tok = get_token("", 0);
    ASSERT_EQ(tok.type, TOK_EOF);
}

static void test_tokenize_all(void) {
    TEST_CASE("tokenize_all: complete tokenization");
    Lexer lexer;
    lexer_init(&lexer, "morph x: blade = 10");
    int count = 0;
    Token *tokens = lexer_tokenize_all(&lexer, &count);
    ASSERT_NOT_NULL(tokens);
    ASSERT_GE(count, 5);
    ASSERT_EQ(tokens[0].type, TOK_MORPH);
    ASSERT_EQ(tokens[count - 1].type, TOK_EOF);
    free(tokens);
}

static void test_increment_decrement(void) {
    TEST_CASE("Operators: increment and decrement");
    ASSERT_EQ(get_token("++", 0).type, TOK_INCREMENT);
    ASSERT_EQ(get_token("--", 0).type, TOK_DECREMENT);
}

/* ===== Test Runner ===== */

void run_lexer_tests(void) {
    TEST_SUITE("Lexer");

    /* Integer literals */
    RUN_TEST(test_integer_decimal);
    RUN_TEST(test_integer_zero);
    RUN_TEST(test_integer_hex);
    RUN_TEST(test_integer_binary);
    RUN_TEST(test_integer_octal);

    /* Float literals */
    RUN_TEST(test_float_basic);
    RUN_TEST(test_float_scientific);

    /* String/rune literals */
    RUN_TEST(test_string_basic);
    RUN_TEST(test_string_empty);
    RUN_TEST(test_rune_literal);

    /* Keywords */
    RUN_TEST(test_keyword_blade);
    RUN_TEST(test_keyword_spark);
    RUN_TEST(test_keyword_scroll);
    RUN_TEST(test_keyword_forge);
    RUN_TEST(test_keyword_entity);
    RUN_TEST(test_keyword_quest);
    RUN_TEST(test_keyword_oracle);
    RUN_TEST(test_keyword_otherwise);
    RUN_TEST(test_keyword_cycle);
    RUN_TEST(test_keyword_while);
    RUN_TEST(test_keyword_unleash);
    RUN_TEST(test_keyword_morph);
    RUN_TEST(test_keyword_eternal);
    RUN_TEST(test_keyword_truth_lies);
    RUN_TEST(test_keyword_abyss);
    RUN_TEST(test_keyword_conjure);
    RUN_TEST(test_keyword_self);
    RUN_TEST(test_keyword_engrave);
    RUN_TEST(test_keyword_at_gpu);
    RUN_TEST(test_keyword_at_ai);

    /* Operators */
    RUN_TEST(test_operators_arithmetic);
    RUN_TEST(test_operators_comparison);
    RUN_TEST(test_operators_logical);
    RUN_TEST(test_operators_assignment);
    RUN_TEST(test_operators_special);

    /* Delimiters */
    RUN_TEST(test_delimiters);

    /* Identifiers */
    RUN_TEST(test_identifier_simple);
    RUN_TEST(test_identifier_underscore);
    RUN_TEST(test_identifier_alphanumeric);

    /* Comments */
    RUN_TEST(test_comment_skipped);

    /* Multi-token */
    RUN_TEST(test_multi_token_expression);
    RUN_TEST(test_multi_token_function_decl);
    RUN_TEST(test_line_tracking);
    RUN_TEST(test_eof_at_end);
    RUN_TEST(test_tokenize_all);
    RUN_TEST(test_increment_decrement);
}
