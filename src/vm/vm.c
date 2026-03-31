/*
 * X# (Xsharp) Virtual Machine Implementation
 */

#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

/* ===== Lifecycle ===== */
void vm_init(VMState *vm) {
    memset(vm, 0, sizeof(VMState));
    vm->stack_top = 0;
    vm->frame_count = 0;
    vm->global_count = 0;
    vm->native_count = 0;
    vm->gc_head = NULL;
    vm->gc_bytes_allocated = 0;
    vm->gc_next_gc = 1024 * 1024;
    vm->had_error = false;
    vm->shield_depth = 0;
}

void vm_free(VMState *vm) {
    /* Free global names */
    for (int i = 0; i < vm->global_count; i++) {
        free(vm->global_names[i]);
    }
    for (int i = 0; i < vm->native_count; i++) {
        free(vm->native_names[i]);
    }
    /* GC sweep all */
    GcObject *obj = vm->gc_head;
    while (obj) {
        GcObject *next = obj->next;
        free(obj);
        obj = next;
    }
    vm->gc_head = NULL;
}

/* ===== Stack ===== */
void vm_push(VMState *vm, XsValue value) {
    if (vm->stack_top >= VM_STACK_MAX) {
        vm_runtime_error(vm, "Stack overflow");
        return;
    }
    vm->stack[vm->stack_top++] = value;
}

XsValue vm_pop(VMState *vm) {
    if (vm->stack_top <= 0) {
        vm_runtime_error(vm, "Stack underflow");
        return xs_abyss();
    }
    return vm->stack[--vm->stack_top];
}

XsValue vm_peek(VMState *vm, int distance) {
    if (vm->stack_top - 1 - distance < 0) return xs_abyss();
    return vm->stack[vm->stack_top - 1 - distance];
}

/* ===== Globals ===== */
void vm_define_global(VMState *vm, const char *name, XsValue value) {
    /* Check if exists */
    for (int i = 0; i < vm->global_count; i++) {
        if (strcmp(vm->global_names[i], name) == 0) {
            vm->global_values[i] = value;
            return;
        }
    }
    if (vm->global_count >= VM_GLOBALS_MAX) {
        vm_runtime_error(vm, "Too many global variables");
        return;
    }
    vm->global_names[vm->global_count] = strdup(name);
    vm->global_values[vm->global_count] = value;
    vm->global_count++;
}

bool vm_get_global(VMState *vm, const char *name, XsValue *out) {
    for (int i = 0; i < vm->global_count; i++) {
        if (strcmp(vm->global_names[i], name) == 0) {
            *out = vm->global_values[i];
            return true;
        }
    }
    /* Check natives */
    for (int i = 0; i < vm->native_count; i++) {
        if (strcmp(vm->native_names[i], name) == 0) {
            *out = xs_native_fn(vm->native_fns[i]);
            return true;
        }
    }
    return false;
}

bool vm_set_global(VMState *vm, const char *name, XsValue value) {
    for (int i = 0; i < vm->global_count; i++) {
        if (strcmp(vm->global_names[i], name) == 0) {
            vm->global_values[i] = value;
            return true;
        }
    }
    return false;
}

void vm_register_native_fn(VMState *vm, const char *name, XsNativeFn fn) {
    if (vm->native_count >= VM_GLOBALS_MAX) return;
    vm->native_names[vm->native_count] = strdup(name);
    vm->native_fns[vm->native_count] = fn;
    vm->native_count++;
}

/* ===== Error ===== */
void vm_runtime_error(VMState *vm, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(vm->error_msg, sizeof(vm->error_msg), fmt, args);
    va_end(args);
    vm->had_error = true;

    /* Print stack trace */
    fprintf(stderr, "Runtime Error: %s\n", vm->error_msg);
    for (int i = vm->frame_count - 1; i >= 0; i--) {
        CallFrame *frame = &vm->frames[i];
        int offset = (int)(frame->ip - frame->chunk->code);
        int line = chunk_get_line(frame->chunk, offset);
        fprintf(stderr, "  [line %d] in %s\n", line,
                frame->func_name ? frame->func_name : "<script>");
    }
}

