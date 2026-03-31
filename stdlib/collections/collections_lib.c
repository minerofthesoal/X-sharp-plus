/*
 * X# Standard Library - Collections Module Implementation
 * =========================================================
 */

#include "collections_lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ===== Helpers ===== */

static XsArsenal* get_arsenal(XsValue v) {
    if (v.type != VAL_ARSENAL || !v.object) return NULL;
    return (XsArsenal*)v.object;
}

static bool values_equal(XsValue a, XsValue b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BLADE:  return a.blade == b.blade;
        case VAL_SPARK:  return a.spark == b.spark;
        case VAL_FATE:   return a.fate == b.fate;
        case VAL_ABYSS:  return true;
        case VAL_SCROLL: return a.scroll && b.scroll && strcmp(a.scroll, b.scroll) == 0;
        default:         return a.object == b.object;
    }
}

/* ===== push: (arsenal, value) -> arsenal ===== */

XsValue xs_col_push(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr) return xs_abyss();
    xs_arsenal_push(arr, args[1]);
    return xs_blade((int64_t)arr->count);
}

/* ===== pop: (arsenal) -> value ===== */

XsValue xs_col_pop(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr || arr->count == 0) return xs_abyss();
    return xs_arsenal_pop(arr);
}

/* ===== shift: remove first element ===== */

XsValue xs_col_shift(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr || arr->count == 0) return xs_abyss();
    XsValue first = arr->items[0];
    memmove(arr->items, arr->items + 1, sizeof(XsValue) * (arr->count - 1));
    arr->count--;
    return first;
}

/* ===== unshift: prepend element ===== */

XsValue xs_col_unshift(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr) return xs_abyss();
    /* Ensure capacity */
    if (arr->count + 1 > arr->capacity) {
        arr->capacity = arr->capacity * 2 + 1;
        arr->items = (XsValue*)realloc(arr->items, sizeof(XsValue) * arr->capacity);
    }
    memmove(arr->items + 1, arr->items, sizeof(XsValue) * arr->count);
    arr->items[0] = args[1];
    arr->count++;
    return xs_blade((int64_t)arr->count);
}

/* ===== slice: (arsenal, start, end?) -> new arsenal ===== */

XsValue xs_col_slice(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr) return xs_abyss();
    int start = (int)xs_as_spark(args[1]);
    int end = (argc >= 3) ? (int)xs_as_spark(args[2]) : arr->count;
    if (start < 0) start = arr->count + start;
    if (end < 0) end = arr->count + end;
    if (start < 0) start = 0;
    if (end > arr->count) end = arr->count;
    if (start >= end) return xs_arsenal(xs_arsenal_new(0));

    XsArsenal* result = xs_arsenal_new(end - start);
    for (int i = start; i < end; i++) {
        xs_arsenal_push(result, arr->items[i]);
    }
    return xs_arsenal(result);
}

/* ===== splice: (arsenal, start, deleteCount, ...items) -> removed ===== */

XsValue xs_col_splice(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr) return xs_abyss();
    int start = (int)xs_as_spark(args[1]);
    int delCount = (int)xs_as_spark(args[2]);
    if (start < 0) start = arr->count + start;
    if (start < 0) start = 0;
    if (start > arr->count) start = arr->count;
    if (delCount < 0) delCount = 0;
    if (start + delCount > arr->count) delCount = arr->count - start;

    /* Collect removed items */
    XsArsenal* removed = xs_arsenal_new(delCount);
    for (int i = 0; i < delCount; i++) {
        xs_arsenal_push(removed, arr->items[start + i]);
    }

    /* Items to insert */
    int insertCount = argc - 3;
    int newCount = arr->count - delCount + insertCount;

    if (insertCount != delCount) {
        /* Shift elements */
        if (newCount > arr->capacity) {
            arr->capacity = newCount * 2;
            arr->items = (XsValue*)realloc(arr->items, sizeof(XsValue) * arr->capacity);
        }
        memmove(arr->items + start + insertCount,
                arr->items + start + delCount,
                sizeof(XsValue) * (arr->count - start - delCount));
    }
    /* Copy new items */
    for (int i = 0; i < insertCount; i++) {
        arr->items[start + i] = args[3 + i];
    }
    arr->count = newCount;
    return xs_arsenal(removed);
}

/* ===== concat: (arsenal, arsenal) -> new arsenal ===== */

