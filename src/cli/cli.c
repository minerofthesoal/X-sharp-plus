/*
 * X# (Xsharp) CLI - Command Line Interface Implementation
 * =========================================================
 */

#include "cli.h"
#include "repl.h"
#include "formatter.h"
#include "linter.h"
#include "shortcuts.h"
#include "project_template.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../compiler/compiler.h"
#include "../codegen/codegen.h"
#include "../vm/vm.h"
#include "../debugger/debugger.h"
#include "../formats/xssc.h"
#include "../formats/xscsc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ===== ANSI Color Codes ===== */
#define CLR_RESET   "\033[0m"
#define CLR_RED     "\033[31m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_BLUE    "\033[34m"
#define CLR_MAGENTA "\033[35m"
#define CLR_CYAN    "\033[36m"
#define CLR_WHITE   "\033[37m"
#define CLR_BOLD    "\033[1m"
#define CLR_DIM     "\033[2m"

/* ===== Colored Output Helpers ===== */
static void print_error(const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "%s%serror:%s ", CLR_BOLD, CLR_RED, CLR_RESET);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static void print_success(const char *fmt, ...) {
    va_list args;
    fprintf(stdout, "%s%ssuccess:%s ", CLR_BOLD, CLR_GREEN, CLR_RESET);
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fprintf(stdout, "\n");
}

static void print_info(const char *fmt, ...) {
    va_list args;
    fprintf(stdout, "%s%sinfo:%s ", CLR_BOLD, CLR_CYAN, CLR_RESET);
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fprintf(stdout, "\n");
}

static void print_warning(const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "%s%swarning:%s ", CLR_BOLD, CLR_YELLOW, CLR_RESET);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

/* ===== Version ===== */
#define XS_VERSION_MAJOR  1
#define XS_VERSION_MINOR  0
#define XS_VERSION_PATCH  0

static void print_version(void) {
    printf("%sX# (Xsharp)%s v%d.%d.%d\n",
           CLR_BOLD, CLR_RESET,
           XS_VERSION_MAJOR, XS_VERSION_MINOR, XS_VERSION_PATCH);
}

/* ===== Usage / Help ===== */
static void print_usage(void) {
    print_version();
    printf("\n");
    printf("%sUSAGE:%s\n", CLR_BOLD, CLR_RESET);
    printf("    xs <command> [options] [arguments]\n\n");

    printf("%sCOMMANDS:%s\n", CLR_BOLD, CLR_RESET);
    printf("    %srun%s <file>          Compile and execute a .xs file\n", CLR_GREEN, CLR_RESET);
    printf("    %sbuild%s <file>        Compile to bytecode (.xsb)\n", CLR_GREEN, CLR_RESET);
    printf("    %sdebug%s <file>        Launch interactive debugger\n", CLR_GREEN, CLR_RESET);
    printf("    %srepl%s                Start interactive REPL\n", CLR_GREEN, CLR_RESET);
    printf("    %sfmt%s <file>          Format source code\n", CLR_GREEN, CLR_RESET);
    printf("    %slint%s <file>         Lint source code\n", CLR_GREEN, CLR_RESET);
    printf("    %stest%s <dir>          Run test files\n", CLR_GREEN, CLR_RESET);
    printf("    %snew%s <name> [tmpl]   Create new project from template\n", CLR_GREEN, CLR_RESET);
    printf("    %sinit%s                Initialize project in current dir\n", CLR_GREEN, CLR_RESET);
    printf("    %spack%s <dir> <out>    Pack directory into .Xssc archive\n", CLR_GREEN, CLR_RESET);
    printf("    %sunpack%s <file> <dir> Unpack .Xssc archive\n", CLR_GREEN, CLR_RESET);
    printf("    %scompress%s <file>     Compress .Xssc to .Xscsc\n", CLR_GREEN, CLR_RESET);
    printf("    %sdecompress%s <file>   Decompress .Xscsc to .Xssc\n", CLR_GREEN, CLR_RESET);
    printf("\n");

    printf("%sSHORTCUTS:%s\n", CLR_BOLD, CLR_RESET);
    printf("    %s@run%s    Find and run main.xs\n", CLR_YELLOW, CLR_RESET);
    printf("    %s@build%s  Compile all .xs files\n", CLR_YELLOW, CLR_RESET);
    printf("    %s@clean%s  Remove build/ directory\n", CLR_YELLOW, CLR_RESET);
    printf("    %s@test%s   Run all tests in tests/\n", CLR_YELLOW, CLR_RESET);
    printf("    %s@watch%s  Watch files and rebuild on change\n", CLR_YELLOW, CLR_RESET);
    printf("    %s@check%s  Lint all .xs files\n", CLR_YELLOW, CLR_RESET);
    printf("    %s@info%s   Print project info\n", CLR_YELLOW, CLR_RESET);
    printf("\n");

    printf("%sOPTIONS:%s\n", CLR_BOLD, CLR_RESET);
    printf("    -h, --help       Show this help message\n");
    printf("    -v, --version    Show version\n");
    printf("    -V, --verbose    Verbose output\n");
    printf("\n");
}

