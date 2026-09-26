/**
 * @file tests/calcd/test_option.c
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
#include "server.h"
#include "option.h"

DEFINE_FFF_GLOBALS;

/* option.c が呼び出す, calc.c と server.c の関数は, モックにする */
FAKE_VOID_FUNC(set_digit, long);
FAKE_VALUE_FUNC(int, set_port_string, const char *);

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
    RESET_FAKE(set_port_string);
    g_gflag = false;
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
 * parse_args() 関数テスト (-p, --port)
 *
 * @return なし
 */
TEST
test_parse_args_port(void)
{
    char *argv0[] = { "testprog", NULL };
    char *argv1[] = { "testprog", "-p", "8080", NULL };
    char *argv2[] = { "testprog", "--port=http", NULL };

    /* 指定しない場合は, デフォルトのポート番号 */
    parse_args(1, argv0);
    TEST_ASSERT_INT(1, set_port_string_fake.call_count);
    TEST_ASSERT_STR(DEFAULT_PORTNO, set_port_string_fake.arg0_history[0]);

    /* ショートオプション */
    optind = 0;
    parse_args(3, argv1);
    TEST_ASSERT_INT(3, set_port_string_fake.call_count);
    TEST_ASSERT_STR(DEFAULT_PORTNO, set_port_string_fake.arg0_history[1]);
    TEST_ASSERT_STR("8080", set_port_string_fake.arg0_history[2]);

    /* ロングオプション */
    optind = 0;
    parse_args(2, argv2);
    TEST_ASSERT_INT(5, set_port_string_fake.call_count);
    TEST_ASSERT_STR("http", set_port_string_fake.arg0_history[4]);
    PASS();
}

/**
 * parse_args() 関数テスト (-p, ポート番号の設定に失敗)
 *
 * @return なし
 */
TEST
test_parse_args_port_failure(void)
{
    char out[BUF_SIZE] = {0};
    char *argv[] = { "testprog", "-p", "1234567", NULL };
    int seq[] = { 0, -1 }; /* デフォルトは成功, -p の指定は失敗 */

    SET_RETURN_SEQ(set_port_string, seq, 2);
    TEST_ASSERT_INT(EXIT_FAILURE, exec_parse_args(out, sizeof(out), 3, argv));
    TEST_ASSERT_MATCH("Portno string length [0-9]+", out);

    /* デフォルトのポート番号の設定に失敗 */
    RESET_FAKE(set_port_string);
    set_port_string_fake.return_val = -1;
    TEST_ASSERT_INT(EXIT_FAILURE, exec_parse_args(out, sizeof(out), 1, argv));
    PASS();
}

/**
 * parse_args() 関数テスト (-g, --debug)
 *
 * @return なし
 */
TEST
test_parse_args_debug(void)
{
    char *argv1[] = { "testprog", "-g", NULL };
    char *argv2[] = { "testprog", "--debug", NULL };

    ASSERT_FALSE(g_gflag);
    parse_args(2, argv1);
    ASSERT(g_gflag);

    g_gflag = false;
    optind = 0;
    parse_args(2, argv2);
    ASSERT(g_gflag);
    PASS();
}

GREATEST_MAIN_DEFS();

int
main(int argc, char **argv)
{
    TEST_MAIN_BEGIN();
    startup();
    SET_SETUP(setup, NULL);
    RUN_TEST(test_parse_args_port);
    RUN_TEST(test_parse_args_port_failure);
    RUN_TEST(test_parse_args_digit);
    RUN_TEST(test_parse_args_digit_failure);
    RUN_TEST(test_parse_args_debug);
    RUN_TEST(test_parse_args_non_option);
    RUN_TEST(test_print_help);
    RUN_TEST(test_print_version);
    RUN_TEST(test_parse_error);
    TEST_MAIN_END();
}
