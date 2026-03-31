/*
 * X# (Xsharp) Linter Implementation
 * =====================================
 */

#include "linter.h"
#include <stdarg.h>
#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include <stdarg.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== ANSI Colors ===== */
#define CLR_RESET "\033[0m"
#define CLR_RED "\033[31m"
#define CLR_YELLOW "\033[33m"
#define CLR_CYAN "\033[36m"
#define CLR_BOLD "\033[1m"
#define CLR_DIM "\033[2m"

/* ===== Symbol Tracker ===== */
#define MAX_SYMBOLS 1024

typedef struct {
    char name[128];
    int line;
    int column;
    bool is_declared;
    bool is_referenced;
    bool is_forge;  /* is this a forge (function) */
    bool is_entity; /* is this an entity (class) */
    bool is_parameter;
} SymbolEntry;

typedef struct {
    SymbolEntry symbols[MAX_SYMBOLS];
    int count;
} SymbolTable;

static void sym_init(SymbolTable* tbl) {
    tbl->count = 0;
}

static SymbolEntry* sym_find(SymbolTable* tbl, const char* name) {
    for (int i = 0; i < tbl->count; i++) {
        if (strcmp(tbl->symbols[i].name, name) == 0) {
            return &tbl->symbols[i];
        }
    }
    return NULL;
}

static SymbolEntry* sym_add(SymbolTable* tbl, const char* name, int line, int col) {
    if (tbl->count >= MAX_SYMBOLS)
        return NULL;
    SymbolEntry* e = &tbl->symbols[tbl->count++];
    memset(e, 0, sizeof(*e));
    strncpy(e->name, name, sizeof(e->name) - 1);
    e->line = line;
    e->column = col;
    return e;
}

/* ===== Lint Result Helpers ===== */
static XsLintResult* result_new(void) {
    XsLintResult* r = (XsLintResult*)calloc(1, sizeof(XsLintResult));
    r->capacity = 64;
    r->diagnostics = (XsLintDiagnostic*)calloc((size_t)r->capacity, sizeof(XsLintDiagnostic));
    return r;
}

static void result_add(XsLintResult* r, XsLintSeverity sev, const char* file, int line, int col,
                       const char* rule, const char* fmt, ...) {
    if (r->count >= r->capacity) {
        r->capacity *= 2;
        r->diagnostics = (XsLintDiagnostic*)realloc(r->diagnostics,
                                                    (size_t)r->capacity * sizeof(XsLintDiagnostic));
    }
    XsLintDiagnostic* d = &r->diagnostics[r->count++];
    d->severity = sev;
    strncpy(d->file, file, sizeof(d->file) - 1);
    d->line = line;
    d->column = col;
    strncpy(d->rule, rule, sizeof(d->rule) - 1);

    va_list args;
    va_start(args, fmt);
    vsnprintf(d->message, sizeof(d->message), fmt, args);
    va_end(args);

    if (sev == LINT_ERROR)
        r->error_count++;
    else if (sev == LINT_WARNING)
        r->warning_count++;
}

/* ===== Naming Convention Checks ===== */
static bool is_camel_case(const char* name) {
    if (!name || !*name)
        return false;
    /* camelCase: starts with lowercase */
    if (!islower((unsigned char)name[0]) && name[0] != '_')
        return false;
    /* Must not contain underscores (except leading _) */
    for (const char* p = name + 1; *p; p++) {
        if (*p == '_')
            return false;
    }
    return true;
}

static bool is_pascal_case(const char* name) {
    if (!name || !*name)
        return false;
    /* PascalCase: starts with uppercase */
    if (!isupper((unsigned char)name[0]))
        return false;
    for (const char* p = name + 1; *p; p++) {
        if (*p == '_')
            return false;
    }
    return true;
}

/* ===== AST Walking for Lint Checks ===== */
static void lint_walk(const AstNode* node, SymbolTable* syms, XsLintResult* result,
                      const char* file, bool in_forge, bool forge_has_return);