/* ===== File Reading Helper ===== */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        print_error("Cannot open file: %s", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) {
        fclose(f);
        print_error("Cannot determine file size: %s", path);
        return NULL;
    }
    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        print_error("Out of memory reading file: %s", path);
        return NULL;
    }
    size_t read_count = fread(buf, 1, (size_t)size, f);
    buf[read_count] = '\0';
    fclose(f);
    return buf;
}

/* ===== File Writing Helper ===== */
static bool write_file(const char *path, const uint8_t *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        print_error("Cannot open file for writing: %s", path);
        return false;
    }
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    if (written != size) {
        print_error("Failed to write all data to: %s", path);
        return false;
    }
    return true;
}

/* ===== Command: run ===== */
static int cmd_run(int argc, char **argv) {
    if (argc < 1) {
        print_error("Usage: xs run <file.xs>");
        return 1;
    }
    const char *path = argv[0];

    /* Read the source file */
    char *source = read_file(path);
    if (!source) return 1;

    /* Lex */
    Lexer lexer;
    lexer_init(&lexer, source);
    int token_count = 0;
    Token *tokens = lexer_tokenize_all(&lexer, &token_count);
    if (lexer.had_error) {
        print_error("Lexer error: %s", lexer.error_msg);
        free(tokens);
        free(source);
        return 1;
    }

    /* Parse */
    Parser parser;
    parser_init(&parser, tokens, token_count);
    AstNode *program = parser_parse_program(&parser);
    if (parser_had_error(&parser)) {
        print_error("Parse errors:");
        parser_print_errors(&parser);
        ast_free(program);
        free(tokens);
        free(source);
        return 1;
    }

    /* Compile */
    Chunk chunk;
    chunk_init(&chunk, path);
    Compiler compiler;
    compiler_init(&compiler, &chunk, SCOPE_SCRIPT);
    if (!compiler_compile(&compiler, program)) {
        print_error("Compilation errors:");
        compiler_print_errors(&compiler);
        chunk_free(&chunk);
        ast_free(program);
        free(tokens);
        free(source);
        return 1;
    }

    /* Execute */
    VMState vm;
    vm_init(&vm);
    VMResult result = vm_execute(&vm, &chunk);
    int exit_code = 0;
    if (result == VM_RUNTIME_ERROR) {
        print_error("Runtime error: %s", vm.error_msg);
        exit_code = 1;
    }

    /* Cleanup */
    vm_free(&vm);
    chunk_free(&chunk);
    ast_free(program);
    free(tokens);
    free(source);
    return exit_code;
}

