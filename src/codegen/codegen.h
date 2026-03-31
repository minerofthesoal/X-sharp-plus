/*
 * X# (Xsharp) Code Generation Utilities
 * =======================================
 * Chunk management, bytecode emission, constant pool, disassembler.
 */

#ifndef XSHARP_CODEGEN_H
#define XSHARP_CODEGEN_H

#include "../runtime/runtime.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ===== Opcodes ===== */
typedef enum {
    /* Stack operations */
    OP_PUSH_BLADE,  /* push int64 constant */
    OP_PUSH_SPARK,  /* push double constant */
    OP_PUSH_SCROLL, /* push string constant */
    OP_PUSH_TRUTH,  /* push true */
    OP_PUSH_LIES,   /* push false */
    OP_PUSH_ABYSS,  /* push null */
    OP_POP,         /* pop top of stack */
    OP_DUP,         /* duplicate top of stack */

    /* Arithmetic */
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_POW,
    OP_NEG, /* unary negate */

    /* Comparison */
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_GT,
    OP_LTE,
    OP_GTE,

    /* Logical */
    OP_AND,
    OP_OR,
    OP_NOT,

    /* Bitwise */
    OP_BIT_AND,
    OP_BIT_OR,
    OP_BIT_XOR,
    OP_BIT_NOT,
    OP_SHL,
    OP_SHR,

    /* Variables */
    OP_LOAD_LOCAL,    /* operand: uint16 slot */
    OP_STORE_LOCAL,   /* operand: uint16 slot */
    OP_LOAD_GLOBAL,   /* operand: uint16 name constant index */
    OP_STORE_GLOBAL,  /* operand: uint16 name constant index */
    OP_LOAD_UPVALUE,  /* operand: uint16 upvalue index */
    OP_STORE_UPVALUE, /* operand: uint16 upvalue index */

    /* Control flow */
    OP_JUMP,          /* operand: int16 offset */
    OP_JUMP_IF_FALSE, /* operand: int16 offset */
    OP_JUMP_IF_TRUE,  /* operand: int16 offset */
    OP_LOOP,          /* operand: uint16 offset (backwards) */

    /* Functions */
    OP_CALL, /* operand: uint8 arg_count */
    OP_RETURN,
    OP_CLOSURE, /* operand: uint16 constant index (function) */

    /* Objects / entities */
    OP_NEW,        /* operand: uint16 entity name constant, uint8 argc */
    OP_GET_FIELD,  /* operand: uint16 field name constant */
    OP_SET_FIELD,  /* operand: uint16 field name constant */
    OP_GET_METHOD, /* operand: uint16 method name constant */
    OP_INVOKE,     /* operand: uint16 method name constant, uint8 argc */

    /* Collections */
    OP_NEW_ARSENAL, /* operand: uint16 element count */
    OP_INDEX_GET,
    OP_INDEX_SET,
    OP_ARSENAL_PUSH,

    /* GPU */
    OP_GPU_DISPATCH,
    OP_GPU_SYNC,

    /* I/O */
    OP_ENGRAVE, /* operand: uint8 arg count */

    /* Pipe */
    OP_PIPE,

    /* Increment/Decrement */
    OP_INC,
    OP_DEC,

    /* Cast */
    OP_CAST, /* operand: uint8 target type */

    /* Range */
    OP_RANGE,

    /* Spread */
    OP_SPREAD,

    /* Exception handling */
    OP_SHIELD_BEGIN, /* operand: uint16 catch offset */
    OP_SHIELD_END,
    OP_SHATTER, /* throw */

    /* Halt */
    OP_HALT,

    OP_COUNT
} OpCode;

/* ===== Constant entry ===== */
typedef struct {
    XsValue value;
} Constant;

/* ===== Chunk: a block of bytecode ===== */
typedef struct {
    uint8_t* code; /* bytecode array */
    int code_count;
    int code_capacity;

    Constant* constants; /* constant pool */
    int const_count;
    int const_capacity;

    int* lines; /* line number for each byte (parallel to code) */
    int line_count;
    int line_capacity;

    char* name; /* chunk name (function/module) */
} Chunk;

/* ===== Chunk lifecycle ===== */
void chunk_init(Chunk* chunk, const char* name);
void chunk_free(Chunk* chunk);

/* ===== Emit bytecode ===== */
void chunk_emit_byte(Chunk* chunk, uint8_t byte, int line);
void chunk_emit_short(Chunk* chunk, uint16_t value, int line);
int chunk_emit_constant(Chunk* chunk, XsValue value, int line);

/* Emit an opcode followed by a uint16 constant index */
int chunk_emit_const_op(Chunk* chunk, OpCode op, XsValue value, int line);

/* ===== Jump patching ===== */
/* Emit a jump instruction with a placeholder offset. Returns the offset of the placeholder. */
int chunk_emit_jump(Chunk* chunk, OpCode jump_op, int line);

/* Patch a previously emitted jump placeholder to jump to the current position. */
void chunk_patch_jump(Chunk* chunk, int offset);

/* Emit a loop instruction that jumps backwards to loop_start. */
void chunk_emit_loop(Chunk* chunk, int loop_start, int line);

/* ===== Constant pool ===== */
int chunk_add_constant(Chunk* chunk, XsValue value);
XsValue chunk_get_constant(Chunk* chunk, int index);

/* ===== Line info ===== */
int chunk_get_line(Chunk* chunk, int offset);

/* ===== Disassembler ===== */
void chunk_disassemble(Chunk* chunk, const char* label);
int chunk_disassemble_instruction(Chunk* chunk, int offset);

/* ===== Opcode names ===== */
const char* opcode_name(OpCode op);

#endif /* XSHARP_CODEGEN_H */
