/*
 * X# (Xsharp) Bytecode Compiler Implementation
 */

#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Lifecycle ===== */
void compiler_init(Compiler *c, Chunk *chunk, ScopeType type) {
    memset(c, 0, sizeof(Compiler));
    c->chunk = chunk;
    c->scope_type = type;
    c->local_count = 0;
    c->scope_depth = 0;
    c->upvalue_count = 0;
    c->loop_depth = 0;
    c->error_count = 0;
    c->had_error = false;
    c->enclosing = NULL;

    /* Reserve slot 0 for the function itself or 'self' */
    Local *local = &c->locals[c->local_count++];
    strcpy(local->name, "");
    local->depth = 0;
    local->is_captured = false;
}

/* ===== Error helpers ===== */
static void compiler_error(Compiler *c, const char *msg, int line) {
    if (c->error_count >= COMPILER_MAX_ERRORS) return;
    snprintf(c->errors[c->error_count], COMPILER_MAX_ERROR,
             "[line %d] Compile error: %s", line, msg);
    c->error_count++;
    c->had_error = true;
}

bool compiler_had_error(const Compiler *c) { return c->had_error; }

void compiler_print_errors(const Compiler *c) {
    for (int i = 0; i < c->error_count; i++) {
        fprintf(stderr, "%s\n", c->errors[i]);
    }
}

/* ===== Scope management ===== */
static void begin_scope(Compiler *c) { c->scope_depth++; }

static void end_scope(Compiler *c) {
    c->scope_depth--;
    while (c->local_count > 0 &&
           c->locals[c->local_count - 1].depth > c->scope_depth) {
        chunk_emit_byte(c->chunk, OP_POP, 0);
        c->local_count--;
    }
}

static int add_local(Compiler *c, const char *name) {
    if (c->local_count >= COMPILER_MAX_LOCALS) {
        compiler_error(c, "Too many local variables", 0);
        return -1;
    }
    Local *local = &c->locals[c->local_count];
    strncpy(local->name, name, 255);
    local->name[255] = '\0';
    local->depth = c->scope_depth;
    local->is_captured = false;
    return c->local_count++;
}

static int resolve_local(Compiler *c, const char *name) {
    for (int i = c->local_count - 1; i >= 0; i--) {
        if (strcmp(c->locals[i].name, name) == 0) return i;
    }
    return -1;
}

/* ===== Loop management ===== */
static void begin_loop(Compiler *c, int loop_start) {
    if (c->loop_depth >= COMPILER_MAX_LOOPS) return;
    LoopContext *loop = &c->loops[c->loop_depth++];
    loop->loop_start = loop_start;
    loop->body_start = c->chunk->code_count;
    loop->scope_depth = c->scope_depth;
    loop->break_count = 0;
}

static void end_loop(Compiler *c) {
    if (c->loop_depth <= 0) return;
    LoopContext *loop = &c->loops[--c->loop_depth];
    /* Patch all break jumps */
    for (int i = 0; i < loop->break_count; i++) {
        chunk_patch_jump(c->chunk, loop->break_jumps[i]);
    }
}

static void emit_break(Compiler *c, int line) {
    if (c->loop_depth <= 0) {
        compiler_error(c, "shatter_cycle outside of loop", line);
        return;
    }
    LoopContext *loop = &c->loops[c->loop_depth - 1];
    if (loop->break_count >= 64) {
        compiler_error(c, "Too many breaks in loop", line);
        return;
    }
    loop->break_jumps[loop->break_count++] = chunk_emit_jump(c->chunk, OP_JUMP, line);
}

