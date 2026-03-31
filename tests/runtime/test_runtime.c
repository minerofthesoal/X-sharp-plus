/*
 * X# Runtime Tests
 * ===================
 * Tests for the VM value system and runtime execution correctness.
 */

#include "../test_framework.h"
#include "../../src/runtime/runtime.h"
#include <string.h>

/* ===== Value Constructor Tests ===== */

static void test_blade_value(void) {
    TEST_CASE("Runtime: blade (integer) value");
    XsValue v = xs_blade(42);
    ASSERT_EQ(v.type, VAL_BLADE);
    ASSERT_EQ(xs_as_blade(v), 42);
}

static void test_blade_negative(void) {
    TEST_CASE("Runtime: blade negative value");
    XsValue v = xs_blade(-100);
    ASSERT_EQ(v.type, VAL_BLADE);
    ASSERT_EQ(xs_as_blade(v), -100);
}

static void test_blade_zero(void) {
    TEST_CASE("Runtime: blade zero");
    XsValue v = xs_blade(0);
    ASSERT_EQ(v.type, VAL_BLADE);
    ASSERT_EQ(xs_as_blade(v), 0);
}

static void test_blade_large(void) {
    TEST_CASE("Runtime: blade large value");
    XsValue v = xs_blade(9999999999LL);
    ASSERT_EQ(xs_as_blade(v), 9999999999LL);
}

static void test_spark_value(void) {
    TEST_CASE("Runtime: spark (float) value");
    XsValue v = xs_spark(3.14);
    ASSERT_EQ(v.type, VAL_SPARK);
    ASSERT_FLOAT_EQ(xs_as_spark(v), 3.14, 0.001);
}

static void test_spark_negative(void) {
    TEST_CASE("Runtime: spark negative");
    XsValue v = xs_spark(-2.718);
    ASSERT_FLOAT_EQ(xs_as_spark(v), -2.718, 0.001);
}

static void test_spark_from_blade(void) {
    TEST_CASE("Runtime: xs_as_spark from blade converts to double");
    XsValue v = xs_blade(42);
    double d = xs_as_spark(v);
    ASSERT_FLOAT_EQ(d, 42.0, 0.001);
}

static void test_fate_value(void) {
    TEST_CASE("Runtime: fate (bool) values");
    XsValue t = xs_fate(true);
    XsValue f = xs_fate(false);
    ASSERT_EQ(t.type, VAL_FATE);
    ASSERT_EQ(f.type, VAL_FATE);
    ASSERT_TRUE(xs_as_fate(t));
    ASSERT_FALSE(xs_as_fate(f));
}

static void test_abyss_value(void) {
    TEST_CASE("Runtime: abyss (null) value");
    XsValue v = xs_abyss();
    ASSERT_EQ(v.type, VAL_ABYSS);
}

static void test_scroll_value(void) {
    TEST_CASE("Runtime: scroll (string) value");
    char *s = xs_strdup("hello");
    XsValue v = xs_scroll(s);
    ASSERT_EQ(v.type, VAL_SCROLL);
    ASSERT_STR_EQ(xs_as_scroll(v), "hello");
    free(s);
}

/* ===== Arsenal (Array) Tests ===== */

static void test_arsenal_create(void) {
    TEST_CASE("Runtime: arsenal create");
    XsArsenal *arr = xs_arsenal_new(4);
    ASSERT_NOT_NULL(arr);
    ASSERT_EQ(arr->count, 0);
    ASSERT_GE(arr->capacity, 4);
    xs_arsenal_free(arr);
}

static void test_arsenal_push_pop(void) {
    TEST_CASE("Runtime: arsenal push and pop");
    XsArsenal *arr = xs_arsenal_new(2);

    xs_arsenal_push(arr, xs_blade(10));
    xs_arsenal_push(arr, xs_blade(20));
    xs_arsenal_push(arr, xs_blade(30));

    ASSERT_EQ(arr->count, 3);

    XsValue popped = xs_arsenal_pop(arr);
    ASSERT_EQ(xs_as_blade(popped), 30);
    ASSERT_EQ(arr->count, 2);

    popped = xs_arsenal_pop(arr);
    ASSERT_EQ(xs_as_blade(popped), 20);

    popped = xs_arsenal_pop(arr);
    ASSERT_EQ(xs_as_blade(popped), 10);

    ASSERT_EQ(arr->count, 0);
    xs_arsenal_free(arr);
}

