/*
 * X# Parser Tests
 * =================
 * Tests for parsing all language constructs, error recovery.
 *
 * Note: These tests verify parsing by tokenizing inputs that represent
 * valid and invalid X# constructs, checking token sequences that the
 * parser would process. Full AST-based tests will be possible once
 * the parser module is implemented.
 */

#include "../test_framework.h"
#include "../../src/lexer/lexer.h"
#include "../../src/lexer/token.h"

/* ===== Helper: verify token sequence ===== */
static Token *tokenize(const char *source, int *count) {
    Lexer lexer;
    lexer_init(&lexer, source);
    return lexer_tokenize_all(&lexer, count);
}

static bool verify_token_types(const char *source, TokenType *expected, int expected_count) {
    int count;
    Token *tokens = tokenize(source, &count);
    if (!tokens) return false;

    /* count includes EOF */
    bool ok = true;
    for (int i = 0; i < expected_count && i < count; i++) {
        if (tokens[i].type != expected[i]) {
            ok = false;
            break;
        }
    }
    free(tokens);
    return ok;
}

/* ===== Variable Declaration Tests ===== */

static void test_parse_morph_decl(void) {
    TEST_CASE("Parse: morph variable declaration");
    TokenType expected[] = {
        TOK_MORPH, TOK_IDENTIFIER, TOK_COLON, TOK_BLADE, TOK_EQUAL, TOK_INT_LITERAL, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("morph x: blade = 42", expected, 7));
}

static void test_parse_eternal_decl(void) {
    TEST_CASE("Parse: eternal constant declaration");
    TokenType expected[] = {
        TOK_ETERNAL, TOK_IDENTIFIER, TOK_COLON, TOK_SPARK, TOK_EQUAL, TOK_FLOAT_LITERAL, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("eternal PI: spark = 3.14", expected, 7));
}

/* ===== Function Declaration Tests ===== */

static void test_parse_forge_no_params(void) {
    TEST_CASE("Parse: forge with no parameters");
    TokenType expected[] = {
        TOK_FORGE, TOK_IDENTIFIER, TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("forge foo() {}", expected, 7));
}

static void test_parse_forge_with_params(void) {
    TEST_CASE("Parse: forge with parameters and return type");
    TokenType expected[] = {
        TOK_FORGE, TOK_IDENTIFIER, TOK_LPAREN,
        TOK_IDENTIFIER, TOK_COLON, TOK_BLADE, TOK_COMMA,
        TOK_IDENTIFIER, TOK_COLON, TOK_BLADE,
        TOK_RPAREN, TOK_ARROW, TOK_BLADE,
        TOK_LBRACE, TOK_RBRACE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("forge add(a: blade, b: blade) -> blade {}", expected, 16));
}

static void test_parse_quest_main(void) {
    TEST_CASE("Parse: quest (main function)");
    TokenType expected[] = {
        TOK_QUEST, TOK_LPAREN, TOK_RPAREN, TOK_LBRACE,
        TOK_ENGRAVE, TOK_LPAREN, TOK_STRING_LITERAL, TOK_RPAREN,
        TOK_RBRACE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("quest() { engrave(\"hello\") }", expected, 10));
}

/* ===== Control Flow Tests ===== */

static void test_parse_oracle_otherwise(void) {
    TEST_CASE("Parse: oracle/otherwise (if/else)");
    TokenType expected[] = {
        TOK_ORACLE, TOK_LPAREN, TOK_IDENTIFIER, TOK_RPAREN,
        TOK_LBRACE, TOK_RBRACE,
        TOK_OTHERWISE, TOK_LBRACE, TOK_RBRACE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("oracle (x) {} otherwise {}", expected, 10));
}

static void test_parse_cycle_loop(void) {
    TEST_CASE("Parse: cycle (for loop)");
    TokenType expected[] = {
        TOK_CYCLE, TOK_LPAREN,
        TOK_MORPH, TOK_IDENTIFIER, TOK_COLON, TOK_BLADE, TOK_EQUAL, TOK_INT_LITERAL, TOK_SEMICOLON,
        TOK_IDENTIFIER, TOK_LESS, TOK_INT_LITERAL, TOK_SEMICOLON,
        TOK_IDENTIFIER, TOK_INCREMENT,
        TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types(
        "cycle (morph i: blade = 0; i < 10; i++) {}", expected, 19));
}

