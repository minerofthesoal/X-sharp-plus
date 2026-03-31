/*
 * X# (Xsharp) Code Generation Utilities - Implementation
 * ========================================================
 * Chunk management, bytecode emission, constant pool, disassembler.
 */

#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Opcode name table ===== */
static const char* opcode_names[] = {
    [OP_PUSH_BLADE] = "OP_PUSH_BLADE",
    [OP_PUSH_SPARK] = "OP_PUSH_SPARK",
    [OP_PUSH_SCROLL] = "OP_PUSH_SCROLL",
    [OP_PUSH_TRUTH] = "OP_PUSH_TRUTH",
    [OP_PUSH_LIES] = "OP_PUSH_LIES",
    [OP_PUSH_ABYSS] = "OP_PUSH_ABYSS",
    [OP_POP] = "OP_POP",
    [OP_DUP] = "OP_DUP",
    [OP_ADD] = "OP_ADD",
    [OP_SUB] = "OP_SUB",
    [OP_MUL] = "OP_MUL",
    [OP_DIV] = "OP_DIV",
    [OP_MOD] = "OP_MOD",
    [OP_POW] = "OP_POW",
    [OP_NEG] = "OP_NEG",
    [OP_EQ] = "OP_EQ",
    [OP_NEQ] = "OP_NEQ",
    [OP_LT] = "OP_LT",
    [OP_GT] = "OP_GT",
    [OP_LTE] = "OP_LTE",
    [OP_GTE] = "OP_GTE",
    [OP_AND] = "OP_AND",
    [OP_OR] = "OP_OR",
    [OP_NOT] = "OP_NOT",
    [OP_BIT_AND] = "OP_BIT_AND",
    [OP_BIT_OR] = "OP_BIT_OR",
    [OP_BIT_XOR] = "OP_BIT_XOR",
    [OP_BIT_NOT] = "OP_BIT_NOT",
    [OP_SHL] = "OP_SHL",
    [OP_SHR] = "OP_SHR",
    [OP_LOAD_LOCAL] = "OP_LOAD_LOCAL",
    [OP_STORE_LOCAL] = "OP_STORE_LOCAL",
    [OP_LOAD_GLOBAL] = "OP_LOAD_GLOBAL",
    [OP_STORE_GLOBAL] = "OP_STORE_GLOBAL",
    [OP_LOAD_UPVALUE] = "OP_LOAD_UPVALUE",
    [OP_STORE_UPVALUE] = "OP_STORE_UPVALUE",
    [OP_JUMP] = "OP_JUMP",
    [OP_JUMP_IF_FALSE] = "OP_JUMP_IF_FALSE",
    [OP_JUMP_IF_TRUE] = "OP_JUMP_IF_TRUE",
    [OP_LOOP] = "OP_LOOP",
    [OP_CALL] = "OP_CALL",
    [OP_RETURN] = "OP_RETURN",
    [OP_CLOSURE] = "OP_CLOSURE",
    [OP_NEW] = "OP_NEW",
    [OP_GET_FIELD] = "OP_GET_FIELD",
    [OP_SET_FIELD] = "OP_SET_FIELD",
    [OP_GET_METHOD] = "OP_GET_METHOD",
    [OP_INVOKE] = "OP_INVOKE",
    [OP_NEW_ARSENAL] = "OP_NEW_ARSENAL",
    [OP_INDEX_GET] = "OP_INDEX_GET",
    [OP_INDEX_SET] = "OP_INDEX_SET",
    [OP_ARSENAL_PUSH] = "OP_ARSENAL_PUSH",
    [OP_GPU_DISPATCH] = "OP_GPU_DISPATCH",
    [OP_GPU_SYNC] = "OP_GPU_SYNC",
    [OP_ENGRAVE] = "OP_ENGRAVE",
    [OP_PIPE] = "OP_PIPE",
    [OP_INC] = "OP_INC",
    [OP_DEC] = "OP_DEC",
    [OP_CAST] = "OP_CAST",
    [OP_RANGE] = "OP_RANGE",
    [OP_SPREAD] = "OP_SPREAD",
    [OP_SHIELD_BEGIN] = "OP_SHIELD_BEGIN",
    [OP_SHIELD_END] = "OP_SHIELD_END",
    [OP_SHATTER] = "OP_SHATTER",
    [OP_HALT] = "OP_HALT",
};

