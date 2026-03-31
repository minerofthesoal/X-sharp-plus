/*
 * X# (Xsharp) Linter
 * ====================
 * Static analysis: unused variables, naming conventions,
 * missing returns, and other diagnostics.
 */

#ifndef XSHARP_LINTER_H
#define XSHARP_LINTER_H

#include <stdbool.h>

/* Diagnostic severity */
typedef enum { LINT_ERROR, LINT_WARNING, LINT_INFO, LINT_HINT } XsLintSeverity;

/* A single diagnostic */
typedef struct {
    XsLintSeverity severity;
    char file[512];
    int line;
    int column;
    char message[512];
    char rule[64]; /* e.g. "unused-variable" */
} XsLintDiagnostic;

/* Lint result for a file */
typedef struct {
    XsLintDiagnostic* diagnostics;
    int count;
    int capacity;
    int error_count;
    int warning_count;
} XsLintResult;

/* Lint a single file. Returns a result that must be freed with xs_lint_result_free. */
XsLintResult* xs_lint_file(const char* path);

/* Lint source text directly. */
XsLintResult* xs_lint_string(const char* source, const char* filename);

/* Free a lint result. */
void xs_lint_result_free(XsLintResult* result);

/* Print lint results to stderr. */
void xs_lint_result_print(const XsLintResult* result);

#endif /* XSHARP_LINTER_H */