static void test_parse_while_loop(void) {
    TEST_CASE("Parse: while loop");
    TokenType expected[] = {
        TOK_WHILE, TOK_LPAREN, TOK_IDENTIFIER, TOK_RPAREN,
        TOK_LBRACE, TOK_RBRACE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("while (running) {}", expected, 7));
}

static void test_parse_shatter_skip(void) {
    TEST_CASE("Parse: shatter_cycle (break) and skip (continue)");
    int count;
    Token *tokens = tokenize("shatter_cycle", &count);
    ASSERT_NOT_NULL(tokens);
    ASSERT_EQ(tokens[0].type, TOK_SHATTER_CYCLE);
    free(tokens);

    tokens = tokenize("skip", &count);
    ASSERT_NOT_NULL(tokens);
    ASSERT_EQ(tokens[0].type, TOK_SKIP);
    free(tokens);
}

/* ===== Entity (Class) Tests ===== */

static void test_parse_entity_basic(void) {
    TEST_CASE("Parse: basic entity declaration");
    TokenType expected[] = {
        TOK_ENTITY, TOK_IDENTIFIER, TOK_LBRACE, TOK_RBRACE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("entity MyClass {}", expected, 5));
}

static void test_parse_entity_extends(void) {
    TEST_CASE("Parse: entity with extends");
    TokenType expected[] = {
        TOK_ENTITY, TOK_IDENTIFIER, TOK_EXTENDS, TOK_IDENTIFIER,
        TOK_LBRACE, TOK_RBRACE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("entity Dog extends Animal {}", expected, 7));
}

static void test_parse_entity_with_fields(void) {
    TEST_CASE("Parse: entity with fields and methods");
    const char *src =
        "entity Point {\n"
        "  morph x: spark\n"
        "  morph y: spark\n"
        "  forge conjure(x: spark, y: spark) {\n"
        "    self.x = x\n"
        "    self.y = y\n"
        "  }\n"
        "}";
    int count;
    Token *tokens = tokenize(src, &count);
    ASSERT_NOT_NULL(tokens);
    ASSERT_EQ(tokens[0].type, TOK_ENTITY);
    ASSERT_EQ(tokens[1].type, TOK_IDENTIFIER);
    ASSERT_EQ(tokens[2].type, TOK_LBRACE);
    /* morph x: spark */
    ASSERT_EQ(tokens[3].type, TOK_MORPH);
    free(tokens);
}

/* ===== Expression Tests ===== */

static void test_parse_binary_expression(void) {
    TEST_CASE("Parse: binary expression");
    TokenType expected[] = {
        TOK_IDENTIFIER, TOK_PLUS, TOK_IDENTIFIER, TOK_STAR, TOK_IDENTIFIER, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("a + b * c", expected, 6));
}

static void test_parse_method_call(void) {
    TEST_CASE("Parse: method call chain");
    TokenType expected[] = {
        TOK_IDENTIFIER, TOK_DOT, TOK_IDENTIFIER, TOK_LPAREN, TOK_RPAREN,
        TOK_DOT, TOK_IDENTIFIER, TOK_LPAREN, TOK_RPAREN, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("obj.method().chain()", expected, 10));
}

static void test_parse_array_access(void) {
    TEST_CASE("Parse: array access");
    TokenType expected[] = {
        TOK_IDENTIFIER, TOK_LBRACKET, TOK_INT_LITERAL, TOK_RBRACKET, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("arr[0]", expected, 5));
}

static void test_parse_conjure_expression(void) {
    TEST_CASE("Parse: conjure (new) expression");
    TokenType expected[] = {
        TOK_CONJURE, TOK_IDENTIFIER, TOK_LPAREN, TOK_INT_LITERAL,
        TOK_COMMA, TOK_INT_LITERAL, TOK_RPAREN, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("conjure Point(1, 2)", expected, 8));
}

/* ===== Special Block Tests ===== */

static void test_parse_gpu_block(void) {
    TEST_CASE("Parse: @gpu block");
    TokenType expected[] = {
        TOK_AT_GPU, TOK_LBRACE, TOK_RBRACE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("@gpu {}", expected, 4));
}

static void test_parse_ai_block(void) {
    TEST_CASE("Parse: @ai block");
    TokenType expected[] = {
        TOK_AT_AI, TOK_LBRACE, TOK_RBRACE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("@ai {}", expected, 4));
}