const char* opcode_name(OpCode op) {
    if (op >= 0 && op < OP_COUNT && opcode_names[op]) {
        return opcode_names[op];
    }
    return "OP_UNKNOWN";
}

/* ===== Chunk lifecycle ===== */

void chunk_init(Chunk* chunk, const char* name) {
    chunk->code = NULL;
    chunk->code_count = 0;
    chunk->code_capacity = 0;

    chunk->constants = NULL;
    chunk->const_count = 0;
    chunk->const_capacity = 0;

    chunk->lines = NULL;
    chunk->line_count = 0;
    chunk->line_capacity = 0;

    chunk->name = name ? strdup(name) : strdup("<script>");
}

void chunk_free(Chunk* chunk) {
    /* Free string constants */
    for (int i = 0; i < chunk->const_count; i++) {
        if (chunk->constants[i].value.type == VAL_SCROLL) {
            free(chunk->constants[i].value.scroll);
        }
    }
    free(chunk->code);
    free(chunk->constants);
    free(chunk->lines);
    free(chunk->name);

    chunk->code = NULL;
    chunk->code_count = 0;
    chunk->code_capacity = 0;
    chunk->constants = NULL;
    chunk->const_count = 0;
    chunk->const_capacity = 0;
    chunk->lines = NULL;
    chunk->line_count = 0;
    chunk->line_capacity = 0;
    chunk->name = NULL;
}

/* ===== Grow helpers ===== */

static void ensure_code_capacity(Chunk* chunk, int extra) {
    int needed = chunk->code_count + extra;
    if (needed <= chunk->code_capacity)
        return;
    int new_cap = chunk->code_capacity < 8 ? 8 : chunk->code_capacity;
    while (new_cap < needed)
        new_cap *= 2;
    chunk->code = (uint8_t*)realloc(chunk->code, new_cap);
    chunk->code_capacity = new_cap;
}

static void ensure_line_capacity(Chunk* chunk, int extra) {
    int needed = chunk->line_count + extra;
    if (needed <= chunk->line_capacity)
        return;
    int new_cap = chunk->line_capacity < 8 ? 8 : chunk->line_capacity;
    while (new_cap < needed)
        new_cap *= 2;
    chunk->lines = (int*)realloc(chunk->lines, sizeof(int) * new_cap);
    chunk->line_capacity = new_cap;
}

static void ensure_const_capacity(Chunk* chunk) {
    if (chunk->const_count < chunk->const_capacity)
        return;
    int new_cap = chunk->const_capacity < 8 ? 8 : chunk->const_capacity * 2;
    chunk->constants = (Constant*)realloc(chunk->constants, sizeof(Constant) * new_cap);
    chunk->const_capacity = new_cap;
}

/* ===== Emit bytecode ===== */

void chunk_emit_byte(Chunk* chunk, uint8_t byte, int line) {
    ensure_code_capacity(chunk, 1);
    ensure_line_capacity(chunk, 1);
    chunk->code[chunk->code_count] = byte;
    chunk->lines[chunk->line_count] = line;
    chunk->code_count++;
    chunk->line_count++;
}

void chunk_emit_short(Chunk* chunk, uint16_t value, int line) {
    chunk_emit_byte(chunk, (uint8_t)((value >> 8) & 0xFF), line);
    chunk_emit_byte(chunk, (uint8_t)(value & 0xFF), line);
}

int chunk_emit_constant(Chunk* chunk, XsValue value, int line) {
    int idx = chunk_add_constant(chunk, value);
    chunk_emit_byte(chunk, (uint8_t)((idx >> 8) & 0xFF), line);
    chunk_emit_byte(chunk, (uint8_t)(idx & 0xFF), line);
    return idx;
}