static void test_arsenal_get_set(void) {
    TEST_CASE("Runtime: arsenal get and set");
    XsArsenal *arr = xs_arsenal_new(4);

    xs_arsenal_push(arr, xs_blade(100));
    xs_arsenal_push(arr, xs_blade(200));
    xs_arsenal_push(arr, xs_blade(300));

    XsValue v = xs_arsenal_get(arr, 1);
    ASSERT_EQ(xs_as_blade(v), 200);

    xs_arsenal_set(arr, 1, xs_blade(999));
    v = xs_arsenal_get(arr, 1);
    ASSERT_EQ(xs_as_blade(v), 999);

    xs_arsenal_free(arr);
}

static void test_arsenal_grow(void) {
    TEST_CASE("Runtime: arsenal grows dynamically");
    XsArsenal *arr = xs_arsenal_new(2);

    for (int i = 0; i < 100; i++) {
        xs_arsenal_push(arr, xs_blade(i));
    }

    ASSERT_EQ(arr->count, 100);
    ASSERT_GE(arr->capacity, 100);

    for (int i = 0; i < 100; i++) {
        XsValue v = xs_arsenal_get(arr, i);
        ASSERT_EQ(xs_as_blade(v), i);
    }

    xs_arsenal_free(arr);
}

static void test_arsenal_mixed_types(void) {
    TEST_CASE("Runtime: arsenal with mixed value types");
    XsArsenal *arr = xs_arsenal_new(4);

    xs_arsenal_push(arr, xs_blade(42));
    xs_arsenal_push(arr, xs_spark(3.14));
    xs_arsenal_push(arr, xs_fate(true));
    xs_arsenal_push(arr, xs_abyss());

    ASSERT_EQ(xs_arsenal_get(arr, 0).type, VAL_BLADE);
    ASSERT_EQ(xs_arsenal_get(arr, 1).type, VAL_SPARK);
    ASSERT_EQ(xs_arsenal_get(arr, 2).type, VAL_FATE);
    ASSERT_EQ(xs_arsenal_get(arr, 3).type, VAL_ABYSS);

    xs_arsenal_free(arr);
}

static void test_arsenal_as_value(void) {
    TEST_CASE("Runtime: arsenal as XsValue");
    XsArsenal *arr = xs_arsenal_new(4);
    xs_arsenal_push(arr, xs_blade(1));

    XsValue v = xs_arsenal(arr);
    ASSERT_EQ(v.type, VAL_ARSENAL);
    ASSERT_EQ(v.object, (void *)arr);

    xs_arsenal_free(arr);
}

/* ===== Entity (Object) Tests ===== */

static void test_entity_create(void) {
    TEST_CASE("Runtime: entity create");
    XsEntity *e = xs_entity_new();
    ASSERT_NOT_NULL(e);
    ASSERT_EQ(e->count, 0);
    xs_entity_free(e);
}

static void test_entity_set_get(void) {
    TEST_CASE("Runtime: entity set and get");
    XsEntity *e = xs_entity_new();

    xs_entity_set(e, "name", xs_scroll(xs_strdup("test")));
    xs_entity_set(e, "value", xs_blade(42));
    xs_entity_set(e, "active", xs_fate(true));

    XsValue name = xs_entity_get(e, "name");
    ASSERT_EQ(name.type, VAL_SCROLL);
    ASSERT_STR_EQ(xs_as_scroll(name), "test");

    XsValue value = xs_entity_get(e, "value");
    ASSERT_EQ(xs_as_blade(value), 42);

    XsValue active = xs_entity_get(e, "active");
    ASSERT_TRUE(xs_as_fate(active));

    xs_entity_free(e);
}

static void test_entity_has(void) {
    TEST_CASE("Runtime: entity has key");
    XsEntity *e = xs_entity_new();

    xs_entity_set(e, "x", xs_blade(10));

    ASSERT_TRUE(xs_entity_has(e, "x"));
    ASSERT_FALSE(xs_entity_has(e, "y"));

    xs_entity_free(e);
}