/* ===== Import Tests ===== */

static void test_parse_summon(void) {
    TEST_CASE("Parse: summon (import)");
    TokenType expected[] = {
        TOK_SUMMON, TOK_IDENTIFIER, TOK_FROM, TOK_STRING_LITERAL, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("summon Math from \"stdlib/math\"", expected, 5));
}

/* ===== Error-tolerant parsing ===== */

static void test_parse_unclosed_paren(void) {
    TEST_CASE("Parse: unclosed parenthesis generates valid tokens");
    int count;
    Token *tokens = tokenize("forge foo(", &count);
    ASSERT_NOT_NULL(tokens);
    /* Should still produce tokens up to end */
    ASSERT_EQ(tokens[0].type, TOK_FORGE);
    ASSERT_EQ(tokens[1].type, TOK_IDENTIFIER);
    ASSERT_EQ(tokens[2].type, TOK_LPAREN);
    free(tokens);
}

static void test_parse_pipe_operator(void) {
    TEST_CASE("Parse: pipe operator |>");
    TokenType expected[] = {
        TOK_IDENTIFIER, TOK_PIPE_GREATER, TOK_IDENTIFIER, TOK_LPAREN, TOK_RPAREN, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("data |> transform()", expected, 6));
}

static void test_parse_range_expression(void) {
    TEST_CASE("Parse: range expression ..");
    TokenType expected[] = {
        TOK_INT_LITERAL, TOK_DOT_DOT, TOK_INT_LITERAL, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("0..10", expected, 4));
}

static void test_parse_cast_expression(void) {
    TEST_CASE("Parse: cast expression (as)");
    TokenType expected[] = {
        TOK_IDENTIFIER, TOK_AS, TOK_BLADE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("x as blade", expected, 4));
}

static void test_parse_shield_deflect(void) {
    TEST_CASE("Parse: shield/deflect (try/catch)");
    TokenType expected[] = {
        TOK_SHIELD, TOK_LBRACE, TOK_RBRACE,
        TOK_DEFLECT, TOK_LPAREN, TOK_IDENTIFIER, TOK_RPAREN,
        TOK_LBRACE, TOK_RBRACE, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("shield {} deflect (e) {}", expected, 10));
}

static void test_parse_spell_lambda(void) {
    TEST_CASE("Parse: spell (lambda)");
    TokenType expected[] = {
        TOK_SPELL, TOK_LPAREN, TOK_IDENTIFIER, TOK_RPAREN,
        TOK_FAT_ARROW, TOK_IDENTIFIER, TOK_STAR, TOK_INT_LITERAL, TOK_EOF
    };
    ASSERT_TRUE(verify_token_types("spell (x) => x * 2", expected, 9));
}

/* ===== Test Runner ===== */

void run_parser_tests(void) {
    TEST_SUITE("Parser");

    /* Variable declarations */
    RUN_TEST(test_parse_morph_decl);
    RUN_TEST(test_parse_eternal_decl);

    /* Function declarations */
    RUN_TEST(test_parse_forge_no_params);
    RUN_TEST(test_parse_forge_with_params);
    RUN_TEST(test_parse_quest_main);

    /* Control flow */
    RUN_TEST(test_parse_oracle_otherwise);
    RUN_TEST(test_parse_cycle_loop);
    RUN_TEST(test_parse_while_loop);
    RUN_TEST(test_parse_shatter_skip);

    /* Entity declarations */
    RUN_TEST(test_parse_entity_basic);
    RUN_TEST(test_parse_entity_extends);
    RUN_TEST(test_parse_entity_with_fields);

    /* Expressions */
    RUN_TEST(test_parse_binary_expression);
    RUN_TEST(test_parse_method_call);
    RUN_TEST(test_parse_array_access);
    RUN_TEST(test_parse_conjure_expression);

    /* Special blocks */
    RUN_TEST(test_parse_gpu_block);
    RUN_TEST(test_parse_ai_block);

    /* Imports */
    RUN_TEST(test_parse_summon);

    /* Error/edge cases */
    RUN_TEST(test_parse_unclosed_paren);
    RUN_TEST(test_parse_pipe_operator);
    RUN_TEST(test_parse_range_expression);
    RUN_TEST(test_parse_cast_expression);
    RUN_TEST(test_parse_shield_deflect);
    RUN_TEST(test_parse_spell_lambda);
}