int chunk_emit_const_op(Chunk* chunk, OpCode op, XsValue value, int line) {
    chunk_emit_byte(chunk, (uint8_t)op, line);
    return chunk_emit_constant(chunk, value, line);
}

/* ===== Jump patching ===== */

int chunk_emit_jump(Chunk* chunk, OpCode jump_op, int line) {
    chunk_emit_byte(chunk, (uint8_t)jump_op, line);
    /* Placeholder for 16-bit offset */
    int offset = chunk->code_count;
    chunk_emit_byte(chunk, 0xFF, line);
    chunk_emit_byte(chunk, 0xFF, line);
    return offset;
}

void chunk_patch_jump(Chunk* chunk, int offset) {
    /* Calculate the jump distance: from after the jump operand to current pos */
    int jump = chunk->code_count - offset - 2;
    if (jump > 0xFFFF) {
        fprintf(stderr, "Error: Jump offset too large (%d)\n", jump);
        return;
    }
    chunk->code[offset] = (uint8_t)((jump >> 8) & 0xFF);
    chunk->code[offset + 1] = (uint8_t)(jump & 0xFF);
}

void chunk_emit_loop(Chunk* chunk, int loop_start, int line) {
    chunk_emit_byte(chunk, (uint8_t)OP_LOOP, line);
    /* Offset is backwards from after this instruction's operand */
    int offset = chunk->code_count - loop_start + 2;
    if (offset > 0xFFFF) {
        fprintf(stderr, "Error: Loop offset too large (%d)\n", offset);
        return;
    }
    chunk_emit_byte(chunk, (uint8_t)((offset >> 8) & 0xFF), line);
    chunk_emit_byte(chunk, (uint8_t)(offset & 0xFF), line);
}

/* ===== Constant pool ===== */

int chunk_add_constant(Chunk* chunk, XsValue value) {
    /* Check for duplicate constants (optimization) */
    for (int i = 0; i < chunk->const_count; i++) {
        XsValue existing = chunk->constants[i].value;
        if (existing.type == value.type) {
            switch (value.type) {
            case VAL_BLADE:
                if (existing.blade == value.blade)
                    return i;
                break;
            case VAL_SPARK:
                if (existing.spark == value.spark)
                    return i;
                break;
            case VAL_SCROLL:
                if (existing.scroll && value.scroll && strcmp(existing.scroll, value.scroll) == 0)
                    return i;
                break;
            case VAL_FATE:
                if (existing.fate == value.fate)
                    return i;
                break;
            case VAL_ABYSS:
                return i;
            default:
                break;
            }
        }
    }

    ensure_const_capacity(chunk);
    /* Deep copy strings */
    if (value.type == VAL_SCROLL && value.scroll) {
        value.scroll = strdup(value.scroll);
    }
    chunk->constants[chunk->const_count].value = value;
    return chunk->const_count++;
}

XsValue chunk_get_constant(Chunk* chunk, int index) {
    if (index < 0 || index >= chunk->const_count) {
        return xs_abyss();
    }
    return chunk->constants[index].value;
}

/* ===== Line info ===== */

int chunk_get_line(Chunk* chunk, int offset) {
    if (offset < 0 || offset >= chunk->line_count)
        return 0;
    return chunk->lines[offset];
}

/* ===== Disassembler ===== */

static void print_value(XsValue val) {
    switch (val.type) {
    case VAL_BLADE:
        printf("%lld", (long long)val.blade);
        break;
    case VAL_SPARK:
        printf("%g", val.spark);
        break;
    case VAL_SCROLL:
        printf("\"%s\"", val.scroll ? val.scroll : "(null)");
        break;
    case VAL_FATE:
        printf("%s", val.fate ? "truth" : "lies");
        break;
    case VAL_ABYSS:
        printf("abyss");
        break;
    default:
        printf("<object>");
        break;
    }
}

/* Disassemble a simple instruction (no operand) */
static int simple_instruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

