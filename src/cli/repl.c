/*
 * X# (Xsharp) REPL - Read-Eval-Print Loop Implementation
 * =========================================================
 */

#include "repl.h"
#include "../codegen/codegen.h"
#include "../compiler/compiler.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../runtime/runtime.h"
#include "../vm/vm.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Try to use readline if available */
#ifdef HAVE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#define REPL_USE_READLINE 1
#else
#define REPL_USE_READLINE 0
#endif

/* ===== ANSI Colors ===== */
#define CLR_RESET "\033[0m"
#define CLR_RED "\033[31m"
#define CLR_GREEN "\033[32m"
#define CLR_YELLOW "\033[33m"
#define CLR_BLUE "\033[34m"
#define CLR_CYAN "\033[36m"
#define CLR_BOLD "\033[1m"
#define CLR_DIM "\033[2m"

/* ===== REPL State ===== */
static VMState repl_vm;
static bool repl_vm_initialized = false;
static char* repl_history[REPL_MAX_HISTORY];
static int repl_history_count = 0;

/* ===== History Management ===== */
static void history_add(const char* line) {
    if (repl_history_count < REPL_MAX_HISTORY) {
        repl_history[repl_history_count++] = strdup(line);
    } else {
        free(repl_history[0]);
        memmove(repl_history, repl_history + 1, (REPL_MAX_HISTORY - 1) * sizeof(char*));
        repl_history[REPL_MAX_HISTORY - 1] = strdup(line);
    }
#if REPL_USE_READLINE
    add_history(line);
#endif
}

static void history_free(void) {
    for (int i = 0; i < repl_history_count; i++) {
        free(repl_history[i]);
        repl_history[i] = NULL;
    }
    repl_history_count = 0;
}

/* ===== Line Reading ===== */
static char* read_line(const char* prompt) {
#if REPL_USE_READLINE
    char* line = readline(prompt);
    return line; /* caller frees; NULL on EOF */
#else
    static char buf[REPL_MAX_LINE];
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin))
        return NULL;
    /* Strip trailing newline */
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';
    return strdup(buf);
#endif
}

/* ===== Brace Counting for Multi-line Input ===== */
static int count_unmatched_braces(const char* text) {
    int depth = 0;
    bool in_string = false;
    bool in_comment = false;
    char prev = 0;
    for (const char* p = text; *p; p++) {
        if (in_comment) {
            if (*p == '\n')
                in_comment = false;
            continue;
        }
        if (*p == '/' && *(p + 1) == '/') {
            in_comment = true;
            continue;
        }
        if (*p == '"' && prev != '\\') {
            in_string = !in_string;
            prev = *p;
            continue;
        }
        if (!in_string) {
            if (*p == '{')
                depth++;
            else if (*p == '}')
                depth--;
        }
        prev = *p;
    }
    return depth;
}

/* ===== Meta-command Handling ===== */
static bool handle_meta_command(const char* line) {
    if (strcmp(line, ".help") == 0) {
        printf("%sMeta-commands:%s\n", CLR_BOLD, CLR_RESET);
        printf("  .help       Show this help\n");
        printf("  .clear      Clear the screen\n");
        printf("  .exit       Exit the REPL\n");
        printf("  .load FILE  Load and execute a file\n");
        printf("  .save FILE  Save REPL history to a file\n");
        printf("  .history    Show input history\n");
        printf("  .reset      Reset VM state\n");
        printf("\n");
        printf("%sExpressions and statements are executed immediately.%s\n", CLR_DIM, CLR_RESET);
        printf("Multi-line input is detected by unclosed braces.\n");
        return true;
    }
    if (strcmp(line, ".clear") == 0) {
        printf("\033[2J\033[H"); /* ANSI clear screen */
        return true;
    }
    if (strcmp(line, ".exit") == 0 || strcmp(line, ".quit") == 0) {
        return false; /* signal to exit; handled by caller */
    }
    if (strncmp(line, ".load ", 6) == 0) {
        const char* path = line + 6;
        while (*path == ' ')
            path++;
        if (xs_repl_load_file(path)) {
            printf("%sLoaded:%s %s\n", CLR_GREEN, CLR_RESET, path);
        } else {
            printf("%sError:%s Failed to load %s\n", CLR_RED, CLR_RESET, path);
        }
        return true;
    }
    if (strncmp(line, ".save ", 6) == 0) {
        const char* path = line + 6;
        while (*path == ' ')
            path++;
        if (xs_repl_save_history(path)) {
            printf("%sSaved:%s History written to %s\n", CLR_GREEN, CLR_RESET, path);
        } else {
            printf("%sError:%s Failed to save to %s\n", CLR_RED, CLR_RESET, path);
        }
        return true;
    }
    if (strcmp(line, ".history") == 0) {
        for (int i = 0; i < repl_history_count; i++) {
            printf("  %s%d%s  %s\n", CLR_DIM, i + 1, CLR_RESET, repl_history[i]);
        }
        return true;
    }
    if (strcmp(line, ".reset") == 0) {
        if (repl_vm_initialized) {
            vm_free(&repl_vm);
        }
        vm_init(&repl_vm);
        repl_vm_initialized = true;
        printf("VM state reset.\n");
        return true;
    }
    printf("%sUnknown command:%s %s (type .help for commands)\n", CLR_RED, CLR_RESET, line);
    return true;
}

