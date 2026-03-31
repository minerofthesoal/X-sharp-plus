/*
 * X# (Xsharp) Test Runner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"

/* External test suite runners */
extern void run_lexer_tests(void);
extern void run_parser_tests(void);
extern void run_compiler_tests(void);
extern void run_runtime_tests(void);
extern int  test_integration_all(void);
extern int  test_stdlib_all(void);

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    printf("\033[36m");
    printf("====================================\n");
    printf("  X# (Xsharp) Test Suite v0.1.0\n");
    printf("====================================\n");
    printf("\033[0m\n");

    printf("\033[1m[Lexer Tests]\033[0m\n");
    run_lexer_tests();

    printf("\n\033[1m[Parser Tests]\033[0m\n");
    run_parser_tests();

    printf("\n\033[1m[Compiler Tests]\033[0m\n");
    run_compiler_tests();

    printf("\n\033[1m[Runtime Tests]\033[0m\n");
    run_runtime_tests();

    printf("\n\033[1m[Integration Tests]\033[0m\n");
    test_integration_all();

    printf("\n\033[1m[Stdlib Tests]\033[0m\n");
    test_stdlib_all();

    return test_summary();
}