static void lint_walk_list(const AstNodeList* list, SymbolTable* syms, XsLintResult* result,
                           const char* file, bool in_forge, bool* has_return) {
    for (int i = 0; i < list->count; i++) {
        AstNode* child = list->items[i];
        if (child->type == NODE_UNLEASH_STMT) {
            *has_return = true;
        }
        lint_walk(child, syms, result, file, in_forge, false);
    }
}

static void lint_walk(const AstNode* node, SymbolTable* syms, XsLintResult* result,
                      const char* file, bool in_forge, bool forge_has_return) {
    if (!node)
        return;

    switch (node->type) {
    case NODE_PROGRAM: {
        bool hr = false;
        lint_walk_list(&node->as.program.declarations, syms, result, file, false, &hr);
        break;
    }

    case NODE_VAR_DECL:
    case NODE_CONST_DECL: {
        const char* name = node->as.var_decl.name;
        if (name) {
            SymbolEntry* e = sym_add(syms, name, node->line, node->column);
            if (e) {
                e->is_declared = true;
            }
            /* Check naming: variables should be camelCase */
            if (!is_camel_case(name)) {
                result_add(result, LINT_WARNING, file, node->line, node->column,
                           "naming-convention", "Variable '%s' should use camelCase", name);
            }
        }
        if (node->as.var_decl.initializer) {
            lint_walk(node->as.var_decl.initializer, syms, result, file, in_forge, false);
        }
        break;
    }

    case NODE_FORGE_DECL: {
        const char* name = node->as.forge.name;
        if (name) {
            SymbolEntry* e = sym_add(syms, name, node->line, node->column);
            if (e) {
                e->is_declared = true;
                e->is_forge = true;
                e->is_referenced = true; /* forges are considered used if declared at top level */
            }
            /* Check naming: forge names should be camelCase */
            if (!is_camel_case(name)) {
                result_add(result, LINT_WARNING, file, node->line, node->column,
                           "naming-convention", "Forge name '%s' should use camelCase", name);
            }
        }

        /* Add parameters as declared symbols */
        for (int i = 0; i < node->as.forge.params.count; i++) {
            const char* pname = node->as.forge.params.items[i].name;
            if (pname) {
                SymbolEntry* pe = sym_add(syms, pname, node->line, node->column);
                if (pe) {
                    pe->is_declared = true;
                    pe->is_parameter = true;
                }
            }
        }

        /* Check body for return statements */
        bool has_return = false;
        if (node->as.forge.body) {
            if (node->as.forge.body->type == NODE_BLOCK_STMT) {
                lint_walk_list(&node->as.forge.body->as.block.statements, syms, result, file, true,
                               &has_return);
            } else {
                lint_walk(node->as.forge.body, syms, result, file, true, false);
            }
        }

        /* Check if non-void forge has a return statement */
        if (node->as.forge.return_type.name &&
            strcmp(node->as.forge.return_type.name, "void") != 0 && !has_return) {
            result_add(result, LINT_WARNING, file, node->line, node->column, "missing-return",
                       "Forge '%s' has return type '%s' but may not return a value",
                       name ? name : "<anonymous>", node->as.forge.return_type.name);
        }
        break;
    }

    case NODE_ENTITY_DECL: {
        const char* name = node->as.entity.name;
        if (name) {
            SymbolEntry* e = sym_add(syms, name, node->line, node->column);
            if (e) {
                e->is_declared = true;
                e->is_entity = true;
                e->is_referenced = true;
            }
            /* Check naming: entity names should be PascalCase */
            if (!is_pascal_case(name)) {
                result_add(result, LINT_WARNING, file, node->line, node->column,
                           "naming-convention", "Entity name '%s' should use PascalCase", name);
            }
        }
        bool hr = false;
        lint_walk_list(&node->as.entity.members, syms, result, file, false, &hr);
        break;
    }

    case NODE_IDENTIFIER: {
        const char* name = node->as.identifier.name;
        if (name) {
            SymbolEntry* e = sym_find(syms, name);
            if (e) {
                e->is_referenced = true;
            }
            /* If not found in symbol table, could be a global/builtin -- don't warn */
        }
        break;
    }

    case NODE_ASSIGNMENT: {
        lint_walk(node->as.assignment.target, syms, result, file, in_forge, false);
        lint_walk(node->as.assignment.value, syms, result, file, in_forge, false);
        break;
    }

    case NODE_BINARY_EXPR: {
        lint_walk(node->as.binary.left, syms, result, file, in_forge, false);
        lint_walk(node->as.binary.right, syms, result, file, in_forge, false);
        break;
    }

    case NODE_UNARY_EXPR: {
        lint_walk(node->as.unary.operand, syms, result, file, in_forge, false);
        break;
    }

    case NODE_CALL_EXPR: {
        lint_walk(node->as.call.callee, syms, result, file, in_forge, false);
        for (int i = 0; i < node->as.call.args.count; i++) {
            lint_walk(node->as.call.args.items[i], syms, result, file, in_forge, false);
        }
        break;
    }

    case NODE_MEMBER_ACCESS: {
        lint_walk(node->as.member_access.object, syms, result, file, in_forge, false);
        break;
    }

    case NODE_INDEX_EXPR: {
        lint_walk(node->as.index_expr.object, syms, result, file, in_forge, false);
        lint_walk(node->as.index_expr.index, syms, result, file, in_forge, false);
        break;
    }

    case NODE_ORACLE_STMT: {
        lint_walk(node->as.oracle.condition, syms, result, file, in_forge, false);
        lint_walk(node->as.oracle.then_branch, syms, result, file, in_forge, false);
        if (node->as.oracle.else_branch) {
            lint_walk(node->as.oracle.else_branch, syms, result, file, in_forge, false);
        }
        break;
    }

    case NODE_CYCLE_STMT: {
        if (node->as.cycle.init)
            lint_walk(node->as.cycle.init, syms, result, file, in_forge, false);
        if (node->as.cycle.condition)
            lint_walk(node->as.cycle.condition, syms, result, file, in_forge, false);
        if (node->as.cycle.update)
            lint_walk(node->as.cycle.update, syms, result, file, in_forge, false);
        lint_walk(node->as.cycle.body, syms, result, file, in_forge, false);
        break;
    }

    case NODE_CYCLE_IN_STMT: {
        /* Declare loop variable */
        if (node->as.cycle_in.var_name) {
            SymbolEntry* e = sym_add(syms, node->as.cycle_in.var_name, node->line, node->column);
            if (e) {
                e->is_declared = true;
                e->is_referenced = true; /* loop variables are implicitly used */
            }
        }
        lint_walk(node->as.cycle_in.iterable, syms, result, file, in_forge, false);
        lint_walk(node->as.cycle_in.body, syms, result, file, in_forge, false);
        break;
    }

    case NODE_WHILE_STMT: {
        lint_walk(node->as.while_stmt.condition, syms, result, file, in_forge, false);
        lint_walk(node->as.while_stmt.body, syms, result, file, in_forge, false);
        break;
    }

    case NODE_BLOCK_STMT: {
        bool hr = false;
        lint_walk_list(&node->as.block.statements, syms, result, file, in_forge, &hr);
        break;
    }

    case NODE_UNLEASH_STMT: {
        if (node->as.unleash.value) {
            lint_walk(node->as.unleash.value, syms, result, file, in_forge, false);
        }
        break;
    }

    case NODE_ENGRAVE_STMT: {
        for (int i = 0; i < node->as.engrave.args.count; i++) {
            lint_walk(node->as.engrave.args.items[i], syms, result, file, in_forge, false);
        }
        break;
    }

    case NODE_EXPR_STMT: {
        lint_walk(node->as.expr_stmt.expr, syms, result, file, in_forge, false);
        break;
    }

    case NODE_SHIELD_STMT: {
        lint_walk(node->as.shield.try_body, syms, result, file, in_forge, false);
        if (node->as.shield.catch_var) {
            SymbolEntry* e = sym_add(syms, node->as.shield.catch_var, node->line, node->column);
            if (e) {
                e->is_declared = true;
            }
        }
        lint_walk(node->as.shield.catch_body, syms, result, file, in_forge, false);
        break;
    }

    case NODE_SHATTER_STMT: {
        lint_walk(node->as.shatter.expr, syms, result, file, in_forge, false);
        break;
    }

    case NODE_COMPOUND_ASSIGN: {
        lint_walk(node->as.compound_assign.target, syms, result, file, in_forge, false);
        lint_walk(node->as.compound_assign.value, syms, result, file, in_forge, false);
        break;
    }

    case NODE_PIPE_EXPR: {
        lint_walk(node->as.pipe.left, syms, result, file, in_forge, false);
        lint_walk(node->as.pipe.right, syms, result, file, in_forge, false);
        break;
    }

    case NODE_SPELL_EXPR: {
        lint_walk(node->as.spell.body, syms, result, file, in_forge, false);
        break;
    }

    case NODE_CONJURE_EXPR: {
        /* Mark entity as referenced */
        if (node->as.conjure.entity_name) {
            SymbolEntry* e = sym_find(syms, node->as.conjure.entity_name);
            if (e)
                e->is_referenced = true;
        }
        for (int i = 0; i < node->as.conjure.args.count; i++) {
            lint_walk(node->as.conjure.args.items[i], syms, result, file, in_forge, false);
        }
        break;
    }

    case NODE_ARSENAL_LITERAL: {
        for (int i = 0; i < node->as.arsenal_literal.elements.count; i++) {
            lint_walk(node->as.arsenal_literal.elements.items[i], syms, result, file, in_forge,
                      false);
        }
        break;
    }

    case NODE_TERNARY_EXPR: {
        lint_walk(node->as.ternary.condition, syms, result, file, in_forge, false);
        lint_walk(node->as.ternary.then_expr, syms, result, file, in_forge, false);
        lint_walk(node->as.ternary.else_expr, syms, result, file, in_forge, false);
        break;
    }

    case NODE_CAST_EXPR: {
        lint_walk(node->as.cast.expr, syms, result, file, in_forge, false);
        break;
    }

    case NODE_RANGE_EXPR: {
        lint_walk(node->as.range.start, syms, result, file, in_forge, false);
        lint_walk(node->as.range.end, syms, result, file, in_forge, false);
        break;
    }

    case NODE_SPREAD_EXPR: {
        lint_walk(node->as.spread.expr, syms, result, file, in_forge, false);
        break;
    }

    case NODE_PREFIX_INC:
    case NODE_PREFIX_DEC:
    case NODE_POSTFIX_INC:
    case NODE_POSTFIX_DEC: {
        lint_walk(node->as.inc_dec.operand, syms, result, file, in_forge, false);
        break;
    }

    case NODE_REALM_DECL: {
        bool hr = false;
        lint_walk_list(&node->as.realm.declarations, syms, result, file, false, &hr);
        break;
    }

    case NODE_SUMMON_DECL: {
        /* Import: mark as used by default */
        break;
    }

    case NODE_QUEST_DECL: {
        lint_walk(node->as.quest.body, syms, result, file, true, false);
        break;
    }

    case NODE_GPU_BLOCK: {
        lint_walk(node->as.gpu_block.body, syms, result, file, in_forge, false);
        break;
    }

    case NODE_AI_BLOCK: {
        lint_walk(node->as.ai_block.body, syms, result, file, in_forge, false);
        break;
    }

    case NODE_INTERPOLATED_STRING: {
        for (int i = 0; i < node->as.interp_string.parts.count; i++) {
            lint_walk(node->as.interp_string.parts.items[i], syms, result, file, in_forge, false);
        }
        break;
    }

    default:
        /* Literals and other nodes need no lint checks */
        break;
    }
}

