/**
 * @file tests/test_helper.h
 * @brief 単体テスト共通ヘッダ (greatest + FFF)
 *
 * greatest がテストの実行とアサーション, FFF (fff.h) がモック関数を担当する.
 * メッセージは標準出力に出す (テストが標準エラー出力をパイプに繋ぐため).
 * TEST_FAIL 系のマクロは, TEST 関数 (enum greatest_test_res を返す関数) の中で
 * のみ使用できる.
 *
 * @author higashi
 * @date 2026-09-27 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2026 Tetsuya Higashi. All Rights Reserved.
 */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <stdio.h>  /* printf */
#include <stdlib.h> /* malloc free */
#include <string.h> /* strcmp memcmp memcpy */
#include <regex.h>  /* regcomp regexec */
#include <errno.h>  /* errno */
#include <dlfcn.h>  /* dlsym RTLD_NEXT */
#include <unistd.h> /* close unlink */

#include "greatest.h"
#include "fff.h"

/** test_tmpname() に必要なバッファサイズ */
#define TEST_TMPNAME_SIZE 64u

/**
 * 一意な一時ファイル名を作る (tmpnam() の代わり. tmpnam() は, リンク時に警告される)
 * mkstemp() でファイルを作って, 名前だけ残して, ファイルは削除する.
 *
 * @param[out] buf 一時ファイル名 (TEST_TMPNAME_SIZE バイト以上)
 * @retval 0 成功
 * @retval -1 失敗
 */
static inline int
test_tmpname(char *buf)
{
    int tmpfd = -1; /* mkstemp戻り値 */

    (void)snprintf(buf, TEST_TMPNAME_SIZE, "/tmp/calc_test_XXXXXX");
    tmpfd = mkstemp(buf);
    if (tmpfd < 0)
        return -1;
    (void)close(tmpfd);
    (void)unlink(buf);
    return 0;
}

/** 通知 (テストは失敗させない) */
#define TEST_NOTIFY(...)           \
    do {                           \
        (void)printf("notify: ");  \
        (void)printf(__VA_ARGS__); \
        (void)printf("\n");        \
    } while (0)

/** エラー表示 (TEST 関数以外, 子プロセスで使う. テストは失敗させない) */
#define TEST_ERROR(...)            \
    do {                           \
        (void)printf("error: ");   \
        (void)printf(__VA_ARGS__); \
        (void)printf("\n");        \
    } while (0)

/** 失敗 (TEST 関数の中でのみ使用可能) */
#define TEST_FAIL(...)             \
    do {                           \
        (void)printf(__VA_ARGS__); \
        (void)printf("\n");        \
        FAIL();                    \
    } while (0)

