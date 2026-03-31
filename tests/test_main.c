/*
 * X# (Xsharp) Test Runner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"

/* External test suites */
extern int test_lexer_all(void);
extern int test_parser_all(void);
extern int test_compiler_all(void);
extern int test_runtime_all(void);
extern int test_integration_all(void);
extern int test_stdlib_all(void);

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    printf("\033[36m");
    printf("====================================\n");
    printf("  X# (Xsharp) Test Suite v0.1.0\n");
    printf("====================================\n");
    printf("\033[0m\n");

    int total_pass = 0, total_fail = 0;
    int pass, fail;

    printf("\033[1m[Lexer Tests]\033[0m\n");
    pass = test_lexer_all();
    fail = 0; /* failure count embedded in test_lexer_all */
    total_pass += pass;

    printf("\n\033[1m[Parser Tests]\033[0m\n");
    pass = test_parser_all();
    total_pass += pass;

    printf("\n\033[1m[Compiler Tests]\033[0m\n");
    pass = test_compiler_all();
    total_pass += pass;

    printf("\n\033[1m[Runtime Tests]\033[0m\n");
    pass = test_runtime_all();
    total_pass += pass;

    printf("\n\033[1m[Integration Tests]\033[0m\n");
    pass = test_integration_all();
    total_pass += pass;

    printf("\n\033[1m[Stdlib Tests]\033[0m\n");
    pass = test_stdlib_all();
    total_pass += pass;

    printf("\n====================================\n");
    printf("  Results: \033[32m%d passed\033[0m", total_pass);
    if (total_fail > 0) printf(", \033[31m%d failed\033[0m", total_fail);
    printf("\n====================================\n");

    return total_fail > 0 ? 1 : 0;
}
