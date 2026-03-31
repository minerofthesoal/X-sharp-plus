/*
 * X# (Xsharp) Runtime - Core Value System and VM Interface
 * =========================================================
 */

#ifndef XSHARP_RUNTIME_H
#define XSHARP_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ===== Value Type Enumeration ===== */
typedef enum {
    VAL_BLADE,      /* integer (int64_t)  */
    VAL_SPARK,      /* float   (double)   */
    VAL_SCROLL,     /* string  (char*)    */
    VAL_FATE,       /* boolean (bool)     */
    VAL_ABYSS,      /* null               */
    VAL_ARSENAL,    /* array              */
    VAL_ENTITY,     /* object / struct    */
    VAL_SPELL,      /* closure / lambda   */
    VAL_NATIVE_FN   /* native C function  */
} ValueType;

/* Forward declarations */
typedef struct XsValue XsValue;
typedef struct VM VM;

/* Native function pointer type */
typedef XsValue (*XsNativeFn)(int argc, XsValue* args);

/* ===== Core Value Struct ===== */
struct XsValue {
    ValueType type;
    union {
        int64_t  blade;    /* VAL_BLADE   */
        double   spark;    /* VAL_SPARK   */
        char*    scroll;   /* VAL_SCROLL  */
        bool     fate;     /* VAL_FATE    */
        void*    object;   /* VAL_ARSENAL / VAL_ENTITY / VAL_SPELL */
    };
};

/* ===== Arsenal (dynamic array) ===== */
typedef struct {
    XsValue* items;
    int      count;
    int      capacity;
} XsArsenal;

/* ===== Entity (hash-map based object) ===== */
typedef struct XsEntityEntry {
    char*    key;
    XsValue  value;
    struct XsEntityEntry* next;
} XsEntityEntry;

typedef struct {
    XsEntityEntry** buckets;
    int             bucket_count;
    int             count;
} XsEntity;

/* ===== VM Interface ===== */
struct VM {
    void* internal; /* opaque VM state */
};

/* Register a native function with the VM */
void vm_register_native(VM* vm, const char* name, XsNativeFn fn);

/* ===== Value Constructors ===== */
static inline XsValue xs_blade(int64_t v)  { XsValue val; val.type = VAL_BLADE;  val.blade  = v; return val; }
static inline XsValue xs_spark(double v)   { XsValue val; val.type = VAL_SPARK;  val.spark  = v; return val; }
static inline XsValue xs_fate(bool v)      { XsValue val; val.type = VAL_FATE;   val.fate   = v; return val; }
static inline XsValue xs_abyss(void)       { XsValue val; val.type = VAL_ABYSS;  val.blade  = 0; return val; }

static inline XsValue xs_scroll(char* s) {
    XsValue val;
    val.type = VAL_SCROLL;
    val.scroll = s;
    return val;
}

static inline XsValue xs_arsenal(XsArsenal* a) {
    XsValue val;
    val.type = VAL_ARSENAL;
    val.object = a;
    return val;
}

static inline XsValue xs_entity(XsEntity* e) {
    XsValue val;
    val.type = VAL_ENTITY;
    val.object = e;
    return val;
}

static inline XsValue xs_native_fn(XsNativeFn fn) {
    XsValue val;
    val.type = VAL_NATIVE_FN;
    val.object = (void*)(uintptr_t)fn;
    return val;
}

/* ===== Arsenal helpers ===== */
XsArsenal* xs_arsenal_new(int initial_cap);
void       xs_arsenal_free(XsArsenal* a);
void       xs_arsenal_push(XsArsenal* a, XsValue v);
XsValue    xs_arsenal_pop(XsArsenal* a);
XsValue    xs_arsenal_get(XsArsenal* a, int index);
void       xs_arsenal_set(XsArsenal* a, int index, XsValue v);

/* ===== Entity helpers ===== */
XsEntity*  xs_entity_new(void);
void       xs_entity_free(XsEntity* e);
void       xs_entity_set(XsEntity* e, const char* key, XsValue v);
XsValue    xs_entity_get(XsEntity* e, const char* key);
bool       xs_entity_has(XsEntity* e, const char* key);
void       xs_entity_delete(XsEntity* e, const char* key);

/* ===== Memory ===== */
char* xs_strdup(const char* s);

/* ===== VM native function registration (implemented in vm.c) ===== */
/* VM is an opaque handle; stdlib modules use this to register natives. */
void vm_register_native(VM* vm, const char* name, XsNativeFn fn);

/* ===== Value to C type helpers ===== */
static inline int64_t xs_as_blade(XsValue v)  { return v.blade;  }
static inline double  xs_as_spark(XsValue v)  { return v.type == VAL_SPARK ? v.spark : (double)v.blade; }
static inline char*   xs_as_scroll(XsValue v) { return v.scroll; }
static inline bool    xs_as_fate(XsValue v)   { return v.fate;   }

#endif /* XSHARP_RUNTIME_H */
