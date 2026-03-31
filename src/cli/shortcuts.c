/*
 * X# (Xsharp) Shortcuts Implementation
 * =======================================
 */

#include "shortcuts.h"
#include "linter.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../compiler/compiler.h"
#include "../codegen/codegen.h"
#include "../vm/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

/* ===== ANSI Colors ===== */
#define CLR_RESET   "\033[0m"
#define CLR_RED     "\033[31m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_CYAN    "\033[36m"
#define CLR_BOLD    "\033[1m"
#define CLR_DIM     "\033[2m"

/* ===== File Helpers ===== */
static char *read_file_contents(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return NULL; }
    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t nread = fread(buf, 1, (size_t)size, f);
    buf[nread] = '\0';
    fclose(f);
    return buf;
}

static bool file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static bool is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

static time_t file_mtime(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return st.st_mtime;
}

/* Recursively remove a directory */
static int remove_dir_recursive(const char *path) {
    DIR *d = opendir(path);
    if (!d) return -1;

    struct dirent *ent;
    int result = 0;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", path, ent->d_name);

        struct stat st;
        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
            result = remove_dir_recursive(full);
        } else {
            result = remove(full);
        }
        if (result != 0) break;
    }
    closedir(d);
    if (result == 0) result = rmdir(path);
    return result;
}

/* Collect .xs files from a directory into a list */
typedef struct {
    char **paths;
    int    count;
    int    capacity;
} FileList;

static void filelist_init(FileList *fl) {
    fl->capacity = 32;
    fl->count = 0;
    fl->paths = (char **)malloc((size_t)fl->capacity * sizeof(char *));
}

static void filelist_add(FileList *fl, const char *path) {
    if (fl->count >= fl->capacity) {
        fl->capacity *= 2;
        fl->paths = (char **)realloc(fl->paths, (size_t)fl->capacity * sizeof(char *));
    }
    fl->paths[fl->count++] = strdup(path);
}

static void filelist_free(FileList *fl) {
    for (int i = 0; i < fl->count; i++) free(fl->paths[i]);
    free(fl->paths);
    fl->paths = NULL;
    fl->count = 0;
}

static void collect_xs_files(const char *dir, FileList *fl) {
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
            collect_xs_files(full, fl);
        } else {
            size_t nlen = strlen(ent->d_name);
            if (nlen > 3 && strcmp(ent->d_name + nlen - 3, ".xs") == 0) {
                filelist_add(fl, full);
            }
        }
    }
    closedir(d);
}

/* Compile and run a single file. Returns 0 on success. */
static int run_xs_file(const char *path) {
    char *source = read_file_contents(path);
    if (!source) {
        fprintf(stderr, "%serror:%s Cannot read %s\n", CLR_RED, CLR_RESET, path);
        return 1;
    }

    Lexer lexer;
    lexer_init(&lexer, source);
    int token_count = 0;
    Token *tokens = lexer_tokenize_all(&lexer, &token_count);
    if (lexer.had_error) {
        fprintf(stderr, "%serror:%s Lexer error in %s: %s\n",
                CLR_RED, CLR_RESET, path, lexer.error_msg);
        free(tokens);
        free(source);
        return 1;
    }

    Parser parser;
    parser_init(&parser, tokens, token_count);
    AstNode *program = parser_parse_program(&parser);
    if (parser_had_error(&parser)) {
        fprintf(stderr, "%serror:%s Parse error in %s\n", CLR_RED, CLR_RESET, path);
        parser_print_errors(&parser);
        ast_free(program);
        free(tokens);
        free(source);
        return 1;
    }

    Chunk chunk;
    chunk_init(&chunk, path);
    Compiler compiler;
    compiler_init(&compiler, &chunk, SCOPE_SCRIPT);
    if (!compiler_compile(&compiler, program)) {
        fprintf(stderr, "%serror:%s Compilation error in %s\n", CLR_RED, CLR_RESET, path);
        compiler_print_errors(&compiler);
        chunk_free(&chunk);
        ast_free(program);
        free(tokens);
        free(source);
        return 1;
    }

    VMState vm;
    vm_init(&vm);
    VMResult result = vm_execute(&vm, &chunk);
    int exit_code = (result == VM_OK) ? 0 : 1;
    if (result == VM_RUNTIME_ERROR) {
        fprintf(stderr, "%serror:%s Runtime error in %s: %s\n",
                CLR_RED, CLR_RESET, path, vm.error_msg);
    }

    vm_free(&vm);
    chunk_free(&chunk);
    ast_free(program);
    free(tokens);
    free(source);
    return exit_code;
}