/* ===== Command: build ===== */
static int cmd_build(int argc, char **argv) {
    if (argc < 1) {
        print_error("Usage: xs build <file.xs> [output.xsb]");
        return 1;
    }
    const char *path = argv[0];

    /* Determine output path */
    char out_path[512];
    if (argc >= 2) {
        snprintf(out_path, sizeof(out_path), "%s", argv[1]);
    } else {
        /* Replace .xs with .xsb */
        snprintf(out_path, sizeof(out_path), "%s", path);
        size_t len = strlen(out_path);
        if (len > 3 && strcmp(out_path + len - 3, ".xs") == 0) {
            out_path[len - 2] = 'x';
            out_path[len - 1] = 's';
            out_path[len]     = 'b';
            out_path[len + 1] = '\0';
        } else {
            strcat(out_path, ".xsb");
        }
    }

    char *source = read_file(path);
    if (!source) return 1;

    /* Lex */
    Lexer lexer;
    lexer_init(&lexer, source);
    int token_count = 0;
    Token *tokens = lexer_tokenize_all(&lexer, &token_count);
    if (lexer.had_error) {
        print_error("Lexer error: %s", lexer.error_msg);
        free(tokens);
        free(source);
        return 1;
    }

    /* Parse */
    Parser parser;
    parser_init(&parser, tokens, token_count);
    AstNode *program = parser_parse_program(&parser);
    if (parser_had_error(&parser)) {
        print_error("Parse errors:");
        parser_print_errors(&parser);
        ast_free(program);
        free(tokens);
        free(source);
        return 1;
    }

    /* Compile */
    Chunk chunk;
    chunk_init(&chunk, path);
    Compiler compiler;
    compiler_init(&compiler, &chunk, SCOPE_SCRIPT);
    if (!compiler_compile(&compiler, program)) {
        print_error("Compilation errors:");
        compiler_print_errors(&compiler);
        chunk_free(&chunk);
        ast_free(program);
        free(tokens);
        free(source);
        return 1;
    }

    /* Write bytecode to file */
    if (!write_file(out_path, chunk.code, (size_t)chunk.code_count)) {
        chunk_free(&chunk);
        ast_free(program);
        free(tokens);
        free(source);
        return 1;
    }

    print_success("Compiled %s -> %s (%d bytes)", path, out_path, chunk.code_count);

    chunk_free(&chunk);
    ast_free(program);
    free(tokens);
    free(source);
    return 0;
}