static void test_entity_delete(void) {
    TEST_CASE("Runtime: entity delete key");
    XsEntity *e = xs_entity_new();

    xs_entity_set(e, "key1", xs_blade(1));
    xs_entity_set(e, "key2", xs_blade(2));

    ASSERT_TRUE(xs_entity_has(e, "key1"));
    xs_entity_delete(e, "key1");
    ASSERT_FALSE(xs_entity_has(e, "key1"));
    ASSERT_TRUE(xs_entity_has(e, "key2"));

    xs_entity_free(e);
}

static void test_entity_overwrite(void) {
    TEST_CASE("Runtime: entity overwrite value");
    XsEntity *e = xs_entity_new();

    xs_entity_set(e, "x", xs_blade(10));
    ASSERT_EQ(xs_as_blade(xs_entity_get(e, "x")), 10);

    xs_entity_set(e, "x", xs_blade(20));
    ASSERT_EQ(xs_as_blade(xs_entity_get(e, "x")), 20);

    xs_entity_free(e);
}

static void test_entity_many_keys(void) {
    TEST_CASE("Runtime: entity with many keys");
    XsEntity *e = xs_entity_new();

    for (int i = 0; i < 50; i++) {
        char key[32];
        snprintf(key, sizeof(key), "field_%d", i);
        xs_entity_set(e, key, xs_blade(i * 100));
    }

    ASSERT_EQ(e->count, 50);

    for (int i = 0; i < 50; i++) {
        char key[32];
        snprintf(key, sizeof(key), "field_%d", i);
        ASSERT_TRUE(xs_entity_has(e, key));
        ASSERT_EQ(xs_as_blade(xs_entity_get(e, key)), i * 100);
    }

    xs_entity_free(e);
}

static void test_entity_as_value(void) {
    TEST_CASE("Runtime: entity as XsValue");
    XsEntity *e = xs_entity_new();
    XsValue v = xs_entity(e);
    ASSERT_EQ(v.type, VAL_ENTITY);
    xs_entity_free(e);
}

/* ===== Native Function Tests ===== */

static XsValue test_native_add(int argc, XsValue *args) {
    if (argc < 2) return xs_abyss();
    return xs_blade(xs_as_blade(args[0]) + xs_as_blade(args[1]));
}

static void test_native_fn_value(void) {
    TEST_CASE("Runtime: native function value");
    XsValue v = xs_native_fn(test_native_add);
    ASSERT_EQ(v.type, VAL_NATIVE_FN);

    /* Call through function pointer */
    XsNativeFn fn = (XsNativeFn)v.object;
    XsValue args[2] = { xs_blade(3), xs_blade(4) };
    XsValue result = fn(2, args);
    ASSERT_EQ(xs_as_blade(result), 7);
}

/* ===== xs_strdup Tests ===== */

static void test_strdup(void) {
    TEST_CASE("Runtime: xs_strdup");
    char *s = xs_strdup("test string");
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "test string");
    free(s);
}

static void test_strdup_empty(void) {
    TEST_CASE("Runtime: xs_strdup empty string");
    char *s = xs_strdup("");
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "");
    free(s);
}

/* ===== Test Runner ===== */

void run_runtime_tests(void) {
    TEST_SUITE("Runtime");

    /* Value constructors */
    RUN_TEST(test_blade_value);
    RUN_TEST(test_blade_negative);
    RUN_TEST(test_blade_zero);
    RUN_TEST(test_blade_large);
    RUN_TEST(test_spark_value);
    RUN_TEST(test_spark_negative);
    RUN_TEST(test_spark_from_blade);
    RUN_TEST(test_fate_value);
    RUN_TEST(test_abyss_value);
    RUN_TEST(test_scroll_value);

    /* Arsenal */
    RUN_TEST(test_arsenal_create);
    RUN_TEST(test_arsenal_push_pop);
    RUN_TEST(test_arsenal_get_set);
    RUN_TEST(test_arsenal_grow);
    RUN_TEST(test_arsenal_mixed_types);
    RUN_TEST(test_arsenal_as_value);

    /* Entity */
    RUN_TEST(test_entity_create);
    RUN_TEST(test_entity_set_get);
    RUN_TEST(test_entity_has);
    RUN_TEST(test_entity_delete);
    RUN_TEST(test_entity_overwrite);
    RUN_TEST(test_entity_many_keys);
    RUN_TEST(test_entity_as_value);

    /* Native functions */
    RUN_TEST(test_native_fn_value);

    /* Utility */
    RUN_TEST(test_strdup);
    RUN_TEST(test_strdup_empty);
}
