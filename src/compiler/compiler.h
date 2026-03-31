/*
 * X# (Xsharp) Bytecode Compiler
 * ===============================
 * Walks the AST and emits bytecode into Chunks.
 */

#ifndef XSHARP_COMPILER_H
#define XSHARP_COMPILER_H

#include "../ast/ast.h"
#include "../codegen/codegen.h"
#include "../lexer/token.h"
#include <stdbool.h>

/* ===== Local variable ===== */
typedef struct {
    char name[256];
    int depth;        /* scope depth */
    bool is_captured; /* captured by a closure */
} Local;

/* ===== Upvalue ===== */
typedef struct {
    uint16_t index;
    bool is_local; /* true = captures a local; false = captures parent upvalue */
} Upvalue;

/* ===== Compiler scope type ===== */
typedef enum {
    SCOPE_SCRIPT,
    SCOPE_FORGE,  /* function */
    SCOPE_SPELL,  /* lambda */
    SCOPE_QUEST,  /* main */
    SCOPE_ENTITY, /* class */
} ScopeType;

/* ===== Loop tracking (for break/continue) ===== */
typedef struct {
    int loop_start;  /* bytecode offset of loop condition */
    int body_start;  /* bytecode offset of loop body */
    int scope_depth; /* scope depth when loop began */
    int break_jumps[64];
    int break_count;
} LoopContext;

#define COMPILER_MAX_LOCALS 256
#define COMPILER_MAX_UPVALUES 256
#define COMPILER_MAX_LOOPS 32
#define COMPILER_MAX_ERRORS 64
#define COMPILER_MAX_ERROR 512

/* ===== Compiler state ===== */
typedef struct Compiler {
    /* Output chunk */
    Chunk* chunk;

    /* Local variables */
    Local locals[COMPILER_MAX_LOCALS];
    int local_count;
    int scope_depth;

    /* Upvalues */
    Upvalue upvalues[COMPILER_MAX_UPVALUES];
    int upvalue_count;

    /* Current scope type */
    ScopeType scope_type;

    /* Loop stack */
    LoopContext loops[COMPILER_MAX_LOOPS];
    int loop_depth;

    /* Enclosing compiler (for nested functions) */
    struct Compiler* enclosing;

    /* Error reporting */
    char errors[COMPILER_MAX_ERRORS][COMPILER_MAX_ERROR];
    int error_count;
    bool had_error;
} Compiler;

/* ===== API ===== */

/* Initialize a compiler. chunk must be pre-initialized. */
void compiler_init(Compiler* compiler, Chunk* chunk, ScopeType type);

/* Compile an entire AST program node into bytecode. Returns true on success. */
bool compiler_compile(Compiler* compiler, AstNode* program);

/* Compile a single expression (for REPL). */
bool compiler_compile_expression(Compiler* compiler, AstNode* expr);

/* Check for errors */
bool compiler_had_error(const Compiler* compiler);

/* Print all errors */
void compiler_print_errors(const Compiler* compiler);

/* ===== Internal: compile individual node types (also usable externally) ===== */
void compile_node(Compiler* c, AstNode* node);
void compile_expression(Compiler* c, AstNode* node);
void compile_statement(Compiler* c, AstNode* node);
void compile_declaration(Compiler* c, AstNode* node);

#endif /* XSHARP_COMPILER_H */