/* ===== Helper: numeric coercion ===== */
static double to_number(XsValue v) {
    switch (v.type) {
        case VAL_BLADE: return (double)v.blade;
        case VAL_SPARK: return v.spark;
        case VAL_FATE:  return v.fate ? 1.0 : 0.0;
        default:        return 0.0;
    }
}

static bool is_truthy(XsValue v) {
    switch (v.type) {
        case VAL_ABYSS: return false;
        case VAL_FATE:  return v.fate;
        case VAL_BLADE: return v.blade != 0;
        case VAL_SPARK: return v.spark != 0.0;
        case VAL_SCROLL: return v.scroll != NULL && v.scroll[0] != '\0';
        default: return true;
    }
}

static bool values_equal(XsValue a, XsValue b) {
    if (a.type != b.type) {
        /* Allow blade/spark comparison */
        if ((a.type == VAL_BLADE || a.type == VAL_SPARK) &&
            (b.type == VAL_BLADE || b.type == VAL_SPARK)) {
            return to_number(a) == to_number(b);
        }
        return false;
    }
    switch (a.type) {
        case VAL_BLADE: return a.blade == b.blade;
        case VAL_SPARK: return a.spark == b.spark;
        case VAL_FATE:  return a.fate == b.fate;
        case VAL_ABYSS: return true;
        case VAL_SCROLL: return strcmp(a.scroll, b.scroll) == 0;
        default: return a.object == b.object;
    }
}

static char *concat_strings(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    char *result = malloc(la + lb + 1);
    memcpy(result, a, la);
    memcpy(result + la, b, lb);
    result[la + lb] = '\0';
    return result;
}

static char *value_to_string(XsValue val) {
    char buf[256];
    switch (val.type) {
        case VAL_BLADE:  snprintf(buf, sizeof(buf), "%ld", (long)val.blade); break;
        case VAL_SPARK:  snprintf(buf, sizeof(buf), "%g", val.spark); break;
        case VAL_FATE:   snprintf(buf, sizeof(buf), "%s", val.fate ? "truth" : "lies"); break;
        case VAL_ABYSS:  snprintf(buf, sizeof(buf), "abyss"); break;
        case VAL_SCROLL: return strdup(val.scroll ? val.scroll : "");
        default:         snprintf(buf, sizeof(buf), "<object@%p>", val.object); break;
    }
    return strdup(buf);
}

/* ===== Read helpers ===== */
static uint8_t read_byte(CallFrame *frame) {
    return *frame->ip++;
}

static uint16_t read_short(CallFrame *frame) {
    uint16_t val = (uint16_t)(frame->ip[0] << 8) | frame->ip[1];
    frame->ip += 2;
    return val;
}

static XsValue read_constant(CallFrame *frame) {
    uint16_t idx = read_short(frame);
    return chunk_get_constant(frame->chunk, idx);
}