/* ===== Expression compilation ===== */
void compile_expression(Compiler *c, AstNode *node) {
    if (!node) return;

    switch (node->type) {
    case NODE_LITERAL_INT:
        chunk_emit_const_op(c->chunk, OP_PUSH_BLADE, xs_blade(node->as.literal_int.value), node->line);
        break;

    case NODE_LITERAL_FLOAT:
        chunk_emit_const_op(c->chunk, OP_PUSH_SPARK, xs_spark(node->as.literal_float.value), node->line);
        break;

    case NODE_LITERAL_STRING:
        chunk_emit_const_op(c->chunk, OP_PUSH_SCROLL, xs_scroll(node->as.literal_string.value), node->line);
        break;

    case NODE_LITERAL_BOOL:
        chunk_emit_byte(c->chunk, node->as.literal_bool.value ? OP_PUSH_TRUTH : OP_PUSH_LIES, node->line);
        break;

    case NODE_LITERAL_NULL:
        chunk_emit_byte(c->chunk, OP_PUSH_ABYSS, node->line);
        break;

    case NODE_IDENTIFIER: {
        int slot = resolve_local(c, node->as.identifier.name);
        if (slot >= 0) {
            chunk_emit_byte(c->chunk, OP_LOAD_LOCAL, node->line);
            chunk_emit_short(c->chunk, (uint16_t)slot, node->line);
        } else {
            chunk_emit_const_op(c->chunk, OP_LOAD_GLOBAL,
                xs_scroll(strdup(node->as.identifier.name)), node->line);
        }
        break;
    }

    case NODE_BINARY_EXPR: {
        compile_expression(c, node->as.binary.left);
        compile_expression(c, node->as.binary.right);
        switch (node->as.binary.op) {
            case TOK_PLUS:          chunk_emit_byte(c->chunk, OP_ADD, node->line); break;
            case TOK_MINUS:         chunk_emit_byte(c->chunk, OP_SUB, node->line); break;
            case TOK_STAR:          chunk_emit_byte(c->chunk, OP_MUL, node->line); break;
            case TOK_SLASH:         chunk_emit_byte(c->chunk, OP_DIV, node->line); break;
            case TOK_PERCENT:       chunk_emit_byte(c->chunk, OP_MOD, node->line); break;
            case TOK_POWER:         chunk_emit_byte(c->chunk, OP_POW, node->line); break;
            case TOK_EQUAL_EQUAL:   chunk_emit_byte(c->chunk, OP_EQ, node->line); break;
            case TOK_BANG_EQUAL:    chunk_emit_byte(c->chunk, OP_NEQ, node->line); break;
            case TOK_LESS:          chunk_emit_byte(c->chunk, OP_LT, node->line); break;
            case TOK_GREATER:       chunk_emit_byte(c->chunk, OP_GT, node->line); break;
            case TOK_LESS_EQUAL:    chunk_emit_byte(c->chunk, OP_LTE, node->line); break;
            case TOK_GREATER_EQUAL: chunk_emit_byte(c->chunk, OP_GTE, node->line); break;
            case TOK_AND_AND:       chunk_emit_byte(c->chunk, OP_AND, node->line); break;
            case TOK_OR_OR:         chunk_emit_byte(c->chunk, OP_OR, node->line); break;
            case TOK_AMPERSAND:     chunk_emit_byte(c->chunk, OP_BIT_AND, node->line); break;
            case TOK_PIPE:          chunk_emit_byte(c->chunk, OP_BIT_OR, node->line); break;
            case TOK_CARET:         chunk_emit_byte(c->chunk, OP_BIT_XOR, node->line); break;
            case TOK_LSHIFT:        chunk_emit_byte(c->chunk, OP_SHL, node->line); break;
            case TOK_RSHIFT:        chunk_emit_byte(c->chunk, OP_SHR, node->line); break;
            default:
                compiler_error(c, "Unknown binary operator", node->line);
        }
        break;
    }

    case NODE_UNARY_EXPR:
        compile_expression(c, node->as.unary.operand);
        switch (node->as.unary.op) {
            case TOK_MINUS: chunk_emit_byte(c->chunk, OP_NEG, node->line); break;
            case TOK_BANG:  chunk_emit_byte(c->chunk, OP_NOT, node->line); break;
            case TOK_TILDE: chunk_emit_byte(c->chunk, OP_BIT_NOT, node->line); break;
            default: compiler_error(c, "Unknown unary operator", node->line);
        }
        break;

    case NODE_CALL_EXPR: {
        compile_expression(c, node->as.call.callee);
        for (int i = 0; i < node->as.call.args.count; i++) {
            compile_expression(c, node->as.call.args.items[i]);
        }
        chunk_emit_byte(c->chunk, OP_CALL, node->line);
        chunk_emit_byte(c->chunk, (uint8_t)node->as.call.args.count, node->line);
        break;
    }

    case NODE_MEMBER_ACCESS:
        compile_expression(c, node->as.member_access.object);
        chunk_emit_const_op(c->chunk, OP_GET_FIELD,
            xs_scroll(strdup(node->as.member_access.member)), node->line);
        break;

    case NODE_INDEX_EXPR:
        compile_expression(c, node->as.index_expr.object);
        compile_expression(c, node->as.index_expr.index);
        chunk_emit_byte(c->chunk, OP_INDEX_GET, node->line);
        break;

    case NODE_ASSIGNMENT:
        compile_expression(c, node->as.assignment.value);
        if (node->as.assignment.target->type == NODE_IDENTIFIER) {
            const char *name = node->as.assignment.target->as.identifier.name;
            int slot = resolve_local(c, name);
            if (slot >= 0) {
                chunk_emit_byte(c->chunk, OP_STORE_LOCAL, node->line);
                chunk_emit_short(c->chunk, (uint16_t)slot, node->line);
            } else {
                chunk_emit_const_op(c->chunk, OP_STORE_GLOBAL,
                    xs_scroll(strdup(name)), node->line);
            }
        } else if (node->as.assignment.target->type == NODE_MEMBER_ACCESS) {
            compile_expression(c, node->as.assignment.target->as.member_access.object);
            chunk_emit_const_op(c->chunk, OP_SET_FIELD,
                xs_scroll(strdup(node->as.assignment.target->as.member_access.member)), node->line);
        } else if (node->as.assignment.target->type == NODE_INDEX_EXPR) {
            compile_expression(c, node->as.assignment.target->as.index_expr.object);
            compile_expression(c, node->as.assignment.target->as.index_expr.index);
            chunk_emit_byte(c->chunk, OP_INDEX_SET, node->line);
        }
        break;

    case NODE_PIPE_EXPR:
        /* a |> f  =>  f(a) */
        compile_expression(c, node->as.pipe.right);
        compile_expression(c, node->as.pipe.left);
        chunk_emit_byte(c->chunk, OP_CALL, node->line);
        chunk_emit_byte(c->chunk, 1, node->line);
        break;

    case NODE_ARSENAL_LITERAL: {
        for (int i = 0; i < node->as.arsenal_literal.elements.count; i++) {
            compile_expression(c, node->as.arsenal_literal.elements.items[i]);
        }
        chunk_emit_byte(c->chunk, OP_NEW_ARSENAL, node->line);
        chunk_emit_short(c->chunk, (uint16_t)node->as.arsenal_literal.elements.count, node->line);
        break;
    }

    case NODE_CONJURE_EXPR: {
        for (int i = 0; i < node->as.conjure.args.count; i++) {
            compile_expression(c, node->as.conjure.args.items[i]);
        }
        uint16_t name_idx = (uint16_t)chunk_add_constant(c->chunk,
            xs_scroll(strdup(node->as.conjure.entity_name)));
        chunk_emit_byte(c->chunk, OP_NEW, node->line);
        chunk_emit_short(c->chunk, name_idx, node->line);
        chunk_emit_byte(c->chunk, (uint8_t)node->as.conjure.args.count, node->line);
        break;
    }

    case NODE_RANGE_EXPR:
        compile_expression(c, node->as.range.start);
        compile_expression(c, node->as.range.end);
        chunk_emit_byte(c->chunk, OP_RANGE, node->line);
        break;

    case NODE_TERNARY_EXPR: {
        compile_expression(c, node->as.ternary.condition);
        int else_jump = chunk_emit_jump(c->chunk, OP_JUMP_IF_FALSE, node->line);
        chunk_emit_byte(c->chunk, OP_POP, node->line);
        compile_expression(c, node->as.ternary.then_expr);
        int end_jump = chunk_emit_jump(c->chunk, OP_JUMP, node->line);
        chunk_patch_jump(c->chunk, else_jump);
        chunk_emit_byte(c->chunk, OP_POP, node->line);
        compile_expression(c, node->as.ternary.else_expr);
        chunk_patch_jump(c->chunk, end_jump);
        break;
    }

    case NODE_SELF_EXPR:
        chunk_emit_byte(c->chunk, OP_LOAD_LOCAL, node->line);
        chunk_emit_short(c->chunk, 0, node->line);
        break;

    case NODE_PREFIX_INC:
        compile_expression(c, node->as.inc_dec.operand);
        chunk_emit_byte(c->chunk, OP_INC, node->line);
        break;

    case NODE_PREFIX_DEC:
        compile_expression(c, node->as.inc_dec.operand);
        chunk_emit_byte(c->chunk, OP_DEC, node->line);
        break;

    default:
        compiler_error(c, "Unexpected expression type", node->line);
        break;
    }
}

