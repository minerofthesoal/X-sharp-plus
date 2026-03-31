/*
 * X# Standard Library Tests
 */

#include "../test_framework.h"
#include "../../stdlib/math/math_lib.h"
#include "../../stdlib/string/string_lib.h"
#include "../../stdlib/collections/collections_lib.h"
#include "../../src/runtime/runtime.h"
#include <math.h>
#include <string.h>

/* Math tests */
static void test_math_abs(void) {
    XsValue args[] = { xs_blade(-5) };
    XsValue r = xs_math_abs(1, args);
    TEST_ASSERT(r.type == VAL_BLADE && r.blade == 5, "abs(-5) should be 5");
}

static void test_math_sqrt(void) {
    XsValue args[] = { xs_spark(16.0) };
    XsValue r = xs_math_sqrt(1, args);
    TEST_ASSERT(r.type == VAL_SPARK && fabs(r.spark - 4.0) < 0.001, "sqrt(16) should be 4");
}

static void test_math_sin(void) {
    XsValue args[] = { xs_spark(0.0) };
    XsValue r = xs_math_sin(1, args);
    TEST_ASSERT(r.type == VAL_SPARK && fabs(r.spark) < 0.001, "sin(0) should be 0");
}

static void test_math_min(void) {
    XsValue args[] = { xs_blade(3), xs_blade(7) };
    XsValue r = xs_math_min(2, args);
    TEST_ASSERT(r.type == VAL_BLADE && r.blade == 3, "min(3,7) should be 3");
}

static void test_math_max(void) {
    XsValue args[] = { xs_blade(3), xs_blade(7) };
    XsValue r = xs_math_max(2, args);
    TEST_ASSERT(r.type == VAL_BLADE && r.blade == 7, "max(3,7) should be 7");
}

static void test_math_clamp(void) {
    XsValue args[] = { xs_spark(15.0), xs_spark(0.0), xs_spark(10.0) };
    XsValue r = xs_math_clamp(3, args);
    TEST_ASSERT(r.type == VAL_SPARK && fabs(r.spark - 10.0) < 0.001, "clamp(15,0,10) should be 10");
}

/* String tests */
static void test_str_length(void) {
    XsValue args[] = { xs_scroll("hello") };
    XsValue r = xs_str_length(1, args);
    TEST_ASSERT(r.type == VAL_BLADE && r.blade == 5, "length('hello') should be 5");
}

static void test_str_to_upper(void) {
    XsValue args[] = { xs_scroll("hello") };
    XsValue r = xs_str_to_upper(1, args);
    TEST_ASSERT(r.type == VAL_SCROLL && strcmp(r.scroll, "HELLO") == 0, "toUpper('hello') should be 'HELLO'");
}

static void test_str_contains(void) {
    XsValue args[] = { xs_scroll("hello world"), xs_scroll("world") };
    XsValue r = xs_str_contains(2, args);
    TEST_ASSERT(r.type == VAL_FATE && r.fate == true, "contains('hello world', 'world') should be true");
}

/* Collection tests */
static void test_col_len(void) {
    XsArsenal *arr = xs_arsenal_new(8);
    xs_arsenal_push(arr, xs_blade(1));
    xs_arsenal_push(arr, xs_blade(2));
    xs_arsenal_push(arr, xs_blade(3));
    XsValue args[] = { xs_arsenal(arr) };
    XsValue r = xs_col_len(1, args);
    TEST_ASSERT(r.type == VAL_BLADE && r.blade == 3, "len([1,2,3]) should be 3");
    xs_arsenal_free(arr);
}

static void test_col_push_pop(void) {
    XsArsenal *arr = xs_arsenal_new(8);
    xs_arsenal_push(arr, xs_blade(10));
    xs_arsenal_push(arr, xs_blade(20));
    TEST_ASSERT(arr->count == 2, "After 2 pushes, count should be 2");
    XsValue popped = xs_arsenal_pop(arr);
    TEST_ASSERT(popped.blade == 20, "Pop should return last element (20)");
    TEST_ASSERT(arr->count == 1, "After pop, count should be 1");
    xs_arsenal_free(arr);
}

int test_stdlib_all(void) {
    int pass = 0;
    TEST_RUN(test_math_abs, &pass);
    TEST_RUN(test_math_sqrt, &pass);
    TEST_RUN(test_math_sin, &pass);
    TEST_RUN(test_math_min, &pass);
    TEST_RUN(test_math_max, &pass);
    TEST_RUN(test_math_clamp, &pass);
    TEST_RUN(test_str_length, &pass);
    TEST_RUN(test_str_to_upper, &pass);
    TEST_RUN(test_str_contains, &pass);
    TEST_RUN(test_col_len, &pass);
    TEST_RUN(test_col_push_pop, &pass);
    return pass;
}