XsValue xs_col_concat(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsArsenal* a = get_arsenal(args[0]);
    XsArsenal* b = get_arsenal(args[1]);
    if (!a || !b) return xs_abyss();
    XsArsenal* result = xs_arsenal_new(a->count + b->count);
    for (int i = 0; i < a->count; i++) xs_arsenal_push(result, a->items[i]);
    for (int i = 0; i < b->count; i++) xs_arsenal_push(result, b->items[i]);
    return xs_arsenal(result);
}

/* ===== map: (arsenal, fn) -> new arsenal ===== */

XsValue xs_col_map(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr || args[1].type != VAL_NATIVE_FN) return xs_abyss();
    XsNativeFn fn = (XsNativeFn)args[1].object;
    XsArsenal* result = xs_arsenal_new(arr->count);
    for (int i = 0; i < arr->count; i++) {
        XsValue callArgs[2];
        callArgs[0] = arr->items[i];
        callArgs[1] = xs_blade((int64_t)i);
        xs_arsenal_push(result, fn(2, callArgs));
    }
    return xs_arsenal(result);
}

/* ===== filter: (arsenal, fn) -> new arsenal ===== */

XsValue xs_col_filter(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr || args[1].type != VAL_NATIVE_FN) return xs_abyss();
    XsNativeFn fn = (XsNativeFn)args[1].object;
    XsArsenal* result = xs_arsenal_new(arr->count);
    for (int i = 0; i < arr->count; i++) {
        XsValue callArgs[2];
        callArgs[0] = arr->items[i];
        callArgs[1] = xs_blade((int64_t)i);
        XsValue r = fn(2, callArgs);
        if (r.type == VAL_FATE && r.fate) {
            xs_arsenal_push(result, arr->items[i]);
        }
    }
    return xs_arsenal(result);
}

/* ===== reduce: (arsenal, fn, initial) -> value ===== */

XsValue xs_col_reduce(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr || args[1].type != VAL_NATIVE_FN) return xs_abyss();
    XsNativeFn fn = (XsNativeFn)args[1].object;
    XsValue acc = args[2];
    for (int i = 0; i < arr->count; i++) {
        XsValue callArgs[3];
        callArgs[0] = acc;
        callArgs[1] = arr->items[i];
        callArgs[2] = xs_blade((int64_t)i);
        acc = fn(3, callArgs);
    }
    return acc;
}

/* ===== forEach: (arsenal, fn) -> abyss ===== */

XsValue xs_col_forEach(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr || args[1].type != VAL_NATIVE_FN) return xs_abyss();
    XsNativeFn fn = (XsNativeFn)args[1].object;
    for (int i = 0; i < arr->count; i++) {
        XsValue callArgs[2];
        callArgs[0] = arr->items[i];
        callArgs[1] = xs_blade((int64_t)i);
        fn(2, callArgs);
    }
    return xs_abyss();
}

/* ===== find: (arsenal, fn) -> value or abyss ===== */

XsValue xs_col_find(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr || args[1].type != VAL_NATIVE_FN) return xs_abyss();
    XsNativeFn fn = (XsNativeFn)args[1].object;
    for (int i = 0; i < arr->count; i++) {
        XsValue callArgs[2];
        callArgs[0] = arr->items[i];
        callArgs[1] = xs_blade((int64_t)i);
        XsValue r = fn(2, callArgs);
        if (r.type == VAL_FATE && r.fate) return arr->items[i];
    }
    return xs_abyss();
}

/* ===== findIndex: (arsenal, fn) -> blade ===== */

XsValue xs_col_findIndex(int argc, XsValue* args) {
    if (argc < 2) return xs_blade(-1);
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr || args[1].type != VAL_NATIVE_FN) return xs_blade(-1);
    XsNativeFn fn = (XsNativeFn)args[1].object;
    for (int i = 0; i < arr->count; i++) {
        XsValue callArgs[2];
        callArgs[0] = arr->items[i];
        callArgs[1] = xs_blade((int64_t)i);
        XsValue r = fn(2, callArgs);
        if (r.type == VAL_FATE && r.fate) return xs_blade((int64_t)i);
    }
    return xs_blade(-1);
}

/* ===== sort (in-place, using a comparison function or default numeric) ===== */

