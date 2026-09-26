/**
 * @file tests/calcp/test_option.c
 * @brief 単体テスト (option.c)
 *
 * @author higashi
 * @date 2026-09-27 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2026 Tetsuya Higashi. All Rights Reserved.
 */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>  /* snprintf */
#include <stdlib.h> /* EXIT_SUCCESS */
#include <getopt.h> /* optind */

#include "test_helper.h"
#include "test_process.h"
#include "def.h"
#include "log.h"
#include "version.h"
#include "calc.h"
#include "option.h"

DEFINE_FFF_GLOBALS;

/* getopt_long() は, モックにして, 通常は本物を呼ぶ (想定外の値を返させる) */
FAKE_VALUE_FUNC(int, getopt_long, int, char *const *, const char *,
                const struct option *, int *);
TEST_PASSTHROUGH(int, getopt_long,
                 (int argc, char *const *argv, const char *shortopts,
                  const struct option *longopts, int *longindex),
                 (argc, argv, shortopts, longopts, longindex))

/* option.c が呼び出す calc.c の関数は, モックにする */
FAKE_VOID_FUNC(set_digit, long);

#define BUF_SIZE 1024 /**< バッファサイズ */

/** parse_args() の引数 */
struct args {
    int argc;    /**< 引数の数 */
    char **argv; /**< 引数 */
};

/**
 * 初期化処理
 *
 * @return なし
 */
static void
startup(void)
{
    set_progname("testprog");
}

/**
 * 初期化処理
 *
 * @return なし
 */
static void
setup(void *data)
{
    (void)data;
    RESET_FAKE(set_digit);
    g_tflag = false;
    TEST_PASSTHROUGH_RESET(getopt_long);
    FFF_RESET_HISTORY();
    optind = 0; /* getopt の状態を初期化 */
}

/**
 * parse_args() を子プロセスで実行するための関数
 *
 * @param[in] arg args構造体
 * @return なし
 */
static void
run_parse_args(void *arg)
{
    struct args *a = (struct args *)arg; /* 引数 */

    optind = 0; /* 親プロセスで実行した getopt の状態を初期化 */
    parse_args(a->argc, a->argv);
}

/**
 * parse_args() を子プロセスで実行する
 *
 * @param[out] out 標準出力と標準エラー出力
 * @param[in] argc 引数の数
 * @param[in] argv 引数
 * @return 終了ステータス
 */
static int
exec_parse_args(char *out, size_t size, int argc, char **argv)
{
    struct args a = { argc, argv }; /* 引数 */

    return test_run_child(run_parse_args, &a, NULL, out, size);
}

/**
 * parse_args() 関数テスト (-d, --digit)
 *
 * @return なし
 */
TEST
test_parse_args_digit(void)
{
    char *argv1[] = { "testprog", "-d", "5", NULL };
    char *argv2[] = { "testprog", "--digit=10", NULL };

    /* ショートオプション */
    parse_args(3, argv1);
    TEST_ASSERT_INT(1, set_digit_fake.call_count);
    TEST_ASSERT_INT(5, set_digit_fake.arg0_val);

    /* ロングオプション */
    optind = 0;
    parse_args(2, argv2);
    TEST_ASSERT_INT(2, set_digit_fake.call_count);
    TEST_ASSERT_INT(10, set_digit_fake.arg0_val);
    PASS();
}

/**
 * parse_args() 関数テスト (-d, 範囲外の値)
 *
 * @return なし
 */
TEST
test_parse_args_digit_failure(void)
{
    char out[BUF_SIZE] = {0};                 /* 出力 */
    char maxplus[16] = {0};                   /* MAX_DIGIT + 1 */
    char *argv0[] = { "testprog", "-d", "0", NULL };
    char *argv1[] = { "testprog", "-d", "-1", NULL };
    char *argv2[] = { "testprog", "-d", "abc", NULL };
    char *argv3[] = { "testprog", "-d", maxplus, NULL };
    char *argv4[] = { "testprog", "-d", NULL };
    char **argvs[] = { argv0, argv1, argv2, argv3 };
    int argcs[] = { 3, 3, 3, 3 };

    (void)snprintf(maxplus, sizeof(maxplus), "%ld", MAX_DIGIT + 1);

    unsigned int i;
    for (i = 0; i < NELEMS(argvs); i++) {
        TEST_ASSERT_INT_MSG(EXIT_FAILURE,
                            exec_parse_args(out, sizeof(out), argcs[i],
                                            argvs[i]),
                            "argv[2]=%s", argvs[i][2]);
        TEST_ASSERT_MATCH("Digits is 1-[0-9]+\\.", out);
    }
    /* 引数が無い場合 */
    TEST_ASSERT_INT(EXIT_FAILURE, exec_parse_args(out, sizeof(out), 2, argv4));
    TEST_ASSERT_MATCH("Try `getopt --help'", out);
    /* 子プロセスの中でだけ呼ばれる */
    TEST_ASSERT_INT(0, set_digit_fake.call_count);
    PASS();
}