/* ===== Command: debug ===== */
static int cmd_debug(int argc, char **argv) {
    if (argc < 1) {
        print_error("Usage: xs debug <file.xs>");
        return 1;
    }
    const char *path = argv[0];

    char *source = read_file(path);
    if (!source) return 1;

    /* Lex */
    Lexer lexer;
    lexer_init(&lexer, source);
    int token_count = 0;
    Token *tokens = lexer_tokenize_all(&lexer, &token_count);
    if (lexer.had_error) {
        print_error("Lexer error: %s", lexer.error_msg);
        free(tokens);
        free(source);
        return 1;
    }

    /* Parse */
    Parser parser;
    parser_init(&parser, tokens, token_count);
    AstNode *program = parser_parse_program(&parser);
    if (parser_had_error(&parser)) {
        print_error("Parse errors:");
        parser_print_errors(&parser);
        ast_free(program);
        free(tokens);
        free(source);
        return 1;
    }

    /* Compile */
    Chunk chunk;
    chunk_init(&chunk, path);
    Compiler compiler;
    compiler_init(&compiler, &chunk, SCOPE_SCRIPT);
    if (!compiler_compile(&compiler, program)) {
        print_error("Compilation errors:");
        compiler_print_errors(&compiler);
        chunk_free(&chunk);
        ast_free(program);
        free(tokens);
        free(source);
        return 1;
    }

    /* Create debug info */
    XsDebugInfo *dbg_info = xs_debug_info_new("main", path);
    xs_debug_info_set_source(dbg_info, source);

    /* Build source map from chunk line info */
    for (int i = 0; i < chunk.code_count; i++) {
        int line = chunk_get_line(&chunk, i);
        xs_source_map_add(&dbg_info->source_map, (uint32_t)i, path, line, 0);
    }

    /* Create and launch debugger */
    XsDebugger *dbg = xs_debugger_new();
    dbg->stop_on_entry = true;
    xs_debugger_load(dbg, chunk.code, (size_t)chunk.code_count, dbg_info);
    xs_debugger_launch(dbg);

    /* Interactive command loop */
    printf("%sX# Debugger%s - type 'help' for commands\n", CLR_BOLD, CLR_RESET);
    printf("Stopped at entry point: %s:%d\n", path, 1);

    char cmd_buf[1024];
    while (dbg->state != DBG_STATE_STOPPED && dbg->state != DBG_STATE_ERROR) {
        printf("%sdbg>%s ", CLR_YELLOW, CLR_RESET);
        fflush(stdout);
        if (!fgets(cmd_buf, sizeof(cmd_buf), stdin)) break;

        /* Remove trailing newline */
        size_t cmd_len = strlen(cmd_buf);
        if (cmd_len > 0 && cmd_buf[cmd_len - 1] == '\n') cmd_buf[cmd_len - 1] = '\0';
        if (cmd_buf[0] == '\0') continue;

        /* Parse debug command */
        if (strcmp(cmd_buf, "help") == 0 || strcmp(cmd_buf, "h") == 0) {
            printf("  continue (c)    - Continue execution\n");
            printf("  step (s)        - Step into\n");
            printf("  next (n)        - Step over\n");
            printf("  finish (f)      - Step out\n");
            printf("  break (b) N     - Set breakpoint at line N\n");
            printf("  delete (d) N    - Remove breakpoint N\n");
            printf("  list (l)        - Show source context\n");
            printf("  locals          - Show local variables\n");
            printf("  globals         - Show global variables\n");
            printf("  stack           - Show call stack\n");
            printf("  watch (w) EXPR  - Add watch expression\n");
            printf("  print (p) EXPR  - Evaluate expression\n");
            printf("  quit (q)        - Stop debugging\n");
        } else if (strcmp(cmd_buf, "c") == 0 || strcmp(cmd_buf, "continue") == 0) {
            xs_debugger_continue(dbg);
            if (dbg->state == DBG_STATE_PAUSED) {
                printf("Breakpoint hit at %s:%d\n", dbg->current_file, dbg->current_line);
            }
        } else if (strcmp(cmd_buf, "s") == 0 || strcmp(cmd_buf, "step") == 0) {
            xs_debugger_step_in(dbg);
            printf("At %s:%d\n", dbg->current_file, dbg->current_line);
        } else if (strcmp(cmd_buf, "n") == 0 || strcmp(cmd_buf, "next") == 0) {
            xs_debugger_step_over(dbg);
            printf("At %s:%d\n", dbg->current_file, dbg->current_line);
        } else if (strcmp(cmd_buf, "f") == 0 || strcmp(cmd_buf, "finish") == 0) {
            xs_debugger_step_out(dbg);
            printf("At %s:%d\n", dbg->current_file, dbg->current_line);
        } else if (strncmp(cmd_buf, "b ", 2) == 0 || strncmp(cmd_buf, "break ", 6) == 0) {
            const char *arg = cmd_buf + (cmd_buf[0] == 'b' && cmd_buf[1] == ' ' ? 2 : 6);
            int line = atoi(arg);
            if (line > 0) {
                int bp_id = xs_debugger_set_breakpoint(dbg, path, line);
                printf("Breakpoint %d set at %s:%d\n", bp_id, path, line);
            } else {
                printf("Invalid line number\n");
            }
        } else if (strncmp(cmd_buf, "d ", 2) == 0 || strncmp(cmd_buf, "delete ", 7) == 0) {
            const char *arg = cmd_buf + (cmd_buf[0] == 'd' && cmd_buf[1] == ' ' ? 2 : 7);
            int bp_id = atoi(arg);
            if (xs_debugger_remove_breakpoint(dbg, bp_id)) {
                printf("Breakpoint %d removed\n", bp_id);
            } else {
                printf("Breakpoint %d not found\n", bp_id);
            }
        } else if (strcmp(cmd_buf, "l") == 0 || strcmp(cmd_buf, "list") == 0) {
            xs_debugger_show_source_context(dbg, dbg->current_line, 5);
        } else if (strcmp(cmd_buf, "locals") == 0) {
            XsDbgVariable vars[64];
            int count = xs_debugger_get_locals(dbg, 0, vars, 64);
            for (int i = 0; i < count; i++) {
                char val_buf[256];
                xs_debugger_value_to_string(vars[i].value, val_buf, sizeof(val_buf));
                printf("  %s%s%s : %s = %s\n", CLR_CYAN, vars[i].name, CLR_RESET,
                       vars[i].type_name, val_buf);
            }
            if (count == 0) printf("  (no locals)\n");
        } else if (strcmp(cmd_buf, "globals") == 0) {
            XsDbgVariable vars[64];
            int count = xs_debugger_get_globals(dbg, vars, 64);
            for (int i = 0; i < count; i++) {
                char val_buf[256];
                xs_debugger_value_to_string(vars[i].value, val_buf, sizeof(val_buf));
                printf("  %s%s%s : %s = %s\n", CLR_CYAN, vars[i].name, CLR_RESET,
                       vars[i].type_name, val_buf);
            }
            if (count == 0) printf("  (no globals)\n");
        } else if (strcmp(cmd_buf, "stack") == 0) {
            XsDbgCallFrame frames[64];
            int count = xs_debugger_get_call_stack(dbg, frames, 64);
            for (int i = 0; i < count; i++) {
                printf("  #%d %s%s%s at %s:%d\n", frames[i].id,
                       CLR_CYAN, frames[i].func_name, CLR_RESET,
                       frames[i].source_file, frames[i].line);
            }
            if (count == 0) printf("  (empty stack)\n");
        } else if (strncmp(cmd_buf, "w ", 2) == 0 || strncmp(cmd_buf, "watch ", 6) == 0) {
            const char *expr = cmd_buf + (cmd_buf[0] == 'w' && cmd_buf[1] == ' ' ? 2 : 6);
            int wid = xs_debugger_add_watch(dbg, expr);
            printf("Watch %d: %s\n", wid, expr);
        } else if (strncmp(cmd_buf, "p ", 2) == 0 || strncmp(cmd_buf, "print ", 6) == 0) {
            const char *expr = cmd_buf + (cmd_buf[0] == 'p' && cmd_buf[1] == ' ' ? 2 : 6);
            bool success = false;
            XsValue val = xs_debugger_evaluate(dbg, expr, 0, &success);
            if (success) {
                char val_buf[256];
                xs_debugger_value_to_string(val, val_buf, sizeof(val_buf));
                printf("  = %s\n", val_buf);
            } else {
                printf("  (evaluation failed)\n");
            }
        } else if (strcmp(cmd_buf, "q") == 0 || strcmp(cmd_buf, "quit") == 0) {
            xs_debugger_stop(dbg);
            break;
        } else {
            printf("Unknown command: %s (type 'help' for commands)\n", cmd_buf);
        }
    }

    if (dbg->state == DBG_STATE_STOPPED) {
        printf("Program exited.\n");
    }

    xs_debugger_free(dbg);
    xs_debug_info_free(dbg_info);
    chunk_free(&chunk);
    ast_free(program);
    free(tokens);
    free(source);
    return 0;
}

