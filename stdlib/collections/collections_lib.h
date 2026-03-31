/*
 * X# Standard Library - Collections Module
 * ==========================================
 * Arsenal (array) + HashMap operations: 30+ functions.
 */

#ifndef XS_COLLECTIONS_LIB_H
#define XS_COLLECTIONS_LIB_H

#include "../../src/runtime/runtime.h"

/* Arsenal (array) operations */
XsValue xs_col_push(int argc, XsValue* args);
XsValue xs_col_pop(int argc, XsValue* args);
XsValue xs_col_shift(int argc, XsValue* args);
XsValue xs_col_unshift(int argc, XsValue* args);
XsValue xs_col_slice(int argc, XsValue* args);
XsValue xs_col_splice(int argc, XsValue* args);
XsValue xs_col_concat(int argc, XsValue* args);
XsValue xs_col_map(int argc, XsValue* args);
XsValue xs_col_filter(int argc, XsValue* args);
XsValue xs_col_reduce(int argc, XsValue* args);
XsValue xs_col_forEach(int argc, XsValue* args);
XsValue xs_col_find(int argc, XsValue* args);
XsValue xs_col_findIndex(int argc, XsValue* args);
XsValue xs_col_sort(int argc, XsValue* args);
XsValue xs_col_reverse(int argc, XsValue* args);
XsValue xs_col_includes(int argc, XsValue* args);
XsValue xs_col_indexOf(int argc, XsValue* args);
XsValue xs_col_flat(int argc, XsValue* args);
XsValue xs_col_zip(int argc, XsValue* args);
XsValue xs_col_enumerate(int argc, XsValue* args);
XsValue xs_col_range(int argc, XsValue* args);
XsValue xs_col_len(int argc, XsValue* args);
XsValue xs_col_fill(int argc, XsValue* args);
XsValue xs_col_every(int argc, XsValue* args);
XsValue xs_col_some(int argc, XsValue* args);

/* HashMap operations */
XsValue xs_col_hashmap_new(int argc, XsValue* args);
XsValue xs_col_hashmap_set(int argc, XsValue* args);
XsValue xs_col_hashmap_get(int argc, XsValue* args);
XsValue xs_col_hashmap_delete(int argc, XsValue* args);
XsValue xs_col_hashmap_has(int argc, XsValue* args);
XsValue xs_col_hashmap_keys(int argc, XsValue* args);
XsValue xs_col_hashmap_values(int argc, XsValue* args);

void xs_collections_register(VM* vm);

#endif /* XS_COLLECTIONS_LIB_H */
