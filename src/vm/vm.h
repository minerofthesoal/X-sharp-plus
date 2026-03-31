/*
 * X# (Xsharp) Virtual Machine
 * =============================
 * Stack-based VM that executes bytecode.
 */

#ifndef XSHARP_VM_H
#define XSHARP_VM_H

#include "../codegen/codegen.h"
#include "../runtime/runtime.h"
#include <stdbool.h>

#define VM_STACK_MAX       4096
#define VM_FRAMES_MAX      256
#define VM_GLOBALS_MAX     1024

/* ===== Call Frame ===== */
typedef struct {
    Chunk      *chunk;
    uint8_t    *ip;         /* instruction pointer within chunk */
    XsValue    *slots;      /* pointer into stack for this frame's locals */
    const char *func_name;
    int         base;       /* stack base index */
} CallFrame;

/* ===== GC Object Header ===== */
typedef struct GcObject {
    struct GcObject *next;
    bool             marked;
    enum { GC_STRING, GC_ARSENAL, GC_ENTITY, GC_CLOSURE } gc_type;
} GcObject;

/* ===== VM State ===== */
typedef struct {
    /* Stack */
    XsValue     stack[VM_STACK_MAX];
    int         stack_top;

    /* Call frames */
    CallFrame   frames[VM_FRAMES_MAX];
    int         frame_count;

    /* Globals */
    char       *global_names[VM_GLOBALS_MAX];
    XsValue     global_values[VM_GLOBALS_MAX];
    int         global_count;

    /* Native functions */
    char       *native_names[VM_GLOBALS_MAX];
    XsNativeFn  native_fns[VM_GLOBALS_MAX];
    int         native_count;

    /* GC */
    GcObject   *gc_head;
    size_t      gc_bytes_allocated;
    size_t      gc_next_gc;

    /* Error state */
    bool        had_error;
    char        error_msg[1024];

    /* Shield/deflect (exception handling) */
    struct {
        uint8_t *catch_ip;
        int      frame;
        int      stack_top;
    } shield_stack[64];
    int         shield_depth;
} VMState;

/* ===== VM Lifecycle ===== */
void    vm_init(VMState *vm);
void    vm_free(VMState *vm);

/* ===== Execution ===== */
typedef enum {
    VM_OK,
    VM_RUNTIME_ERROR,
    VM_COMPILE_ERROR
} VMResult;

VMResult vm_execute(VMState *vm, Chunk *chunk);
VMResult vm_call(VMState *vm, int arg_count);

/* ===== Stack operations ===== */
void    vm_push(VMState *vm, XsValue value);
XsValue vm_pop(VMState *vm);
XsValue vm_peek(VMState *vm, int distance);

/* ===== Globals ===== */
void    vm_define_global(VMState *vm, const char *name, XsValue value);
bool    vm_get_global(VMState *vm, const char *name, XsValue *out);
bool    vm_set_global(VMState *vm, const char *name, XsValue value);

/* ===== Native functions ===== */
void    vm_register_native_fn(VMState *vm, const char *name, XsNativeFn fn);

/* ===== GC ===== */
void    vm_gc_collect(VMState *vm);
void    vm_gc_mark_value(VMState *vm, XsValue val);

/* ===== Error reporting ===== */
void    vm_runtime_error(VMState *vm, const char *fmt, ...);

#endif /* XSHARP_VM_H */
