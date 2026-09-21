#ifndef _TEST_FRAMEWORK_H_
#define _TEST_FRAMEWORK_H_

#include <stdio.h>
#include <string.h>

static int g_tests_run = 0;
static int g_tests_failed = 0;

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        printf("  [FAIL] %s:%d: ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #cond); \
        return 1; \
    } \
} while (0)

#define ASSERT_FALSE(cond) do { \
    if (cond) { \
        printf("  [FAIL] %s:%d: ASSERT_FALSE(%s)\n", __FILE__, __LINE__, #cond); \
        return 1; \
    } \
} while (0)

#define ASSERT_EQ(actual, expected) do { \
    long long _act = (long long)(actual); \
    long long _exp = (long long)(expected); \
    if (_act != _exp) { \
        printf("  [FAIL] %s:%d: ASSERT_EQ(%s, %s) -> got %lld, expected %lld\n", \
               __FILE__, __LINE__, #actual, #expected, _act, _exp); \
        return 1; \
    } \
} while (0)

#define ASSERT_STR_EQ(actual, expected) do { \
    const char *_act = (const char *)(actual); \
    const char *_exp = (const char *)(expected); \
    if (_act == NULL || _exp == NULL) { \
        if (_act != _exp) { \
            printf("  [FAIL] %s:%d: ASSERT_STR_EQ(%s, %s) -> one is NULL (%p vs %p)\n", \
                   __FILE__, __LINE__, #actual, #expected, (void*)_act, (void*)_exp); \
            return 1; \
        } \
    } else if (strcmp(_act, _exp) != 0) { \
        printf("  [FAIL] %s:%d: ASSERT_STR_EQ(%s, %s) -> got \"%s\", expected \"%s\"\n", \
               __FILE__, __LINE__, #actual, #expected, _act, _exp); \
        return 1; \
    } \
} while (0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("  [FAIL] %s:%d: ASSERT_NOT_NULL(%s)\n", __FILE__, __LINE__, #ptr); \
        return 1; \
    } \
} while (0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf("  [FAIL] %s:%d: ASSERT_NULL(%s) -> got %p\n", __FILE__, __LINE__, #ptr, (void*)(ptr)); \
        return 1; \
    } \
} while (0)

#define RUN_TEST(test_func) do { \
    g_tests_run++; \
    printf("RUN: %s\n", #test_func); \
    if (test_func() != 0) { \
        g_tests_failed++; \
        printf("RESULT: %s FAILED\n", #test_func); \
    } else { \
        printf("RESULT: %s PASSED\n", #test_func); \
    } \
} while (0)

#define TEST_MAIN_BEGIN() \
int main(int argc, char **argv) { \
    (void)argc; (void)argv; \
    printf("=== Starting Test Suite: %s ===\n", __FILE__);

#define TEST_MAIN_END() \
    printf("=== Summary: %d/%d tests passed ===\n", g_tests_run - g_tests_failed, g_tests_run); \
    return g_tests_failed ? 1 : 0; \
}

#endif /* _TEST_FRAMEWORK_H_ */