/* Compile a file to bytecode. Returns 0 on success. */
static int build_xs_file(const char *path, const char *out_dir) {
    char *source = read_file_contents(path);
    if (!source) return 1;

    Lexer lexer;
    lexer_init(&lexer, source);
    int token_count = 0;
    Token *tokens = lexer_tokenize_all(&lexer, &token_count);
    if (lexer.had_error) {
        free(tokens);
        free(source);
        return 1;
    }

    Parser parser;
    parser_init(&parser, tokens, token_count);
    AstNode *program = parser_parse_program(&parser);
    if (parser_had_error(&parser)) {
        ast_free(program);
        free(tokens);
        free(source);
        return 1;
    }

    Chunk chunk;
    chunk_init(&chunk, path);
    Compiler compiler;
    compiler_init(&compiler, &chunk, SCOPE_SCRIPT);
    if (!compiler_compile(&compiler, program)) {
        chunk_free(&chunk);
        ast_free(program);
        free(tokens);
        free(source);
        return 1;
    }

    /* Determine output path */
    const char *basename = strrchr(path, '/');
    basename = basename ? basename + 1 : path;

    char out_path[1024];
    snprintf(out_path, sizeof(out_path), "%s/%s", out_dir, basename);
    size_t olen = strlen(out_path);
    if (olen > 3 && strcmp(out_path + olen - 3, ".xs") == 0) {
        out_path[olen - 2] = 'x';
        out_path[olen - 1] = 's';
        out_path[olen]     = 'b';
        out_path[olen + 1] = '\0';
    }

    FILE *f = fopen(out_path, "wb");
    if (f) {
        fwrite(chunk.code, 1, (size_t)chunk.code_count, f);
        fclose(f);
    }

    chunk_free(&chunk);
    ast_free(program);
    free(tokens);
    free(source);
    return f ? 0 : 1;
}

/* ===== @run: Find main.xs and run it ===== */
int xs_shortcut_run(void) {
    /* Search order: main.xs, src/main.xs */
    const char *candidates[] = { "main.xs", "src/main.xs", NULL };
    const char *found = NULL;

    for (int i = 0; candidates[i]; i++) {
        if (file_exists(candidates[i])) {
            found = candidates[i];
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "%serror:%s No main.xs found in project\n", CLR_RED, CLR_RESET);
        return 1;
    }

    printf("%s@run%s %s\n", CLR_YELLOW, CLR_RESET, found);
    return run_xs_file(found);
}

/* ===== @build: Compile all .xs files ===== */
int xs_shortcut_build(void) {
    printf("%s@build%s Compiling all .xs files...\n", CLR_YELLOW, CLR_RESET);

    /* Create build/ directory if it doesn't exist */
    mkdir("build", 0755);

    FileList files;
    filelist_init(&files);
    collect_xs_files(".", &files);

    if (files.count == 0) {
        /* Try src/ */
        collect_xs_files("src", &files);
    }

    if (files.count == 0) {
        fprintf(stderr, "%serror:%s No .xs files found\n", CLR_RED, CLR_RESET);
        filelist_free(&files);
        return 1;
    }

    int failures = 0;
    for (int i = 0; i < files.count; i++) {
        printf("  Compiling %s...\n", files.paths[i]);
        if (build_xs_file(files.paths[i], "build") != 0) {
            fprintf(stderr, "  %sFailed:%s %s\n", CLR_RED, CLR_RESET, files.paths[i]);
            failures++;
        }
    }

    if (failures == 0) {
        printf("%ssuccess:%s Built %d file(s)\n", CLR_GREEN, CLR_RESET, files.count);
    } else {
        fprintf(stderr, "%serror:%s %d of %d file(s) failed\n",
                CLR_RED, CLR_RESET, failures, files.count);
    }

    filelist_free(&files);
    return failures > 0 ? 1 : 0;
}