/* ===== Command: repl ===== */
static int cmd_repl(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return xs_repl_start();
}

/* ===== Command: fmt ===== */
static int cmd_fmt(int argc, char **argv) {
    if (argc < 1) {
        print_error("Usage: xs fmt <file.xs> [file2.xs ...]");
        return 1;
    }
    XsFormatConfig config = xs_format_config_default();
    int failures = 0;
    for (int i = 0; i < argc; i++) {
        if (xs_format_file(argv[i], &config)) {
            print_success("Formatted %s", argv[i]);
        } else {
            print_error("Failed to format %s", argv[i]);
            failures++;
        }
    }
    return failures > 0 ? 1 : 0;
}

/* ===== Command: lint ===== */
static int cmd_lint(int argc, char **argv) {
    if (argc < 1) {
        print_error("Usage: xs lint <file.xs> [file2.xs ...]");
        return 1;
    }
    int total_errors = 0;
    int total_warnings = 0;
    for (int i = 0; i < argc; i++) {
        XsLintResult *result = xs_lint_file(argv[i]);
        if (result) {
            xs_lint_result_print(result);
            total_errors += result->error_count;
            total_warnings += result->warning_count;
            xs_lint_result_free(result);
        } else {
            print_error("Failed to lint %s", argv[i]);
            total_errors++;
        }
    }
    if (total_errors == 0 && total_warnings == 0) {
        print_success("No issues found.");
    } else {
        printf("\n%d error(s), %d warning(s)\n", total_errors, total_warnings);
    }
    return total_errors > 0 ? 1 : 0;
}

/* ===== Command: test ===== */
static int cmd_test(int argc, char **argv) {
    /* If a directory given, use it; otherwise default to "tests/" */
    const char *test_dir = (argc >= 1) ? argv[0] : "tests";
    (void)test_dir;

    /* Delegate to the @test shortcut which has the full implementation */
    return xs_shortcut_test();
}

/* ===== Command: new ===== */
static int cmd_new(int argc, char **argv) {
    if (argc < 1) {
        print_error("Usage: xs new <project-name> [template]");
        printf("Available templates: console, game, ai, library\n");
        return 1;
    }
    const char *name = argv[0];
    const char *tmpl = (argc >= 2) ? argv[1] : "console";
    if (xs_create_project(name, tmpl)) {
        print_success("Created project '%s' with template '%s'", name, tmpl);
        return 0;
    } else {
        print_error("Failed to create project '%s'", name);
        return 1;
    }
}