/* ===== Statement compilation ===== */
void compile_statement(Compiler *c, AstNode *node) {
    if (!node) return;

    switch (node->type) {
    case NODE_EXPR_STMT:
        compile_expression(c, node->as.expr_stmt.expr);
        chunk_emit_byte(c->chunk, OP_POP, node->line);
        break;

    case NODE_BLOCK_STMT:
        begin_scope(c);
        for (int i = 0; i < node->as.block.statements.count; i++) {
            compile_node(c, node->as.block.statements.items[i]);
        }
        end_scope(c);
        break;

    case NODE_ENGRAVE_STMT: {
        for (int i = 0; i < node->as.engrave.args.count; i++) {
            compile_expression(c, node->as.engrave.args.items[i]);
        }
        chunk_emit_byte(c->chunk, OP_ENGRAVE, node->line);
        chunk_emit_byte(c->chunk, (uint8_t)node->as.engrave.args.count, node->line);
        break;
    }

    case NODE_ORACLE_STMT: {
        compile_expression(c, node->as.oracle.condition);
        int then_jump = chunk_emit_jump(c->chunk, OP_JUMP_IF_FALSE, node->line);
        chunk_emit_byte(c->chunk, OP_POP, node->line);
        compile_node(c, node->as.oracle.then_branch);

        if (node->as.oracle.else_branch) {
            int else_jump = chunk_emit_jump(c->chunk, OP_JUMP, node->line);
            chunk_patch_jump(c->chunk, then_jump);
            chunk_emit_byte(c->chunk, OP_POP, node->line);
            compile_node(c, node->as.oracle.else_branch);
            chunk_patch_jump(c->chunk, else_jump);
        } else {
            chunk_patch_jump(c->chunk, then_jump);
            chunk_emit_byte(c->chunk, OP_POP, node->line);
        }
        break;
    }

    case NODE_WHILE_STMT: {
        int loop_start = c->chunk->code_count;
        begin_loop(c, loop_start);
        compile_expression(c, node->as.while_stmt.condition);
        int exit_jump = chunk_emit_jump(c->chunk, OP_JUMP_IF_FALSE, node->line);
        chunk_emit_byte(c->chunk, OP_POP, node->line);
        compile_node(c, node->as.while_stmt.body);
        chunk_emit_loop(c->chunk, loop_start, node->line);
        chunk_patch_jump(c->chunk, exit_jump);
        chunk_emit_byte(c->chunk, OP_POP, node->line);
        end_loop(c);
        break;
    }

    case NODE_CYCLE_STMT: {
        begin_scope(c);
        if (node->as.cycle.init) compile_node(c, node->as.cycle.init);
        int loop_start = c->chunk->code_count;
        begin_loop(c, loop_start);
        if (node->as.cycle.condition) {
            compile_expression(c, node->as.cycle.condition);
            int exit_jump = chunk_emit_jump(c->chunk, OP_JUMP_IF_FALSE, node->line);
            chunk_emit_byte(c->chunk, OP_POP, node->line);
            compile_node(c, node->as.cycle.body);
            if (node->as.cycle.update) {
                compile_expression(c, node->as.cycle.update);
                chunk_emit_byte(c->chunk, OP_POP, node->line);
            }
            chunk_emit_loop(c->chunk, loop_start, node->line);
            chunk_patch_jump(c->chunk, exit_jump);
            chunk_emit_byte(c->chunk, OP_POP, node->line);
        }
        end_loop(c);
        end_scope(c);
        break;
    }

    case NODE_UNLEASH_STMT:
        if (node->as.unleash.value) {
            compile_expression(c, node->as.unleash.value);
        } else {
            chunk_emit_byte(c->chunk, OP_PUSH_ABYSS, node->line);
        }
        chunk_emit_byte(c->chunk, OP_RETURN, node->line);
        break;

    case NODE_SHATTER_CYCLE_STMT:
        emit_break(c, node->line);
        break;

    case NODE_SKIP_STMT:
        if (c->loop_depth > 0) {
            chunk_emit_loop(c->chunk, c->loops[c->loop_depth - 1].loop_start, node->line);
        } else {
            compiler_error(c, "skip outside of loop", node->line);
        }
        break;

    case NODE_SHIELD_STMT: {
        int catch_jump = chunk_emit_jump(c->chunk, OP_SHIELD_BEGIN, node->line);
        compile_node(c, node->as.shield.try_body);
        chunk_emit_byte(c->chunk, OP_SHIELD_END, node->line);
        int end_jump = chunk_emit_jump(c->chunk, OP_JUMP, node->line);
        chunk_patch_jump(c->chunk, catch_jump);
        /* catch var */
        begin_scope(c);
        if (node->as.shield.catch_var) {
            add_local(c, node->as.shield.catch_var);
        }
        compile_node(c, node->as.shield.catch_body);
        end_scope(c);
        chunk_patch_jump(c->chunk, end_jump);
        break;
    }

    case NODE_SHATTER_STMT:
        compile_expression(c, node->as.shatter.expr);
        chunk_emit_byte(c->chunk, OP_SHATTER, node->line);
        break;

    default:
        compile_node(c, node);
        break;
    }
}