static int default_compare(const void* a, const void* b) {
    const XsValue* va = (const XsValue*)a;
    const XsValue* vb = (const XsValue*)b;
    double da = (va->type == VAL_SPARK) ? va->spark : (va->type == VAL_BLADE) ? (double)va->blade : 0.0;
    double db = (vb->type == VAL_SPARK) ? vb->spark : (vb->type == VAL_BLADE) ? (double)vb->blade : 0.0;
    if (da < db) return -1;
    if (da > db) return 1;
    /* For scrolls, compare lexicographically */
    if (va->type == VAL_SCROLL && vb->type == VAL_SCROLL) {
        return strcmp(va->scroll ? va->scroll : "", vb->scroll ? vb->scroll : "");
    }
    return 0;
}

XsValue xs_col_sort(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr) return xs_abyss();
    /* Simple insertion sort for stability */
    for (int i = 1; i < arr->count; i++) {
        XsValue key = arr->items[i];
        int j = i - 1;
        while (j >= 0 && default_compare(&arr->items[j], &key) > 0) {
            arr->items[j + 1] = arr->items[j];
            j--;
        }
        arr->items[j + 1] = key;
    }
    return args[0];
}

/* ===== reverse (in-place) ===== */

XsValue xs_col_reverse(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr) return xs_abyss();
    for (int i = 0, j = arr->count - 1; i < j; i++, j--) {
        XsValue tmp = arr->items[i];
        arr->items[i] = arr->items[j];
        arr->items[j] = tmp;
    }
    return args[0];
}

/* ===== includes: (arsenal, value) -> fate ===== */

XsValue xs_col_includes(int argc, XsValue* args) {
    if (argc < 2) return xs_fate(false);
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr) return xs_fate(false);
    for (int i = 0; i < arr->count; i++) {
        if (values_equal(arr->items[i], args[1])) return xs_fate(true);
    }
    return xs_fate(false);
}

/* ===== indexOf: (arsenal, value) -> blade ===== */

XsValue xs_col_indexOf(int argc, XsValue* args) {
    if (argc < 2) return xs_blade(-1);
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr) return xs_blade(-1);
    for (int i = 0; i < arr->count; i++) {
        if (values_equal(arr->items[i], args[1])) return xs_blade((int64_t)i);
    }
    return xs_blade(-1);
}

/* ===== flat: flatten nested arsenals one level ===== */

XsValue xs_col_flat(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr) return xs_abyss();
    XsArsenal* result = xs_arsenal_new(arr->count * 2);
    for (int i = 0; i < arr->count; i++) {
        if (arr->items[i].type == VAL_ARSENAL) {
            XsArsenal* inner = (XsArsenal*)arr->items[i].object;
            if (inner) {
                for (int j = 0; j < inner->count; j++) {
                    xs_arsenal_push(result, inner->items[j]);
                }
            }
        } else {
            xs_arsenal_push(result, arr->items[i]);
        }
    }
    return xs_arsenal(result);
}

/* ===== zip: (arsenal_a, arsenal_b) -> arsenal of pairs ===== */

XsValue xs_col_zip(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsArsenal* a = get_arsenal(args[0]);
    XsArsenal* b = get_arsenal(args[1]);
    if (!a || !b) return xs_abyss();
    int len = a->count < b->count ? a->count : b->count;
    XsArsenal* result = xs_arsenal_new(len);
    for (int i = 0; i < len; i++) {
        XsArsenal* pair = xs_arsenal_new(2);
        xs_arsenal_push(pair, a->items[i]);
        xs_arsenal_push(pair, b->items[i]);
        xs_arsenal_push(result, xs_arsenal(pair));
    }
    return xs_arsenal(result);
}

/* ===== enumerate: (arsenal) -> arsenal of [index, value] ===== */

XsValue xs_col_enumerate(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr) return xs_abyss();
    XsArsenal* result = xs_arsenal_new(arr->count);
    for (int i = 0; i < arr->count; i++) {
        XsArsenal* pair = xs_arsenal_new(2);
        xs_arsenal_push(pair, xs_blade((int64_t)i));
        xs_arsenal_push(pair, arr->items[i]);
        xs_arsenal_push(result, xs_arsenal(pair));
    }
    return xs_arsenal(result);
}

/* ===== range: (start, end, step?) -> arsenal ===== */