/* Disassemble an instruction with a uint8 operand */
static int byte_instruction(const char* name, Chunk* chunk, int offset) {
    uint8_t operand = chunk->code[offset + 1];
    printf("%-20s %4d\n", name, operand);
    return offset + 2;
}

/* Disassemble an instruction with a uint16 operand */
static int short_instruction(const char* name, Chunk* chunk, int offset) {
    uint16_t operand = (uint16_t)(chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
    printf("%-20s %4d\n", name, operand);
    return offset + 3;
}

/* Disassemble a constant instruction: opcode + uint16 constant index */
static int constant_instruction(const char* name, Chunk* chunk, int offset) {
    uint16_t idx = (uint16_t)(chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
    printf("%-20s %4d  (", name, idx);
    if (idx < chunk->const_count) {
        print_value(chunk->constants[idx].value);
    } else {
        printf("???");
    }
    printf(")\n");
    return offset + 3;
}

/* Disassemble a jump instruction */
static int jump_instruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
    int target = offset + 3 + sign * jump;
    printf("%-20s %4d -> %d\n", name, jump, target);
    return offset + 3;
}

/* Disassemble an invoke instruction: opcode + uint16 name + uint8 argc */
static int invoke_instruction(const char* name, Chunk* chunk, int offset) {
    uint16_t name_idx = (uint16_t)(chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
    uint8_t argc = chunk->code[offset + 3];
    printf("%-20s %4d (", name, name_idx);
    if (name_idx < chunk->const_count) {
        print_value(chunk->constants[name_idx].value);
    }
    printf(") argc=%d\n", argc);
    return offset + 4;
}

int chunk_disassemble_instruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    /* Show line info */
    if (offset > 0 && chunk_get_line(chunk, offset) == chunk_get_line(chunk, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", chunk_get_line(chunk, offset));
    }

    uint8_t instruction = chunk->code[offset];

    switch ((OpCode)instruction) {
    case OP_PUSH_BLADE:
        return constant_instruction("OP_PUSH_BLADE", chunk, offset);
    case OP_PUSH_SPARK:
        return constant_instruction("OP_PUSH_SPARK", chunk, offset);
    case OP_PUSH_SCROLL:
        return constant_instruction("OP_PUSH_SCROLL", chunk, offset);
    case OP_PUSH_TRUTH:
        return simple_instruction("OP_PUSH_TRUTH", offset);
    case OP_PUSH_LIES:
        return simple_instruction("OP_PUSH_LIES", offset);
    case OP_PUSH_ABYSS:
        return simple_instruction("OP_PUSH_ABYSS", offset);
    case OP_POP:
        return simple_instruction("OP_POP", offset);
    case OP_DUP:
        return simple_instruction("OP_DUP", offset);

    case OP_ADD:
        return simple_instruction("OP_ADD", offset);
    case OP_SUB:
        return simple_instruction("OP_SUB", offset);
    case OP_MUL:
        return simple_instruction("OP_MUL", offset);
    case OP_DIV:
        return simple_instruction("OP_DIV", offset);
    case OP_MOD:
        return simple_instruction("OP_MOD", offset);
    case OP_POW:
        return simple_instruction("OP_POW", offset);
    case OP_NEG:
        return simple_instruction("OP_NEG", offset);

    case OP_EQ:
        return simple_instruction("OP_EQ", offset);
    case OP_NEQ:
        return simple_instruction("OP_NEQ", offset);
    case OP_LT:
        return simple_instruction("OP_LT", offset);
    case OP_GT:
        return simple_instruction("OP_GT", offset);
    case OP_LTE:
        return simple_instruction("OP_LTE", offset);
    case OP_GTE:
        return simple_instruction("OP_GTE", offset);

    case OP_AND:
        return simple_instruction("OP_AND", offset);
    case OP_OR:
        return simple_instruction("OP_OR", offset);
    case OP_NOT:
        return simple_instruction("OP_NOT", offset);

    case OP_BIT_AND:
        return simple_instruction("OP_BIT_AND", offset);
    case OP_BIT_OR:
        return simple_instruction("OP_BIT_OR", offset);
    case OP_BIT_XOR:
        return simple_instruction("OP_BIT_XOR", offset);
    case OP_BIT_NOT:
        return simple_instruction("OP_BIT_NOT", offset);
    case OP_SHL:
        return simple_instruction("OP_SHL", offset);
    case OP_SHR:
        return simple_instruction("OP_SHR", offset);

    case OP_LOAD_LOCAL:
        return short_instruction("OP_LOAD_LOCAL", chunk, offset);
    case OP_STORE_LOCAL:
        return short_instruction("OP_STORE_LOCAL", chunk, offset);
    case OP_LOAD_GLOBAL:
        return constant_instruction("OP_LOAD_GLOBAL", chunk, offset);
    case OP_STORE_GLOBAL:
        return constant_instruction("OP_STORE_GLOBAL", chunk, offset);
    case OP_LOAD_UPVALUE:
        return short_instruction("OP_LOAD_UPVALUE", chunk, offset);
    case OP_STORE_UPVALUE:
        return short_instruction("OP_STORE_UPVALUE", chunk, offset);

    case OP_JUMP:
        return jump_instruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
        return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_JUMP_IF_TRUE:
        return jump_instruction("OP_JUMP_IF_TRUE", 1, chunk, offset);
    case OP_LOOP:
        return jump_instruction("OP_LOOP", -1, chunk, offset);

    case OP_CALL:
        return byte_instruction("OP_CALL", chunk, offset);
    case OP_RETURN:
        return simple_instruction("OP_RETURN", offset);
    case OP_CLOSURE:
        return constant_instruction("OP_CLOSURE", chunk, offset);

    case OP_NEW:
        return invoke_instruction("OP_NEW", chunk, offset);
    case OP_GET_FIELD:
        return constant_instruction("OP_GET_FIELD", chunk, offset);
    case OP_SET_FIELD:
        return constant_instruction("OP_SET_FIELD", chunk, offset);
    case OP_GET_METHOD:
        return constant_instruction("OP_GET_METHOD", chunk, offset);
    case OP_INVOKE:
        return invoke_instruction("OP_INVOKE", chunk, offset);

    case OP_NEW_ARSENAL:
        return short_instruction("OP_NEW_ARSENAL", chunk, offset);
    case OP_INDEX_GET:
        return simple_instruction("OP_INDEX_GET", offset);
    case OP_INDEX_SET:
        return simple_instruction("OP_INDEX_SET", offset);
    case OP_ARSENAL_PUSH:
        return simple_instruction("OP_ARSENAL_PUSH", offset);

    case OP_GPU_DISPATCH:
        return simple_instruction("OP_GPU_DISPATCH", offset);
    case OP_GPU_SYNC:
        return simple_instruction("OP_GPU_SYNC", offset);

    case OP_ENGRAVE:
        return byte_instruction("OP_ENGRAVE", chunk, offset);
    case OP_PIPE:
        return simple_instruction("OP_PIPE", offset);

    case OP_INC:
        return simple_instruction("OP_INC", offset);
    case OP_DEC:
        return simple_instruction("OP_DEC", offset);

    case OP_CAST:
        return byte_instruction("OP_CAST", chunk, offset);
    case OP_RANGE:
        return simple_instruction("OP_RANGE", offset);
    case OP_SPREAD:
        return simple_instruction("OP_SPREAD", offset);

    case OP_SHIELD_BEGIN:
        return jump_instruction("OP_SHIELD_BEGIN", 1, chunk, offset);
    case OP_SHIELD_END:
        return simple_instruction("OP_SHIELD_END", offset);
    case OP_SHATTER:
        return simple_instruction("OP_SHATTER", offset);

    case OP_HALT:
        return simple_instruction("OP_HALT", offset);

    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

void chunk_disassemble(Chunk* chunk, const char* label) {
    printf("== %s ==\n", label ? label : chunk->name);
    int offset = 0;
    while (offset < chunk->code_count) {
        offset = chunk_disassemble_instruction(chunk, offset);
    }
    printf("== end %s ==\n", label ? label : chunk->name);
}