/* ===== Declaration compilation ===== */
void compile_declaration(Compiler *c, AstNode *node) {
    if (!node) return;

    switch (node->type) {
    case NODE_VAR_DECL:
    case NODE_CONST_DECL: {
        if (node->as.var_decl.initializer) {
            compile_expression(c, node->as.var_decl.initializer);
        } else {
            chunk_emit_byte(c->chunk, OP_PUSH_ABYSS, node->line);
        }
        if (c->scope_depth > 0) {
            add_local(c, node->as.var_decl.name);
        } else {
            chunk_emit_const_op(c->chunk, OP_STORE_GLOBAL,
                xs_scroll(strdup(node->as.var_decl.name)), node->line);
            chunk_emit_byte(c->chunk, OP_POP, node->line);
        }
        break;
    }

    case NODE_FORGE_DECL: {
        /* Compile function as a global */
        /* For now, store function as a global (simplified) */
        chunk_emit_byte(c->chunk, OP_PUSH_ABYSS, node->line);
        if (c->scope_depth > 0) {
            add_local(c, node->as.forge.name);
        } else {
            chunk_emit_const_op(c->chunk, OP_STORE_GLOBAL,
                xs_scroll(strdup(node->as.forge.name)), node->line);
            chunk_emit_byte(c->chunk, OP_POP, node->line);
        }
        break;
    }

    case NODE_ENTITY_DECL: {
        /* Compile entity as a global */
        chunk_emit_byte(c->chunk, OP_PUSH_ABYSS, node->line);
        chunk_emit_const_op(c->chunk, OP_STORE_GLOBAL,
            xs_scroll(strdup(node->as.entity.name)), node->line);
        chunk_emit_byte(c->chunk, OP_POP, node->line);
        break;
    }

    case NODE_QUEST_DECL:
        compile_node(c, node->as.quest.body);
        break;

    case NODE_REALM_DECL:
        for (int i = 0; i < node->as.realm.declarations.count; i++) {
            compile_node(c, node->as.realm.declarations.items[i]);
        }
        break;

    case NODE_SUMMON_DECL:
        /* Import: no-op for now */
        break;

    case NODE_GPU_BLOCK:
        compile_node(c, node->as.gpu_block.body);
        break;

    case NODE_AI_BLOCK:
        compile_node(c, node->as.ai_block.body);
        break;

    default:
        compile_statement(c, node);
        break;
    }
}