/* ===== Print an XsValue ===== */
static void print_value(XsValue val) {
    switch (val.type) {
    case VAL_BLADE:
        printf("%s%lld%s", CLR_CYAN, (long long)val.blade, CLR_RESET);
        break;
    case VAL_SPARK:
        printf("%s%g%s", CLR_CYAN, val.spark, CLR_RESET);
        break;
    case VAL_SCROLL:
        printf("%s\"%s\"%s", CLR_GREEN, val.scroll ? val.scroll : "(null)", CLR_RESET);
        break;
    case VAL_FATE:
        printf("%s%s%s", CLR_YELLOW, val.fate ? "truth" : "lies", CLR_RESET);
        break;
    case VAL_ABYSS:
        printf("%sabyss%s", CLR_DIM, CLR_RESET);
        break;
    case VAL_ARSENAL:
        printf("[arsenal]");
        break;
    case VAL_ENTITY:
        printf("{entity}");
        break;
    case VAL_SPELL:
        printf("<spell>");
        break;
    case VAL_NATIVE_FN:
        printf("<native fn>");
        break;
    }
}

/* ===== Execute a Single Input ===== */
bool xs_repl_execute_line(const char* line) {
    if (!repl_vm_initialized) {
        vm_init(&repl_vm);
        repl_vm_initialized = true;
    }

    /* Lex */
    Lexer lexer;
    lexer_init(&lexer, line);
    int token_count = 0;
    Token* tokens = lexer_tokenize_all(&lexer, &token_count);
    if (lexer.had_error) {
        fprintf(stderr, "%sLexer error:%s %s\n", CLR_RED, CLR_RESET, lexer.error_msg);
        free(tokens);
        return false;
    }

    /* Parse: try as expression first, fall back to program */
    Parser parser;
    parser_init(&parser, tokens, token_count);
    AstNode* ast = NULL;
    bool is_expr = false;

    /* Heuristic: if it starts with a keyword that is a statement/decl, parse as program */
    bool starts_with_stmt = false;
    if (token_count > 0) {
        TokenType first = tokens[0].type;
        starts_with_stmt = (first == TOK_MORPH || first == TOK_ETERNAL || first == TOK_FORGE ||
                            first == TOK_ENTITY || first == TOK_ORACLE || first == TOK_CYCLE ||
                            first == TOK_WHILE || first == TOK_UNLEASH || first == TOK_ENGRAVE ||
                            first == TOK_SHIELD || first == TOK_SUMMON || first == TOK_REALM ||
                            first == TOK_QUEST);
    }

    if (!starts_with_stmt) {
        /* Try parsing as expression */
        ast = parser_parse_expression(&parser);
        if (!parser_had_error(&parser) && parser.current >= parser.token_count - 1) {
            is_expr = true;
        } else {
            /* Reset and parse as full program */
            ast_free(ast);
            parser_init(&parser, tokens, token_count);
            ast = parser_parse_program(&parser);
            is_expr = false;
        }
    } else {
        ast = parser_parse_program(&parser);
        is_expr = false;
    }

    if (parser_had_error(&parser)) {
        parser_print_errors(&parser);
        ast_free(ast);
        free(tokens);
        return false;
    }

    /* Compile */
    Chunk chunk;
    chunk_init(&chunk, "<repl>");
    Compiler compiler;
    compiler_init(&compiler, &chunk, SCOPE_SCRIPT);

    bool compiled;
    if (is_expr) {
        compiled = compiler_compile_expression(&compiler, ast);
    } else {
        compiled = compiler_compile(&compiler, ast);
    }

    if (!compiled) {
        compiler_print_errors(&compiler);
        chunk_free(&chunk);
        ast_free(ast);
        free(tokens);
        return false;
    }

    /* Execute */
    int stack_before = repl_vm.stack_top;
    VMResult result = vm_execute(&repl_vm, &chunk);

    if (result == VM_RUNTIME_ERROR) {
        fprintf(stderr, "%sRuntime error:%s %s\n", CLR_RED, CLR_RESET, repl_vm.error_msg);
        repl_vm.had_error = false; /* reset for next input */
        chunk_free(&chunk);
        ast_free(ast);
        free(tokens);
        return false;
    }

    /* If it was an expression and something was pushed on the stack, print it */
    if (is_expr && repl_vm.stack_top > stack_before) {
        XsValue val = vm_pop(&repl_vm);
        printf("=> ");
        print_value(val);
        printf("\n");
    }

    chunk_free(&chunk);
    ast_free(ast);
    free(tokens);
    return true;
}