/* ===== Main execution loop ===== */
VMResult vm_execute(VMState *vm, Chunk *chunk) {
    /* Set up initial frame */
    CallFrame *frame = &vm->frames[vm->frame_count++];
    frame->chunk = chunk;
    frame->ip = chunk->code;
    frame->slots = vm->stack;
    frame->func_name = chunk->name;
    frame->base = 0;

    vm->had_error = false;

    for (;;) {
        if (vm->had_error) return VM_RUNTIME_ERROR;

        uint8_t instruction = read_byte(frame);

        switch ((OpCode)instruction) {
        case OP_PUSH_BLADE: {
            XsValue c = read_constant(frame);
            vm_push(vm, c);
            break;
        }
        case OP_PUSH_SPARK: {
            XsValue c = read_constant(frame);
            vm_push(vm, c);
            break;
        }
        case OP_PUSH_SCROLL: {
            XsValue c = read_constant(frame);
            vm_push(vm, c);
            break;
        }
        case OP_PUSH_TRUTH:
            vm_push(vm, xs_fate(true));
            break;
        case OP_PUSH_LIES:
            vm_push(vm, xs_fate(false));
            break;
        case OP_PUSH_ABYSS:
            vm_push(vm, xs_abyss());
            break;
        case OP_POP:
            vm_pop(vm);
            break;
        case OP_DUP:
            vm_push(vm, vm_peek(vm, 0));
            break;

        /* Arithmetic */
        case OP_ADD: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            if (a.type == VAL_SCROLL || b.type == VAL_SCROLL) {
                char *sa = value_to_string(a);
                char *sb = value_to_string(b);
                char *r = concat_strings(sa, sb);
                free(sa); free(sb);
                vm_push(vm, xs_scroll(r));
            } else if (a.type == VAL_SPARK || b.type == VAL_SPARK) {
                vm_push(vm, xs_spark(to_number(a) + to_number(b)));
            } else {
                vm_push(vm, xs_blade(a.blade + b.blade));
            }
            break;
        }
        case OP_SUB: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            if (a.type == VAL_SPARK || b.type == VAL_SPARK)
                vm_push(vm, xs_spark(to_number(a) - to_number(b)));
            else
                vm_push(vm, xs_blade(a.blade - b.blade));
            break;
        }
        case OP_MUL: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            if (a.type == VAL_SPARK || b.type == VAL_SPARK)
                vm_push(vm, xs_spark(to_number(a) * to_number(b)));
            else
                vm_push(vm, xs_blade(a.blade * b.blade));
            break;
        }
        case OP_DIV: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            double db = to_number(b);
            if (db == 0.0) { vm_runtime_error(vm, "Division by zero"); break; }
            vm_push(vm, xs_spark(to_number(a) / db));
            break;
        }
        case OP_MOD: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            if (a.type == VAL_BLADE && b.type == VAL_BLADE) {
                if (b.blade == 0) { vm_runtime_error(vm, "Modulo by zero"); break; }
                vm_push(vm, xs_blade(a.blade % b.blade));
            } else {
                vm_push(vm, xs_spark(fmod(to_number(a), to_number(b))));
            }
            break;
        }
        case OP_POW: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            vm_push(vm, xs_spark(pow(to_number(a), to_number(b))));
            break;
        }
        case OP_NEG: {
            XsValue a = vm_pop(vm);
            if (a.type == VAL_SPARK) vm_push(vm, xs_spark(-a.spark));
            else vm_push(vm, xs_blade(-a.blade));
            break;
        }

        /* Comparison */
        case OP_EQ: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            vm_push(vm, xs_fate(values_equal(a, b)));
            break;
        }
        case OP_NEQ: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            vm_push(vm, xs_fate(!values_equal(a, b)));
            break;
        }
        case OP_LT: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            vm_push(vm, xs_fate(to_number(a) < to_number(b)));
            break;
        }
        case OP_GT: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            vm_push(vm, xs_fate(to_number(a) > to_number(b)));
            break;
        }
        case OP_LTE: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            vm_push(vm, xs_fate(to_number(a) <= to_number(b)));
            break;
        }
        case OP_GTE: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            vm_push(vm, xs_fate(to_number(a) >= to_number(b)));
            break;
        }

        /* Logical */
        case OP_AND: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            vm_push(vm, xs_fate(is_truthy(a) && is_truthy(b)));
            break;
        }
        case OP_OR: {
            XsValue b = vm_pop(vm), a = vm_pop(vm);
            vm_push(vm, xs_fate(is_truthy(a) || is_truthy(b)));
            break;
        }
        case OP_NOT: {
            XsValue a = vm_pop(vm);
            vm_push(vm, xs_fate(!is_truthy(a)));
            break;
        }

        /* Bitwise */
        case OP_BIT_AND: { XsValue b = vm_pop(vm), a = vm_pop(vm); vm_push(vm, xs_blade(a.blade & b.blade)); break; }
        case OP_BIT_OR:  { XsValue b = vm_pop(vm), a = vm_pop(vm); vm_push(vm, xs_blade(a.blade | b.blade)); break; }
        case OP_BIT_XOR: { XsValue b = vm_pop(vm), a = vm_pop(vm); vm_push(vm, xs_blade(a.blade ^ b.blade)); break; }
        case OP_BIT_NOT: { XsValue a = vm_pop(vm); vm_push(vm, xs_blade(~a.blade)); break; }
        case OP_SHL:     { XsValue b = vm_pop(vm), a = vm_pop(vm); vm_push(vm, xs_blade(a.blade << b.blade)); break; }
        case OP_SHR:     { XsValue b = vm_pop(vm), a = vm_pop(vm); vm_push(vm, xs_blade(a.blade >> b.blade)); break; }

        /* Variables */
        case OP_LOAD_LOCAL: {
            uint16_t slot = read_short(frame);
            vm_push(vm, frame->slots[slot]);
            break;
        }
        case OP_STORE_LOCAL: {
            uint16_t slot = read_short(frame);
            frame->slots[slot] = vm_peek(vm, 0);
            break;
        }
        case OP_LOAD_GLOBAL: {
            XsValue name_val = read_constant(frame);
            XsValue val;
            if (!vm_get_global(vm, name_val.scroll, &val)) {
                vm_runtime_error(vm, "Undefined variable '%s'", name_val.scroll);
                break;
            }
            vm_push(vm, val);
            break;
        }
        case OP_STORE_GLOBAL: {
            XsValue name_val = read_constant(frame);
            vm_define_global(vm, name_val.scroll, vm_peek(vm, 0));
            break;
        }
        case OP_LOAD_UPVALUE: {
            /* Simplified: treat as global for now */
            uint16_t slot = read_short(frame);
            (void)slot;
            vm_push(vm, xs_abyss());
            break;
        }
        case OP_STORE_UPVALUE: {
            uint16_t slot = read_short(frame);
            (void)slot;
            break;
        }

        /* Control flow */
        case OP_JUMP: {
            int16_t offset = (int16_t)read_short(frame);
            frame->ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE: {
            int16_t offset = (int16_t)read_short(frame);
            if (!is_truthy(vm_peek(vm, 0))) frame->ip += offset;
            break;
        }
        case OP_JUMP_IF_TRUE: {
            int16_t offset = (int16_t)read_short(frame);
            if (is_truthy(vm_peek(vm, 0))) frame->ip += offset;
            break;
        }
        case OP_LOOP: {
            uint16_t offset = read_short(frame);
            frame->ip -= offset;
            break;
        }

        /* Functions */
        case OP_CALL: {
            uint8_t argc = read_byte(frame);
            XsValue callee = vm_peek(vm, argc);

            if (callee.type == VAL_NATIVE_FN) {
                XsNativeFn fn = (XsNativeFn)(uintptr_t)callee.object;
                XsValue *args = &vm->stack[vm->stack_top - argc];
                XsValue result = fn(argc, args);
                vm->stack_top -= argc + 1;
                vm_push(vm, result);
            } else {
                vm_runtime_error(vm, "Can only call functions");
            }
            break;
        }
        case OP_RETURN: {
            XsValue result = vm_pop(vm);
            vm->frame_count--;
            if (vm->frame_count == 0) {
                vm_pop(vm); /* pop the script function */
                return VM_OK;
            }
            vm->stack_top = frame->base;
            vm_push(vm, result);
            frame = &vm->frames[vm->frame_count - 1];
            break;
        }
        case OP_CLOSURE: {
            /* Simplified closure: just read the constant */
            read_short(frame);
            vm_push(vm, xs_abyss());
            break;
        }

        /* Objects */
        case OP_NEW: {
            uint16_t name_idx = read_short(frame);
            uint8_t argc = read_byte(frame);
            (void)name_idx; (void)argc;
            XsEntity *ent = xs_entity_new();
            vm_push(vm, xs_entity(ent));
            break;
        }
        case OP_GET_FIELD: {
            XsValue name_val = read_constant(frame);
            XsValue obj = vm_pop(vm);
            if (obj.type == VAL_ENTITY) {
                XsEntity *ent = (XsEntity *)obj.object;
                XsValue val = xs_entity_get(ent, name_val.scroll);
                vm_push(vm, val);
            } else {
                vm_runtime_error(vm, "Only entities have fields");
            }
            break;
        }
        case OP_SET_FIELD: {
            XsValue name_val = read_constant(frame);
            XsValue val = vm_pop(vm);
            XsValue obj = vm_pop(vm);
            if (obj.type == VAL_ENTITY) {
                xs_entity_set((XsEntity *)obj.object, name_val.scroll, val);
                vm_push(vm, val);
            } else {
                vm_runtime_error(vm, "Only entities have fields");
            }
            break;
        }
        case OP_GET_METHOD: {
            read_constant(frame);
            /* Simplified */
            break;
        }
        case OP_INVOKE: {
            read_constant(frame);
            read_byte(frame);
            vm_push(vm, xs_abyss());
            break;
        }

        /* Collections */
        case OP_NEW_ARSENAL: {
            uint16_t count = read_short(frame);
            XsArsenal *arr = xs_arsenal_new(count > 0 ? count : 8);
            for (int i = count - 1; i >= 0; i--) {
                XsValue v = vm->stack[vm->stack_top - count + i];
                xs_arsenal_push(arr, v);
            }
            vm->stack_top -= count;
            vm_push(vm, xs_arsenal(arr));
            break;
        }
        case OP_INDEX_GET: {
            XsValue idx = vm_pop(vm);
            XsValue obj = vm_pop(vm);
            if (obj.type == VAL_ARSENAL) {
                XsArsenal *arr = (XsArsenal *)obj.object;
                vm_push(vm, xs_arsenal_get(arr, (int)idx.blade));
            } else if (obj.type == VAL_SCROLL) {
                int i = (int)idx.blade;
                char buf[2] = { obj.scroll[i], '\0' };
                vm_push(vm, xs_scroll(strdup(buf)));
            } else {
                vm_runtime_error(vm, "Cannot index this type");
            }
            break;
        }
        case OP_INDEX_SET: {
            XsValue val = vm_pop(vm);
            XsValue idx = vm_pop(vm);
            XsValue obj = vm_pop(vm);
            if (obj.type == VAL_ARSENAL) {
                xs_arsenal_set((XsArsenal *)obj.object, (int)idx.blade, val);
            }
            vm_push(vm, val);
            break;
        }
        case OP_ARSENAL_PUSH: {
            XsValue val = vm_pop(vm);
            XsValue arr = vm_pop(vm);
            if (arr.type == VAL_ARSENAL) {
                xs_arsenal_push((XsArsenal *)arr.object, val);
            }
            vm_push(vm, arr);
            break;
        }

        /* GPU */
        case OP_GPU_DISPATCH:
        case OP_GPU_SYNC:
            /* Placeholder for GPU operations */
            break;

        /* IO */
        case OP_ENGRAVE: {
            uint8_t argc = read_byte(frame);
            /* Print args from bottom to top */
            int base = vm->stack_top - argc;
            for (int i = 0; i < argc; i++) {
                if (i > 0) printf(" ");
                char *s = value_to_string(vm->stack[base + i]);
                printf("%s", s);
                free(s);
            }
            printf("\n");
            vm->stack_top -= argc;
            vm_push(vm, xs_abyss());
            break;
        }

        /* Pipe */
        case OP_PIPE: {
            /* a |> f  =>  f(a): already handled at compile time */
            break;
        }

        /* Inc/Dec */
        case OP_INC: {
            XsValue a = vm_pop(vm);
            if (a.type == VAL_BLADE) vm_push(vm, xs_blade(a.blade + 1));
            else vm_push(vm, xs_spark(a.spark + 1.0));
            break;
        }
        case OP_DEC: {
            XsValue a = vm_pop(vm);
            if (a.type == VAL_BLADE) vm_push(vm, xs_blade(a.blade - 1));
            else vm_push(vm, xs_spark(a.spark - 1.0));
            break;
        }

        case OP_CAST: {
            uint8_t target = read_byte(frame);
            XsValue v = vm_pop(vm);
            switch (target) {
                case VAL_BLADE: vm_push(vm, xs_blade((int64_t)to_number(v))); break;
                case VAL_SPARK: vm_push(vm, xs_spark(to_number(v))); break;
                case VAL_SCROLL: { char *s = value_to_string(v); vm_push(vm, xs_scroll(s)); break; }
                case VAL_FATE:  vm_push(vm, xs_fate(is_truthy(v))); break;
                default: vm_push(vm, v); break;
            }
            break;
        }

        case OP_RANGE: {
            XsValue end = vm_pop(vm), start = vm_pop(vm);
            int s = (int)start.blade, e = (int)end.blade;
            XsArsenal *arr = xs_arsenal_new(e - s > 0 ? e - s : 8);
            for (int i = s; i < e; i++) xs_arsenal_push(arr, xs_blade(i));
            vm_push(vm, xs_arsenal(arr));
            break;
        }

        case OP_SPREAD:
            /* Simplified */
            break;

        /* Exception handling */
        case OP_SHIELD_BEGIN: {
            uint16_t catch_off = read_short(frame);
            if (vm->shield_depth < 64) {
                vm->shield_stack[vm->shield_depth].catch_ip = frame->ip + catch_off;
                vm->shield_stack[vm->shield_depth].frame = vm->frame_count - 1;
                vm->shield_stack[vm->shield_depth].stack_top = vm->stack_top;
                vm->shield_depth++;
            }
            break;
        }
        case OP_SHIELD_END:
            if (vm->shield_depth > 0) vm->shield_depth--;
            break;
        case OP_SHATTER: {
            XsValue err = vm_pop(vm);
            if (vm->shield_depth > 0) {
                vm->shield_depth--;
                frame->ip = vm->shield_stack[vm->shield_depth].catch_ip;
                vm->stack_top = vm->shield_stack[vm->shield_depth].stack_top;
                vm_push(vm, err);
            } else {
                char *s = value_to_string(err);
                vm_runtime_error(vm, "Unhandled exception: %s", s);
                free(s);
            }
            break;
        }

        case OP_HALT:
            return VM_OK;

        default:
            vm_runtime_error(vm, "Unknown opcode: %d", instruction);
            return VM_RUNTIME_ERROR;
        }
    }
}

