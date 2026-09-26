/**
 * @file tests/test_helper.h
 * @brief 単体テスト共通ヘッダ (greatest + FFF)
 *
 * greatest がテストの実行とアサーション, FFF (fff.h) がモック関数を担当する.
 * メッセージは標準出力に出す (テストが標準エラー出力をパイプに繋ぐため).
 * TEST_FAIL 系のマクロは, TEST 関数 (enum greatest_test_res を返す関数)
 * の中でのみ使用できる.
 */

#ifndef _TEST_HELPER_H_
#define _TEST_HELPER_H_

#include <stdio.h>    /* printf */
#include <stdlib.h>   /* malloc free */
#include <string.h>   /* strcmp memcmp */
#include <regex.h>    /* regcomp regexec */

#include "greatest.h"
#include "fff.h"

/** 通知 (テストは失敗させない) */
#define TEST_NOTIFY(...)                                        \
    do {                                                        \
        (void)printf("notify: ");                               \
        (void)printf(__VA_ARGS__);                              \
        (void)printf("\n");                                     \
    } while (0)

/** エラー表示 (TEST 関数以外, 子プロセスで使う. テストは失敗させない) */
#define TEST_ERROR(...)                                         \
    do {                                                        \
        (void)printf("error: ");                                \
        (void)printf(__VA_ARGS__);                              \
        (void)printf("\n");                                     \
    } while (0)

/** 失敗 (TEST 関数の中でのみ使用可能) */
#define TEST_FAIL(...)                                          \
    do {                                                        \
        (void)printf(__VA_ARGS__);                              \
        (void)printf("\n");                                     \
        FAIL();                                                 \
    } while (0)

/** 条件が偽なら, メッセージを出力して失敗 */
#define TEST_ASSERT_MSG(cond, ...)                              \
    do {                                                        \
        if (!(cond)) {                                          \
            (void)printf("%s:%d: %s: ", __FILE__, __LINE__,     \
                         #cond);                                \
            (void)printf(__VA_ARGS__);                          \
            (void)printf("\n");                                 \
            FAIL();                                             \
        }                                                       \
    } while (0)

/** 整数の一致 */
#define TEST_ASSERT_INT(exp, act)                               \
    do {                                                        \
        long long e_ = (long long)(exp);                        \
        long long a_ = (long long)(act);                        \
        ASSERT_EQ_FMT(e_, a_, "%lld");                          \
    } while (0)

/** 整数の一致 (メッセージ付き) */
#define TEST_ASSERT_INT_MSG(exp, act, ...)                      \
    do {                                                        \
        long long e_ = (long long)(exp);                        \
        long long a_ = (long long)(act);                        \
        TEST_ASSERT_MSG(e_ == a_, __VA_ARGS__);                 \
    } while (0)

/** 整数の不一致 */
#define TEST_ASSERT_NOT_INT(exp, act)                           \
    do {                                                        \
        long long e_ = (long long)(exp);                        \
        long long a_ = (long long)(act);                        \
        TEST_ASSERT_MSG(e_ != a_, "not expected=%lld", e_);     \
    } while (0)

/** 浮動小数点の一致 (誤差 err 以内) */
#define TEST_ASSERT_DOUBLE(exp, err, act)                       \
    TEST_ASSERT_DOUBLE_MSG((exp), (err), (act), "expected=%f actual=%f", \
                           (double)(exp), (double)(act))

/** 浮動小数点の一致 (誤差 err 以内, メッセージ付き) */
#define TEST_ASSERT_DOUBLE_MSG(exp, err, act, ...)              \
    do {                                                        \
        double e_ = (double)(exp);                              \
        double a_ = (double)(act);                              \
        double t_ = (double)(err);                              \
        TEST_ASSERT_MSG((a_ >= e_ - t_) && (a_ <= e_ + t_),     \
                        __VA_ARGS__);                           \
    } while (0)

/** NULL なら "(null)" にする (メッセージ出力用) */
static inline const char *
test_str(const char *str)
{
    return str ? str : "(null)";
}

/** 文字列の一致 (NULL 同士は一致) */
static inline int
test_streq(const char *a, const char *b)
{
    if (!a || !b)
        return a == b;
    return strcmp(a, b) == 0;
}

/** 文字列の一致 */
#define TEST_ASSERT_STR(exp, act)                               \
    TEST_ASSERT_MSG(test_streq((exp), (act)),                   \
                    "expected=%s actual=%s",                    \
                    test_str(exp), test_str(act))

/** 文字列の一致 (メッセージ付き) */
#define TEST_ASSERT_STR_MSG(exp, act, ...)                      \
    TEST_ASSERT_MSG(test_streq((exp), (act)), __VA_ARGS__)

/** メモリの一致 */
#define TEST_ASSERT_MEM(exp, elen, act, alen)                   \
    TEST_ASSERT_MSG((elen) == (alen) &&                         \
                    memcmp((exp), (act), (elen)) == 0,          \
                    "memory differs")

/** メモリの一致 (メッセージ付き) */
#define TEST_ASSERT_MEM_MSG(exp, elen, act, alen, ...)          \
    TEST_ASSERT_MSG((elen) == (alen) &&                         \
                    memcmp((exp), (act), (elen)) == 0,          \
                    __VA_ARGS__)

/** 正規表現 (POSIX 拡張) に一致するか */
static inline int
test_match(const char *pattern, const char *str)
{
    regex_t re;
    int retval = 0;

    char *pat = NULL;
    size_t i = 0, j = 0;

    if (!pattern || !str)
        return 0;
    /* "\n" は改行として扱う (POSIX の拡張正規表現には無いので置換) */
    pat = (char *)malloc(strlen(pattern) + 1);
    if (!pat)
        return 0;
    while (pattern[i]) {
        if (pattern[i] == '\\' && pattern[i + 1] == 'n') {
            pat[j++] = '\n';
            i += 2;
        } else if (pattern[i] == '\\' && pattern[i + 1]) {
            pat[j++] = pattern[i++];
            pat[j++] = pattern[i++];
        } else {
            pat[j++] = pattern[i++];
        }
    }
    pat[j] = '\0';
    if (regcomp(&re, pat, REG_EXTENDED) == 0) {
        retval = (regexec(&re, str, 0, NULL, 0) == 0);
        regfree(&re);
    }
    free(pat);
    return retval;
}

/** 正規表現に一致 */
#define TEST_ASSERT_MATCH(pattern, str)                         \
    TEST_ASSERT_MSG(test_match((pattern), (str)),               \
                    "pattern=%s actual=%s", test_str(pattern),  \
                    test_str(str))

/** 正規表現に一致 (メッセージ付き) */
#define TEST_ASSERT_MATCH_MSG(pattern, str, ...)                \
    TEST_ASSERT_MSG(test_match((pattern), (str)), __VA_ARGS__)

#define TEST_ASSERT_NULL(ptr)      ASSERT_EQ(NULL, (ptr))
#define TEST_ASSERT_NOT_NULL(ptr)  ASSERT_NEQ(NULL, (ptr))

/**
 * 実行 (greatest)
 * fork した子プロセスが, 親のバッファを二重に出力しないように, 標準出力は
 * バッファリングしない.
 */
#define TEST_MAIN_BEGIN()                                       \
    do {                                                        \
        (void)setvbuf(stdout, NULL, _IONBF, 0);                 \
        GREATEST_MAIN_BEGIN();                                  \
    } while (0)
#define TEST_MAIN_END()    GREATEST_MAIN_END()

#endif /* _TEST_HELPER_H_ */