/** 条件が偽なら, メッセージを出力して失敗 */
#define TEST_ASSERT_MSG(cond, ...)                                  \
    do {                                                            \
        if (!(cond)) {                                              \
            (void)printf("%s:%d: %s: ", __FILE__, __LINE__, #cond); \
            (void)printf(__VA_ARGS__);                              \
            (void)printf("\n");                                     \
            FAIL();                                                 \
        }                                                           \
    } while (0)

/** 整数の一致 */
#define TEST_ASSERT_INT(exp, act)        \
    do {                                 \
        long long e_ = (long long)(exp); \
        long long a_ = (long long)(act); \
        ASSERT_EQ_FMT(e_, a_, "%lld");   \
    } while (0)

/** 整数の一致 (メッセージ付き) */
#define TEST_ASSERT_INT_MSG(exp, act, ...)      \
    do {                                        \
        long long e_ = (long long)(exp);        \
        long long a_ = (long long)(act);        \
        TEST_ASSERT_MSG(e_ == a_, __VA_ARGS__); \
    } while (0)

/** 整数の不一致 */
#define TEST_ASSERT_NOT_INT(exp, act)                       \
    do {                                                    \
        long long e_ = (long long)(exp);                    \
        long long a_ = (long long)(act);                    \
        TEST_ASSERT_MSG(e_ != a_, "not expected=%lld", e_); \
    } while (0)

/** 浮動小数点の一致 (誤差 err 以内) */
#define TEST_ASSERT_DOUBLE(exp, err, act)                                               \
    TEST_ASSERT_DOUBLE_MSG((exp), (err), (act), "expected=%f actual=%f", (double)(exp), \
                           (double)(act))

/** 浮動小数点の一致 (誤差 err 以内, メッセージ付き) */
#define TEST_ASSERT_DOUBLE_MSG(exp, err, act, ...)                        \
    do {                                                                  \
        double e_ = (double)(exp);                                        \
        double a_ = (double)(act);                                        \
        double t_ = (double)(err);                                        \
        TEST_ASSERT_MSG((a_ >= e_ - t_) && (a_ <= e_ + t_), __VA_ARGS__); \
    } while (0)

/**
 * NULL なら "(null)" にする (メッセージ出力用)
 *
 * @param[in] str 文字列
 * @return 文字列 (NULL のときは "(null)")
 */
static inline const char *
test_str(const char *str)
{
    return ((str != NULL) ? str : "(null)");
}

/**
 * 文字列の一致 (NULL 同士は一致)
 *
 * @param[in] a 文字列
 * @param[in] b 文字列
 * @retval 1 一致
 * @retval 0 不一致
 */
static inline int
test_streq(const char *a, const char *b)
{
    if ((a == NULL) || (b == NULL))
        return a == b;
    return strcmp(a, b) == 0;
}

/** 文字列の一致 */
#define TEST_ASSERT_STR(exp, act) \
    TEST_ASSERT_MSG(test_streq((exp), (act)), "expected=%s actual=%s", test_str(exp), test_str(act))

/** 文字列の一致 (メッセージ付き) */
#define TEST_ASSERT_STR_MSG(exp, act, ...) TEST_ASSERT_MSG(test_streq((exp), (act)), __VA_ARGS__)

/** メモリの一致 */
#define TEST_ASSERT_MEM(exp, elen, act, alen) \
    TEST_ASSERT_MSG((elen) == (alen) && memcmp((exp), (act), (elen)) == 0, "memory differs")

/** メモリの一致 (メッセージ付き) */
#define TEST_ASSERT_MEM_MSG(exp, elen, act, alen, ...) \
    TEST_ASSERT_MSG((elen) == (alen) && memcmp((exp), (act), (elen)) == 0, __VA_ARGS__)

/**
 * 正規表現 (POSIX 拡張) に一致するか
 *
 * @param[in] pattern 正規表現 (POSIX 拡張)
 * @param[in] str 文字列
 * @retval 1 一致
 * @retval 0 不一致 (正規表現が不正な場合も含む)
 */
static inline int
test_match(const char *pattern, const char *str)
{
    regex_t re;
    int retval = 0;

    char *pat = NULL;
    size_t i = 0u, j = 0u;

    if ((pattern == NULL) || (str == NULL))
        return 0;
    /* "\n" は改行として扱う (POSIX の拡張正規表現には無いので置換) */
    pat = (char *)malloc(strlen(pattern) + 1u);
    if (pat == NULL)
        return 0;
    while (pattern[i] != '\0') {
        if ((pattern[i] == '\\') && (pattern[i + 1u] == 'n')) {
            pat[j++] = '\n';
            i += 2u;
        } else if ((pattern[i] == '\\') && (pattern[i + 1u] != '\0')) {
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
#define TEST_ASSERT_MATCH(pattern, str)                                                      \
    TEST_ASSERT_MSG(test_match((pattern), (str)), "pattern=%s actual=%s", test_str(pattern), \
                    test_str(str))

/** 正規表現に一致 (メッセージ付き) */
#define TEST_ASSERT_MATCH_MSG(pattern, str, ...) \
    TEST_ASSERT_MSG(test_match((pattern), (str)), __VA_ARGS__)

/** ポインタが NULL */
#define TEST_ASSERT_NULL(ptr)     ASSERT_EQ(NULL, (ptr))
/** ポインタが NULL でない */
#define TEST_ASSERT_NOT_NULL(ptr) ASSERT_NEQ(NULL, (ptr))

/** 障害を注入する設定 */
struct test_inject {
    int skip;        /**< 呼び出しを, 何回見送るか (その間は本物を呼ぶ) */
    int count;       /**< 見送ったあとで, 何回, 失敗させるか */
    long long value; /**< 失敗のときの戻り値 */
    int err;         /**< 失敗のときの errno */
};

/**
 * libc の関数を FFF のモックにして, 通常は本物の関数を呼ぶ (素通し).
 * 必要なテストだけが, TEST_INJECT() で, 失敗を返させる.
 * テストの実行ファイルに同名の関数を定義するので, 共有ライブラリの中の呼び出し
 * も, このモックになる. 本物の関数は dlsym(RTLD_NEXT) で探す.
 *
 * FAKE_VALUE_FUNC(ret, name, ...) の直後に書く.
 * 例: TEST_PASSTHROUGH(ssize_t, send,
 *                      (int fd, const void *buf, size_t n, int flags),
 *                      (fd, buf, n, flags))
 *
 * rettype: 戻り値の型, name: 関数名, params: 仮引数 (括弧付き),
 * args: 実引数 (括弧付き)
 */
#define TEST_PASSTHROUGH(rettype, name, params, args)        \
    static struct test_inject inject_##name;                 \
    static rettype(*real_##name) params;                     \
    static rettype pass_##name params                        \
    {                                                        \
        if (real_##name == NULL) {                           \
            void *sym_ = dlsym(RTLD_NEXT, #name);            \
            (void)memcpy(&real_##name, &sym_, sizeof(sym_)); \
        }                                                    \
        if (inject_##name.skip > 0) {                        \
            inject_##name.skip--;                            \
        } else if (inject_##name.count > 0) {                \
            inject_##name.count--;                           \
            errno = inject_##name.err;                       \
            return (rettype)inject_##name.value;             \
        }                                                    \
        return real_##name args;                             \
    }

/** TEST_PASSTHROUGH() のモックを, 初期状態 (素通し) に戻す. setup で呼ぶ */
#define TEST_PASSTHROUGH_RESET(name)                            \
    do {                                                        \
        RESET_FAKE(name);                                       \
        (void)memset(&inject_##name, 0, sizeof(inject_##name)); \
        name##_fake.custom_fake = pass_##name;                  \
    } while (0)

/**
 * 関数を失敗させる. skipn 回は本物を呼び, 次の countn 回を, retval と errnum
 * で失敗させる. それ以降は, また本物を呼ぶ.
 */
#define TEST_INJECT(name, skipn, countn, retval, errnum) \
    do {                                                 \
        inject_##name.skip = (skipn);                    \
        inject_##name.count = (countn);                  \
        inject_##name.value = (long long)(retval);       \
        inject_##name.err = (errnum);                    \
    } while (0)

/** TEST_INJECT() で仕込んだ失敗が, 全て使われた (関数が呼ばれた) ことを確認 */
#define TEST_ASSERT_INJECTED(name) \
    TEST_ASSERT_MSG(inject_##name.count == 0, "%s() was not called", #name)

/**
 * 実行 (greatest)
 * fork した子プロセスが, 親のバッファを二重に出力しないように, 標準出力は
 * バッファリングしない.
 */
#define TEST_MAIN_BEGIN()                       \
    do {                                        \
        (void)setvbuf(stdout, NULL, _IONBF, 0); \
        GREATEST_MAIN_BEGIN();                  \
    } while (0)
/** テストの main() の終了処理 */
#define TEST_MAIN_END() GREATEST_MAIN_END()

#endif /* TEST_HELPER_H */