/* ===== Load and Execute a File ===== */
bool xs_repl_load_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f)
        return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buf = (char*)malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return false;
    }

    size_t nread = fread(buf, 1, (size_t)size, f);
    buf[nread] = '\0';
    fclose(f);

    bool ok = xs_repl_execute_line(buf);
    free(buf);
    return ok;
}

/* ===== Save History ===== */
bool xs_repl_save_history(const char* path) {
    FILE* f = fopen(path, "w");
    if (!f)
        return false;
    for (int i = 0; i < repl_history_count; i++) {
        fprintf(f, "%s\n", repl_history[i]);
    }
    fclose(f);
    return true;
}

/* ===== REPL Main Loop ===== */
int xs_repl_start(void) {
    printf("%s%sX# (Xsharp) REPL%s v1.0.0\n", CLR_BOLD, CLR_CYAN, CLR_RESET);
    printf("Type %s.help%s for commands, %s.exit%s to quit.\n\n", CLR_YELLOW, CLR_RESET, CLR_YELLOW,
           CLR_RESET);

    /* Initialize VM */
    vm_init(&repl_vm);
    repl_vm_initialized = true;

    char input_buf[REPL_MAX_INPUT];
    input_buf[0] = '\0';
    int brace_depth = 0;

    while (true) {
        const char* prompt;
        if (brace_depth > 0) {
            prompt = "... ";
        } else {
            prompt = "xs> ";
        }

        char* line = read_line(prompt);
        if (!line) {
            /* EOF */
            printf("\n");
            break;
        }

        /* Skip empty lines */
        if (line[0] == '\0' && brace_depth == 0) {
            free(line);
            continue;
        }

        /* Handle meta-commands (only at top level, not during multi-line) */
        if (brace_depth == 0 && line[0] == '.') {
            if (strcmp(line, ".exit") == 0 || strcmp(line, ".quit") == 0) {
                free(line);
                break;
            }
            handle_meta_command(line);
            free(line);
            continue;
        }

        /* Append to multi-line buffer */
        if (brace_depth > 0) {
            size_t cur_len = strlen(input_buf);
            size_t line_len = strlen(line);
            if (cur_len + line_len + 2 < REPL_MAX_INPUT) {
                strcat(input_buf, "\n");
                strcat(input_buf, line);
            }
        } else {
            strncpy(input_buf, line, REPL_MAX_INPUT - 1);
            input_buf[REPL_MAX_INPUT - 1] = '\0';
        }

        brace_depth = count_unmatched_braces(input_buf);

        if (brace_depth > 0) {
            /* Need more input */
            free(line);
            continue;
        }

        if (brace_depth < 0) {
            fprintf(stderr, "%sError:%s Unexpected closing brace\n", CLR_RED, CLR_RESET);
            input_buf[0] = '\0';
            brace_depth = 0;
            free(line);
            continue;
        }

        /* Execute the complete input */
        history_add(input_buf);
        xs_repl_execute_line(input_buf);

        input_buf[0] = '\0';
        brace_depth = 0;
        free(line);
    }

    /* Cleanup */
    if (repl_vm_initialized) {
        vm_free(&repl_vm);
        repl_vm_initialized = false;
    }
    history_free();

    printf("Goodbye.\n");
    return 0;
}