/* ===== @clean: Remove build/ directory ===== */
int xs_shortcut_clean(void) {
    printf("%s@clean%s Removing build/ directory...\n", CLR_YELLOW, CLR_RESET);

    if (!is_directory("build")) {
        printf("  No build/ directory found.\n");
        return 0;
    }

    if (remove_dir_recursive("build") == 0) {
        printf("%ssuccess:%s build/ removed\n", CLR_GREEN, CLR_RESET);
        return 0;
    } else {
        fprintf(stderr, "%serror:%s Failed to remove build/\n", CLR_RED, CLR_RESET);
        return 1;
    }
}

/* ===== @test: Find tests/*.xs and run each ===== */
int xs_shortcut_test(void) {
    printf("%s@test%s Running tests...\n", CLR_YELLOW, CLR_RESET);

    /* Look for tests/ or test/ directory */
    const char *test_dirs[] = { "tests", "test", NULL };
    const char *test_dir = NULL;
    for (int i = 0; test_dirs[i]; i++) {
        if (is_directory(test_dirs[i])) {
            test_dir = test_dirs[i];
            break;
        }
    }

    if (!test_dir) {
        fprintf(stderr, "%serror:%s No tests/ or test/ directory found\n", CLR_RED, CLR_RESET);
        return 1;
    }

    FileList files;
    filelist_init(&files);
    collect_xs_files(test_dir, &files);

    if (files.count == 0) {
        printf("  No test files found in %s/\n", test_dir);
        filelist_free(&files);
        return 0;
    }

    int passed = 0, failed = 0;
    for (int i = 0; i < files.count; i++) {
        printf("  %sRUN%s  %s ", CLR_CYAN, CLR_RESET, files.paths[i]);
        fflush(stdout);

        int result = run_xs_file(files.paths[i]);
        if (result == 0) {
            printf("%sPASS%s\n", CLR_GREEN, CLR_RESET);
            passed++;
        } else {
            printf("%sFAIL%s\n", CLR_RED, CLR_RESET);
            failed++;
        }
    }

    printf("\n%sResults:%s %d passed, %d failed, %d total\n",
           CLR_BOLD, CLR_RESET, passed, failed, files.count);

    filelist_free(&files);
    return failed > 0 ? 1 : 0;
}

/* ===== @watch: stat() loop checking mtimes, rebuild on change ===== */
int xs_shortcut_watch(void) {
    printf("%s@watch%s Watching for file changes (Ctrl+C to stop)...\n",
           CLR_YELLOW, CLR_RESET);

    FileList files;
    filelist_init(&files);
    collect_xs_files(".", &files);
    if (files.count == 0) {
        collect_xs_files("src", &files);
    }

    if (files.count == 0) {
        fprintf(stderr, "%serror:%s No .xs files found to watch\n", CLR_RED, CLR_RESET);
        filelist_free(&files);
        return 1;
    }

    /* Store initial mtimes */
    time_t *mtimes = (time_t *)calloc((size_t)files.count, sizeof(time_t));
    for (int i = 0; i < files.count; i++) {
        mtimes[i] = file_mtime(files.paths[i]);
    }

    printf("  Watching %d file(s)...\n", files.count);

    /* Polling loop */
    while (1) {
        usleep(500000); /* 500ms */

        bool changed = false;
        for (int i = 0; i < files.count; i++) {
            time_t now_mtime = file_mtime(files.paths[i]);
            if (now_mtime != mtimes[i]) {
                printf("\n  %sChanged:%s %s\n", CLR_CYAN, CLR_RESET, files.paths[i]);
                mtimes[i] = now_mtime;
                changed = true;
            }
        }

        if (changed) {
            printf("  Rebuilding...\n");
            mkdir("build", 0755);
            int failures = 0;
            for (int i = 0; i < files.count; i++) {
                if (build_xs_file(files.paths[i], "build") != 0) {
                    failures++;
                }
            }
            if (failures == 0) {
                printf("  %sBuild successful%s\n", CLR_GREEN, CLR_RESET);
            } else {
                fprintf(stderr, "  %sBuild failed%s (%d error(s))\n",
                        CLR_RED, CLR_RESET, failures);
            }
        }

        /* Re-scan for new files periodically */
        /* (For simplicity, we don't re-scan in this implementation) */
    }

    /* Unreachable unless interrupted */
    free(mtimes);
    filelist_free(&files);
    return 0;
}