VMResult vm_call(VMState *vm, int arg_count) {
    XsValue callee = vm_peek(vm, arg_count);
    if (callee.type == VAL_NATIVE_FN) {
        XsNativeFn fn = (XsNativeFn)(uintptr_t)callee.object;
        XsValue *args = &vm->stack[vm->stack_top - arg_count];
        XsValue result = fn(arg_count, args);
        vm->stack_top -= arg_count + 1;
        vm_push(vm, result);
        return VM_OK;
    }
    vm_runtime_error(vm, "Value is not callable");
    return VM_RUNTIME_ERROR;
}

/* ===== GC ===== */
void vm_gc_mark_value(VMState *vm, XsValue val) {
    (void)vm;
    (void)val;
    /* Simplified GC - mark phase placeholder */
}

void vm_gc_collect(VMState *vm) {
    /* Mark phase */
    for (int i = 0; i < vm->stack_top; i++) {
        vm_gc_mark_value(vm, vm->stack[i]);
    }
    for (int i = 0; i < vm->global_count; i++) {
        vm_gc_mark_value(vm, vm->global_values[i]);
    }

    /* Sweep phase */
    GcObject **obj = &vm->gc_head;
    while (*obj) {
        if (!(*obj)->marked) {
            GcObject *unreached = *obj;
            *obj = unreached->next;
            free(unreached);
        } else {
            (*obj)->marked = false;
            obj = &(*obj)->next;
        }
    }
}

/* ===== vm_register_native: bridge for stdlib modules ===== */
/* stdlib modules see VM as an opaque type (forward-declared in runtime.h).
   This function casts VM* to VMState* and delegates to vm_register_native_fn. */
void vm_register_native(VM *vm, const char *name, XsNativeFn fn) {
    vm_register_native_fn((VMState *)vm, name, fn);
}