/**
 * parse_args() 関数テスト (オプション以外の引数)
 *
 * @return なし
 */
TEST
test_parse_args_non_option(void)
{
    char out[BUF_SIZE] = {0};
    char *argv[] = { "testprog", "abc", "def", NULL };

    TEST_ASSERT_INT(EXIT_SUCCESS, exec_parse_args(out, sizeof(out), 3, argv));
    TEST_ASSERT_STR("non-option ARGV-elements: abc def \n", out);
    PASS();
}

/**
 * print_help() 関数テスト (-h, --help)
 *
 * @return なし
 */
TEST
test_print_help(void)
{
    char out[BUF_SIZE] = {0};
    char *argv1[] = { "testprog", "-h", NULL };
    char *argv2[] = { "testprog", "--help", NULL };

    TEST_ASSERT_INT(EXIT_SUCCESS, exec_parse_args(out, sizeof(out), 2, argv1));
    TEST_ASSERT_MATCH("Usage: testprog \\[OPTION\\]", out);
    TEST_ASSERT_MATCH("-h, --help", out);
    TEST_ASSERT_INT(EXIT_SUCCESS, exec_parse_args(out, sizeof(out), 2, argv2));
    TEST_ASSERT_MATCH("Usage: testprog", out);
    PASS();
}

/**
 * print_version() 関数テスト (-V, --version)
 *
 * @return なし
 */
TEST
test_print_version(void)
{
    char out[BUF_SIZE] = {0};
    char *argv1[] = { "testprog", "-V", NULL };
    char *argv2[] = { "testprog", "--version", NULL };

    TEST_ASSERT_INT(EXIT_SUCCESS, exec_parse_args(out, sizeof(out), 2, argv1));
    TEST_ASSERT_STR("testprog " VERSION "\n", out);
    TEST_ASSERT_INT(EXIT_SUCCESS, exec_parse_args(out, sizeof(out), 2, argv2));
    TEST_ASSERT_STR("testprog " VERSION "\n", out);
    PASS();
}

/**
 * parse_error() 関数テスト (不正なオプション)
 *
 * @return なし
 */
TEST
test_parse_error(void)
{
    char out[BUF_SIZE] = {0};
    char *argv[] = { "testprog", "-x", NULL };

    TEST_ASSERT_INT(EXIT_FAILURE, exec_parse_args(out, sizeof(out), 2, argv));
    TEST_ASSERT_MATCH("Try `getopt --help' for more information", out);
    PASS();
}

/**
 * parse_args() 関数テスト (-t, --time)
 *
 * @return なし
 */
TEST
test_parse_args_time(void)
{
    char *argv1[] = { "testprog", "-t", NULL };
    char *argv2[] = { "testprog", "--time", NULL };

    ASSERT_FALSE(g_tflag);
    parse_args(2, argv1);
    ASSERT(g_tflag);

    g_tflag = false;
    optind = 0;
    parse_args(2, argv2);
    ASSERT(g_tflag);
    PASS();
}

/**
 * parse_args() 関数テスト (getopt_long() が想定外の値を返す)
 *
 * @return なし
 */
TEST
test_parse_args_internal_error(void)
{
    char out[BUF_SIZE] = {0};
    char *argv[] = { "testprog", NULL };

    TEST_INJECT(getopt_long, 0, 1, 'z', 0);
    TEST_ASSERT_INT(EXIT_FAILURE, exec_parse_args(out, sizeof(out), 1, argv));
    TEST_ASSERT_MATCH("getopt\\[122\\]: internal error", out);
    PASS();
}

GREATEST_MAIN_DEFS();

int
main(int argc, char **argv)
{
    TEST_MAIN_BEGIN();
    startup();
    SET_SETUP(setup, NULL);
    RUN_TEST(test_parse_args_digit);
    RUN_TEST(test_parse_args_digit_failure);
    RUN_TEST(test_parse_args_time);
    RUN_TEST(test_parse_args_non_option);
    RUN_TEST(test_print_help);
    RUN_TEST(test_print_version);
    RUN_TEST(test_parse_error);
    RUN_TEST(test_parse_args_internal_error);
    TEST_MAIN_END();
}
