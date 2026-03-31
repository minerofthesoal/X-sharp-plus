/*
 * X# Test Framework
 * ===================
 * Simple test framework with assert macros and test suite management.
 */

#ifndef XS_TEST_FRAMEWORK_H
#define XS_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ===== Test result counters ===== */
static int xs_test_pass_count = 0;
static int xs_test_fail_count = 0;
static int xs_test_total_count = 0;
static const char *xs_current_suite = "";
static const char *xs_current_test = "";

/* ===== Macros ===== */

#define TEST_SUITE(name)                                        \
    do {                                                        \
        xs_current_suite = name;                                \
        printf("\n=== Test Suite: %s ===\n", name);             \
    } while (0)

#define TEST_CASE(name)                                         \
    do {                                                        \
        xs_current_test = name;                                 \
        xs_test_total_count++;                                  \
    } while (0)

#define TEST_PASS()                                             \
    do {                                                        \
        xs_test_pass_count++;                                   \
        printf("  [PASS] %s\n", xs_current_test);              \
    } while (0)

#define TEST_FAIL(msg)                                          \
    do {                                                        \
        xs_test_fail_count++;                                   \
        printf("  [FAIL] %s: %s (%s:%d)\n",                    \
               xs_current_test, msg, __FILE__, __LINE__);       \
    } while (0)

#define ASSERT_TRUE(expr)                                       \
    do {                                                        \
        if (!(expr)) {                                          \
            TEST_FAIL("Expected true: " #expr);                 \
            return;                                             \
        }                                                       \
    } while (0)

#define ASSERT_FALSE(expr)                                      \
    do {                                                        \
        if ((expr)) {                                           \
            TEST_FAIL("Expected false: " #expr);                \
            return;                                             \
        }                                                       \
    } while (0)

#define ASSERT_EQ(a, b)                                         \
    do {                                                        \
        if ((a) != (b)) {                                       \
            TEST_FAIL("Expected equal: " #a " == " #b);         \
            return;                                             \
        }                                                       \
    } while (0)

#define ASSERT_NEQ(a, b)                                        \
    do {                                                        \
        if ((a) == (b)) {                                       \
            TEST_FAIL("Expected not equal: " #a " != " #b);     \
            return;                                             \
        }                                                       \
    } while (0)

#define ASSERT_STR_EQ(a, b)                                     \
    do {                                                        \
        if (strcmp((a), (b)) != 0) {                            \
            char _buf[256];                                     \
            snprintf(_buf, sizeof(_buf),                        \
                     "Expected \"%s\" == \"%s\"", (a), (b));    \
            TEST_FAIL(_buf);                                    \
            return;                                             \
        }                                                       \
    } while (0)

#define ASSERT_NOT_NULL(ptr)                                    \
    do {                                                        \
        if ((ptr) == NULL) {                                    \
            TEST_FAIL("Expected non-NULL: " #ptr);              \
            return;                                             \
        }                                                       \
    } while (0)

#define ASSERT_NULL(ptr)                                        \
    do {                                                        \
        if ((ptr) != NULL) {                                    \
            TEST_FAIL("Expected NULL: " #ptr);                  \
            return;                                             \
        }                                                       \
    } while (0)

#define ASSERT_FLOAT_EQ(a, b, eps)                              \
    do {                                                        \
        if (fabs((double)(a) - (double)(b)) > (eps)) {          \
            char _buf[256];                                     \
            snprintf(_buf, sizeof(_buf),                        \
                     "Expected %g == %g (eps=%g)",              \
                     (double)(a), (double)(b), (double)(eps));  \
            TEST_FAIL(_buf);                                    \
            return;                                             \
        }                                                       \
    } while (0)

#define ASSERT_GE(a, b)                                         \
    do {                                                        \
        if ((a) < (b)) {                                        \
            TEST_FAIL("Expected " #a " >= " #b);                \
            return;                                             \
        }                                                       \
    } while (0)

#define ASSERT_LE(a, b)                                         \
    do {                                                        \
        if ((a) > (b)) {                                        \
            TEST_FAIL("Expected " #a " <= " #b);                \
            return;                                             \
        }                                                       \
    } while (0)

#define ASSERT_GT(a, b)                                         \
    do {                                                        \
        if ((a) <= (b)) {                                       \
            TEST_FAIL("Expected " #a " > " #b);                 \
            return;                                             \
        }                                                       \
    } while (0)

#define ASSERT_MEM_EQ(a, b, len)                                \
    do {                                                        \
        if (memcmp((a), (b), (len)) != 0) {                    \
            TEST_FAIL("Memory not equal: " #a " vs " #b);       \
            return;                                             \
        }                                                       \
    } while (0)

/* Assert with message (used by tests) */
#define TEST_ASSERT(expr, msg)                                  \
    do {                                                        \
        if (!(expr)) {                                          \
            TEST_FAIL(msg);                                     \
            return;                                             \
        }                                                       \
    } while (0)

/* Run a test function and track pass count */
#define TEST_RUN(fn, pass_ptr)                                  \
    do {                                                        \
        TEST_CASE(#fn);                                         \
        int _before = xs_test_fail_count;                       \
        fn();                                                   \
        if (xs_test_fail_count == _before) {                    \
            TEST_PASS();                                        \
            if (pass_ptr) (*(pass_ptr))++;                      \
        }                                                       \
    } while (0)

/* Run a test function. Call TEST_CASE inside the function. */
#define RUN_TEST(fn)                                            \
    do {                                                        \
        int _before = xs_test_fail_count;                       \
        fn();                                                   \
        if (xs_test_fail_count == _before)                      \
            TEST_PASS();                                        \
    } while (0)

/* Print summary and return exit code */
static inline int test_summary(void) {
    printf("\n========================================\n");
    printf("Tests: %d total, %d passed, %d failed\n",
           xs_test_total_count, xs_test_pass_count, xs_test_fail_count);
    printf("========================================\n");
    return xs_test_fail_count > 0 ? 1 : 0;
}

#endif /* XS_TEST_FRAMEWORK_H */