/* ===== Public API ===== */
XsLintResult* xs_lint_string(const char* source, const char* filename) {
    if (!source)
        return NULL;

    XsLintResult* result = result_new();
    if (!result)
        return NULL;

    /* Lex */
    Lexer lexer;
    lexer_init(&lexer, source);
    int token_count = 0;
    Token* tokens = lexer_tokenize_all(&lexer, &token_count);
    if (lexer.had_error) {
        result_add(result, LINT_ERROR, filename, lexer.line, lexer.column, "syntax-error",
                   "Lexer error: %s", lexer.error_msg);
        free(tokens);
        return result;
    }

    /* Parse */
    Parser parser;
    parser_init(&parser, tokens, token_count);
    AstNode* program = parser_parse_program(&parser);
    if (parser_had_error(&parser)) {
        for (int i = 0; i < parser.error_count; i++) {
            result_add(result, LINT_ERROR, filename, 0, 0, "syntax-error", "%s", parser.errors[i]);
        }
        ast_free(program);
        free(tokens);
        return result;
    }

    /* Walk AST and collect diagnostics */
    SymbolTable syms;
    sym_init(&syms);
    lint_walk(program, &syms, result, filename, false, false);

    /* Check for unused variables */
    for (int i = 0; i < syms.count; i++) {
        SymbolEntry* e = &syms.symbols[i];
        if (e->is_declared && !e->is_referenced && !e->is_forge && !e->is_entity &&
            !e->is_parameter) {
            result_add(result, LINT_WARNING, filename, e->line, e->column, "unused-variable",
                       "Variable '%s' is declared but never used", e->name);
        }
        if (e->is_declared && !e->is_referenced && e->is_parameter) {
            result_add(result, LINT_HINT, filename, e->line, e->column, "unused-parameter",
                       "Parameter '%s' is never used", e->name);
        }
    }

    ast_free(program);
    free(tokens);
    return result;
}