/* ===== Command: init ===== */
static int cmd_init(int argc, char **argv) {
    (void)argc;
    (void)argv;
    /* Create a .xsproj and main.xs in the current directory */
    if (xs_create_project(".", "console")) {
        print_success("Initialized X# project in current directory");
        return 0;
    }
    print_error("Failed to initialize project");
    return 1;
}

/* ===== Command: pack ===== */
static int cmd_pack(int argc, char **argv) {
    if (argc < 2) {
        print_error("Usage: xs pack <directory> <output.Xssc>");
        return 1;
    }
    const char *dir = argv[0];
    const char *out = argv[1];

    XsscArchive *archive = xssc_create();
    if (!archive) {
        print_error("Failed to create archive");
        return 1;
    }

    xssc_set_metadata(archive, "creator", "xs-cli");
    xssc_set_metadata(archive, "version", "1.0.0");

    /* Walk directory and add .xs files */
    /* Simple approach: use opendir/readdir */
    #include <dirent.h>
    DIR *d = opendir(dir);
    if (!d) {
        print_error("Cannot open directory: %s", dir);
        xssc_free(archive);
        return 1;
    }
    struct dirent *ent;
    int file_count = 0;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        size_t nlen = strlen(ent->d_name);
        if (nlen > 3 && strcmp(ent->d_name + nlen - 3, ".xs") == 0) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir, ent->d_name);
            int idx = xssc_add_file(archive, full_path, ent->d_name, XSSC_ENTRY_SOURCE);
            if (idx >= 0) file_count++;
        }
    }
    closedir(d);

    if (file_count == 0) {
        print_warning("No .xs files found in %s", dir);
    }

    if (xssc_write(archive, out)) {
        print_success("Packed %d file(s) into %s", file_count, out);
    } else {
        print_error("Failed to write archive: %s", out);
        xssc_free(archive);
        return 1;
    }

    xssc_free(archive);
    return 0;
}

/* ===== Command: unpack ===== */
static int cmd_unpack(int argc, char **argv) {
    if (argc < 2) {
        print_error("Usage: xs unpack <archive.Xssc> <output-dir>");
        return 1;
    }
    const char *archive_path = argv[0];
    const char *out_dir = argv[1];

    XsscArchive *archive = xssc_read(archive_path);
    if (!archive) {
        print_error("Failed to read archive: %s", archive_path);
        return 1;
    }

    int count = xssc_extract_all(archive, out_dir);
    if (count < 0) {
        print_error("Failed to extract archive");
        xssc_free(archive);
        return 1;
    }

    print_success("Extracted %d file(s) to %s", count, out_dir);
    xssc_free(archive);
    return 0;
}

/* ===== Command: compress ===== */
static int cmd_compress(int argc, char **argv) {
    if (argc < 1) {
        print_error("Usage: xs compress <archive.Xssc> [output.Xscsc]");
        return 1;
    }
    const char *in_path = argv[0];

    char out_path[512];
    if (argc >= 2) {
        snprintf(out_path, sizeof(out_path), "%s", argv[1]);
    } else {
        /* Replace .Xssc with .Xscsc */
        snprintf(out_path, sizeof(out_path), "%s", in_path);
        size_t len = strlen(out_path);
        if (len > 5 && strcmp(out_path + len - 5, ".Xssc") == 0) {
            out_path[len - 4] = 'X';
            out_path[len - 3] = 's';
            out_path[len - 2] = 'c';
            out_path[len - 1] = 's';
            out_path[len]     = 'c';
            out_path[len + 1] = '\0';
        } else {
            strcat(out_path, ".Xscsc");
        }
    }

    XsscArchive *xssc = xssc_read(in_path);
    if (!xssc) {
        print_error("Failed to read archive: %s", in_path);
        return 1;
    }

    XscscArchive *xscsc = xscsc_compress(xssc, LZMA2_DEFAULT_LEVEL);
    if (!xscsc) {
        print_error("Compression failed");
        xssc_free(xssc);
        return 1;
    }

    if (!xscsc_write(xscsc, out_path)) {
        print_error("Failed to write compressed archive: %s", out_path);
        xscsc_free(xscsc);
        xssc_free(xssc);
        return 1;
    }

    print_success("Compressed %s -> %s (%.1f%% ratio)",
                  in_path, out_path,
                  xscsc->uncompressed_size > 0
                    ? (100.0 * (double)xscsc->compressed_size / (double)xscsc->uncompressed_size)
                    : 0.0);

    xscsc_free(xscsc);
    xssc_free(xssc);
    return 0;
}

