/*
 * X# Integration Tests - End-to-end compilation and execution
 */

#include "../test_framework.h"
#include "../../src/lexer/lexer.h"
#include "../../src/parser/parser.h"
#include "../../src/compiler/compiler.h"
#include "../../src/codegen/codegen.h"
#include "../../src/vm/vm.h"
#include "../../src/runtime/runtime.h"
#include <string.h>

/* Helper: compile and run X# source, return VM result */
static VMResult run_source(const char *source) {
    Lexer lexer;
    lexer_init(&lexer, source);
    Token tokens[4096];
    int count = 0;
    for (;;) {
        Token tok = lexer_next_token(&lexer);
        tokens[count++] = tok;
        if (tok.type == TOK_EOF || count >= 4095) break;
    }

    Parser parser;
    parser_init(&parser, tokens, count);
    AstNode *program = parser_parse_program(&parser);
    if (parser_had_error(&parser)) {
        ast_free(program);
        return VM_COMPILE_ERROR;
    }

    Chunk chunk;
    chunk_init(&chunk, "<test>");
    Compiler compiler;
    compiler_init(&compiler, &chunk, SCOPE_SCRIPT);
    if (!compiler_compile(&compiler, program)) {
        chunk_free(&chunk);
        ast_free(program);
        return VM_COMPILE_ERROR;
    }

    VMState vm;
    vm_init(&vm);
    VMResult result = vm_execute(&vm, &chunk);

    vm_free(&vm);
    chunk_free(&chunk);
    ast_free(program);
    return result;
}

/* Test: Hello World */
static void test_hello_world(void) {
    VMResult r = run_source("engrave(\"Hello, World!\")");
    TEST_ASSERT(r == VM_OK, "Hello world should execute successfully");
}

/* Test: Variable declaration */
static void test_var_decl(void) {
    VMResult r = run_source("morph x: blade = 42\nengrave(x)");
    TEST_ASSERT(r == VM_OK, "Variable declaration should work");
}

/* Test: Arithmetic */
static void test_arithmetic(void) {
    VMResult r = run_source("morph x = 10 + 20 * 2\nengrave(x)");
    TEST_ASSERT(r == VM_OK, "Arithmetic should work");
}

/* Test: If/else (oracle/otherwise) */
static void test_oracle(void) {
    VMResult r = run_source(
        "morph x = 10\n"
        "oracle (x > 5) {\n"
        "    engrave(\"big\")\n"
        "} otherwise {\n"
        "    engrave(\"small\")\n"
        "}\n"
    );
    TEST_ASSERT(r == VM_OK, "Oracle/otherwise should work");
}

/* Test: While loop */
static void test_while_loop(void) {
    VMResult r = run_source(
        "morph i = 0\n"
        "while (i < 5) {\n"
        "    engrave(i)\n"
        "    i = i + 1\n"
        "}\n"
    );
    TEST_ASSERT(r == VM_OK, "While loop should work");
}

/* Test: Boolean values */
static void test_booleans(void) {
    VMResult r = run_source(
        "morph a: fate = truth\n"
        "morph b: fate = lies\n"
        "engrave(a)\n"
        "engrave(b)\n"
    );
    TEST_ASSERT(r == VM_OK, "Boolean truth/lies should work");
}

/* Test: String operations */
static void test_strings(void) {
    VMResult r = run_source(
        "morph name: scroll = \"X#\"\n"
        "engrave(name)\n"
    );
    TEST_ASSERT(r == VM_OK, "String operations should work");
}

/* Test: Arsenal (array) literal */
static void test_arsenal(void) {
    VMResult r = run_source(
        "morph arr = [1, 2, 3, 4, 5]\n"
        "engrave(arr)\n"
    );
    TEST_ASSERT(r == VM_OK, "Arsenal literal should work");
}

/* Test: Quest (main) */
static void test_quest(void) {
    VMResult r = run_source(
        "quest() {\n"
        "    engrave(\"In quest\")\n"
        "}\n"
    );
    TEST_ASSERT(r == VM_OK, "Quest should work");
}

/* Test: Null (abyss) */
static void test_abyss(void) {
    VMResult r = run_source(
        "morph x = abyss\n"
        "engrave(x)\n"
    );
    TEST_ASSERT(r == VM_OK, "Abyss (null) should work");
}

int test_integration_all(void) {
    int pass = 0;
    TEST_RUN(test_hello_world, &pass);
    TEST_RUN(test_var_decl, &pass);
    TEST_RUN(test_arithmetic, &pass);
    TEST_RUN(test_oracle, &pass);
    TEST_RUN(test_while_loop, &pass);
    TEST_RUN(test_booleans, &pass);
    TEST_RUN(test_strings, &pass);
    TEST_RUN(test_arsenal, &pass);
    TEST_RUN(test_quest, &pass);
    TEST_RUN(test_abyss, &pass);
    return pass;
}