/* ===== Generic node compilation ===== */
void compile_node(Compiler *c, AstNode *node) {
    if (!node) return;

    switch (node->type) {
    /* Declarations */
    case NODE_VAR_DECL: case NODE_CONST_DECL: case NODE_FORGE_DECL:
    case NODE_ENTITY_DECL: case NODE_REALM_DECL: case NODE_SUMMON_DECL:
    case NODE_QUEST_DECL: case NODE_GPU_BLOCK: case NODE_AI_BLOCK:
        compile_declaration(c, node);
        break;

    /* Statements */
    case NODE_EXPR_STMT: case NODE_BLOCK_STMT: case NODE_ORACLE_STMT:
    case NODE_CYCLE_STMT: case NODE_WHILE_STMT: case NODE_UNLEASH_STMT:
    case NODE_SHATTER_CYCLE_STMT: case NODE_SKIP_STMT: case NODE_ENGRAVE_STMT:
    case NODE_SHIELD_STMT: case NODE_SHATTER_STMT:
        compile_statement(c, node);
        break;

    /* Program */
    case NODE_PROGRAM:
        for (int i = 0; i < node->as.program.declarations.count; i++) {
            compile_node(c, node->as.program.declarations.items[i]);
        }
        break;

    /* Expressions */
    default:
        compile_expression(c, node);
        break;
    }
}

/* ===== Public API ===== */
bool compiler_compile(Compiler *c, AstNode *program) {
    compile_node(c, program);
    chunk_emit_byte(c->chunk, OP_HALT, 0);
    return !c->had_error;
}

bool compiler_compile_expression(Compiler *c, AstNode *expr) {
    compile_expression(c, expr);
    chunk_emit_byte(c->chunk, OP_HALT, 0);
    return !c->had_error;
}