XsValue xs_col_range(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    int64_t start = (int64_t)xs_as_spark(args[0]);
    int64_t end   = (int64_t)xs_as_spark(args[1]);
    int64_t step  = (argc >= 3) ? (int64_t)xs_as_spark(args[2]) : 1;
    if (step == 0) return xs_abyss();

    int64_t count = 0;
    if (step > 0 && end > start) count = (end - start + step - 1) / step;
    else if (step < 0 && end < start) count = (start - end - step - 1) / (-step);
    if (count <= 0) return xs_arsenal(xs_arsenal_new(0));
    if (count > 1000000) count = 1000000; /* safety limit */

    XsArsenal* result = xs_arsenal_new((int)count);
    for (int64_t v = start; step > 0 ? v < end : v > end; v += step) {
        xs_arsenal_push(result, xs_blade(v));
    }
    return xs_arsenal(result);
}

/* ===== len: (arsenal) -> blade ===== */

XsValue xs_col_len(int argc, XsValue* args) {
    if (argc < 1) return xs_blade(0);
    if (args[0].type == VAL_ARSENAL) {
        XsArsenal* arr = get_arsenal(args[0]);
        return xs_blade(arr ? (int64_t)arr->count : 0);
    }
    if (args[0].type == VAL_SCROLL) {
        return xs_blade((int64_t)strlen(args[0].scroll ? args[0].scroll : ""));
    }
    return xs_blade(0);
}

/* ===== fill: (arsenal, value, start?, end?) -> arsenal ===== */

XsValue xs_col_fill(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr) return xs_abyss();
    int start = (argc >= 3) ? (int)xs_as_spark(args[2]) : 0;
    int end   = (argc >= 4) ? (int)xs_as_spark(args[3]) : arr->count;
    if (start < 0) start = 0;
    if (end > arr->count) end = arr->count;
    for (int i = start; i < end; i++) {
        arr->items[i] = args[1];
    }
    return args[0];
}

/* ===== every: (arsenal, fn) -> fate ===== */

XsValue xs_col_every(int argc, XsValue* args) {
    if (argc < 2) return xs_fate(true);
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr || args[1].type != VAL_NATIVE_FN) return xs_fate(false);
    XsNativeFn fn = (XsNativeFn)args[1].object;
    for (int i = 0; i < arr->count; i++) {
        XsValue callArgs[2];
        callArgs[0] = arr->items[i];
        callArgs[1] = xs_blade((int64_t)i);
        XsValue r = fn(2, callArgs);
        if (r.type == VAL_FATE && !r.fate) return xs_fate(false);
        if (r.type != VAL_FATE) return xs_fate(false);
    }
    return xs_fate(true);
}

/* ===== some: (arsenal, fn) -> fate ===== */

XsValue xs_col_some(int argc, XsValue* args) {
    if (argc < 2) return xs_fate(false);
    XsArsenal* arr = get_arsenal(args[0]);
    if (!arr || args[1].type != VAL_NATIVE_FN) return xs_fate(false);
    XsNativeFn fn = (XsNativeFn)args[1].object;
    for (int i = 0; i < arr->count; i++) {
        XsValue callArgs[2];
        callArgs[0] = arr->items[i];
        callArgs[1] = xs_blade((int64_t)i);
        XsValue r = fn(2, callArgs);
        if (r.type == VAL_FATE && r.fate) return xs_fate(true);
    }
    return xs_fate(false);
}

/* ===== HashMap operations (backed by XsEntity) ===== */

XsValue xs_col_hashmap_new(int argc, XsValue* args) {
    (void)argc; (void)args;
    XsEntity* e = xs_entity_new();
    return xs_entity(e);
}

XsValue xs_col_hashmap_set(int argc, XsValue* args) {
    if (argc < 3 || args[0].type != VAL_ENTITY) return xs_abyss();
    XsEntity* e = (XsEntity*)args[0].object;
    if (!e || args[1].type != VAL_SCROLL) return xs_abyss();
    xs_entity_set(e, args[1].scroll, args[2]);
    return args[0];
}

XsValue xs_col_hashmap_get(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_ENTITY) return xs_abyss();
    XsEntity* e = (XsEntity*)args[0].object;
    if (!e || args[1].type != VAL_SCROLL) return xs_abyss();
    return xs_entity_get(e, args[1].scroll);
}