/* ===== Command: decompress ===== */
static int cmd_decompress(int argc, char **argv) {
    if (argc < 1) {
        print_error("Usage: xs decompress <archive.Xscsc> [output.Xssc]");
        return 1;
    }
    const char *in_path = argv[0];

    char out_path[512];
    if (argc >= 2) {
        snprintf(out_path, sizeof(out_path), "%s", argv[1]);
    } else {
        /* Replace .Xscsc with .Xssc */
        snprintf(out_path, sizeof(out_path), "%s", in_path);
        size_t len = strlen(out_path);
        if (len > 6 && strcmp(out_path + len - 6, ".Xscsc") == 0) {
            out_path[len - 6] = '\0';
            strcat(out_path, ".Xssc");
        } else {
            strcat(out_path, ".Xssc");
        }
    }

    XscscArchive *xscsc = xscsc_read(in_path);
    if (!xscsc) {
        print_error("Failed to read compressed archive: %s", in_path);
        return 1;
    }

    XsscArchive *xssc = xscsc_decompress(xscsc);
    if (!xssc) {
        print_error("Decompression failed");
        xscsc_free(xscsc);
        return 1;
    }

    if (!xssc_write(xssc, out_path)) {
        print_error("Failed to write decompressed archive: %s", out_path);
        xssc_free(xssc);
        xscsc_free(xscsc);
        return 1;
    }

    print_success("Decompressed %s -> %s", in_path, out_path);

    xssc_free(xssc);
    xscsc_free(xscsc);
    return 0;
}

/* ===== Main Dispatch ===== */
int xs_cli_dispatch(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 0;
    }

    const char *cmd = argv[1];

    /* Check for flags first */
    if (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
        print_usage();
        return 0;
    }
    if (strcmp(cmd, "-v") == 0 || strcmp(cmd, "--version") == 0) {
        print_version();
        return 0;
    }

    /* Check for shortcuts (@run, @build, etc.) */
    if (cmd[0] == '@') {
        return xs_handle_shortcut(cmd + 1);
    }

    /* Initialize runtime */
    xs_runtime_init();

    int result = 0;
    int sub_argc = argc - 2;
    char **sub_argv = argv + 2;

    if (strcmp(cmd, "run") == 0) {
        result = cmd_run(sub_argc, sub_argv);
    } else if (strcmp(cmd, "build") == 0) {
        result = cmd_build(sub_argc, sub_argv);
    } else if (strcmp(cmd, "debug") == 0) {
        result = cmd_debug(sub_argc, sub_argv);
    } else if (strcmp(cmd, "repl") == 0) {
        result = cmd_repl(sub_argc, sub_argv);
    } else if (strcmp(cmd, "fmt") == 0) {
        result = cmd_fmt(sub_argc, sub_argv);
    } else if (strcmp(cmd, "lint") == 0) {
        result = cmd_lint(sub_argc, sub_argv);
    } else if (strcmp(cmd, "test") == 0) {
        result = cmd_test(sub_argc, sub_argv);
    } else if (strcmp(cmd, "new") == 0) {
        result = cmd_new(sub_argc, sub_argv);
    } else if (strcmp(cmd, "init") == 0) {
        result = cmd_init(sub_argc, sub_argv);
    } else if (strcmp(cmd, "pack") == 0) {
        result = cmd_pack(sub_argc, sub_argv);
    } else if (strcmp(cmd, "unpack") == 0) {
        result = cmd_unpack(sub_argc, sub_argv);
    } else if (strcmp(cmd, "compress") == 0) {
        result = cmd_compress(sub_argc, sub_argv);
    } else if (strcmp(cmd, "decompress") == 0) {
        result = cmd_decompress(sub_argc, sub_argv);
    } else {
        print_error("Unknown command: '%s'", cmd);
        printf("Run 'xs --help' for usage information.\n");
        result = 1;
    }

    xs_runtime_shutdown();
    return result;
}