XsLintResult* xs_lint_file(const char* path) {
    if (!path)
        return NULL;

    FILE* f = fopen(path, "rb");
    if (!f) {
        XsLintResult* r = result_new();
        result_add(r, LINT_ERROR, path, 0, 0, "file-error", "Cannot open file: %s", path);
        return r;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* source = (char*)malloc((size_t)size + 1);
    if (!source) {
        fclose(f);
        return NULL;
    }

    size_t nread = fread(source, 1, (size_t)size, f);
    source[nread] = '\0';
    fclose(f);

    XsLintResult* result = xs_lint_string(source, path);
    free(source);
    return result;
}

void xs_lint_result_free(XsLintResult* result) {
    if (!result)
        return;
    free(result->diagnostics);
    free(result);
}

void xs_lint_result_print(const XsLintResult* result) {
    if (!result)
        return;

    for (int i = 0; i < result->count; i++) {
        const XsLintDiagnostic* d = &result->diagnostics[i];
        const char* sev_str;
        const char* sev_color;
        switch (d->severity) {
        case LINT_ERROR:
            sev_str = "error";
            sev_color = CLR_RED;
            break;
        case LINT_WARNING:
            sev_str = "warning";
            sev_color = CLR_YELLOW;
            break;
        case LINT_INFO:
            sev_str = "info";
            sev_color = CLR_CYAN;
            break;
        case LINT_HINT:
            sev_str = "hint";
            sev_color = CLR_DIM;
            break;
        default:
            sev_str = "?";
            sev_color = CLR_RESET;
            break;
        }

        fprintf(stderr, "%s%s:%d:%d%s %s%s%s%s [%s] %s\n", CLR_BOLD, d->file, d->line, d->column,
                CLR_RESET, CLR_BOLD, sev_color, sev_str, CLR_RESET, d->rule, d->message);
    }
}