/* ===== @check: Lint all .xs files ===== */
int xs_shortcut_check(void) {
    printf("%s@check%s Linting all .xs files...\n", CLR_YELLOW, CLR_RESET);

    FileList files;
    filelist_init(&files);
    collect_xs_files(".", &files);
    if (files.count == 0) {
        collect_xs_files("src", &files);
    }

    if (files.count == 0) {
        fprintf(stderr, "%serror:%s No .xs files found\n", CLR_RED, CLR_RESET);
        filelist_free(&files);
        return 1;
    }

    int total_errors = 0, total_warnings = 0;
    for (int i = 0; i < files.count; i++) {
        XsLintResult *result = xs_lint_file(files.paths[i]);
        if (result) {
            if (result->count > 0) {
                xs_lint_result_print(result);
            }
            total_errors += result->error_count;
            total_warnings += result->warning_count;
            xs_lint_result_free(result);
        }
    }

    printf("\n%sChecked %d file(s):%s ", CLR_BOLD, files.count, CLR_RESET);
    if (total_errors == 0 && total_warnings == 0) {
        printf("%sNo issues found.%s\n", CLR_GREEN, CLR_RESET);
    } else {
        printf("%d error(s), %d warning(s)\n", total_errors, total_warnings);
    }

    filelist_free(&files);
    return total_errors > 0 ? 1 : 0;
}

/* ===== @info: Print project info from .xsproj ===== */
int xs_shortcut_info(void) {
    printf("%s@info%s Project Information\n", CLR_YELLOW, CLR_RESET);
    printf("  ─────────────────────────\n");

    /* Try to read .xsproj */
    char *proj = read_file_contents(".xsproj");
    if (proj) {
        /* Simple key-value parsing (lines like "key = value") */
        char *line = strtok(proj, "\n");
        while (line) {
            /* Skip comments and empty lines */
            while (*line == ' ' || *line == '\t') line++;
            if (*line == '#' || *line == '\0' || *line == '[') {
                line = strtok(NULL, "\n");
                continue;
            }

            /* Find '=' separator */
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *key = line;
                char *val = eq + 1;

                /* Trim whitespace */
                while (*key == ' ') key++;
                char *kend = key + strlen(key) - 1;
                while (kend > key && *kend == ' ') *kend-- = '\0';

                while (*val == ' ') val++;
                /* Trim quotes */
                size_t vlen = strlen(val);
                if (vlen >= 2 && val[0] == '"' && val[vlen - 1] == '"') {
                    val[vlen - 1] = '\0';
                    val++;
                }

                printf("  %s%-12s%s %s\n", CLR_CYAN, key, CLR_RESET, val);
            }
            line = strtok(NULL, "\n");
        }
        free(proj);
    } else {
        printf("  (no .xsproj file found)\n");
    }

    /* Count source files */
    FileList files;
    filelist_init(&files);
    collect_xs_files(".", &files);

    printf("  %s%-12s%s %d\n", CLR_CYAN, "Source files", CLR_RESET, files.count);

    /* Total lines of code */
    int total_lines = 0;
    for (int i = 0; i < files.count; i++) {
        char *src = read_file_contents(files.paths[i]);
        if (src) {
            for (const char *p = src; *p; p++) {
                if (*p == '\n') total_lines++;
            }
            total_lines++; /* last line */
            free(src);
        }
    }
    printf("  %s%-12s%s %d\n", CLR_CYAN, "Total lines", CLR_RESET, total_lines);

    /* Check for build directory */
    if (is_directory("build")) {
        printf("  %s%-12s%s exists\n", CLR_CYAN, "Build dir", CLR_RESET);
    }

    filelist_free(&files);
    return 0;
}

/* ===== Main Shortcut Dispatcher ===== */
int xs_handle_shortcut(const char *name) {
    if (strcmp(name, "run") == 0)   return xs_shortcut_run();
    if (strcmp(name, "build") == 0) return xs_shortcut_build();
    if (strcmp(name, "clean") == 0) return xs_shortcut_clean();
    if (strcmp(name, "test") == 0)  return xs_shortcut_test();
    if (strcmp(name, "watch") == 0) return xs_shortcut_watch();
    if (strcmp(name, "check") == 0) return xs_shortcut_check();
    if (strcmp(name, "info") == 0)  return xs_shortcut_info();

    fprintf(stderr, "%serror:%s Unknown shortcut '@%s'\n", CLR_RED, CLR_RESET, name);
    fprintf(stderr, "Available: @run, @build, @clean, @test, @watch, @check, @info\n");
    return 1;
}