XsValue xs_col_hashmap_delete(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_ENTITY) return xs_abyss();
    XsEntity* e = (XsEntity*)args[0].object;
    if (!e || args[1].type != VAL_SCROLL) return xs_abyss();
    xs_entity_delete(e, args[1].scroll);
    return xs_fate(true);
}

XsValue xs_col_hashmap_has(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_ENTITY) return xs_fate(false);
    XsEntity* e = (XsEntity*)args[0].object;
    if (!e || args[1].type != VAL_SCROLL) return xs_fate(false);
    return xs_fate(xs_entity_has(e, args[1].scroll));
}

XsValue xs_col_hashmap_keys(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_ENTITY) return xs_abyss();
    XsEntity* e = (XsEntity*)args[0].object;
    if (!e) return xs_abyss();
    XsArsenal* arr = xs_arsenal_new(e->count);
    for (int i = 0; i < e->bucket_count; i++) {
        XsEntityEntry* entry = e->buckets[i];
        while (entry) {
            xs_arsenal_push(arr, xs_scroll(xs_strdup(entry->key)));
            entry = entry->next;
        }
    }
    return xs_arsenal(arr);
}

XsValue xs_col_hashmap_values(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_ENTITY) return xs_abyss();
    XsEntity* e = (XsEntity*)args[0].object;
    if (!e) return xs_abyss();
    XsArsenal* arr = xs_arsenal_new(e->count);
    for (int i = 0; i < e->bucket_count; i++) {
        XsEntityEntry* entry = e->buckets[i];
        while (entry) {
            xs_arsenal_push(arr, entry->value);
            entry = entry->next;
        }
    }
    return xs_arsenal(arr);
}

/* ===== Registration ===== */

void xs_collections_register(VM* vm) {
    /* Arsenal operations */
    vm_register_native(vm, "Arsenal.push",      xs_col_push);
    vm_register_native(vm, "Arsenal.pop",       xs_col_pop);
    vm_register_native(vm, "Arsenal.shift",     xs_col_shift);
    vm_register_native(vm, "Arsenal.unshift",   xs_col_unshift);
    vm_register_native(vm, "Arsenal.slice",     xs_col_slice);
    vm_register_native(vm, "Arsenal.splice",    xs_col_splice);
    vm_register_native(vm, "Arsenal.concat",    xs_col_concat);
    vm_register_native(vm, "Arsenal.map",       xs_col_map);
    vm_register_native(vm, "Arsenal.filter",    xs_col_filter);
    vm_register_native(vm, "Arsenal.reduce",    xs_col_reduce);
    vm_register_native(vm, "Arsenal.forEach",   xs_col_forEach);
    vm_register_native(vm, "Arsenal.find",      xs_col_find);
    vm_register_native(vm, "Arsenal.findIndex", xs_col_findIndex);
    vm_register_native(vm, "Arsenal.sort",      xs_col_sort);
    vm_register_native(vm, "Arsenal.reverse",   xs_col_reverse);
    vm_register_native(vm, "Arsenal.includes",  xs_col_includes);
    vm_register_native(vm, "Arsenal.indexOf",   xs_col_indexOf);
    vm_register_native(vm, "Arsenal.flat",      xs_col_flat);
    vm_register_native(vm, "Arsenal.zip",       xs_col_zip);
    vm_register_native(vm, "Arsenal.enumerate", xs_col_enumerate);
    vm_register_native(vm, "Arsenal.range",     xs_col_range);
    vm_register_native(vm, "Arsenal.len",       xs_col_len);
    vm_register_native(vm, "Arsenal.fill",      xs_col_fill);
    vm_register_native(vm, "Arsenal.every",     xs_col_every);
    vm_register_native(vm, "Arsenal.some",      xs_col_some);

    /* Global len */
    vm_register_native(vm, "len",               xs_col_len);

    /* HashMap operations */
    vm_register_native(vm, "HashMap.new",       xs_col_hashmap_new);
    vm_register_native(vm, "HashMap.set",       xs_col_hashmap_set);
    vm_register_native(vm, "HashMap.get",       xs_col_hashmap_get);
    vm_register_native(vm, "HashMap.delete",    xs_col_hashmap_delete);
    vm_register_native(vm, "HashMap.has",       xs_col_hashmap_has);
    vm_register_native(vm, "HashMap.keys",      xs_col_hashmap_keys);
    vm_register_native(vm, "HashMap.values",    xs_col_hashmap_values);
}
